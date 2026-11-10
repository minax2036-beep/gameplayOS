#include "drivers/display/boot_screen.hpp"
#include "drivers/display/firmware_framebuffer.hpp"

extern "C" void kernel_main()
{
    using namespace gameplayos::display;

    Framebuffer framebuffer{};

    if (firmware_framebuffer::initialize(framebuffer)) {
        draw_boot_test_screen(framebuffer);

        constexpr BootLogEntry boot_log[] = {
            {LogLevel::Success, "FIRMWARE FRAMEBUFFER READY"},
            {LogLevel::Info, "GAMEPLAYOS KERNEL STARTED"}
        };

        draw_boot_log(
            framebuffer,
            boot_log,
            sizeof(boot_log) / sizeof(boot_log[0])
        );
    }

    // Keep the primary CPU alive after the one-shot display initialization.
    for (;;) {
        asm volatile("wfe");
    }
}
