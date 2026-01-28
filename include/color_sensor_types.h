// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

namespace helix::sensors {

/// @brief Role assigned to a color sensor
enum class ColorSensorRole {
    NONE = 0,           ///< Discovered but not assigned to a role
    FILAMENT_COLOR = 1, ///< Used for detecting filament color
};

/// @brief Configuration for a color sensor (TD-1 device)
struct ColorSensorConfig {
    std::string device_id;   ///< Device ID (e.g., "td1_lane0")
    std::string sensor_name; ///< Display name (e.g., "TD-1 Lane 0")
    ColorSensorRole role = ColorSensorRole::NONE;
    bool enabled = true;

    ColorSensorConfig() = default;

    ColorSensorConfig(std::string device_id_, std::string sensor_name_)
        : device_id(std::move(device_id_)), sensor_name(std::move(sensor_name_)) {}
};

/// @brief Runtime state for a color sensor
struct ColorSensorState {
    std::string color_hex;              ///< Detected color as "#RRGGBB"
    float transmission_distance = 0.0f; ///< TD value from sensor
    bool available = false;             ///< Sensor available in current config
};

/// @brief Convert role enum to config string
/// @param role The role to convert
/// @return Config-safe string for JSON storage
[[nodiscard]] inline std::string color_role_to_string(ColorSensorRole role) {
    switch (role) {
    case ColorSensorRole::NONE:
        return "none";
    case ColorSensorRole::FILAMENT_COLOR:
        return "filament_color";
    default:
        return "none";
    }
}

/// @brief Parse role string to enum
/// @param str The config string to parse
/// @return Parsed role, or NONE if unrecognized
[[nodiscard]] inline ColorSensorRole color_role_from_string(const std::string& str) {
    if (str == "filament_color")
        return ColorSensorRole::FILAMENT_COLOR;
    return ColorSensorRole::NONE;
}

/// @brief Convert role to display string
/// @param role The role to convert
/// @return Human-readable role name for UI display
[[nodiscard]] inline std::string color_role_to_display_string(ColorSensorRole role) {
    switch (role) {
    case ColorSensorRole::NONE:
        return "Unassigned";
    case ColorSensorRole::FILAMENT_COLOR:
        return "Filament Color";
    default:
        return "Unassigned";
    }
}

} // namespace helix::sensors
