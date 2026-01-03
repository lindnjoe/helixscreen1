// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file memory_monitor.h
 * @brief Background thread that samples and logs memory usage
 *
 * Periodically reads /proc/self/status (Linux) and logs RSS, VmSize, etc.
 * at TRACE level. Useful for diagnosing memory spikes and leaks.
 *
 * Usage:
 *   MemoryMonitor::instance().start();  // Start monitoring
 *   MemoryMonitor::instance().stop();   // Stop monitoring
 *
 * Logs appear at TRACE level (-vvv), e.g.:
 *   [MemoryMonitor] RSS=6520kB VmSize=69476kB VmData=60624kB Heap=1234kB
 */

#pragma once

#include <atomic>
#include <chrono>
#include <thread>

namespace helix {

/**
 * @brief Memory usage snapshot
 */
struct MemoryStats {
    size_t vm_size_kb = 0; ///< Virtual memory size (total mapped)
    size_t vm_rss_kb = 0;  ///< Resident set size (actual RAM)
    size_t vm_data_kb = 0; ///< Data + stack
    size_t vm_swap_kb = 0; ///< Swapped out memory
    size_t vm_peak_kb = 0; ///< Peak virtual memory
    size_t vm_hwm_kb = 0;  ///< Peak RSS (high water mark)
};

/**
 * @brief Background memory monitoring thread
 *
 * Singleton that periodically samples memory usage and logs at TRACE level.
 * Only active on Linux (reads /proc/self/status).
 */
class MemoryMonitor {
  public:
    static MemoryMonitor& instance();

    /**
     * @brief Start the monitoring thread
     * @param interval_ms Sampling interval in milliseconds (default: 5000ms)
     */
    void start(int interval_ms = 5000);

    /**
     * @brief Stop the monitoring thread
     */
    void stop();

    /**
     * @brief Check if monitoring is active
     */
    bool is_running() const {
        return running_.load();
    }

    /**
     * @brief Get current memory stats (can be called from any thread)
     */
    static MemoryStats get_current_stats();

    /**
     * @brief Log current memory stats immediately (useful for specific events)
     * @param context Optional context string to include in log
     */
    static void log_now(const char* context = nullptr);

  private:
    MemoryMonitor() = default;
    ~MemoryMonitor();

    MemoryMonitor(const MemoryMonitor&) = delete;
    MemoryMonitor& operator=(const MemoryMonitor&) = delete;

    void monitor_loop();

    std::atomic<bool> running_{false};
    std::atomic<int> interval_ms_{5000};
    std::thread monitor_thread_;
};

} // namespace helix
