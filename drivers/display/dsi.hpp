#pragma once

#include <cstdint>

namespace gameplayos::display::dsi {

enum class Result : std::uint8_t {
    Ok,
    InvalidConfiguration,
    UnsupportedPanel,
    NotImplemented
};

struct PanelConfiguration {
    std::uint32_t width;
    std::uint32_t height;
    std::uint8_t data_lanes;
};

Result initialize(const PanelConfiguration& configuration);

} // namespace gameplayos::display::dsi
