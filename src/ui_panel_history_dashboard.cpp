// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_panel_history_dashboard.h"

#include "ui_nav.h"
#include "ui_theme.h"
#include "ui_toast.h"

#include "moonraker_api.h"
#include "printer_state.h"

#include <spdlog/spdlog.h>

#include <cmath>
#include <ctime>

// Global instance (singleton pattern)
static std::unique_ptr<HistoryDashboardPanel> g_history_dashboard_panel;

HistoryDashboardPanel& get_global_history_dashboard_panel() {
    if (!g_history_dashboard_panel) {
        spdlog::error("get_global_history_dashboard_panel() called before initialization!");
        throw std::runtime_error("HistoryDashboardPanel not initialized");
    }
    return *g_history_dashboard_panel;
}

void init_global_history_dashboard_panel(PrinterState& printer_state, MoonrakerAPI* api) {
    g_history_dashboard_panel = std::make_unique<HistoryDashboardPanel>(printer_state, api);

    // Register XML event callbacks (must be done before XML is created)
    lv_xml_register_event_cb(nullptr, "history_filter_day_clicked",
                             HistoryDashboardPanel::on_filter_day_clicked);
    lv_xml_register_event_cb(nullptr, "history_filter_week_clicked",
                             HistoryDashboardPanel::on_filter_week_clicked);
    lv_xml_register_event_cb(nullptr, "history_filter_month_clicked",
                             HistoryDashboardPanel::on_filter_month_clicked);
    lv_xml_register_event_cb(nullptr, "history_filter_year_clicked",
                             HistoryDashboardPanel::on_filter_year_clicked);
    lv_xml_register_event_cb(nullptr, "history_filter_all_clicked",
                             HistoryDashboardPanel::on_filter_all_clicked);
    lv_xml_register_event_cb(nullptr, "history_view_full_clicked",
                             HistoryDashboardPanel::on_view_history_clicked);

    spdlog::debug("[History Dashboard] Event callbacks registered");
}

// ============================================================================
// CONSTRUCTOR
// ============================================================================

HistoryDashboardPanel::HistoryDashboardPanel(PrinterState& printer_state, MoonrakerAPI* api)
    : PanelBase(printer_state, api) {
    spdlog::trace("[{}] Constructor", get_name());
}

// ============================================================================
// PANELBASE IMPLEMENTATION
// ============================================================================

void HistoryDashboardPanel::init_subjects() {
    // Initialize subject for empty state visibility binding
    // 0 = no history (show empty state), 1 = has history (show stats grid)
    lv_subject_init_int(&history_has_jobs_subject_, 0);
    lv_xml_register_subject(nullptr, "history_has_jobs", &history_has_jobs_subject_);

    subjects_initialized_ = true;
    spdlog::debug("[{}] Subjects initialized", get_name());
}

void HistoryDashboardPanel::setup(lv_obj_t* panel, lv_obj_t* parent_screen) {
    PanelBase::setup(panel, parent_screen);

    if (!panel_) {
        spdlog::error("[{}] NULL panel", get_name());
        return;
    }

    // Find widget references
    filter_day_ = lv_obj_find_by_name(panel_, "filter_day");
    filter_week_ = lv_obj_find_by_name(panel_, "filter_week");
    filter_month_ = lv_obj_find_by_name(panel_, "filter_month");
    filter_year_ = lv_obj_find_by_name(panel_, "filter_year");
    filter_all_ = lv_obj_find_by_name(panel_, "filter_all");

    stat_total_prints_ = lv_obj_find_by_name(panel_, "stat_total_prints");
    stat_print_time_ = lv_obj_find_by_name(panel_, "stat_print_time");
    stat_filament_ = lv_obj_find_by_name(panel_, "stat_filament");
    stat_success_rate_ = lv_obj_find_by_name(panel_, "stat_success_rate");
    stat_longest_ = lv_obj_find_by_name(panel_, "stat_longest");
    stat_failed_ = lv_obj_find_by_name(panel_, "stat_failed");

    stats_grid_ = lv_obj_find_by_name(panel_, "stats_grid");
    empty_state_ = lv_obj_find_by_name(panel_, "empty_state");
    btn_view_history_ = lv_obj_find_by_name(panel_, "btn_view_history");

    // Log found widgets
    spdlog::debug("[{}] Widget refs - filters: {}/{}/{}/{}/{}, stats: {}/{}/{}/{}/{}/{}",
                  get_name(), filter_day_ != nullptr, filter_week_ != nullptr,
                  filter_month_ != nullptr, filter_year_ != nullptr, filter_all_ != nullptr,
                  stat_total_prints_ != nullptr, stat_print_time_ != nullptr,
                  stat_filament_ != nullptr, stat_success_rate_ != nullptr,
                  stat_longest_ != nullptr, stat_failed_ != nullptr);

    spdlog::info("[{}] Setup complete", get_name());
}

