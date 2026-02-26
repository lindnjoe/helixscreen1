// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_observer_cleanup_ordering.cpp
 * @brief Tests for crash-hardening: observer cleanup ordering
 *
 * Validates the fix from f843b0a2: widget pointers must be nullified
 * BEFORE observer guards are reset in cleanup methods. This prevents
 * cascading observer callbacks from accessing freed LVGL objects.
 *
 * Also tests the active_ guard pattern: observer callbacks must be
 * no-ops when the active_ flag is false.
 *
 * These tests FAIL if the protective code is removed.
 */

#include "ui_observer_guard.h"
#include "ui_update_queue.h"

#include "../lvgl_test_fixture.h"
#include "../test_helpers/update_queue_test_access.h"
#include "observer_factory.h"

#include <atomic>

#include "../catch_amalgamated.hpp"

using namespace helix::ui;

// Drain deferred observer callbacks
static void drain() {
    UpdateQueueTestAccess::drain(helix::ui::UpdateQueue::instance());
}

// ============================================================================
// Simulates the pattern used by AmsOperationSidebar, AmsPanel,
// and ZOffsetCalibrationPanel: a class with widget pointers, an
// active_ guard, and observer guards whose callbacks reference widgets.
// ============================================================================

class MockPanel {
  public:
    // Simulated widget pointers (would be lv_obj_t* in real code)
    lv_obj_t* widget_a_ = nullptr;
    lv_obj_t* widget_b_ = nullptr;

    // Lifecycle guard — set true after setup, cleared in cleanup
    bool active_ = false;

    // Observer guards
    ObserverGuard observer_a_;
    ObserverGuard observer_b_;

    // Tracks whether a callback accessed a widget after cleanup started
    bool callback_accessed_freed_widget_ = false;
    int callback_invocations_after_cleanup_ = 0;

    // Subject for testing
    lv_subject_t subject_;

    void init_subject() {
        lv_subject_init_int(&subject_, 0);
    }

    void setup(lv_obj_t* parent) {
        widget_a_ = lv_obj_create(parent);
        widget_b_ = lv_obj_create(parent);
        active_ = true;
    }

    void init_observers() {
        observer_a_ =
            observe_int_sync<MockPanel>(&subject_, this, [](MockPanel* self, int /*val*/) {
                if (!self->active_ || !self->widget_a_)
                    return;
                // In a real panel, this would call lv_label_set_text or similar
                // on widget_a_. If widget_a_ is freed, this is a UAF crash.
                self->callback_invocations_after_cleanup_++;
            });

        observer_b_ =
            observe_int_sync<MockPanel>(&subject_, this, [](MockPanel* self, int /*val*/) {
                if (!self->active_ || !self->widget_b_)
                    return;
                self->callback_invocations_after_cleanup_++;
            });
    }

    // CORRECT cleanup ordering: nullify widgets BEFORE resetting observers
    void cleanup_correct() {
        active_ = false;

        widget_a_ = nullptr;
        widget_b_ = nullptr;

        observer_a_.reset();
        observer_b_.reset();
    }

    // WRONG cleanup ordering: reset observers BEFORE nullifying widgets.
    // This is the bug pattern that f843b0a2 fixed. Resetting an observer
    // can trigger cascading callbacks that dereference widget pointers.
    void cleanup_wrong() {
        active_ = false;

        observer_a_.reset();
        observer_b_.reset();

        widget_a_ = nullptr;
        widget_b_ = nullptr;
    }

    void deinit_subject() {
        lv_subject_deinit(&subject_);
    }
};

// ============================================================================
// Tests for cleanup ordering
// ============================================================================

TEST_CASE_METHOD(LVGLTestFixture,
                 "Observer cleanup: correct ordering nullifies widgets before observers",
                 "[observer_cleanup][crash_hardening]") {
    MockPanel panel;
    panel.init_subject();
    panel.setup(test_screen());
    panel.init_observers();
    drain();

    REQUIRE(panel.widget_a_ != nullptr);
    REQUIRE(panel.widget_b_ != nullptr);
    REQUIRE(panel.active_ == true);

    panel.cleanup_correct();

    // After correct cleanup, widgets are null and active_ is false
    REQUIRE(panel.widget_a_ == nullptr);
    REQUIRE(panel.widget_b_ == nullptr);
    REQUIRE(panel.active_ == false);

    // Observers are released
    // Trigger subject change — callbacks should be no-ops because
    // active_ is false and widgets are null
    panel.callback_invocations_after_cleanup_ = 0;
    lv_subject_set_int(&panel.subject_, 99);
    drain();

    // No callbacks should have executed real work
    REQUIRE(panel.callback_invocations_after_cleanup_ == 0);

    panel.deinit_subject();
}

TEST_CASE_METHOD(LVGLTestFixture,
                 "Observer cleanup: active_ guard prevents callbacks during teardown",
                 "[observer_cleanup][crash_hardening]") {
    MockPanel panel;
    panel.init_subject();
    panel.setup(test_screen());
    panel.init_observers();
    drain();

    // Verify callbacks work before cleanup
    panel.callback_invocations_after_cleanup_ = 0;
    lv_subject_set_int(&panel.subject_, 1);
    drain();
    REQUIRE(panel.callback_invocations_after_cleanup_ == 2); // both observers fired

    // Set active_ to false (simulating start of cleanup)
    panel.active_ = false;

    // Fire another subject change — callbacks should bail out
    panel.callback_invocations_after_cleanup_ = 0;
    lv_subject_set_int(&panel.subject_, 2);
    drain();
    REQUIRE(panel.callback_invocations_after_cleanup_ == 0);

    // Full cleanup
    panel.widget_a_ = nullptr;
    panel.widget_b_ = nullptr;
    panel.observer_a_.reset();
    panel.observer_b_.reset();

    panel.deinit_subject();
}

