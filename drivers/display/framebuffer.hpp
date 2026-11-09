#pragma once

#include <cstdint>

namespace gameplayos::display {

enum class PixelFormat : std::uint8_t {
    RGBX8888,
    BGRX8888,
    RGB565
};

struct Framebuffer {
    volatile std::uint8_t* address;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t pitch;
    PixelFormat format;
};

bool is_valid(const Framebuffer& framebuffer);
bool put_pixel(
    const Framebuffer& framebuffer,
    std::uint32_t x,
    std::uint32_t y,
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue
);
bool clear(
    const Framebuffer& framebuffer,
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue
);

} // namespace gameplayos::display
