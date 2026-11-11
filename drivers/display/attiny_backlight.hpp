#pragma once

#include <cstdint>

namespace gameplayos::display::attiny {

// Minimal I2C transport required by the panel controller.
// write() sends exactly the supplied bytes in one I2C transaction.
// read() receives bytes in one I2C transaction.
// delay_ms() must provide a blocking delay of at least the requested time.
struct I2cTransport {
    void* context;
    bool (*write)(void* context, std::uint8_t address,
                  const std::uint8_t* data, std::uint32_t length);
    bool (*read)(void* context, std::uint8_t address,
                 std::uint8_t* data, std::uint32_t length);
    void (*delay_ms)(void* context, std::uint32_t milliseconds);
};

enum class Result : std::uint8_t {
    Ok,
    InvalidTransport,
    NotInitialized,
    InvalidAddress,
    I2cError,
    UnsupportedController
};

struct Controller {
    I2cTransport transport{};
    std::uint8_t address{0x45};
    bool initialized{false};
};

// The observed Ubuntu device tree exposed this controller at I2C address 0x45.
constexpr std::uint8_t DEFAULT_I2C_ADDRESS = 0x45;

// Reads the controller ID, initializes panel power, and turns the backlight
// fully on. The caller must provide a working RP1 I2C transport.
Result initialize(
    Controller& controller,
    const I2cTransport& transport,
    std::uint8_t address = DEFAULT_I2C_ADDRESS
);

// Sets brightness from 0 (off) to 255 (maximum).
Result set_brightness(Controller& controller, std::uint8_t brightness);

} // namespace gameplayos::display::attiny
