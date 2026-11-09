#include "dsi.hpp"

namespace gameplayos::display::dsi {

Result initialize(const PanelConfiguration& configuration)
{
    if (configuration.width == 0 ||
        configuration.height == 0 ||
        configuration.data_lanes == 0 ||
        configuration.data_lanes > 4) {
        return Result::InvalidConfiguration;
    }

    // Hardware initialization is intentionally not attempted yet.
    //
    // Raspberry Pi 5 routes its DSI controller through RP1. The correct
    // register setup, PHY timing, panel command sequence, and power/reset
    // handling depend on the exact display panel and DSI connector.
    // Add those only after the panel model and board connection are known.
    return Result::NotImplemented;
}

} // namespace gameplayos::display::dsi
