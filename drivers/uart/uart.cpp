#include "uart.hpp"

#include <cstdint>

namespace gameplayos::drivers::uart {
namespace {

// Raspberry Pi 5 exposes the RP1 peripheral BAR in the CPU physical
// address space at 0x1F00_0000_00. These offsets are from the RP1
// peripheral address map.
constexpr std::uintptr_t Rp1BarBase       = 0x1F00000000ULL;
constexpr std::uintptr_t Uart0Base        = Rp1BarBase + 0x00030000ULL;
constexpr std::uintptr_t IoBank0Base      = Rp1BarBase + 0x000D0000ULL;
constexpr std::uintptr_t PadsBank0Base    = Rp1BarBase + 0x000F0000ULL;

// RP1 IO_BANK0: each GPIO has STATUS at (n * 8), CTRL at (n * 8 + 4).
constexpr std::uintptr_t Gpio14Ctrl       = IoBank0Base + 0x04ULL + (14ULL * 8ULL);
constexpr std::uintptr_t Gpio14Pad        = PadsBank0Base + 0x04ULL + (14ULL * 4ULL);

// PL011 register offsets.
constexpr std::uintptr_t UartDr           = Uart0Base + 0x000;
constexpr std::uintptr_t UartFr           = Uart0Base + 0x018;
constexpr std::uintptr_t UartIbrd         = Uart0Base + 0x024;
constexpr std::uintptr_t UartFbrd         = Uart0Base + 0x028;
constexpr std::uintptr_t UartLcrH         = Uart0Base + 0x02C;
constexpr std::uintptr_t UartCr           = Uart0Base + 0x030;
constexpr std::uintptr_t UartImsc         = Uart0Base + 0x038;
constexpr std::uintptr_t UartIcr          = Uart0Base + 0x044;

constexpr std::uint32_t GpioFuncSelMask   = 0x1FU;
constexpr std::uint32_t GpioFuncUart0Tx   = 4U; // GPIO14 ALT4 = UART0_TX
constexpr std::uint32_t GpioOutOverMask   = 3U << 8;
constexpr std::uint32_t GpioOeOverMask    = 3U << 12;
constexpr std::uint32_t PadOutputDisable  = 1U << 7;

constexpr std::uint32_t UartFrBusy         = 1U << 3;
constexpr std::uint32_t UartFrTxFifoFull   = 1U << 5;

constexpr std::uint32_t UartCrEnable       = 1U << 0;
constexpr std::uint32_t UartCrTxEnable     = 1U << 8;

constexpr std::uint32_t UartLcrH8BitFifo   = (3U << 5) | (1U << 4);
constexpr std::uint32_t UartInterruptMask  = 0x7FFU;

bool g_initialized = false;

volatile std::uint32_t& reg(std::uintptr_t address)
{
    return *reinterpret_cast<volatile std::uint32_t*>(address);
}

void memory_barrier()
{
    asm volatile("dsb sy" ::: "memory");
}

void configure_gpio14_tx()
{
    // Route GPIO14 to UART0_TX (ALT4). Keep unrelated control bits.
    std::uint32_t control = reg(Gpio14Ctrl);
    control &= ~(GpioFuncSelMask | GpioOutOverMask | GpioOeOverMask);
    control |= GpioFuncUart0Tx;
    reg(Gpio14Ctrl) = control;

    // Clear OD (output-disable) in the GPIO14 pad control register.
    // The default drive-strength and other pad settings are preserved.
    std::uint32_t pad = reg(Gpio14Pad);
    pad &= ~PadOutputDisable;
    reg(Gpio14Pad) = pad;

    memory_barrier();
}

} // namespace

Result initialize(std::uint32_t baud_rate, std::uint32_t uart_clock_hz)
{
    g_initialized = false;

    if (baud_rate == 0 || uart_clock_hz == 0) {
        return Result::InvalidBaudRate;
    }

    // PL011 baud divisor:
    //     baud = UARTCLK / (16 * (IBRD + FBRD / 64))
    // Calculate the divisor in units of 1/64, rounding to nearest.
    const std::uint64_t divisor_x64 =
        (static_cast<std::uint64_t>(uart_clock_hz) * 4ULL +
         (static_cast<std::uint64_t>(baud_rate) / 2ULL)) /
        static_cast<std::uint64_t>(baud_rate);

    const std::uint64_t integer_divisor = divisor_x64 / 64ULL;
    const std::uint64_t fractional_divisor = divisor_x64 % 64ULL;

    if (integer_divisor == 0 || integer_divisor > 0xFFFFULL) {
        return Result::BaudRateOutOfRange;
    }

    configure_gpio14_tx();

    // Disable the UART before changing its line and baud-rate settings.
    reg(UartCr) = 0;
    reg(UartLcrH) = 0; // Disable FIFO while changing line settings.
    memory_barrier();

    // Wait briefly for a character already in progress to finish.
    bool became_idle = false;
    for (std::uint32_t count = 0; count < 1'000'000U; ++count) {
        if ((reg(UartFr) & UartFrBusy) == 0) {
            became_idle = true;
            break;
        }
    }

    if (!became_idle) {
        return Result::HardwareBusyTimeout;
    }

    reg(UartIbrd) = static_cast<std::uint32_t>(integer_divisor);
    reg(UartFbrd) = static_cast<std::uint32_t>(fractional_divisor);
    reg(UartLcrH) = UartLcrH8BitFifo; // 8-N-1, FIFO enabled.
    reg(UartImsc) = 0;                // Polling driver: no interrupts.
    reg(UartIcr) = UartInterruptMask; // Clear any pending interrupts.
    memory_barrier();

    // Enable UART0 and its transmitter. Receiver remains disabled.
    reg(UartCr) = UartCrEnable | UartCrTxEnable;
    memory_barrier();

    g_initialized = true;
    return Result::Ok;
}

bool is_initialized()
{
    return g_initialized;
}

void put_char(char value)
{
    if (!g_initialized) {
        return;
    }

    // Wait until the transmit FIFO has room.
    while ((reg(UartFr) & UartFrTxFifoFull) != 0) {
        asm volatile("nop");
    }

    reg(UartDr) = static_cast<std::uint32_t>(
        static_cast<std::uint8_t>(value));
}

void write(const char* text)
{
    if (!g_initialized || text == nullptr) {
        return;
    }

    while (*text != '\0') {
        put_char(*text);
        ++text;
    }
}

void write_line(const char* text)
{
    write(text);
    put_char('\r');
    put_char('\n');
}

} // namespace gameplayos::drivers::uart
