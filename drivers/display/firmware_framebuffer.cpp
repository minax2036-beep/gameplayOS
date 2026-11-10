#include "firmware_framebuffer.hpp"

#include <cstddef>
#include <cstdint>

namespace gameplayos::display::firmware_framebuffer {

namespace {

constexpr std::uintptr_t MAILBOX_BASE = 0x107C013880ull;
constexpr std::uintptr_t MAILBOX0_READ = MAILBOX_BASE + 0x00;
constexpr std::uintptr_t MAILBOX0_STATUS = MAILBOX_BASE + 0x18;
constexpr std::uintptr_t MAILBOX1_WRITE = MAILBOX_BASE + 0x20;
constexpr std::uintptr_t MAILBOX1_STATUS = MAILBOX_BASE + 0x38;

constexpr std::uint32_t MAILBOX_FULL = 0x80000000u;
constexpr std::uint32_t MAILBOX_EMPTY = 0x40000000u;
constexpr std::uint32_t MAILBOX_RESPONSE = 0x80000000u;
constexpr std::uint32_t MAILBOX_CHANNEL_PROPERTY = 8u;

constexpr std::uint32_t TAG_SET_PHYSICAL_SIZE = 0x00048003u;
constexpr std::uint32_t TAG_SET_VIRTUAL_SIZE = 0x00048004u;
constexpr std::uint32_t TAG_SET_DEPTH = 0x00048005u;
constexpr std::uint32_t TAG_SET_PIXEL_ORDER = 0x00048006u;
constexpr std::uint32_t TAG_ALLOCATE_FRAMEBUFFER = 0x00040001u;
constexpr std::uint32_t TAG_GET_PITCH = 0x00040008u;

constexpr std::uint32_t REQUEST_WIDTH = 800;
constexpr std::uint32_t REQUEST_HEIGHT = 480;
constexpr std::uint32_t REQUEST_DEPTH = 32;
constexpr std::uint32_t REQUEST_RGB_ORDER = 1;
constexpr std::uint32_t FRAMEBUFFER_ALIGNMENT = 16;

constexpr std::uint32_t POLL_LIMIT = 10000000u;

// Both buffers are shared with the VideoCore firmware and must be 16-byte aligned.
alignas(16) volatile std::uint32_t depth_message[7];
alignas(16) volatile std::uint32_t framebuffer_message[26];

volatile std::uint32_t& mmio(std::uintptr_t address)
{
    return *reinterpret_cast<volatile std::uint32_t*>(address);
}

bool send_property_message(volatile std::uint32_t* message)
{
    const std::uintptr_t address =
        reinterpret_cast<std::uintptr_t>(message);

    // The property mailbox carries a 28-bit, 16-byte-aligned buffer address.
    if ((address & 0xFu) != 0 || address > 0x0FFFFFFFu) {
        return false;
    }

    const std::uint32_t request =
        static_cast<std::uint32_t>(address) | MAILBOX_CHANNEL_PROPERTY;

    asm volatile("dsb sy" ::: "memory");

    std::uint32_t timeout = POLL_LIMIT;
    while ((mmio(MAILBOX1_STATUS) & MAILBOX_FULL) != 0) {
        if (--timeout == 0) {
            return false;
        }
    }

    mmio(MAILBOX1_WRITE) = request;

    timeout = POLL_LIMIT;
    while (timeout != 0) {
        while ((mmio(MAILBOX0_STATUS) & MAILBOX_EMPTY) != 0) {
            if (--timeout == 0) {
                return false;
            }
        }

        const std::uint32_t response = mmio(MAILBOX0_READ);
        if ((response & ~0xFu) == (request & ~0xFu) &&
            (response & 0xFu) == MAILBOX_CHANNEL_PROPERTY) {
            asm volatile("dsb sy" ::: "memory");
            return message[1] == MAILBOX_RESPONSE;
        }

        --timeout;
    }

    return false;
}

bool tag_succeeded(
    const volatile std::uint32_t* message,
    std::size_t tag_index,
    std::uint32_t minimum_response_size
)
{
    const std::uint32_t response_size = message[tag_index + 2];

    return (response_size & MAILBOX_RESPONSE) != 0 &&
           (response_size & 0x7FFFFFFFu) >= minimum_response_size;
}

bool set_depth()
{
    depth_message[0] = sizeof(depth_message);
    depth_message[1] = 0;

    depth_message[2] = TAG_SET_DEPTH;
    depth_message[3] = 4;
    depth_message[4] = 4;
    depth_message[5] = REQUEST_DEPTH;

    depth_message[6] = 0;

    if (!send_property_message(depth_message)) {
        return false;
    }

    return tag_succeeded(depth_message, 2, 4) &&
           depth_message[5] == REQUEST_DEPTH;
}

bool request_framebuffer()
{
    for (std::size_t i = 0; i < 26; ++i) {
        framebuffer_message[i] = 0;
    }

    framebuffer_message[0] = sizeof(framebuffer_message);
    framebuffer_message[1] = 0;

    // Set physical display dimensions.
    framebuffer_message[2] = TAG_SET_PHYSICAL_SIZE;
    framebuffer_message[3] = 8;
    framebuffer_message[4] = 8;
    framebuffer_message[5] = REQUEST_WIDTH;
    framebuffer_message[6] = REQUEST_HEIGHT;

    // Set virtual framebuffer dimensions.
    framebuffer_message[7] = TAG_SET_VIRTUAL_SIZE;
    framebuffer_message[8] = 8;
    framebuffer_message[9] = 8;
    framebuffer_message[10] = REQUEST_WIDTH;
    framebuffer_message[11] = REQUEST_HEIGHT;

    // Set RGB pixel order.
    framebuffer_message[12] = TAG_SET_PIXEL_ORDER;
    framebuffer_message[13] = 4;
    framebuffer_message[14] = 4;
    framebuffer_message[15] = REQUEST_RGB_ORDER;

    // Allocate the framebuffer.
    framebuffer_message[16] = TAG_ALLOCATE_FRAMEBUFFER;
    framebuffer_message[17] = 8;
    framebuffer_message[18] = 4;
    framebuffer_message[19] = FRAMEBUFFER_ALIGNMENT;
    framebuffer_message[20] = 0;
    framebuffer_message[21] = 0;

    // Ask the firmware for the actual line pitch.
    framebuffer_message[22] = TAG_GET_PITCH;
    framebuffer_message[23] = 4;
    framebuffer_message[24] = 0;

    framebuffer_message[25] = 0;

    return send_property_message(framebuffer_message);
}

} // namespace

bool initialize(Framebuffer& framebuffer)
{
    framebuffer = {
        nullptr,
        0,
        0,
        0,
        PixelFormat::BGRX8888
    };

    // Raspberry Pi 5 firmware requires depth to be set in its own call
    // before the framebuffer allocation request.
    if (!set_depth() || !request_framebuffer()) {
        return false;
    }

    if (!tag_succeeded(framebuffer_message, 2, 8) ||
        !tag_succeeded(framebuffer_message, 7, 8) ||
        !tag_succeeded(framebuffer_message, 12, 4) ||
        !tag_succeeded(framebuffer_message, 16, 8) ||
        !tag_succeeded(framebuffer_message, 22, 4)) {
        return false;
    }

    const std::uint32_t width = framebuffer_message[5];
    const std::uint32_t height = framebuffer_message[6];
    const std::uint32_t virtual_width = framebuffer_message[10];
    const std::uint32_t virtual_height = framebuffer_message[11];
    const std::uint32_t pixel_order = framebuffer_message[15];
    const std::uint32_t bus_address = framebuffer_message[19];
    const std::uint32_t buffer_size = framebuffer_message[20];
    const std::uint32_t pitch = framebuffer_message[24];

    if (width == 0 ||
        height == 0 ||
        virtual_width < width ||
        virtual_height < height ||
        pixel_order != REQUEST_RGB_ORDER ||
        bus_address == 0 ||
        pitch < width * 4u) {
        return false;
    }

    const std::uint64_t required_size =
        static_cast<std::uint64_t>(pitch) * height;

    if (required_size > buffer_size) {
        return false;
    }

    // Raspberry Pi firmware returns a VideoCore bus address. The low
    // 30-bit alias is the ARM-visible physical address for this setup.
    const std::uint32_t physical_address =
        bus_address & 0x3FFFFFFFu;

    if (physical_address == 0) {
        return false;
    }

    framebuffer.address =
        reinterpret_cast<volatile std::uint8_t*>(
            static_cast<std::uintptr_t>(physical_address)
        );
    framebuffer.width = width;
    framebuffer.height = height;
    framebuffer.pitch = pitch;
    framebuffer.format = PixelFormat::BGRX8888;

    return is_valid(framebuffer);
}

} // namespace gameplayos::display::firmware_framebuffer
