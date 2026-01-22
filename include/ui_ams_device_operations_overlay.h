// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_ams_device_operations_overlay.h
 * @brief AMS Device Operations consolidated overlay
 *
 * This overlay consolidates multiple AMS settings panels into one:
 * - Quick Actions & Behavior: Home, Recover, Abort + Bypass Mode toggle
 * - Calibration: Dynamic backend-specific calibration actions
 * - Speed Settings: Dynamic backend-specific speed controls
 *
 * @pattern Overlay (lazy init, singleton)
 * @threading Main thread only
 */

#pragma once

#include "ams_types.h"
#include "overlay_base.h"

#include <lvgl/lvgl.h>

#include <memory>
#include <string>
#include <vector>

// Forward declarations
class AmsBackend;

namespace helix::ui {

/**
 * @class AmsDeviceOperationsOverlay
 * @brief Consolidated overlay for AMS device operations
 *
 * This overlay provides a unified interface for AMS device operations:
 *
 * Card 1 - Quick Actions & Behavior:
 * - Home: Reset AMS to home position
 * - Recover: Attempt error recovery
 * - Abort: Cancel current operation
 * - Bypass Mode toggle (if supported)
 * - Auto-Heat status indicator (if supported)
 *
 * Card 2 - Calibration:
 * - Dynamic actions from backend->get_device_actions("calibration")
 *
 * Card 3 - Speed Settings:
 * - Dynamic actions from backend->get_device_actions("speed")
 *
 * ## Usage:
 *
 * @code
 * auto& overlay = helix::ui::get_ams_device_operations_overlay();
 * if (!overlay.are_subjects_initialized()) {
 *     overlay.init_subjects();
 *     overlay.register_callbacks();
 * }
 * overlay.show(parent_screen);
 * @endcode
 */
class AmsDeviceOperationsOverlay : public OverlayBase {
  public:
    /**
     * @brief Default constructor
     */
    AmsDeviceOperationsOverlay();

    /**
     * @brief Destructor
     */
    ~AmsDeviceOperationsOverlay() override;

    // Non-copyable
    AmsDeviceOperationsOverlay(const AmsDeviceOperationsOverlay&) = delete;
    AmsDeviceOperationsOverlay& operator=(const AmsDeviceOperationsOverlay&) = delete;

    //
    // === OverlayBase Interface ===
    //

    /**
     * @brief Initialize subjects for reactive binding
     *
     * Registers subjects for:
     * - ams_device_ops_status: Current status text
     * - ams_device_ops_supports_bypass: Whether bypass mode is supported (0/1)
     * - ams_device_ops_bypass_active: Whether bypass is currently active (0/1)
     * - ams_device_ops_supports_auto_heat: Whether auto-heat is supported (0/1)
     * - ams_device_ops_has_backend: Whether an AMS backend is connected (0/1)
     * - ams_device_ops_has_calibration: Whether calibration actions exist (0/1)
     * - ams_device_ops_has_speed: Whether speed actions exist (0/1)
     */
    void init_subjects() override;

    /**
     * @brief Register event callbacks with lv_xml system
     *
     * Registers callbacks for:
     * - Home, Recover, Abort buttons
     * - Bypass toggle
     * - Dynamic action buttons
     */
    void register_callbacks() override;

    /**
     * @brief Create the overlay UI (called lazily)
     *
     * @param parent Parent widget to attach overlay to (usually screen)
     * @return Root object of overlay, or nullptr on failure
     */
    lv_obj_t* create(lv_obj_t* parent) override;

    /**
     * @brief Get human-readable overlay name
     * @return "Device Operations"
     */
    const char* get_name() const override {
        return "Device Operations";
    }

    //
    // === Public API ===
    //

    /**
     * @brief Show the overlay
     *
     * This method:
     * 1. Ensures overlay is created (lazy init)
     * 2. Queries backend for capabilities and actions
     * 3. Updates subjects and dynamic UI
     * 4. Pushes overlay onto navigation stack
     *
     * @param parent_screen The parent screen for overlay creation
     */
    void show(lv_obj_t* parent_screen);

