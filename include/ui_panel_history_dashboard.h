// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui_panel_base.h"

#include "print_history_data.h"

#include <vector>

/**
 * @file ui_panel_history_dashboard.h
 * @brief Print History Dashboard Panel - Statistics overview with time filtering
 *
 * The History Dashboard Panel displays aggregated print statistics including:
 * - Total prints, print time, filament used
 * - Success rate, longest print, failed/cancelled count
 *
 * ## Navigation:
 * - Entry: Advanced Panel → "Print History" action row
 * - Back: Returns to Advanced Panel
 * - "View Full History": Opens HistoryListPanel (Stage 3)
 *
 * ## Time Filtering:
 * The panel supports 5 time filters (Day/Week/Month/Year/All) that update
 * all displayed statistics. Filter selection is maintained across panel activations.
 *
 * ## Data Flow:
 * 1. On activate, calls MoonrakerAPI::get_history_list() with time filter
 * 2. Parses response to calculate statistics client-side
 * 3. Updates stat labels via direct widget manipulation
 *
 * Note: Moonraker's server.history.totals doesn't provide breakdown counts,
 * so we calculate success/fail/cancelled from the job list.
 *
 * @see print_history_data.h for data structures
 * @see PanelBase for base class documentation
 */
class HistoryDashboardPanel : public PanelBase {
  public:
    /**
     * @brief Construct HistoryDashboardPanel with injected dependencies
     *
     * @param printer_state Reference to PrinterState
     * @param api Pointer to MoonrakerAPI
     */
    HistoryDashboardPanel(PrinterState& printer_state, MoonrakerAPI* api);

    ~HistoryDashboardPanel() override = default;

    //
    // === PanelBase Implementation ===
    //

    /**
     * @brief Initialize subjects for reactive bindings
     *
     * Creates:
     * - history_has_jobs: 0 = no history, 1 = has history (for empty state)
     */
    void init_subjects() override;

    /**
     * @brief Setup the dashboard panel with widget references and event handlers
     *
     * @param panel Root panel object from lv_xml_create()
     * @param parent_screen Parent screen for overlay creation
     */
    void setup(lv_obj_t* panel, lv_obj_t* parent_screen) override;

    const char* get_name() const override {
        return "History Dashboard";
    }
    const char* get_xml_component_name() const override {
        return "history_dashboard_panel";
    }

    //
    // === Lifecycle Hooks ===
    //

    /**
     * @brief Refresh statistics when panel becomes visible
     *
     * Fetches history data with current time filter and updates display.
     */
    void on_activate() override;

    //
    // === Public API ===
    //

    /**
     * @brief Set the time filter and refresh statistics
     *
     * @param filter The time filter to apply
     */
    void set_time_filter(HistoryTimeFilter filter);

    /**
     * @brief Get the current time filter
     */
    HistoryTimeFilter get_time_filter() const {
        return current_filter_;
    }

    //
    // === Static Event Callbacks (registered with lv_xml_register_event_cb) ===
    // Must be public for LVGL XML system registration
    //

    static void on_filter_day_clicked(lv_event_t* e);
    static void on_filter_week_clicked(lv_event_t* e);
    static void on_filter_month_clicked(lv_event_t* e);
    static void on_filter_year_clicked(lv_event_t* e);
    static void on_filter_all_clicked(lv_event_t* e);
    static void on_view_history_clicked(lv_event_t* e);

  private:
    //
    // === Widget References ===
    //

    // Filter buttons
    lv_obj_t* filter_day_ = nullptr;
    lv_obj_t* filter_week_ = nullptr;
    lv_obj_t* filter_month_ = nullptr;
    lv_obj_t* filter_year_ = nullptr;
    lv_obj_t* filter_all_ = nullptr;

    // Stat labels
    lv_obj_t* stat_total_prints_ = nullptr;
    lv_obj_t* stat_print_time_ = nullptr;
    lv_obj_t* stat_filament_ = nullptr;
    lv_obj_t* stat_success_rate_ = nullptr;
    lv_obj_t* stat_longest_ = nullptr;
    lv_obj_t* stat_failed_ = nullptr;

    // Containers
    lv_obj_t* stats_grid_ = nullptr;
    lv_obj_t* empty_state_ = nullptr;
    lv_obj_t* btn_view_history_ = nullptr;

    //
    // === State ===
    //

    HistoryTimeFilter current_filter_ = HistoryTimeFilter::ALL_TIME;
    std::vector<PrintHistoryJob> cached_jobs_;

    // Subject for empty state binding (must persist for LVGL binding lifetime)
    lv_subject_t history_has_jobs_subject_;

    //
    // === Data Fetching ===
    //

    /**
     * @brief Fetch history data from Moonraker with current filter
     */
    void refresh_data();

    /**
     * @brief Calculate and display statistics from job list
     *
     * @param jobs Vector of print history jobs
     */
    void update_statistics(const std::vector<PrintHistoryJob>& jobs);

    /**
     * @brief Update filter button visual states
     *
     * Highlights the active filter button, dims others.
     */
    void update_filter_button_states();

    //
    // === Formatting Helpers ===
    //

    /**
     * @brief Format seconds as human-readable duration
     * @param seconds Duration in seconds
     * @return "2h 15m", "45m", "30s"
     */
    static std::string format_duration(double seconds);

    /**
     * @brief Format filament length for display
     * @param mm Filament in millimeters
     * @return "12.5m" or "1.2km"
     */
    static std::string format_filament(double mm);
};

/**
 * @brief Global instance accessor
 *
 * Returns reference to singleton HistoryDashboardPanel used by main.cpp.
 */
HistoryDashboardPanel& get_global_history_dashboard_panel();

/**
 * @brief Initialize the global HistoryDashboardPanel instance
 *
 * Must be called by main.cpp before accessing get_global_history_dashboard_panel().
 *
 * @param printer_state Reference to PrinterState
 * @param api Pointer to MoonrakerAPI
 */
void init_global_history_dashboard_panel(PrinterState& printer_state, MoonrakerAPI* api);