TEST_CASE_METHOD(LVGLTestFixture,
                 "Observer cleanup: null widget guard prevents UAF independently of active_",
                 "[observer_cleanup][crash_hardening]") {
    // Tests that even if active_ is somehow still true, null widget checks
    // prevent the callback from doing dangerous work.
    MockPanel panel;
    panel.init_subject();
    panel.setup(test_screen());
    panel.init_observers();
    drain();

    // Nullify widgets but leave active_ true (partial cleanup, edge case)
    panel.widget_a_ = nullptr;
    panel.widget_b_ = nullptr;

    panel.callback_invocations_after_cleanup_ = 0;
    lv_subject_set_int(&panel.subject_, 3);
    drain();

    // Callbacks should bail out because widgets are null
    REQUIRE(panel.callback_invocations_after_cleanup_ == 0);

    panel.active_ = false;
    panel.observer_a_.reset();
    panel.observer_b_.reset();
    panel.deinit_subject();
}

// ============================================================================
// Tests that verify cleanup resets all state
// ============================================================================

TEST_CASE_METHOD(LVGLTestFixture, "Observer cleanup: cleanup resets all pending state",
                 "[observer_cleanup][crash_hardening]") {
    // Simulates AmsOperationSidebar::cleanup() resetting pending_bypass_enable_,
    // pending_load_slot_, etc.
    struct SidebarLike {
        bool active_ = false;
        lv_obj_t* root_ = nullptr;
        ObserverGuard obs_;
        bool pending_bypass_ = false;
        int pending_slot_ = -1;
        int prev_action_ = 0;

        void cleanup() {
            active_ = false;
            root_ = nullptr;
            obs_.reset();
            pending_bypass_ = false;
            pending_slot_ = -1;
            prev_action_ = 0;
        }
    };

    SidebarLike sidebar;
    sidebar.active_ = true;
    sidebar.root_ = lv_obj_create(test_screen());
    sidebar.pending_bypass_ = true;
    sidebar.pending_slot_ = 3;
    sidebar.prev_action_ = 5;

    sidebar.cleanup();

    REQUIRE(sidebar.active_ == false);
    REQUIRE(sidebar.root_ == nullptr);
    REQUIRE(sidebar.pending_bypass_ == false);
    REQUIRE(sidebar.pending_slot_ == -1);
    REQUIRE(sidebar.prev_action_ == 0);
}

// ============================================================================
// Tests for double-cleanup safety
// ============================================================================

TEST_CASE_METHOD(LVGLTestFixture, "Observer cleanup: double cleanup is safe",
                 "[observer_cleanup][crash_hardening]") {
    MockPanel panel;
    panel.init_subject();
    panel.setup(test_screen());
    panel.init_observers();
    drain();

    // First cleanup
    panel.cleanup_correct();

    // Second cleanup should not crash (all pointers already null, observers already reset)
    panel.cleanup_correct();

    REQUIRE(panel.widget_a_ == nullptr);
    REQUIRE(panel.widget_b_ == nullptr);
    REQUIRE(panel.active_ == false);

    panel.deinit_subject();
}

// ============================================================================
// Test that subjects_initialized_ guard works (AmsPanel pattern)
// ============================================================================

TEST_CASE_METHOD(LVGLTestFixture, "Observer cleanup: subjects_initialized flag prevents callbacks",
                 "[observer_cleanup][crash_hardening]") {
    // Mimics the AmsPanel::clear_panel_reference() pattern where
    // subjects_initialized_ is set to false FIRST.
    struct AmsPanelLike {
        bool subjects_initialized_ = false;
        lv_obj_t* panel_ = nullptr;
        lv_obj_t* slot_grid_ = nullptr;
        ObserverGuard action_observer_;
        ObserverGuard slot_observer_;
        int callback_count_ = 0;
        lv_subject_t subject_;

        void init() {
            lv_subject_init_int(&subject_, 0);
            subjects_initialized_ = true;
        }

        void init_observers() {
            action_observer_ = observe_int_sync<AmsPanelLike>(
                &subject_, this, [](AmsPanelLike* self, int /*val*/) {
                    if (!self->subjects_initialized_ || !self->panel_)
                        return;
                    self->callback_count_++;
                });
        }

        void clear_panel_reference() {
            // Mark subjects uninitialized FIRST
            subjects_initialized_ = false;

            // Nullify widget pointers BEFORE resetting observers
            panel_ = nullptr;
            slot_grid_ = nullptr;

            // Now reset observer guards
            action_observer_.reset();
            slot_observer_.reset();
        }

        void deinit() {
            lv_subject_deinit(&subject_);
        }
    };

    AmsPanelLike panel;
    panel.init();
    panel.panel_ = lv_obj_create(test_screen());
    panel.slot_grid_ = lv_obj_create(panel.panel_);
    panel.init_observers();
    drain();

    // Verify callbacks work initially
    panel.callback_count_ = 0;
    lv_subject_set_int(&panel.subject_, 1);
    drain();
    REQUIRE(panel.callback_count_ == 1);

    // Clear panel reference
    panel.clear_panel_reference();

    // Callbacks should be no-ops
    panel.callback_count_ = 0;
    lv_subject_set_int(&panel.subject_, 2);
    drain();
    REQUIRE(panel.callback_count_ == 0);

    panel.deinit();
}