void HistoryDashboardPanel::on_activate() {
    spdlog::debug("[{}] Activated - refreshing data with filter {}", get_name(),
                  static_cast<int>(current_filter_));
    refresh_data();
}

// ============================================================================
// PUBLIC API
// ============================================================================

void HistoryDashboardPanel::set_time_filter(HistoryTimeFilter filter) {
    if (current_filter_ == filter) {
        return;
    }

    current_filter_ = filter;
    update_filter_button_states();
    refresh_data();
}

// ============================================================================
// DATA FETCHING
// ============================================================================

void HistoryDashboardPanel::refresh_data() {
    if (!api_) {
        spdlog::warn("[{}] No API available", get_name());
        return;
    }

    // Calculate time range based on filter
    double since = 0.0;
    double now = static_cast<double>(std::time(nullptr));

    switch (current_filter_) {
    case HistoryTimeFilter::DAY:
        since = now - (24 * 60 * 60);
        break;
    case HistoryTimeFilter::WEEK:
        since = now - (7 * 24 * 60 * 60);
        break;
    case HistoryTimeFilter::MONTH:
        since = now - (30 * 24 * 60 * 60);
        break;
    case HistoryTimeFilter::YEAR:
        since = now - (365 * 24 * 60 * 60);
        break;
    case HistoryTimeFilter::ALL_TIME:
    default:
        since = 0.0;
        break;
    }

    spdlog::debug("[{}] Fetching history since {} (filter={})", get_name(), since,
                  static_cast<int>(current_filter_));

    // Fetch with generous limit - dashboard just needs aggregate stats
    api_->get_history_list(
        200, // limit
        0,   // start
        since, 0.0,
        [this](const std::vector<PrintHistoryJob>& jobs, uint64_t total_count) {
            spdlog::info("[{}] Received {} jobs (total: {})", get_name(), jobs.size(), total_count);
            cached_jobs_ = jobs;
            update_statistics(jobs);
        },
        [this](const MoonrakerError& error) {
            spdlog::error("[{}] Failed to fetch history: {}", get_name(), error.message);
            ui_toast_show(ToastSeverity::ERROR, "Failed to load print history", 3000);
        });
}

