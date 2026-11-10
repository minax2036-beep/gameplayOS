#include "attiny_backlight.hpp"

namespace gameplayos::display::attiny {

namespace {

constexpr std::uint8_t REG_ID = 0x80;
constexpr std::uint8_t REG_PORTA = 0x81;
constexpr std::uint8_t REG_PORTB = 0x82;
constexpr std::uint8_t REG_PORTC = 0x83;
constexpr std::uint8_t REG_POWERON = 0x85;
constexpr std::uint8_t REG_PWM = 0x86;

constexpr std::uint8_t ID_REVISION_1 = 0xDE;
constexpr std::uint8_t ID_REVISION_2 = 0xC3;

constexpr std::uint8_t PA_LCD_LR = 1u << 2;
constexpr std::uint8_t PB_LCD_MAIN = 1u << 7;
constexpr std::uint8_t PC_LED_EN = 1u << 0;

bool valid_transport(const I2cTransport& transport)
{
    return transport.write != nullptr &&
           transport.read != nullptr &&
           transport.delay_ms != nullptr;
}

bool write_register(
    const I2cTransport& transport,
    std::uint8_t address,
    std::uint8_t reg,
    std::uint8_t value
)
{
    const std::uint8_t message[2] = {reg, value};
    return transport.write(transport.context, address, message, 2);
}

bool read_register(
    const I2cTransport& transport,
    std::uint8_t address,
    std::uint8_t reg,
    std::uint8_t& value
)
{
    if (!transport.write(transport.context, address, &reg, 1)) {
        return false;
    }

    // Match the delay used by the Linux ATTINY panel driver between
    // selecting a register and reading its value.
    transport.delay_ms(transport.context, 5);

    return transport.read(transport.context, address, &value, 1);
}

} // namespace

Result initialize(
    Controller& controller,
    const I2cTransport& transport,
    std::uint8_t address
)
{
    controller.initialized = false;

    if (!valid_transport(transport)) {
        return Result::InvalidTransport;
    }

    if (address > 0x7F) {
        return Result::InvalidAddress;
    }

    controller.transport = transport;
    controller.address = address;

    std::uint8_t controller_id = 0;
    if (!read_register(transport, address, REG_ID, controller_id)) {
        return Result::I2cError;
    }

    if (controller_id != ID_REVISION_1 &&
        controller_id != ID_REVISION_2) {
        return Result::UnsupportedController;
    }

    // Put the controller into its documented startup state.
    if (!write_register(transport, address, REG_POWERON, 0)) {
        return Result::I2cError;
    }
    transport.delay_ms(transport.context, 30);

    if (!write_register(transport, address, REG_PWM, 0)) {
        return Result::I2cError;
    }

    // Reproduce the panel power sequence used by the Linux driver:
    // hold bridge/touch controller reset, set panel orientation,
    // enable the main panel supply, then enable the LED control output.
    if (!write_register(transport, address, REG_PORTC, 0)) {
        return Result::I2cError;
    }
    transport.delay_ms(5);

    if (!write_register(transport, address, REG_PORTA, PA_LCD_LR)) {
        return Result::I2cError;
    }
    transport.delay_ms(5);

    if (!write_register(transport, address, REG_PORTB, PB_LCD_MAIN)) {
        return Result::I2cError;
    }
    transport.delay_ms(5);

    if (!write_register(transport, address, REG_PORTC, PC_LED_EN)) {
        return Result::I2cError;
    }
    transport.delay_ms(80);

    controller.initialized = true;
    return set_brightness(controller, 0xFF);
}

Result set_brightness(Controller& controller, std::uint8_t brightness)
{
    if (!controller.initialized) {
        return Result::NotInitialized;
    }

    if (!write_register(
            controller.transport,
            controller.address,
            REG_PWM,
            brightness)) {
        return Result::I2cError;
    }

    return Result::Ok;
}

} // namespace gameplayos::display::attiny
