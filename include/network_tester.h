// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file network_tester.h
 * @brief Async network connectivity testing
 *
 * Tests network connectivity by pinging gateway and internet hosts.
 * Uses background thread to avoid blocking the UI during network operations.
 */

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

/**
 * @brief Network connectivity tester
 *
 * Performs async network connectivity tests:
 * 1. Gateway reachability (ping default gateway)
 * 2. Internet connectivity (ping 8.8.8.8 and 1.1.1.1)
 *
 * Usage:
 * @code
 * auto tester = std::make_shared<NetworkTester>();
 * tester->init_self_reference(tester);
 * tester->start_test([](NetworkTester::TestState state, const NetworkTester::TestResult& result) {
 *     if (state == NetworkTester::TestState::COMPLETED) {
 *         spdlog::info("Gateway: {}, Internet: {}", result.gateway_ok, result.internet_ok);
 *     }
 * });
 * @endcode
 */
class NetworkTester {
  public:
    /**
     * @brief Test execution state
     */
    enum class TestState {
        IDLE,             ///< No test running
        TESTING_GATEWAY,  ///< Testing gateway connectivity
        TESTING_INTERNET, ///< Testing internet connectivity
        COMPLETED,        ///< Test completed successfully
        FAILED            ///< Test failed with error
    };

    /**
     * @brief Test results
     */
    struct TestResult {
        bool gateway_ok = false;   ///< Gateway is reachable
        bool internet_ok = false;  ///< Internet is reachable
        std::string gateway_ip;    ///< Default gateway IP address
        std::string error_message; ///< Error message if test failed
    };

    /**
     * @brief Callback invoked on state changes
     * @param state Current test state
     * @param result Test results (partial during execution, final on COMPLETED/FAILED)
     */
    using Callback = std::function<void(TestState, const TestResult&)>;

    /**
     * @brief Constructor
     */
    NetworkTester();

    /**
     * @brief Destructor - ensures thread cleanup
     */
    ~NetworkTester();

    /**
     * @brief Initialize self-reference for async callback safety
     *
     * MUST be called immediately after construction when using shared_ptr.
     * Enables async callbacks to safely check if tester still exists.
     *
     * @param self Shared pointer to this NetworkTester instance
     */
    void init_self_reference(std::shared_ptr<NetworkTester> self);

    /**
     * @brief Start async connectivity test
     *
     * Spawns background thread to test gateway and internet connectivity.
     * Callback is invoked on LVGL thread for each state change.
     *
     * @param callback Function to call on state changes
     */
    void start_test(Callback callback);

    /**
     * @brief Cancel running test
     *
     * Signals test thread to stop and blocks until thread exits.
     */
    void cancel();

    /**
     * @brief Check if test is currently running
     * @return true if test is in progress
     */
    bool is_running() const;

  private:
    // Self-reference for async callback safety
    std::weak_ptr<NetworkTester> self_;

    // Thread state
    std::atomic<bool> running_{false};
    std::atomic<bool> cancelled_{false};
    std::thread worker_thread_;

    // Test state
    Callback callback_;
    TestResult result_;

    /**
     * @brief Worker thread entry point
     */
    void run_test();

    /**
     * @brief Report state change to callback (thread-safe)
     * @param state New test state
     */
    void report_state(TestState state);

    /**
     * @brief Get default gateway IP address
     * @return Gateway IP string, or empty on error
     */
    std::string get_default_gateway();

    /**
     * @brief Ping a host
     * @param host IP address or hostname to ping
     * @param timeout_sec Timeout in seconds
     * @return true if host responded
     */
    bool ping_host(const std::string& host, int timeout_sec = 2);
};
