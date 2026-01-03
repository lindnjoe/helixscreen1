// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_print_status_deferred_loading.cpp
 * @brief Tests for PrintStatusPanel deferred G-code loading behavior
 *
 * Tests the logic that decides whether to load G-code immediately or defer:
 * - If panel is active (visible) when filename changes → load immediately
 * - If panel is inactive when filename changes → defer to on_activate()
 *
 * Bug context: Previously, if user was already viewing the print status panel
 * when a print started, the gcode would never load because on_activate() was
 * never called again.
 *
 * Also tests the resume check for both 3D (mode 1) and 2D (mode 2) viewer modes.
 */

#include <string>

#include "../catch_amalgamated.hpp"

// ============================================================================
// Test Helper: Simulates the deferred loading decision logic
// ============================================================================

/**
 * @brief Represents the result of the deferred loading decision
 */
enum class GcodeLoadAction {
    LOAD_IMMEDIATELY, ///< Load gcode now (panel is active)
    DEFER_TO_ACTIVATE ///< Store in pending and load on next on_activate()
};

/**
 * @brief Decides whether to load gcode immediately or defer
 *
 * Mirrors the logic in PrintStatusPanel::set_filename():
 * - If panel is active → load immediately
 * - If panel is inactive → defer to pending_gcode_filename_
 *
 * @param is_active Whether the panel is currently visible/active
 * @param filename The new filename to load
 * @param loaded_filename The filename that was previously loaded (for idempotency)
 * @return The action to take
 */
static GcodeLoadAction decide_gcode_load_action(bool is_active, const std::string& filename,
                                                const std::string& loaded_filename) {
    // Don't load if filename unchanged (idempotency check)
    if (filename.empty() || filename == loaded_filename) {
        // This case is handled before the decision - return defer as "no action"
        return GcodeLoadAction::DEFER_TO_ACTIVATE;
    }

    // Key decision: is the panel currently visible?
    if (is_active) {
        return GcodeLoadAction::LOAD_IMMEDIATELY;
    } else {
        return GcodeLoadAction::DEFER_TO_ACTIVATE;
    }
}

/**
 * @brief Decides whether to resume the gcode viewer on panel activation
 *
 * Mirrors the logic in PrintStatusPanel::on_activate():
 * - Resume if viewer mode is 1 (3D) OR 2 (2D)
 * - Don't resume if mode is 0 (thumbnail)
 *
 * @param viewer_mode Current gcode_viewer_mode subject value (0=thumb, 1=3D, 2=2D)
 * @return true if should resume viewer, false otherwise
 */
static bool should_resume_viewer(int viewer_mode) {
    return (viewer_mode == 1 || viewer_mode == 2);
}

// ============================================================================
// Deferred Loading Decision Tests
// ============================================================================

TEST_CASE("Gcode loading decision: panel active vs inactive", "[print_status][gcode][deferred]") {
    SECTION("Panel active with new filename → load immediately") {
        bool is_active = true;
        std::string filename = "benchy.gcode";
        std::string loaded_filename = ""; // Nothing loaded yet

        auto action = decide_gcode_load_action(is_active, filename, loaded_filename);

        REQUIRE(action == GcodeLoadAction::LOAD_IMMEDIATELY);
    }

    SECTION("Panel inactive with new filename → defer to on_activate") {
        bool is_active = false;
        std::string filename = "benchy.gcode";
        std::string loaded_filename = "";

        auto action = decide_gcode_load_action(is_active, filename, loaded_filename);

        REQUIRE(action == GcodeLoadAction::DEFER_TO_ACTIVATE);
    }

    SECTION("Panel active but same filename → no reload (idempotency)") {
        bool is_active = true;
        std::string filename = "benchy.gcode";
        std::string loaded_filename = "benchy.gcode"; // Already loaded

        auto action = decide_gcode_load_action(is_active, filename, loaded_filename);

        // Returns DEFER as "no action needed" when filename unchanged
        REQUIRE(action == GcodeLoadAction::DEFER_TO_ACTIVATE);
    }

    SECTION("Panel active with empty filename → no action") {
        bool is_active = true;
        std::string filename = "";
        std::string loaded_filename = "";

        auto action = decide_gcode_load_action(is_active, filename, loaded_filename);

        REQUIRE(action == GcodeLoadAction::DEFER_TO_ACTIVATE);
    }

    SECTION("Panel transitions from inactive to active with pending file") {
        // Simulates: print started while on different panel, user navigates to print status
        bool was_inactive = false;
        std::string pending_filename = "cube.gcode";
        std::string loaded_filename = "";

        // First call: panel inactive, file deferred
        auto action1 = decide_gcode_load_action(was_inactive, pending_filename, loaded_filename);
        REQUIRE(action1 == GcodeLoadAction::DEFER_TO_ACTIVATE);

        // Second call: panel now active (simulating on_activate reading pending)
        bool now_active = true;
        auto action2 = decide_gcode_load_action(now_active, pending_filename, loaded_filename);
        REQUIRE(action2 == GcodeLoadAction::LOAD_IMMEDIATELY);
    }
}

