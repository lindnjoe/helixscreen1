// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_active_print_media_manager.cpp
 * @brief Unit tests for ActivePrintMediaManager class
 *
 * Tests the media manager that:
 * - Observes print_filename_ subject from PrinterState
 * - Processes raw filename to display name
 * - Loads thumbnails via MoonrakerAPI
 * - Updates print_display_filename_ and print_thumbnail_path_ subjects
 * - Uses generation counter for stale callback detection
 *
 * TEST-FIRST: Implementation follows these tests.
 */

#include "printer_state.h"

#include <spdlog/spdlog.h>

#include <atomic>
#include <functional>
#include <string>

#include "../catch_amalgamated.hpp"

using json = nlohmann::json;

// ============================================================================
// Test Fixture for ActivePrintMediaManager tests
// ============================================================================

class ActivePrintMediaManagerTestFixture {
  public:
    ActivePrintMediaManagerTestFixture() {
        // Initialize LVGL once (static guard)
        static bool lvgl_initialized = false;
        if (!lvgl_initialized) {
            lv_init();
            lvgl_initialized = true;
        }

        // Create a headless display for testing
        if (!display_created_) {
            display_ = lv_display_create(480, 320);
            alignas(64) static lv_color_t buf[480 * 10];
            lv_display_set_buffers(display_, buf, nullptr, sizeof(buf),
                                   LV_DISPLAY_RENDER_MODE_PARTIAL);
            lv_display_set_flush_cb(display_, [](lv_display_t* disp, const lv_area_t*, uint8_t*) {
                lv_display_flush_ready(disp);
            });
            display_created_ = true;
        }

        // Reset PrinterState for test isolation
        state_.reset_for_testing();

        // Initialize subjects (without XML registration in tests)
        state_.init_subjects(false);
    }

    ~ActivePrintMediaManagerTestFixture() {
        // Reset after each test
        state_.reset_for_testing();
    }

  protected:
    PrinterState& state() {
        return state_;
    }

    // Helper to update print filename via status JSON (simulates Moonraker notification)
    void set_print_filename(const std::string& filename) {
        json status = {{"print_stats", {{"filename", filename}}}};
        state_.update_from_status(status);
    }

    // Get current print_filename (raw)
    std::string get_print_filename() {
        return lv_subject_get_string(state_.get_print_filename_subject());
    }

    // Get current print_display_filename (processed for UI)
    std::string get_display_filename() {
        return lv_subject_get_string(state_.get_print_display_filename_subject());
    }

    // Get current print_thumbnail_path
    std::string get_thumbnail_path() {
        return lv_subject_get_string(state_.get_print_thumbnail_path_subject());
    }

  private:
    PrinterState state_;
    static lv_display_t* display_;
    static bool display_created_;
};

lv_display_t* ActivePrintMediaManagerTestFixture::display_ = nullptr;
bool ActivePrintMediaManagerTestFixture::display_created_ = false;

