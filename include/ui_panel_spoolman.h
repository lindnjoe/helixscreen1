// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui_panel_base.h"

#include "advanced_panel_types.h"
#include "moonraker_api.h"
#include "printer_state.h"

#include <lvgl.h>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Standalone Spoolman panel for filament inventory management
 *
 * This panel displays all spools from Spoolman and allows users to:
 * - Browse their filament inventory
 * - See remaining weight and percentage
 * - Set a spool as active (for filament tracking)
 * - View low-filament warnings
 *
 * The panel is capability-gated - only shown when printer_has_spoolman = 1.
 * Works independently of AMS (supports single-extruder printers).
 */
class SpoolmanPanel : public PanelBase {
  public:
    /**
     * @brief Construct Spoolman panel
     * @param printer_state Reference to printer state for capability checking
     * @param api Moonraker API for Spoolman queries (can be nullptr)
     */
    SpoolmanPanel(PrinterState& printer_state, MoonrakerAPI* api);
    ~SpoolmanPanel() override = default;

    // Non-copyable
    SpoolmanPanel(const SpoolmanPanel&) = delete;
    SpoolmanPanel& operator=(const SpoolmanPanel&) = delete;

    /**
     * @brief Initialize reactive subjects and event callbacks
     *
     * Must be called BEFORE XML is created. Registers:
     * - spoolman_status subject for status text
     * - on_spoolman_spool_clicked callback
     */
    void init_subjects() override;

    /**
     * @brief Setup panel after XML creation
     * @param panel The created LVGL panel object
     * @param parent_screen The parent screen for navigation
     */
    void setup(lv_obj_t* panel, lv_obj_t* parent_screen) override;

    [[nodiscard]] const char* get_name() const override {
        return "Spoolman";
    }

    [[nodiscard]] const char* get_xml_component_name() const override {
        return "spoolman_panel";
    }

    /**
     * @brief Refresh the spool list from Spoolman
     *
     * Called automatically on setup, can be called manually to refresh.
     */
    void refresh_spools();

  private:
    MoonrakerAPI* api_ = nullptr;

    // Subjects for reactive binding
    lv_subject_t status_subject_;
    char status_buf_[128] = "Loading...";

    // Widget references
    lv_obj_t* spool_list_container_ = nullptr;
    lv_obj_t* empty_state_container_ = nullptr;

    // Spool data cache
    struct SpoolRow {
        lv_obj_t* container = nullptr;
        int spool_id = 0;
        bool is_active = false;
    };
    std::vector<SpoolRow> spool_rows_;
    std::vector<SpoolInfo> cached_spools_;

    // Low filament threshold (grams)
    static constexpr double LOW_THRESHOLD_GRAMS = 100.0;

    /**
     * @brief Clear all spool rows from the list
     */
    void clear_list();

    /**
     * @brief Populate the list with spools
     * @param spools Vector of spool info from Spoolman
     */
    void populate_list(const std::vector<SpoolInfo>& spools);

    /**
     * @brief Create a single spool row in the list
     * @param spool Spool info to display
     */
    void create_spool_row(const SpoolInfo& spool);

    /**
     * @brief Update visual state of a row based on spool data
     * @param row The row container
     * @param spool The spool data
     */
    void update_row_visuals(lv_obj_t* row, const SpoolInfo& spool);

    /**
     * @brief Handle spool row click - set as active
     * @param spool_id The clicked spool's ID
     */
    void handle_spool_clicked(int spool_id);

    /**
     * @brief Update which spool shows as active
     * @param active_id The newly active spool ID
     */
    void update_active_indicator(int active_id);

    // Static event callback
    static void on_spool_clicked(lv_event_t* e);
};

/**
 * @brief Get the global SpoolmanPanel instance
 *
 * Creates the instance on first call. Uses PrinterState and MoonrakerAPI
 * from global getters.
 *
 * @return Reference to the global SpoolmanPanel
 */
SpoolmanPanel& get_global_spoolman_panel();
