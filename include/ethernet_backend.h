// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>

/**
 * @brief Ethernet connection information
 */
struct EthernetInfo {
    bool connected;          ///< True if interface is up with valid IP
    std::string interface;   ///< Interface name (e.g., "eth0", "en0")
    std::string ip_address;  ///< IPv4 address (e.g., "192.168.1.100")
    std::string mac_address; ///< MAC address (e.g., "aa:bb:cc:dd:ee:ff")
    std::string status;      ///< Human-readable status ("Connected", "No cable", "Unknown")

    EthernetInfo()
        : connected(false), interface(""), ip_address(""), mac_address(""), status("Unknown") {}
};

/**
 * @brief Abstract Ethernet backend interface
 *
 * Provides a clean, platform-agnostic API for querying Ethernet status.
 * Concrete implementations handle platform-specific details:
 * - EthernetBackendMacOS: macOS native APIs + libhv ifconfig
 * - EthernetBackendLinux: Linux /sys/class/net + libhv ifconfig
 * - EthernetBackendMock: Simulator mode with fake data
 *
 * Design principles:
 * - Query-only API (no enable/disable, no configuration)
 * - Synchronous operations (no async complexity)
 * - Simple status checking for UI display
 * - Clean error handling with meaningful messages
 */
class EthernetBackend {
  public:
    virtual ~EthernetBackend() = default;

    // ========================================================================
    // Status Queries
    // ========================================================================

    /**
     * @brief Check if any Ethernet interface exists
     *
     * Returns true if hardware is detected, regardless of connection status.
     *
     * @return true if at least one Ethernet interface is present
     */
    virtual bool has_interface() = 0;

    /**
     * @brief Get detailed Ethernet connection information
     *
     * Returns comprehensive status including IP address, MAC, and link state.
     * If multiple Ethernet interfaces exist, returns info for the first
     * connected interface, or first interface if none connected.
     *
     * @return EthernetInfo struct with current state
     */
    virtual EthernetInfo get_info() = 0;

    // ========================================================================
    // Factory Methods
    // ========================================================================

    /**
     * @brief Create appropriate backend for current platform
     *
     * Tries platform-specific backend first, falls back to mock on failure:
     * - macOS: EthernetBackendMacOS → EthernetBackendMock (fallback)
     * - Linux: EthernetBackendLinux → EthernetBackendMock (fallback)
     *
     * @return Unique pointer to backend instance
     */
    static std::unique_ptr<EthernetBackend> create();
};