// ============================================================================
// Display Name Formatting Tests
// ============================================================================

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: simple filename produces correct display name",
                 "[media][display_name]") {
    // When ActivePrintMediaManager observes a simple filename like "benchy.gcode",
    // it should produce a display name like "benchy" (no path, no extension)
    set_print_filename("benchy.gcode");

    // After the manager processes the filename, display_filename should be set
    // NOTE: This test documents expected behavior. Implementation will update
    // print_display_filename_ subject when it observes print_filename_ changes.
    //
    // For now, we test the raw filename is set and expect implementation to add
    // the display filename processing.
    REQUIRE(get_print_filename() == "benchy.gcode");

    // TODO: When ActivePrintMediaManager is implemented, uncomment:
    // REQUIRE(get_display_filename() == "benchy");
}

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: filename with path produces correct display name",
                 "[media][display_name]") {
    // Moonraker can report paths like "subfolder/benchy.gcode"
    set_print_filename("my_models/benchy.gcode");

    REQUIRE(get_print_filename() == "my_models/benchy.gcode");

    // After processing, should just be "benchy"
    // TODO: When ActivePrintMediaManager is implemented, uncomment:
    // REQUIRE(get_display_filename() == "benchy");
}

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: helix_temp filename resolves to original",
                 "[media][display_name]") {
    // When HelixScreen modifies G-code, it creates temp files like:
    // .helix_temp/modified_1234567890_Original_Model.gcode
    // The display name should show "Original_Model", not the temp filename
    set_print_filename(".helix_temp/modified_1234567890_Body1.gcode");

    REQUIRE(get_print_filename() == ".helix_temp/modified_1234567890_Body1.gcode");

    // After processing, should resolve to original name
    // TODO: When ActivePrintMediaManager is implemented, uncomment:
    // REQUIRE(get_display_filename() == "Body1");
}

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: complex helix_temp path resolves correctly",
                 "[media][display_name]") {
    // Test with a more complex original filename
    set_print_filename(".helix_temp/modified_9876543210_My_Cool_Print.gcode");

    // TODO: When ActivePrintMediaManager is implemented, uncomment:
    // REQUIRE(get_display_filename() == "My_Cool_Print");
}

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: deeply nested path produces correct display name",
                 "[media][display_name]") {
    // Test with deeply nested path
    set_print_filename("projects/2025/january/test_models/benchy_0.2mm_PLA.gcode");

    REQUIRE(get_print_filename() == "projects/2025/january/test_models/benchy_0.2mm_PLA.gcode");

    // TODO: When ActivePrintMediaManager is implemented, uncomment:
    // REQUIRE(get_display_filename() == "benchy_0.2mm_PLA");
}

// ============================================================================
// Empty Filename Handling Tests
// ============================================================================

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: empty filename clears display name", "[media][empty]") {
    // First set a filename
    set_print_filename("test.gcode");
    REQUIRE(get_print_filename() == "test.gcode");

    // Then clear it (printer goes to standby)
    set_print_filename("");
    REQUIRE(get_print_filename() == "");

    // Display filename should also be cleared
    // TODO: When ActivePrintMediaManager is implemented, uncomment:
    // REQUIRE(get_display_filename() == "");
}

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: empty filename clears thumbnail path",
                 "[media][empty]") {
    // Set a thumbnail path (simulating a loaded thumbnail)
    state().set_print_thumbnail_path("A:/tmp/thumbnail_abc123.bin");
    REQUIRE(get_thumbnail_path() == "A:/tmp/thumbnail_abc123.bin");

    // When filename is cleared, thumbnail path should also be cleared
    set_print_filename("");

    // TODO: When ActivePrintMediaManager is implemented, uncomment:
    // REQUIRE(get_thumbnail_path() == "");
}

// ============================================================================
// Thumbnail Source Override Tests
// ============================================================================

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: manual thumbnail source takes precedence",
                 "[media][thumbnail][override]") {
    // When PrintSelectPanel starts a print, it may have already loaded the thumbnail
    // and can provide it directly via set_thumbnail_source() to avoid redundant loading
    //
    // This tests that the manual thumbnail path is used instead of triggering a load

    // Set the print filename (normally would trigger thumbnail load)
    set_print_filename("my_print.gcode");

    // If a thumbnail source was set before the filename, it should be used
    // TODO: When ActivePrintMediaManager is implemented:
    // manager.set_thumbnail_source("/tmp/already_loaded_thumb.bin");
    // set_print_filename("my_print.gcode");
    // REQUIRE(get_thumbnail_path() == "A:/tmp/already_loaded_thumb.bin");
    // // And verify no API call was made (need mock API for this)
}

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: thumbnail source override is one-shot",
                 "[media][thumbnail][override]") {
    // The thumbnail source override should only apply to the next filename change
    // Subsequent filename changes should trigger normal thumbnail loading

    // TODO: When ActivePrintMediaManager is implemented:
    // manager.set_thumbnail_source("/tmp/override_thumb.bin");
    // set_print_filename("first_print.gcode");
    // REQUIRE(get_thumbnail_path() contains "override_thumb.bin");

    // // Second print should NOT use the override
    // set_print_filename("second_print.gcode");
    // // Thumbnail should be loaded via API (or empty if mock API returns nothing)
}

