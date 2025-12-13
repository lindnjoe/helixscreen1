// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "lvgl/lvgl.h"

#include <string>

class MoonrakerClient;
class PrinterState;

/**
 * @file ui_panel_calibration_zoffset.h
 * @brief Z-Offset calibration panel using PROBE_CALIBRATE workflow
 *
 * Interactive panel that guides the user through the paper test calibration
 * process. Uses Klipper's PROBE_CALIBRATE, TESTZ, ACCEPT, and ABORT commands.
 *
 * ## State Machine:
 * - IDLE (0): Shows instructions and Start button
 * - PROBING (1): Waiting for PROBE_CALIBRATE to complete (homes + probes)
 * - ADJUSTING (2): User adjusts Z with paper test (+/- buttons)
 * - SAVING (3): ACCEPT was pressed, saving config (Klipper restarts)
 * - COMPLETE (4): Calibration successful
 * - ERROR (5): Something went wrong
 *
 * ## Usage:
 * ```cpp
 * // At startup (before XML creation):
 * ui_panel_calibration_zoffset_register_callbacks();
 *
 * // When opening panel:
 * ZOffsetCalibrationPanel& panel = get_global_zoffset_cal_panel();
 * panel.setup(lv_obj, parent_screen, moonraker_client);
 * ui_nav_push_overlay(lv_obj);
 * ```
 */
class ZOffsetCalibrationPanel {
  public:
    /**
     * @brief Calibration state machine states
     *
     * Values must match XML bind_flag_if_not_eq ref_value attributes.
     */
    enum class State {
        IDLE = 0,      ///< Ready to start, showing instructions
        PROBING = 1,   ///< PROBE_CALIBRATE running
        ADJUSTING = 2, ///< Interactive Z adjustment phase
        SAVING = 3,    ///< ACCEPT sent, waiting for SAVE_CONFIG
        COMPLETE = 4,  ///< Calibration finished successfully
        ERROR = 5      ///< Error occurred
    };

    ZOffsetCalibrationPanel() = default;
    ~ZOffsetCalibrationPanel();

    /**
     * @brief Initialize LVGL subjects for reactive state management
     *
     * Must be called once before setup(), typically during application init.
     * Registers the state subject used by XML bind_flag_if_not_eq bindings.
     */
    void init_subjects();

    /**
     * @brief Register XML event callbacks
     *
     * Must be called once during application init to wire up XML event_cb elements.
     */
    static void register_callbacks();

    /**
     * @brief Setup the panel with event handlers
     *
     * @param panel Root panel object from lv_xml_create()
     * @param parent_screen Parent screen (for navigation)
     * @param client Moonraker client for sending commands
     */
    void setup(lv_obj_t* panel, lv_obj_t* parent_screen, MoonrakerClient* client);

    /**
     * @brief Get current calibration state
     * @return Current State
     */
    State get_state() const {
        return state_;
    }

    /**
     * @brief Update Z position display (called from external state updates)
     * @param z_position Current Z position from printer state
     */
    void update_z_position(float z_position);

    /**
     * @brief Handle calibration completion/error from Moonraker
     * @param success true if calibration completed successfully
     * @param message Status message
     */
    void on_calibration_result(bool success, const std::string& message);

  private:
    // State management
    State state_ = State::IDLE;
    void set_state(State new_state);

    // State subject for reactive visibility control
    lv_subject_t zoffset_cal_state_{};
    bool subjects_initialized_ = false;

    // Gcode command helpers
    void send_probe_calibrate();
    void send_testz(float delta);
    void send_accept();
    void send_abort();

    // Event handlers
    void handle_start_clicked();
    void handle_z_adjust(float delta);
    void handle_accept_clicked();
    void handle_abort_clicked();
    void handle_done_clicked();
    void handle_retry_clicked();

    // Static trampolines (for XML event_cb registration)
    static void on_start_clicked(lv_event_t* e);
    static void on_z_down_1(lv_event_t* e);
    static void on_z_down_01(lv_event_t* e);
    static void on_z_down_005(lv_event_t* e);
    static void on_z_down_001(lv_event_t* e);
    static void on_z_up_001(lv_event_t* e);
    static void on_z_up_005(lv_event_t* e);
    static void on_z_up_01(lv_event_t* e);
    static void on_z_up_1(lv_event_t* e);
    static void on_accept_clicked(lv_event_t* e);
    static void on_abort_clicked(lv_event_t* e);
    static void on_done_clicked(lv_event_t* e);
    static void on_retry_clicked(lv_event_t* e);

    // Widget references
    lv_obj_t* panel_ = nullptr;
    lv_obj_t* parent_screen_ = nullptr;
    MoonrakerClient* client_ = nullptr;

    // Interactive elements (still needed for dynamic text updates)
    lv_obj_t* z_position_display_ = nullptr;
    lv_obj_t* final_offset_label_ = nullptr;
    lv_obj_t* error_message_ = nullptr;

    // Current Z position during calibration
    float current_z_ = 0.0f;
    float final_offset_ = 0.0f;

    // Observer for manual_probe state changes (for real Klipper integration)
    lv_observer_t* manual_probe_active_observer_ = nullptr;
    lv_observer_t* manual_probe_z_observer_ = nullptr;

    // Observer callbacks (static for LVGL)
    static void on_manual_probe_active_changed(lv_observer_t* observer, lv_subject_t* subject);
    static void on_manual_probe_z_changed(lv_observer_t* observer, lv_subject_t* subject);
};

// Global instance accessor
ZOffsetCalibrationPanel& get_global_zoffset_cal_panel();

/**
 * @brief Register XML event callbacks and initialize subjects for Z-Offset panel
 *
 * Call this once at startup before creating any calibration_zoffset_panel XML.
 * Registers callbacks for all button events and initializes state subject.
 */
void ui_panel_calibration_zoffset_register_callbacks();

/**
 * @brief Initialize row click callback for opening from Advanced panel
 *
 * @deprecated Use ui_panel_calibration_zoffset_register_callbacks() instead.
 * This function now just calls that one for backward compatibility.
 */
void init_zoffset_row_handler();
