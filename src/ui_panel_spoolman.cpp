// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_panel_spoolman.h"

#include "ui_nav_manager.h"
#include "ui_panel_common.h"
#include "ui_spool_canvas.h"
#include "ui_subject_registry.h"
#include "ui_theme.h"
#include "ui_toast.h"

#include "app_globals.h"

#include <spdlog/spdlog.h>

#include <cstdio>
#include <memory>

// ============================================================================
// CONSTRUCTION
// ============================================================================

SpoolmanPanel::SpoolmanPanel(PrinterState& printer_state, MoonrakerAPI* api)
    : PanelBase(printer_state, api), api_(api) {
    // Initialize status subject buffer
    std::snprintf(status_buf_, sizeof(status_buf_), "Loading...");
}

// ============================================================================
// PANELBASE IMPLEMENTATION
// ============================================================================

void SpoolmanPanel::init_subjects() {
    if (subjects_initialized_) {
        spdlog::warn("[{}] init_subjects() called twice - ignoring", get_name());
        return;
    }

    // Initialize status subject for reactive binding
    UI_SUBJECT_INIT_AND_REGISTER_STRING(status_subject_, status_buf_, status_buf_,
                                        "spoolman_status");

    // Register XML event callbacks (MUST be done BEFORE XML created)
    lv_xml_register_event_cb(nullptr, "on_spoolman_spool_clicked", on_spool_clicked);

    subjects_initialized_ = true;
    spdlog::debug("[{}] Subjects initialized", get_name());
}

void SpoolmanPanel::setup(lv_obj_t* panel, lv_obj_t* parent_screen) {
    PanelBase::setup(panel, parent_screen);

    if (!panel_) {
        spdlog::error("[{}] NULL panel", get_name());
        return;
    }

    // Use standard overlay panel setup (wires header, back button, responsive padding)
    ui_overlay_panel_setup_standard(panel_, parent_screen_, "overlay_header", "overlay_content");

    // Find widget references
    lv_obj_t* overlay_content = lv_obj_find_by_name(panel_, "overlay_content");
    if (overlay_content) {
        spool_list_container_ = lv_obj_find_by_name(overlay_content, "spool_list");
        empty_state_container_ = lv_obj_find_by_name(overlay_content, "empty_state");
    }

    if (!spool_list_container_) {
        spdlog::error("[{}] spool_list container not found!", get_name());
        return;
    }

    // Fetch spools from API
    refresh_spools();

    spdlog::info("[{}] Setup complete!", get_name());
}

// ============================================================================
// SPOOL LIST MANAGEMENT
// ============================================================================