// ============================================================================
// Generation Counter / Stale Callback Detection Tests
// ============================================================================

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: rapid filename changes use latest generation",
                 "[media][generation]") {
    // When filename changes rapidly (user quickly switches prints),
    // callbacks from earlier requests should be ignored.
    //
    // This tests the generation counter mechanism.

    // Simulate rapid filename changes
    set_print_filename("print1.gcode");
    set_print_filename("print2.gcode");
    set_print_filename("print3.gcode");

    // Only print3 should be reflected in the display name
    REQUIRE(get_print_filename() == "print3.gcode");

    // TODO: When ActivePrintMediaManager is implemented:
    // The manager should have incremented its generation counter 3 times.
    // If a thumbnail callback arrives for print1 or print2 (stale generations),
    // it should be ignored.
    //
    // To test this properly, we need to:
    // 1. Capture the callback from first API call
    // 2. Change filename twice more
    // 3. Invoke the stale callback
    // 4. Verify thumbnail path is NOT updated with stale data
}

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: stale thumbnail callback is ignored",
                 "[media][generation][stale]") {
    // More explicit test for stale callback detection

    // This requires a mock API that captures callbacks and lets us invoke them later
    // The implementation will use an atomic generation counter:
    //
    // uint32_t request_generation = generation_.fetch_add(1) + 1;
    // api->get_thumbnail(..., [this, request_generation](data) {
    //     if (request_generation != generation_.load()) {
    //         return; // Stale callback, ignore
    //     }
    //     // Process thumbnail
    // });

    // TODO: Implement with mock API when ActivePrintMediaManager is created
}

// ============================================================================
// Integration with PrinterState Subjects
// ============================================================================

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: updates print_display_filename subject",
                 "[media][subjects]") {
    // Verify that the manager updates the correct subject in PrinterState
    set_print_filename("test_model.gcode");

    // The print_display_filename_ subject in PrinterState should be updated
    // TODO: When ActivePrintMediaManager is implemented:
    // REQUIRE(get_display_filename() == "test_model");
}

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: updates print_thumbnail_path subject",
                 "[media][subjects]") {
    // Verify that loaded thumbnails update the correct subject

    // TODO: When ActivePrintMediaManager is implemented with mock API:
    // Mock API returns thumbnail at "/tmp/thumb.bin"
    // set_print_filename("model_with_thumbnail.gcode");
    // Wait for async thumbnail load
    // REQUIRE(get_thumbnail_path() == "A:/tmp/thumb.bin");
}

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: observer fires on display_filename change",
                 "[media][subjects][observer]") {
    // Verify observers on print_display_filename_ are notified

    int observer_count = 0;
    auto observer_cb = [](lv_observer_t* observer, lv_subject_t*) {
        int* count = static_cast<int*>(lv_observer_get_user_data(observer));
        (*count)++;
    };

    lv_subject_add_observer(state().get_print_display_filename_subject(), observer_cb,
                            &observer_count);

    // Initial observer registration fires once
    REQUIRE(observer_count == 1);

    // Change filename - should fire observer after processing
    // TODO: When ActivePrintMediaManager is implemented:
    // set_print_filename("new_model.gcode");
    // REQUIRE(observer_count == 2);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: handles filename with special characters",
                 "[media][edge_case]") {
    // Test filenames with spaces, unicode, etc.
    set_print_filename("My Model (v2) - Final.gcode");

    REQUIRE(get_print_filename() == "My Model (v2) - Final.gcode");

    // TODO: When ActivePrintMediaManager is implemented:
    // REQUIRE(get_display_filename() == "My Model (v2) - Final");
}

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: handles very long filename", "[media][edge_case]") {
    // Test truncation or handling of very long filenames
    std::string long_name(200, 'x');
    long_name += ".gcode";

    set_print_filename(long_name);

    // The raw filename should be stored (up to buffer limits)
    // Display filename processing should handle this gracefully
}

TEST_CASE_METHOD(ActivePrintMediaManagerTestFixture,
                 "ActivePrintMediaManager: thumbnail load failure is handled gracefully",
                 "[media][error]") {
    // When thumbnail loading fails (file not found, network error, etc.),
    // the thumbnail path should remain empty (or cleared)

    // TODO: When ActivePrintMediaManager is implemented with mock API:
    // Configure mock to return error on thumbnail request
    // set_print_filename("model_without_thumbnail.gcode");
    // REQUIRE(get_thumbnail_path() == "");  // Graceful failure
}
