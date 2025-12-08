// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_print_history_api.cpp
 * @brief Unit tests for Print History API (Stage 1 validation)
 *
 * Tests the Moonraker history API implementation:
 * - get_history_list() returns mock jobs with correct structure
 * - get_history_totals() returns aggregate statistics
 * - delete_history_job() removes job from history
 */

#include "../../include/moonraker_api.h"
#include "../../include/moonraker_client_mock.h"
#include "../../include/print_history_data.h"
#include "../../include/printer_state.h"
#include "../../lvgl/lvgl.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "../catch_amalgamated.hpp"

// ============================================================================
// Global LVGL Initialization
// ============================================================================

namespace {
struct LVGLInitializerHistory {
    LVGLInitializerHistory() {
        static bool initialized = false;
        if (!initialized) {
            lv_init();
            lv_display_t* disp = lv_display_create(800, 480);
            alignas(64) static lv_color_t buf[800 * 10];
            lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
            initialized = true;
        }
    }
};

static LVGLInitializerHistory lvgl_init;
} // namespace

// ============================================================================
// Test Fixture
// ============================================================================

class PrintHistoryTestFixture {
  public:
    PrintHistoryTestFixture() : client_(MoonrakerClientMock::PrinterType::VORON_24, 1000.0) {
        printer_state_.init_subjects(false);
        client_.connect("ws://mock/websocket", []() {}, []() {});
        api_ = std::make_unique<MoonrakerAPI>(client_, printer_state_);
    }

    ~PrintHistoryTestFixture() {
        client_.disconnect();
        api_.reset();
    }

  protected:
    MoonrakerClientMock client_;
    PrinterState printer_state_;
    std::unique_ptr<MoonrakerAPI> api_;
};

// ============================================================================
// get_history_list Tests
// ============================================================================

TEST_CASE_METHOD(PrintHistoryTestFixture, "get_history_list returns mock jobs",
                 "[moonraker][history][api]") {
    std::atomic<bool> success_called{false};
    std::atomic<bool> error_called{false};
    std::vector<PrintHistoryJob> captured_jobs;
    uint64_t captured_total = 0;

    api_->get_history_list(
        50, 0, 0.0, 0.0,
        [&](const std::vector<PrintHistoryJob>& jobs, uint64_t total) {
            captured_jobs = jobs;
            captured_total = total;
            success_called.store(true);
        },
        [&](const MoonrakerError&) { error_called.store(true); });

    // Wait for async callback
    for (int i = 0; i < 50 && !success_called.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    REQUIRE(success_called.load());
    REQUIRE_FALSE(error_called.load());
    REQUIRE(captured_jobs.size() > 0);
    REQUIRE(captured_total >= captured_jobs.size());

    // Verify job structure
    const auto& first_job = captured_jobs[0];
    REQUIRE_FALSE(first_job.job_id.empty());
    REQUIRE_FALSE(first_job.filename.empty());
    REQUIRE(first_job.start_time > 0.0);
    REQUIRE_FALSE(first_job.duration_str.empty());
    REQUIRE_FALSE(first_job.date_str.empty());
}

TEST_CASE_METHOD(PrintHistoryTestFixture, "get_history_list jobs have valid status",
                 "[moonraker][history][api]") {
    std::atomic<bool> done{false};
    std::vector<PrintHistoryJob> captured_jobs;

    api_->get_history_list(
        50, 0, 0.0, 0.0,
        [&](const std::vector<PrintHistoryJob>& jobs, uint64_t) {
            captured_jobs = jobs;
            done.store(true);
        },
        [&](const MoonrakerError&) { done.store(true); });

    for (int i = 0; i < 50 && !done.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    REQUIRE(captured_jobs.size() > 0);

    // Check that all jobs have valid status
    for (const auto& job : captured_jobs) {
        REQUIRE(job.status != PrintJobStatus::UNKNOWN);
        // Status should be one of the expected values
        bool valid_status =
            (job.status == PrintJobStatus::COMPLETED || job.status == PrintJobStatus::CANCELLED ||
             job.status == PrintJobStatus::ERROR || job.status == PrintJobStatus::IN_PROGRESS);
        REQUIRE(valid_status);
    }
}

// ============================================================================
// get_history_totals Tests
// ============================================================================

TEST_CASE_METHOD(PrintHistoryTestFixture, "get_history_totals returns statistics",
                 "[moonraker][history][api]") {
    std::atomic<bool> success_called{false};
    std::atomic<bool> error_called{false};
    PrintHistoryTotals captured_totals;

    api_->get_history_totals(
        [&](const PrintHistoryTotals& totals) {
            captured_totals = totals;
            success_called.store(true);
        },
        [&](const MoonrakerError&) { error_called.store(true); });

    for (int i = 0; i < 50 && !success_called.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    REQUIRE(success_called.load());
    REQUIRE_FALSE(error_called.load());

    // Mock should return reasonable statistics
    REQUIRE(captured_totals.total_jobs > 0);
    REQUIRE(captured_totals.total_time > 0);
    REQUIRE(captured_totals.total_filament_used > 0.0);
    REQUIRE(captured_totals.longest_job > 0.0);

    // Note: Real Moonraker doesn't provide breakdown counts (completed/cancelled/failed)
    // These must be calculated client-side from the job list if needed
    // So we don't test for them here
}

// ============================================================================
// delete_history_job Tests
// ============================================================================

TEST_CASE_METHOD(PrintHistoryTestFixture, "delete_history_job calls success callback",
                 "[moonraker][history][api]") {
    std::atomic<bool> success_called{false};
    std::atomic<bool> error_called{false};

    // First get a job ID to delete
    std::string job_id_to_delete;
    api_->get_history_list(
        1, 0, 0.0, 0.0,
        [&](const std::vector<PrintHistoryJob>& jobs, uint64_t) {
            if (!jobs.empty()) {
                job_id_to_delete = jobs[0].job_id;
            }
        },
        [](const MoonrakerError&) {});

    for (int i = 0; i < 50 && job_id_to_delete.empty(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    REQUIRE_FALSE(job_id_to_delete.empty());

    // Now delete it
    api_->delete_history_job(
        job_id_to_delete, [&]() { success_called.store(true); },
        [&](const MoonrakerError&) { error_called.store(true); });

    for (int i = 0; i < 50 && !success_called.load() && !error_called.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    REQUIRE(success_called.load());
    REQUIRE_FALSE(error_called.load());
}
