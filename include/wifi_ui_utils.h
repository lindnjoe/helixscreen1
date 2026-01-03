// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

/**
 * @brief WiFi UI utilities namespace
 *
 * Shared utility functions for WiFi UI components (wizard, settings panel, etc.)
 * Provides signal strength icon calculations and device information.
 */
namespace helix {
namespace ui {
namespace wifi {

/**
 * @brief Compute signal icon state from signal strength and security status
 *
 * Returns a state value 1-8 for use with multi-state signal icon images.
 * This enables reactive UI bindings where icon visibility is controlled by
 * comparing the current state to a reference value.
 *
 * State mapping:
 * - 1-4: Open networks (weak to strong signal)
 * - 5-8: Secured networks (weak to strong signal)
 *
 * Signal strength thresholds:
 * - State 1/5: 0-25% (weak)
 * - State 2/6: 26-50% (fair)
 * - State 3/7: 51-75% (good)
 * - State 4/8: 76-100% (excellent)
 *
 * @param strength_percent Signal strength as percentage (0-100)
 * @param secured Whether network is password-protected
 * @return State value 1-8 for icon visibility binding
 *
 * @example
 * int state = wifi_compute_signal_icon_state(75, true);  // Returns 7 (secured, good signal)
 * lv_obj_bind_flag_if_not_eq(icon_obj, state_subject, LV_OBJ_FLAG_HIDDEN, state);
 */
int wifi_compute_signal_icon_state(int strength_percent, bool secured);

/**
 * @brief Get device MAC address for specified network interface
 *
 * Retrieves the hardware MAC address from the system in formatted form.
 *
 * Platform-specific implementation:
 * - Linux: Reads from /sys/class/net/{interface}/address
 * - macOS: Parses `ifconfig {interface}` output for ether line
 *
 * @param interface Network interface name (default: "wlan0")
 * @return Formatted MAC address (e.g., "50:41:1C:XX:XX:XX") or empty string on error
 *
 * @example
 * std::string mac = wifi_get_device_mac("wlan0");
 * if (!mac.empty()) {
 *     spdlog::info("Device MAC: {}", mac);
 * }
 */
std::string wifi_get_device_mac(const std::string& interface = "wlan0");

} // namespace wifi
} // namespace ui
} // namespace helix
