#include "dsi.hpp"

namespace gameplayos::display::dsi {

namespace {

// Values decoded from the active Linux DRM mode:
// clock=30 MHz, horizontal total=978, vertical total=511.
// The resulting refresh rate is approximately 60.03 Hz.
constexpr PanelConfiguration REFERENCE_PANEL{
    800,
    480,
    1,
    {
        30000000,
        800,
        931,
        933,
        978,
        480,
        487,
        489,
        511
    }
};

} // namespace

const PanelConfiguration& reference_800x480_panel()
{
    return REFERENCE_PANEL;
}

bool is_valid_configuration(const PanelConfiguration& configuration)
{
    const DisplayMode& mode = configuration.mode;

    if (configuration.width == 0 ||
        configuration.height == 0 ||
        configuration.data_lanes == 0 ||
        configuration.data_lanes > 4 ||
        mode.pixel_clock_hz == 0) {
        return false;
    }

    if (mode.hdisplay != configuration.width ||
        mode.vdisplay != configuration.height) {
        return false;
    }

    if (mode.hsync_start < mode.hdisplay ||
        mode.hsync_end <= mode.hsync_start ||
        mode.htotal <= mode.hsync_end) {
        return false;
    }

    if (mode.vsync_start < mode.vdisplay ||
        mode.vsync_end <= mode.vsync_start ||
        mode.vtotal <= mode.vsync_end) {
        return false;
    }

    return true;
}

Result initialize(const PanelConfiguration& configuration)
{
    if (!is_valid_configuration(configuration)) {
        return Result::InvalidConfiguration;
    }

    // Do not write guessed values to RP1 or TC358762 registers.
    // Linux reports the DSI host, bridge, panel, and display mode as active,
    // but this is not enough to infer the complete power-on and PHY sequence.
    // Hardware initialization will be added after those sequences are
    // verified against the matching Linux driver and panel documentation.
    return Result::NotImplemented;
}

} // namespace gameplayos::display::dsi
