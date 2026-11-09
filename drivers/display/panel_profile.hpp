#pragma once

#include "dsi.hpp"

namespace gameplayos::display::profiles {

// Freenove FNK0078-family 7-inch IPS DSI touchscreen.
// Confirmed user-provided product listing: 800x480, 5-point capacitive touch.
inline constexpr PanelConfiguration freenove_7inch_800x480 = {
    .width = 800,
    .height = 480,
    .data_lanes = 0 // Unknown from the listing; must be confirmed before DSI init.
};

inline constexpr unsigned freenove_touch_points = 5;

} // namespace gameplayos::display::profiles