void HistoryDashboardPanel::update_statistics(const std::vector<PrintHistoryJob>& jobs) {
    // Update empty state subject (use class member directly)
    lv_subject_set_int(&history_has_jobs_subject_, jobs.empty() ? 0 : 1);

    // Show/hide appropriate containers
    if (stats_grid_) {
        if (jobs.empty()) {
            lv_obj_add_flag(stats_grid_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(stats_grid_, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (jobs.empty()) {
        // Clear stats
        if (stat_total_prints_)
            lv_label_set_text(stat_total_prints_, "0");
        if (stat_print_time_)
            lv_label_set_text(stat_print_time_, "0h");
        if (stat_filament_)
            lv_label_set_text(stat_filament_, "0m");
        if (stat_success_rate_)
            lv_label_set_text(stat_success_rate_, "0%");
        if (stat_longest_)
            lv_label_set_text(stat_longest_, "0h");
        if (stat_failed_)
            lv_label_set_text(stat_failed_, "0");
        return;
    }

    // Calculate statistics
    uint64_t total_prints = jobs.size();
    double total_time = 0.0;
    double total_filament = 0.0;
    double longest_print = 0.0;
    uint64_t completed_count = 0;
    uint64_t failed_count = 0;

    for (const auto& job : jobs) {
        total_time += job.print_duration;
        total_filament += job.filament_used;

        if (job.print_duration > longest_print) {
            longest_print = job.print_duration;
        }

        // job.status is already PrintJobStatus enum
        if (job.status == PrintJobStatus::COMPLETED) {
            completed_count++;
        } else if (job.status == PrintJobStatus::ERROR || job.status == PrintJobStatus::CANCELLED) {
            failed_count++;
        }
    }

    // Calculate success rate
    double success_rate = 0.0;
    if (total_prints > 0) {
        success_rate =
            (static_cast<double>(completed_count) / static_cast<double>(total_prints)) * 100.0;
    }

    // Update labels
    if (stat_total_prints_) {
        lv_label_set_text_fmt(stat_total_prints_, "%llu",
                              static_cast<unsigned long long>(total_prints));
    }

    if (stat_print_time_) {
        std::string time_str = format_duration(total_time);
        lv_label_set_text(stat_print_time_, time_str.c_str());
    }

    if (stat_filament_) {
        std::string filament_str = format_filament(total_filament);
        lv_label_set_text(stat_filament_, filament_str.c_str());
    }

    if (stat_success_rate_) {
        lv_label_set_text_fmt(stat_success_rate_, "%.0f%%", success_rate);
    }

    if (stat_longest_) {
        std::string longest_str = format_duration(longest_print);
        lv_label_set_text(stat_longest_, longest_str.c_str());
    }

    if (stat_failed_) {
        lv_label_set_text_fmt(stat_failed_, "%llu", static_cast<unsigned long long>(failed_count));
    }

    spdlog::debug("[{}] Stats updated: {} prints, {} time, {} filament, {:.0f}% success",
                  get_name(), total_prints, format_duration(total_time),
                  format_filament(total_filament), success_rate);
}

void HistoryDashboardPanel::update_filter_button_states() {
    // Helper to update button styling
    auto update_button = [](lv_obj_t* btn, bool active) {
        if (!btn)
            return;

        if (active) {
            lv_obj_set_style_bg_color(btn, ui_theme_parse_color("#primary_color"), 0);
            // Find text label child and update color
            lv_obj_t* label = lv_obj_get_child(btn, 0);
            if (label && lv_obj_check_type(label, &lv_label_class)) {
                lv_obj_set_style_text_color(label, ui_theme_parse_color("#text_primary"), 0);
            }
        } else {
            lv_obj_set_style_bg_color(btn, ui_theme_parse_color("#surface_bg_dark"), 0);
            lv_obj_t* label = lv_obj_get_child(btn, 0);
            if (label && lv_obj_check_type(label, &lv_label_class)) {
                lv_obj_set_style_text_color(label, ui_theme_parse_color("#text_secondary"), 0);
            }
        }
    };

    update_button(filter_day_, current_filter_ == HistoryTimeFilter::DAY);
    update_button(filter_week_, current_filter_ == HistoryTimeFilter::WEEK);
    update_button(filter_month_, current_filter_ == HistoryTimeFilter::MONTH);
    update_button(filter_year_, current_filter_ == HistoryTimeFilter::YEAR);
    update_button(filter_all_, current_filter_ == HistoryTimeFilter::ALL_TIME);
}

// ============================================================================
// FORMATTING HELPERS
// ============================================================================

std::string HistoryDashboardPanel::format_duration(double seconds) {
    if (seconds < 60) {
        return std::to_string(static_cast<int>(seconds)) + "s";
    }

    int total_minutes = static_cast<int>(seconds / 60);
    int hours = total_minutes / 60;
    int minutes = total_minutes % 60;

    if (hours == 0) {
        return std::to_string(minutes) + "m";
    }

    if (minutes == 0) {
        return std::to_string(hours) + "h";
    }

    return std::to_string(hours) + "h " + std::to_string(minutes) + "m";
}

std::string HistoryDashboardPanel::format_filament(double mm) {
    if (mm < 1000) {
        return std::to_string(static_cast<int>(mm)) + "mm";
    }

    double meters = mm / 1000.0;
    if (meters < 1000) {
        // Show 1 decimal for meters
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1fm", meters);
        return buf;
    }

    // Kilometers for really large values
    double km = meters / 1000.0;
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1fkm", km);
    return buf;
}

// ============================================================================
// STATIC EVENT CALLBACKS
// ============================================================================

void HistoryDashboardPanel::on_filter_day_clicked(lv_event_t* e) {
    (void)e;
    spdlog::debug("[History Dashboard] Filter: Day clicked");
    get_global_history_dashboard_panel().set_time_filter(HistoryTimeFilter::DAY);
}

void HistoryDashboardPanel::on_filter_week_clicked(lv_event_t* e) {
    (void)e;
    spdlog::debug("[History Dashboard] Filter: Week clicked");
    get_global_history_dashboard_panel().set_time_filter(HistoryTimeFilter::WEEK);
}

void HistoryDashboardPanel::on_filter_month_clicked(lv_event_t* e) {
    (void)e;
    spdlog::debug("[History Dashboard] Filter: Month clicked");
    get_global_history_dashboard_panel().set_time_filter(HistoryTimeFilter::MONTH);
}

void HistoryDashboardPanel::on_filter_year_clicked(lv_event_t* e) {
    (void)e;
    spdlog::debug("[History Dashboard] Filter: Year clicked");
    get_global_history_dashboard_panel().set_time_filter(HistoryTimeFilter::YEAR);
}

void HistoryDashboardPanel::on_filter_all_clicked(lv_event_t* e) {
    (void)e;
    spdlog::debug("[History Dashboard] Filter: All clicked");
    get_global_history_dashboard_panel().set_time_filter(HistoryTimeFilter::ALL_TIME);
}

void HistoryDashboardPanel::on_view_history_clicked(lv_event_t* e) {
    (void)e;
    spdlog::debug("[History Dashboard] View Full History clicked");
    // TODO: Stage 3 - Navigate to HistoryListPanel
    ui_toast_show(ToastSeverity::INFO, "Full history list: Coming in Stage 3", 2000);
}
