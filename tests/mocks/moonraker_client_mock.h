// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MOONRAKER_CLIENT_MOCK_H
#define MOONRAKER_CLIENT_MOCK_H

#include <functional>
#include <string>
#include <vector>

#include "hv/json.hpp"

using json = nlohmann::json;

/**
 * @brief Mock MoonrakerClient for testing wizard connection flow
 *
 * Simulates WebSocket connection behavior without real network I/O.
 * Allows tests to trigger connection success/failure and verify URL.
 */
class MoonrakerClientMock {
  public:
    MoonrakerClientMock();
    ~MoonrakerClientMock() = default;

    /**
     * @brief Mock connection attempt (stores callbacks, does not connect)
     *
     * @param url WebSocket URL to connect to (stored for verification)
     * @param on_connected Callback to invoke when connection succeeds
     * @param on_disconnected Callback to invoke when connection fails/closes
     * @return Always returns 0 (success)
     */
    int connect(const char* url, std::function<void()> on_connected,
                std::function<void()> on_disconnected);

    /**
     * @brief Mock send_jsonrpc (no-op, stores method for verification)
     */
    int send_jsonrpc(const std::string& method, const json& params);

    /**
     * @brief Mock send_jsonrpc with callback
     */
    int send_jsonrpc(const std::string& method, const json& params, std::function<void(json&)> cb);

    /**
     * @brief Mock gcode_script (no-op)
     */
    int gcode_script(const std::string& gcode);

    /**
     * @brief Mock discover_printer (no-op)
     */
    void discover_printer(std::function<void()> on_complete);

    /**
     * @brief Check if connected
     */
    bool is_connected() const {
        return connected_;
    }

    // Test control methods

    /**
     * @brief Simulate successful connection (triggers on_connected callback)
     */
    void trigger_connected();

    /**
     * @brief Simulate connection failure (triggers on_disconnected callback)
     */
    void trigger_disconnected();

    /**
     * @brief Get the last URL passed to connect()
     */
    std::string get_last_connect_url() const {
        return last_url_;
    }

    /**
     * @brief Get all RPC methods called via send_jsonrpc
     */
    const std::vector<std::string>& get_rpc_methods() const {
        return rpc_methods_;
    }

    /**
     * @brief Reset mock state (clears callbacks, URL, methods)
     */
    void reset();

    /**
     * @brief Check if client has been identified to Moonraker
     * @return True if server.connection.identify has been sent
     */
    bool is_identified() const {
        return identified_;
    }

    /**
     * @brief Reset identification state (called on disconnect)
     */
    void reset_identified() {
        identified_ = false;
    }

  private:
    std::function<void()> connected_callback_;
    std::function<void()> disconnected_callback_;
    std::string last_url_;
    std::vector<std::string> rpc_methods_;
    bool connected_ = false;
    bool identified_ = false; // Tracks if server.connection.identify was sent
};

#endif // MOONRAKER_CLIENT_MOCK_H
