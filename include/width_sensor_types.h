// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

namespace helix::sensors {

/// @brief Role assigned to a width sensor
enum class WidthSensorRole {
    NONE = 0,              ///< Discovered but not assigned to a role
    FLOW_COMPENSATION = 1, ///< Used for flow rate compensation based on filament diameter
};

/// @brief Type of width sensor hardware
enum class WidthSensorType {
    TSL1401CL = 1, ///< TSL1401CL linear array sensor
    HALL = 2,      ///< Hall effect based sensor
};

/// @brief Configuration for a width sensor
struct WidthSensorConfig {
    std::string klipper_name; ///< Full Klipper name (e.g., "tsl1401cl_filament_width_sensor")
    std::string sensor_name;  ///< Short name (e.g., "tsl1401cl")
    WidthSensorType type = WidthSensorType::TSL1401CL;
    WidthSensorRole role = WidthSensorRole::NONE;
    bool enabled = true;

    WidthSensorConfig() = default;

    WidthSensorConfig(std::string klipper_name_, std::string sensor_name_, WidthSensorType type_)
        : klipper_name(std::move(klipper_name_)), sensor_name(std::move(sensor_name_)),
          type(type_) {}
};

/// @brief Runtime state for a width sensor
struct WidthSensorState {
    float diameter = 0.0f;  ///< Measured filament diameter in mm
    float raw_value = 0.0f; ///< Raw sensor value
    bool available = false; ///< Sensor available in current config
};

/// @brief Convert role enum to config string
/// @param role The role to convert
/// @return Config-safe string for JSON storage
[[nodiscard]] inline std::string width_role_to_string(WidthSensorRole role) {
    switch (role) {
    case WidthSensorRole::NONE:
        return "none";
    case WidthSensorRole::FLOW_COMPENSATION:
        return "flow_compensation";
    default:
        return "none";
    }
}

/// @brief Parse role string to enum
/// @param str The config string to parse
/// @return Parsed role, or NONE if unrecognized
[[nodiscard]] inline WidthSensorRole width_role_from_string(const std::string& str) {
    if (str == "flow_compensation")
        return WidthSensorRole::FLOW_COMPENSATION;
    return WidthSensorRole::NONE;
}

/// @brief Convert role to display string
/// @param role The role to convert
/// @return Human-readable role name for UI display
[[nodiscard]] inline std::string width_role_to_display_string(WidthSensorRole role) {
    switch (role) {
    case WidthSensorRole::NONE:
        return "Unassigned";
    case WidthSensorRole::FLOW_COMPENSATION:
        return "Flow Compensation";
    default:
        return "Unassigned";
    }
}

/// @brief Convert type enum to config string
/// @param type The type to convert
/// @return Config-safe string
[[nodiscard]] inline std::string width_type_to_string(WidthSensorType type) {
    switch (type) {
    case WidthSensorType::TSL1401CL:
        return "tsl1401cl";
    case WidthSensorType::HALL:
        return "hall";
    default:
        return "tsl1401cl";
    }
}

/// @brief Parse type string to enum
/// @param str The config string to parse
/// @return Parsed type, defaults to TSL1401CL if unrecognized
[[nodiscard]] inline WidthSensorType width_type_from_string(const std::string& str) {
    if (str == "hall")
        return WidthSensorType::HALL;
    return WidthSensorType::TSL1401CL;
}

} // namespace helix::sensors
