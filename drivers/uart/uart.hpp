#pragma once

#include <cstdint>

namespace gameplayos::drivers::uart {

enum class Result : std::uint8_t {
    Ok,
    InvalidBaudRate,
    BaudRateOutOfRange,
    HardwareBusyTimeout
};

// RP1 clk_uart is typically 48 MHz. Pass another value if the
// firmware configuration or board setup uses a different UART clock.
// The RP1 datasheet says typically 48 MHz; Pi 5 clock-tree measurements
// report 50 MHz. Keep this overridable for other firmware/clock setups.
constexpr std::uint32_t DefaultClockHz = 50'000'000;
constexpr std::uint32_t DefaultBaudRate = 9'600;

// Configure RP1 UART0 and route GPIO14 to UART0_TX.
// This first driver revision is transmit-only (8 data bits, no parity,
// one stop bit) and does not use interrupts.
Result initialize(
    std::uint32_t baud_rate = DefaultBaudRate,
    std::uint32_t uart_clock_hz = DefaultClockHz);

bool is_initialized();

void put_char(char value);
void write(const char* text);
void write_line(const char* text);

} // namespace gameplayos::drivers::uart
