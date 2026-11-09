#pragma once
#include "framebuffer.hpp"
#include <cstddef>
#include <cstdint>
namespace gameplayos::display {
enum class LogLevel : std::uint8_t { Info, Success, Warning, Error };
struct BootLogEntry { LogLevel level; const char* message; };
bool draw_boot_test_screen(const Framebuffer& framebuffer);
bool draw_boot_log(const Framebuffer& framebuffer, const BootLogEntry* entries, std::size_t count);
} // namespace gameplayos::display