void SpoolmanPanel::refresh_spools() {
    if (!api_) {
        spdlog::warn("[{}] No MoonrakerAPI available", get_name());
        std::snprintf(status_buf_, sizeof(status_buf_), "Not connected");
        lv_subject_copy_string(&status_subject_, status_buf_);

        // Show empty state
        if (spool_list_container_) {
            lv_obj_add_flag(spool_list_container_, LV_OBJ_FLAG_HIDDEN);
        }
        if (empty_state_container_) {
            lv_obj_remove_flag(empty_state_container_, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    std::snprintf(status_buf_, sizeof(status_buf_), "Loading...");
    lv_subject_copy_string(&status_subject_, status_buf_);

    api_->get_spoolman_spools(
        [this](const std::vector<SpoolInfo>& spools) {
            spdlog::info("[{}] Received {} spools from Spoolman", get_name(), spools.size());
            cached_spools_ = spools;
            populate_list(spools);
        },
        [this](const MoonrakerError& err) {
            spdlog::error("[{}] Failed to fetch spools: {}", get_name(), err.message);
            std::snprintf(status_buf_, sizeof(status_buf_), "Failed to load spools");
            lv_subject_copy_string(&status_subject_, status_buf_);
        });
}

void SpoolmanPanel::clear_list() {
    for (auto& row : spool_rows_) {
        if (row.container) {
            lv_obj_delete(row.container);
        }
    }
    spool_rows_.clear();
}

void SpoolmanPanel::populate_list(const std::vector<SpoolInfo>& spools) {
    clear_list();

    bool has_spools = !spools.empty();

    // Toggle visibility: show list OR empty state
    if (spool_list_container_) {
        if (has_spools) {
            lv_obj_remove_flag(spool_list_container_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(spool_list_container_, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (empty_state_container_) {
        if (has_spools) {
            lv_obj_add_flag(empty_state_container_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(empty_state_container_, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (!has_spools) {
        std::snprintf(status_buf_, sizeof(status_buf_), "");
        lv_subject_copy_string(&status_subject_, status_buf_);
        return;
    }

    // Create rows for each spool
    for (const auto& spool : spools) {
        create_spool_row(spool);
    }

    // Update status
    int low_count = 0;
    for (const auto& spool : spools) {
        if (spool.is_low(LOW_THRESHOLD_GRAMS)) {
            ++low_count;
        }
    }

    if (low_count > 0) {
        std::snprintf(status_buf_, sizeof(status_buf_), "%d spool%s low on filament", low_count,
                      low_count == 1 ? "" : "s");
    } else {
        std::snprintf(status_buf_, sizeof(status_buf_), "%zu spools", spools.size());
    }
    lv_subject_copy_string(&status_subject_, status_buf_);
}

void SpoolmanPanel::create_spool_row(const SpoolInfo& spool) {
    if (!spool_list_container_) {
        return;
    }

    // Create row using XML component
    lv_obj_t* row =
        static_cast<lv_obj_t*>(lv_xml_create(spool_list_container_, "spoolman_spool_row", nullptr));

    if (!row) {
        spdlog::error("[{}] Failed to create spoolman_spool_row for spool {}", get_name(),
                      spool.id);
        return;
    }

    // Store spool info in row
    SpoolRow spool_row;
    spool_row.container = row;
    spool_row.spool_id = spool.id;
    spool_row.is_active = spool.is_active;
    spool_rows_.push_back(spool_row);

    // Store spool_id for click handling (use the address in spool_rows_ since it's stable)
    lv_obj_set_user_data(row, reinterpret_cast<void*>(static_cast<intptr_t>(spool.id)));

    // Make row clickable
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, on_spool_clicked, LV_EVENT_CLICKED, nullptr);

    // Update visuals
    update_row_visuals(row, spool);
}

void SpoolmanPanel::update_row_visuals(lv_obj_t* row, const SpoolInfo& spool) {
    if (!row) {
        return;
    }

    // 3D Spool canvas - set color and fill level
    lv_obj_t* spool_canvas = lv_obj_find_by_name(row, "spool_canvas");
    if (spool_canvas) {
        // Parse hex color
        lv_color_t color = lv_color_hex(0x808080); // Default gray
        if (!spool.color_hex.empty()) {
            std::string hex = spool.color_hex;
            if (hex[0] == '#') {
                hex = hex.substr(1);
            }
            unsigned int color_val = 0;
            if (sscanf(hex.c_str(), "%x", &color_val) == 1) {
                color = lv_color_hex(color_val);
            }
        }
        ui_spool_canvas_set_color(spool_canvas, color);

        // Set fill level (0.0 to 1.0)
        float fill_level = static_cast<float>(spool.remaining_percent() / 100.0);
        ui_spool_canvas_set_fill_level(spool_canvas, fill_level);
        ui_spool_canvas_redraw(spool_canvas);
    }

    // Spool name (material + color)
    lv_obj_t* name_label = lv_obj_find_by_name(row, "spool_name");
    if (name_label) {
        std::string name = spool.material;
        if (!spool.color_name.empty()) {
            name += " - " + spool.color_name;
        }
        lv_label_set_text(name_label, name.c_str());
    }

    // Vendor
    lv_obj_t* vendor_label = lv_obj_find_by_name(row, "spool_vendor");
    if (vendor_label) {
        lv_label_set_text(vendor_label, spool.vendor.c_str());
    }

    // Weight text
    lv_obj_t* weight_label = lv_obj_find_by_name(row, "weight_text");
    if (weight_label) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.0fg", spool.remaining_weight_g);
        lv_label_set_text(weight_label, buf);

        // Color red if low
        if (spool.is_low(LOW_THRESHOLD_GRAMS)) {
            lv_obj_set_style_text_color(weight_label, ui_theme_get_color("error_color"), 0);
        } else {
            lv_obj_set_style_text_color(weight_label, ui_theme_get_color("text_primary"), 0);
        }
    }

    // Percent text
    lv_obj_t* percent_label = lv_obj_find_by_name(row, "percent_text");
    if (percent_label) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.0f%%", spool.remaining_percent());
        lv_label_set_text(percent_label, buf);
    }

    // Active indicator
    lv_obj_t* active_indicator = lv_obj_find_by_name(row, "active_indicator");
    if (active_indicator) {
        if (spool.is_active) {
            lv_obj_remove_flag(active_indicator, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(active_indicator, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Highlight row background if low
    if (spool.is_low(LOW_THRESHOLD_GRAMS)) {
        // Subtle red tint on background
        lv_obj_set_style_bg_color(row, lv_color_hex(0x3D2020), 0);
    }
}

// ============================================================================
// CLICK HANDLING
// ============================================================================

void SpoolmanPanel::handle_spool_clicked(int spool_id) {
    spdlog::info("[{}] Spool {} clicked - setting as active", get_name(), spool_id);

    if (!api_) {
        ui_toast_show(ToastSeverity::WARNING, "Not connected to printer", 2000);
        return;
    }

    // Find spool info
    std::string spool_name = "Spool";
    for (const auto& spool : cached_spools_) {
        if (spool.id == spool_id) {
            spool_name = spool.display_name();
            break;
        }
    }

    api_->set_active_spool(
        spool_id,
        [this, spool_id, spool_name]() {
            spdlog::info("[{}] Set active spool to {}", get_name(), spool_id);
            ui_toast_show(ToastSeverity::SUCCESS, ("Active: " + spool_name).c_str(), 2000);

            // Update UI to show new active spool
            update_active_indicator(spool_id);

            // Update cached state
            for (auto& spool : cached_spools_) {
                spool.is_active = (spool.id == spool_id);
            }
        },
        [this](const MoonrakerError& err) {
            spdlog::error("[{}] Failed to set active spool: {}", get_name(), err.message);
            ui_toast_show(ToastSeverity::ERROR, "Failed to set active spool", 2000);
        });
}

void SpoolmanPanel::update_active_indicator(int active_id) {
    for (auto& row : spool_rows_) {
        if (!row.container) {
            continue;
        }

        lv_obj_t* indicator = lv_obj_find_by_name(row.container, "active_indicator");
        if (indicator) {
            if (row.spool_id == active_id) {
                lv_obj_remove_flag(indicator, LV_OBJ_FLAG_HIDDEN);
                row.is_active = true;
            } else {
                lv_obj_add_flag(indicator, LV_OBJ_FLAG_HIDDEN);
                row.is_active = false;
            }
        }
    }
}

// ============================================================================
// STATIC CALLBACKS
// ============================================================================

void SpoolmanPanel::on_spool_clicked(lv_event_t* e) {
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
    if (!target) {
        return;
    }

    // Get spool_id from user data
    void* user_data = lv_obj_get_user_data(target);
    int spool_id = static_cast<int>(reinterpret_cast<intptr_t>(user_data));

    if (spool_id > 0) {
        get_global_spoolman_panel().handle_spool_clicked(spool_id);
    }
}

// ============================================================================
// GLOBAL INSTANCE
// ============================================================================

static std::unique_ptr<SpoolmanPanel> g_spoolman_panel;

SpoolmanPanel& get_global_spoolman_panel() {
    if (!g_spoolman_panel) {
        g_spoolman_panel =
            std::make_unique<SpoolmanPanel>(get_printer_state(), get_moonraker_api());
    }
    return *g_spoolman_panel;
}