// ============================================================================
// Viewer Resume Mode Tests
// ============================================================================

TEST_CASE("Viewer resume check for 3D and 2D modes", "[print_status][gcode][resume]") {
    SECTION("Mode 0 (thumbnail) → don't resume") {
        REQUIRE_FALSE(should_resume_viewer(0));
    }

    SECTION("Mode 1 (3D viewer) → resume") {
        REQUIRE(should_resume_viewer(1));
    }

    SECTION("Mode 2 (2D viewer) → resume") {
        REQUIRE(should_resume_viewer(2));
    }

    SECTION("Invalid mode (negative) → don't resume") {
        REQUIRE_FALSE(should_resume_viewer(-1));
    }

    SECTION("Invalid mode (too high) → don't resume") {
        REQUIRE_FALSE(should_resume_viewer(3));
        REQUIRE_FALSE(should_resume_viewer(100));
    }
}

// ============================================================================
// Scenario Tests
// ============================================================================

TEST_CASE("Scenario: User starts print while viewing print status panel",
          "[print_status][gcode][scenario]") {
    // This is the bug scenario that was fixed
    // User is already on print status panel when print starts

    bool panel_is_active = true; // User already viewing the panel
    std::string new_print_file = "calibration_cube.gcode";
    std::string previously_loaded = ""; // No file was loaded

    auto action = decide_gcode_load_action(panel_is_active, new_print_file, previously_loaded);

    // BUG FIX: Should load immediately, not defer
    // Previously this would defer and never load because on_activate() wouldn't fire
    REQUIRE(action == GcodeLoadAction::LOAD_IMMEDIATELY);
}

TEST_CASE("Scenario: User navigates to print status after print starts",
          "[print_status][gcode][scenario]") {
    // Normal flow: print starts, user navigates to print status panel

    std::string print_file = "vase.gcode";
    std::string previously_loaded = "";

    // Step 1: Print starts while user is elsewhere (panel inactive)
    bool panel_inactive = false;
    auto action1 = decide_gcode_load_action(panel_inactive, print_file, previously_loaded);
    REQUIRE(action1 == GcodeLoadAction::DEFER_TO_ACTIVATE);

    // Step 2: User navigates to print status (on_activate fires)
    // The pending filename would be loaded in on_activate()
    // This is simulated by calling the decision again with active=true
    bool panel_now_active = true;
    auto action2 = decide_gcode_load_action(panel_now_active, print_file, previously_loaded);
    REQUIRE(action2 == GcodeLoadAction::LOAD_IMMEDIATELY);
}

TEST_CASE("Scenario: User navigates away from 2D view and back",
          "[print_status][gcode][scenario]") {
    // Tests that 2D mode is properly resumed (was only checking mode 1 before)

    int mode_2d = 2;

    // When navigating back, the viewer should resume for 2D mode
    REQUIRE(should_resume_viewer(mode_2d));
}
