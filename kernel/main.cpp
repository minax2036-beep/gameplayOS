#include <cstdint>

#include "drivers/uart/uart.hpp"

extern "C" void kernel_main()
{
    using gameplayos::drivers::uart::Result;

    const Result uart_result =
        gameplayos::drivers::uart::initialize(9600);

    if (uart_result == Result::Ok) {
        gameplayos::drivers::uart::write_line("");
        gameplayos::drivers::uart::write_line("================================");
        gameplayos::drivers::uart::write_line("gameplayOS - Raspberry Pi 5");
        gameplayos::drivers::uart::write_line("AArch64 kernel entered C++.");
        gameplayos::drivers::uart::write_line("RP1 UART0 TX: GPIO14 / pin 8");
        gameplayos::drivers::uart::write_line("UART: 9600 baud, 8-N-1");
        gameplayos::drivers::uart::write_line("UART initialization: OK");
        gameplayos::drivers::uart::write_line("================================");
    }

    // Keep the primary CPU alive. The first UART test is one-shot at boot.
    for (;;) {
        asm volatile("wfe");
    }
}
