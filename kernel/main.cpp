#include "drivers/display/boot_screen.hpp"
#include "drivers/display/dsi.hpp"
#include "drivers/display/firmware_framebuffer.hpp"

extern "C" void kernel_main()
{
    using namespace gameplayos::display;

    Framebuffer framebuffer{};

    // The firmware framebuffer is the first available output path.
    // If allocation fails, there is no safe display surface to write to.
    if (!firmware_framebuffer::initialize(framebuffer)) {
        for (;;) {
            asm volatile("wfe");
        }
    }

    // Draw the diagnostic pattern first so that framebuffer operation can
    // be visually checked independently of the DSI host implementation.
    draw_boot_test_screen(framebuffer);

    const dsi::Result dsi_result =
        dsi::initialize(dsi::reference_800x480_panel());

    BootLogEntry boot_log[] = {
        {LogLevel::Success, "GAMEPLAYOS KERNEL STARTED"},
        {LogLevel::Success, "FRAMEBUFFER READY"},
        {LogLevel::Warning, "BACKLIGHT I2C NOT CONNECTED"},
        {
            dsi_result == dsi::Result::Ok
                ? LogLevel::Success
                : LogLevel::Warning,
            dsi_result == dsi::Result::Ok
                ? "DSI INITIALIZED"
                : "DSI INIT NOT IMPLEMENTED"
        },
        {LogLevel::Info, "BOOT DIAGNOSTIC SCREEN"}
    };

    draw_boot_log(
        framebuffer,
        boot_log,
        sizeof(boot_log) / sizeof(boot_log[0])
    );

    // Keep the primary CPU alive after one-shot display initialization.
    for (;;) {
        asm volatile("wfe");
    }
}
