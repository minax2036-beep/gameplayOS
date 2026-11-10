#pragma once

#include "framebuffer.hpp"

namespace gameplayos::display::firmware_framebuffer {

// Requests an 800x480, 32-bit framebuffer from the Raspberry Pi firmware.
// The returned framebuffer is valid only if the firmware accepts the request.
bool initialize(Framebuffer& framebuffer);

} // namespace gameplayos::display::firmware_framebuffer
