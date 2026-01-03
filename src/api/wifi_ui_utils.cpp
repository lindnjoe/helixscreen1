// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "wifi_ui_utils.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>

#ifdef __APPLE__
#include <array>
#include <memory>
#endif

namespace helix {
namespace ui {
namespace wifi {

int wifi_compute_signal_icon_state(int strength_percent, bool secured) {
    // Clamp to valid range
    strength_percent = std::max(0, std::min(100, strength_percent));

    // Determine base state from signal strength (1-4)
    int base_state;
    if (strength_percent <= 25) {
        base_state = 1; // Weak
    } else if (strength_percent <= 50) {
        base_state = 2; // Fair
    } else if (strength_percent <= 75) {
        base_state = 3; // Good
    } else {
        base_state = 4; // Excellent
    }

    // Add 4 for secured networks (1-4 = open, 5-8 = secured)
    return secured ? base_state + 4 : base_state;
}

std::string wifi_get_device_mac(const std::string& interface) {
#ifdef __APPLE__
    // macOS: Parse ifconfig output for ether line
    std::string command = "ifconfig " + interface + " 2>/dev/null";
    std::array<char, 256> buffer;
    std::string result;

    // Use popen to execute command and read output
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        spdlog::error("[wifi_ui] Failed to execute ifconfig for interface '{}'", interface);
        return "";
    }

    // Read command output line by line
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    // Parse for "ether XX:XX:XX:XX:XX:XX" line
    const std::string ether_prefix = "ether ";
    size_t ether_pos = result.find(ether_prefix);
    if (ether_pos == std::string::npos) {
        spdlog::debug("[wifi_ui] No ether address found for interface '{}'", interface);
        return "";
    }

    // Extract MAC address (format: XX:XX:XX:XX:XX:XX)
    size_t mac_start = ether_pos + ether_prefix.length();
    size_t mac_end = result.find_first_of(" \t\n", mac_start);
    if (mac_end == std::string::npos) {
        mac_end = result.length();
    }

    std::string mac = result.substr(mac_start, mac_end - mac_start);

    // Remove trailing whitespace
    mac.erase(
        std::find_if(mac.rbegin(), mac.rend(), [](unsigned char ch) { return !std::isspace(ch); })
            .base(),
        mac.end());

    spdlog::debug("[wifi_ui] Found MAC address for '{}': {}", interface, mac);
    return mac;

#else
    // Linux: Read from /sys/class/net/{interface}/address
    std::string path = "/sys/class/net/" + interface + "/address";
    std::ifstream file(path);

    if (!file.is_open()) {
        spdlog::debug("[wifi_ui] Failed to open {} (interface may not exist)", path);
        return "";
    }

    std::string mac;
    std::getline(file, mac);

    // Remove trailing newline/whitespace
    mac.erase(
        std::find_if(mac.rbegin(), mac.rend(), [](unsigned char ch) { return !std::isspace(ch); })
            .base(),
        mac.end());

    if (mac.empty()) {
        spdlog::error("[wifi_ui] MAC address file {} is empty", path);
        return "";
    }

    spdlog::debug("[wifi_ui] Found MAC address for '{}': {}", interface, mac);
    return mac;
#endif
}

} // namespace wifi
} // namespace ui
} // namespace helix
