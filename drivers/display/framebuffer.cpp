#include "framebuffer.hpp"

namespace gameplayos::display {

namespace {

std::uint32_t bytes_per_pixel(PixelFormat format)
{
    switch (format) {
    case PixelFormat::RGBX8888:
    case PixelFormat::BGRX8888:
        return 4;
    case PixelFormat::RGB565:
        return 2;
    }

    return 0;
}

} // namespace

bool is_valid(const Framebuffer& framebuffer)
{
    const std::uint32_t bpp = bytes_per_pixel(framebuffer.format);

    if (framebuffer.address == nullptr ||
        framebuffer.width == 0 ||
        framebuffer.height == 0 ||
        bpp == 0) {
        return false;
    }

    if (framebuffer.width > (0xFFFFFFFFu / bpp)) {
        return false;
    }

    return framebuffer.pitch >= framebuffer.width * bpp;
}

bool put_pixel(
    const Framebuffer& framebuffer,
    std::uint32_t x,
    std::uint32_t y,
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue
)
{
    if (!is_valid(framebuffer) ||
        x >= framebuffer.width ||
        y >= framebuffer.height) {
        return false;
    }

    volatile std::uint8_t* pixel =
        framebuffer.address + (y * framebuffer.pitch) +
        (x * bytes_per_pixel(framebuffer.format));

    switch (framebuffer.format) {
    case PixelFormat::RGBX8888:
        pixel[0] = red;
        pixel[1] = green;
        pixel[2] = blue;
        pixel[3] = 0xFF;
        break;

    case PixelFormat::BGRX8888:
        pixel[0] = blue;
        pixel[1] = green;
        pixel[2] = red;
        pixel[3] = 0xFF;
        break;

    case PixelFormat::RGB565: {
        const std::uint16_t value =
            static_cast<std::uint16_t>(
                ((red >> 3) << 11) |
                ((green >> 2) << 5) |
                (blue >> 3)
            );

        pixel[0] = static_cast<std::uint8_t>(value & 0xFF);
        pixel[1] = static_cast<std::uint8_t>(value >> 8);
        break;
    }
    }

    return true;
}

bool clear(
    const Framebuffer& framebuffer,
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue
)
{
    if (!is_valid(framebuffer)) {
        return false;
    }

    for (std::uint32_t y = 0; y < framebuffer.height; ++y) {
        for (std::uint32_t x = 0; x < framebuffer.width; ++x) {
            if (!put_pixel(framebuffer, x, y, red, green, blue)) {
                return false;
            }
        }
    }

    return true;
}

} // namespace gameplayos::display