    /**
     * @brief Refresh the overlay from backend
     *
     * Re-queries backend and updates all subjects and dynamic actions.
     */
    void refresh();

  private:
    //
    // === Internal Methods ===
    //

    /**
     * @brief Update subjects from backend state
     *
     * Queries backend for current capabilities and state:
     * - supports_bypass, is_bypass_active()
     * - supports_auto_heat_on_load()
     * - get_current_action() for status
     */
    void update_from_backend();

    /**
     * @brief Populate dynamic actions for a section
     *
     * Queries backend for actions in the specified section and creates
     * UI controls for each action.
     *
     * @param container Parent container for action controls
     * @param section_id Section ID to filter actions by ("calibration" or "speed")
     * @return Number of actions created
     */
    int populate_section_actions(lv_obj_t* container, const std::string& section_id);

    /**
     * @brief Create control for a single device action
     *
     * Creates the appropriate control based on action type.
     *
     * @param parent Container to add control to
     * @param action Action metadata
     */
    void create_action_control(lv_obj_t* parent, const helix::printer::DeviceAction& action);

    /**
     * @brief Clear dynamic actions from a container
     *
     * @param container Container to clear
     */
    void clear_actions(lv_obj_t* container);

    /**
     * @brief Convert AmsAction enum to human-readable string
     *
     * @param action The action to convert
     * @return Human-readable status string
     */
    static const char* action_to_string(int action);

    //
    // === Static Callbacks ===
    //

    /**
     * @brief Callback for Home button click
     */
    static void on_home_clicked(lv_event_t* e);

    /**
     * @brief Callback for Recover button click
     */
    static void on_recover_clicked(lv_event_t* e);

    /**
     * @brief Callback for Abort button click
     */
    static void on_abort_clicked(lv_event_t* e);

    /**
     * @brief Callback for bypass toggle change
     */
    static void on_bypass_toggled(lv_event_t* e);

    /**
     * @brief Callback for dynamic action button click
     */
    static void on_action_clicked(lv_event_t* e);

    /**
     * @brief Callback for dynamic toggle value change
     */
    static void on_toggle_changed(lv_event_t* e);

    //
    // === State ===
    //

    /// Alias for overlay_root_ to match existing pattern
    lv_obj_t*& overlay_ = overlay_root_;

    /// Container for calibration actions
    lv_obj_t* calibration_container_ = nullptr;

    /// Container for speed actions
    lv_obj_t* speed_container_ = nullptr;

    /// Subject for status text display
    lv_subject_t status_subject_;

    /// Buffer for status text
    char status_buf_[128] = {};

    /// Subject for bypass support (0=not supported, 1=supported)
    lv_subject_t supports_bypass_subject_;

    /// Subject for bypass active state (0=inactive, 1=active)
    lv_subject_t bypass_active_subject_;

    /// Subject for auto-heat support (0=not supported, 1=supported)
    lv_subject_t supports_auto_heat_subject_;

    /// Subject for backend presence (0=no backend, 1=has backend)
    lv_subject_t has_backend_subject_;

    /// Subject for calibration actions presence (0=none, 1=has actions)
    lv_subject_t has_calibration_subject_;

    /// Subject for speed actions presence (0=none, 1=has actions)
    lv_subject_t has_speed_subject_;

    /// Cached actions from backend
    std::vector<helix::printer::DeviceAction> cached_actions_;

    /// Action IDs for callback lookup (index stored in user_data)
    std::vector<std::string> action_ids_;
};

/**
 * @brief Global instance accessor
 *
 * Creates the overlay on first access and registers it for cleanup
 * with StaticPanelRegistry.
 *
 * @return Reference to singleton AmsDeviceOperationsOverlay
 */
AmsDeviceOperationsOverlay& get_ams_device_operations_overlay();

} // namespace helix::ui
