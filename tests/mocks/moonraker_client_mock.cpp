// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "moonraker_client_mock.h"

#include <spdlog/spdlog.h>

MoonrakerClientMock::MoonrakerClientMock() {
    reset();
}

int MoonrakerClientMock::connect(const char* url, std::function<void()> on_connected,
                                 std::function<void()> on_disconnected) {
    spdlog::debug("[MockMR] connect() called: {}", url);
    last_url_ = url;
    connected_callback_ = on_connected;
    disconnected_callback_ = on_disconnected;
    return 0; // Success
}

int MoonrakerClientMock::send_jsonrpc(const std::string& method, const json& params) {
    (void)params; // Not used in mock
    spdlog::debug("[MockMR] send_jsonrpc() called: {}", method);
    rpc_methods_.push_back(method);

    // Track identification state like real client
    if (method == "server.connection.identify") {
        identified_ = true;
    }
    return 0;
}

int MoonrakerClientMock::send_jsonrpc(const std::string& method, const json& params,
                                      std::function<void(json&)> cb) {
    (void)params; // Not used in mock
    (void)cb;     // Could store for later triggering, but not needed yet
    spdlog::debug("[MockMR] send_jsonrpc() with callback called: {}", method);
    rpc_methods_.push_back(method);

    // Track identification state like real client
    if (method == "server.connection.identify") {
        identified_ = true;
    }
    return 0;
}

int MoonrakerClientMock::gcode_script(const std::string& gcode) {
    spdlog::debug("[MockMR] gcode_script() called: {}", gcode);
    return 0;
}

void MoonrakerClientMock::discover_printer(std::function<void()> on_complete) {
    (void)on_complete; // Could trigger for testing, but not needed yet
    spdlog::debug("[MockMR] discover_printer() called");
    // No-op for now
}

void MoonrakerClientMock::trigger_connected() {
    spdlog::debug("[MockMR] trigger_connected() - simulating successful connection");
    connected_ = true;
    if (connected_callback_) {
        connected_callback_();
    }
}

void MoonrakerClientMock::trigger_disconnected() {
    spdlog::debug("[MockMR] trigger_disconnected() - simulating connection failure");
    connected_ = false;
    identified_ = false; // Reset like real client does on disconnect
    if (disconnected_callback_) {
        disconnected_callback_();
    }
}

void MoonrakerClientMock::reset() {
    spdlog::debug("[MockMR] reset() - clearing all mock state");
    connected_callback_ = nullptr;
    disconnected_callback_ = nullptr;
    last_url_.clear();
    rpc_methods_.clear();
    connected_ = false;
    identified_ = false;
}
