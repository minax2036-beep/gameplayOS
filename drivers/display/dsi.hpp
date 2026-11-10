#pragma once

#include <cstdint>

namespace gameplayos::display::dsi {

enum class Result : std::uint8_t {
    Ok,
    InvalidConfiguration,
    UnsupportedPanel,
    NotImplemented
};

// Display timings are expressed in pixel-clock cycles.
struct DisplayMode {
    std::uint32_t pixel_clock_hz;
    std::uint32_t hdisplay;
    std::uint32_t hsync_start;
    std::uint32_t hsync_end;
    std::uint32_t htotal;
    std::uint32_t vdisplay;
    std::uint32_t vsync_start;
    std::uint32_t vsync_end;
    std::uint32_t vtotal;
};

struct PanelConfiguration {
    std::uint32_t width;
    std::uint32_t height;
    std::uint8_t data_lanes;
    DisplayMode mode;
};

// Timing observed from Ubuntu's active DRM mode on the Raspberry Pi 5
// and the connected 800x480 DSI panel.
const PanelConfiguration& reference_800x480_panel();

bool is_valid_configuration(const PanelConfiguration& configuration);

// Hardware programming is deliberately not performed yet. This entry point
// remains a safe placeholder until RP1 DSI register definitions, PHY setup,
// TC358762 programming, and panel power sequencing are verified.
Result initialize(const PanelConfiguration& configuration);

} // namespace gameplayos::display::dsi
