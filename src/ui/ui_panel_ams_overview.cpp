// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_panel_ams_overview.h"

#include "ui_ams_device_operations_overlay.h"
#include "ui_ams_slot.h"
#include "ui_ams_slot_layout.h"
#include "ui_error_reporting.h"
#include "ui_event_safety.h"
#include "ui_filament_path_canvas.h"
#include "ui_nav.h"
#include "ui_nav_manager.h"
#include "ui_panel_ams.h"
#include "ui_panel_common.h"
#include "ui_spool_canvas.h"
#include "ui_system_path_canvas.h"
#include "ui_utils.h"

#include "ams_backend.h"
#include "ams_state.h"
#include "ams_types.h"
#include "app_globals.h"
#include "lvgl/src/xml/lv_xml.h"
#include "observer_factory.h"
#include "printer_detector.h"
#include "settings_manager.h"
#include "static_panel_registry.h"
#include "theme_manager.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <functional>
#include <memory>

// ============================================================================
// Layout Constants
// ============================================================================

/// Minimum bar width for mini slot bars (prevents invisible bars)
static constexpr int32_t MINI_BAR_MIN_WIDTH_PX = 6;

/// Maximum bar width for mini slot bars
static constexpr int32_t MINI_BAR_MAX_WIDTH_PX = 14;

/// Height of each mini slot bar
/// TODO: Replace with theme_manager_get_spacing("ams_bars_height") to use
/// the responsive value from globals.xml instead of this compile-time constant.
static constexpr int32_t MINI_BAR_HEIGHT_PX = 40;

/// Border radius for bar corners
static constexpr int32_t MINI_BAR_RADIUS_PX = 4;

/// Zoom animation duration (ms) for detail view transitions
static constexpr uint32_t DETAIL_ZOOM_DURATION_MS = 200;

/// Zoom animation start scale (25% = 64/256)
static constexpr int32_t DETAIL_ZOOM_SCALE_MIN = 64;

/// Zoom animation end scale (100% = 256/256)
static constexpr int32_t DETAIL_ZOOM_SCALE_MAX = 256;

/// Height of status indicator line below each bar
static constexpr int32_t STATUS_LINE_HEIGHT_PX = 3;

/// Gap between bar and status line
static constexpr int32_t STATUS_LINE_GAP_PX = 2;

// Global instance pointer for XML callback access
static std::atomic<AmsOverviewPanel*> g_overview_panel_instance{nullptr};

/// Get a display name for a unit, falling back to "Unit N" (1-based)
static std::string get_unit_display_name(const AmsUnit& unit, int unit_index) {
    if (!unit.name.empty()) {
        return unit.name;
    }
    return "Unit " + std::to_string(unit_index + 1);
}

/// Set a label to "N slots" text, with null-safety
static void set_slot_count_label(lv_obj_t* label, int slot_count) {
    if (!label) {
        return;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%d slots", slot_count);
    lv_label_set_text(label, buf);
}

// ============================================================================
// XML Event Callback Wrappers
// ============================================================================

static void on_settings_clicked_xml(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[AMS Overview] on_settings_clicked");
    LV_UNUSED(e);

    spdlog::info("[AMS Overview] Opening AMS Device Operations overlay");

    auto& overlay = helix::ui::get_ams_device_operations_overlay();
    if (!overlay.are_subjects_initialized()) {
        overlay.init_subjects();
        overlay.register_callbacks();
    }

    auto* target = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    lv_obj_t* parent = lv_obj_get_screen(target);
    overlay.show(parent);

    LVGL_SAFE_EVENT_CB_END();
}

/// Execute a backend operation with standard error handling
static void dispatch_backend_op(const char* op_name,
                                std::function<AmsError(AmsBackend*)> operation) {
    AmsBackend* backend = AmsState::instance().get_backend();
    if (!backend) {
        NOTIFY_WARNING("AMS not available");
        return;
    }

    spdlog::info("[AMS Overview] {} requested", op_name);
    AmsError error = operation(backend);
    if (error.result != AmsResult::SUCCESS) {
        NOTIFY_ERROR("{} failed: {}", op_name, error.user_msg);
    }
}

static void on_unload_clicked_xml(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[AMS Overview] on_unload_clicked");
    LV_UNUSED(e);
    dispatch_backend_op("Unload", [](AmsBackend* b) { return b->unload_filament(); });
    LVGL_SAFE_EVENT_CB_END();
}

static void on_reset_clicked_xml(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[AMS Overview] on_reset_clicked");
    LV_UNUSED(e);
    dispatch_backend_op("Reset", [](AmsBackend* b) { return b->reset(); });
    LVGL_SAFE_EVENT_CB_END();
}

// ============================================================================
// Construction
// ============================================================================

AmsOverviewPanel::AmsOverviewPanel(PrinterState& printer_state, MoonrakerAPI* api)
    : PanelBase(printer_state, api) {
    spdlog::debug("[AMS Overview] Constructed");
}

// ============================================================================
// PanelBase Interface
// ============================================================================

void AmsOverviewPanel::init_subjects() {
    init_subjects_guarded([this]() {
        // AmsState handles all subject registration centrally.
        // Overview panel reuses existing AMS subjects (slots_version, etc.)
        AmsState::instance().init_subjects(true);

        // Observe slots_version to auto-refresh when slot data changes
        slots_version_observer_ = ObserverGuard(
            AmsState::instance().get_slots_version_subject(),
            [](lv_observer_t* observer, lv_subject_t* /*subject*/) {
                auto* self = static_cast<AmsOverviewPanel*>(lv_observer_get_user_data(observer));
                if (self && self->panel_) {
                    if (self->detail_unit_index_ >= 0) {
                        // In detail mode — refresh the detail slot view
                        self->show_unit_detail(self->detail_unit_index_);
                    } else {
                        self->refresh_units();
                    }
                }
            },
            this);
    });
}

void AmsOverviewPanel::setup(lv_obj_t* panel, lv_obj_t* parent_screen) {
    PanelBase::setup(panel, parent_screen);

    if (!panel_) {
        spdlog::error("[{}] NULL panel", get_name());
        return;
    }

    spdlog::debug("[{}] Setting up...", get_name());

    // Standard overlay panel setup (header bar, responsive padding)
    ui_overlay_panel_setup_standard(panel_, parent_screen_, "overlay_header", "overview_content");

    // Find the unit cards row container from XML
    cards_row_ = lv_obj_find_by_name(panel_, "unit_cards_row");
    if (!cards_row_) {
        spdlog::error("[{}] Could not find 'unit_cards_row' in XML", get_name());
        return;
    }

    // Find system path area and create path canvas widget
    system_path_area_ = lv_obj_find_by_name(panel_, "system_path_area");
    if (system_path_area_) {
        system_path_ = ui_system_path_canvas_create(system_path_area_);
        if (system_path_) {
            lv_obj_set_size(system_path_, LV_PCT(100), LV_PCT(100));
            spdlog::debug("[{}] Created system path canvas", get_name());
        }
    }

    // Find detail view containers
    detail_container_ = lv_obj_find_by_name(panel_, "unit_detail_container");
    detail_slot_grid_ = lv_obj_find_by_name(panel_, "detail_slot_grid");
    detail_labels_layer_ = lv_obj_find_by_name(panel_, "detail_labels_layer");
    detail_slot_tray_ = lv_obj_find_by_name(panel_, "detail_slot_tray");
    detail_path_canvas_ = lv_obj_find_by_name(panel_, "detail_path_canvas");

    // Store global instance for callback access
    g_overview_panel_instance.store(this);

    // Initial population from backend state
    refresh_units();

    spdlog::debug("[{}] Setup complete!", get_name());
}

void AmsOverviewPanel::on_activate() {
    spdlog::debug("[{}] Activated - syncing from backend", get_name());

    AmsState::instance().sync_from_backend();

    if (detail_unit_index_ >= 0) {
        // Re-entering while in detail mode — refresh the detail slots
        show_unit_detail(detail_unit_index_);
    } else {
        refresh_units();
    }
}

void AmsOverviewPanel::on_deactivate() {
    spdlog::debug("[{}] Deactivated", get_name());

    // Reset to overview mode so next open starts at the cards view
    if (detail_unit_index_ >= 0) {
        show_overview();
    }
}

// ============================================================================
// Unit Card Management
// ============================================================================

void AmsOverviewPanel::refresh_units() {
    if (!cards_row_) {
        return;
    }

    // TODO: Iterate all backends (0..backend_count) to aggregate units across
    // multiple simultaneous AMS systems. Currently only queries backend 0.
    auto* backend = AmsState::instance().get_backend();
    if (!backend) {
        spdlog::debug("[{}] No backend available", get_name());
        return;
    }

    AmsSystemInfo info = backend->get_system_info();
    int current_slot = lv_subject_get_int(AmsState::instance().get_current_slot_subject());

    int new_unit_count = static_cast<int>(info.units.size());
    int old_unit_count = static_cast<int>(unit_cards_.size());

    if (new_unit_count != old_unit_count) {
        // Unit count changed - rebuild all cards
        spdlog::debug("[{}] Unit count changed {} -> {}, rebuilding cards", get_name(),
                      old_unit_count, new_unit_count);
        create_unit_cards(info);
    } else {
        // Same number of units - update existing cards in place
        for (int i = 0; i < new_unit_count; ++i) {
            update_unit_card(unit_cards_[i], info.units[i], current_slot);
        }
    }

    // Update system path visualization
    refresh_system_path(info, current_slot);
}

void AmsOverviewPanel::create_unit_cards(const AmsSystemInfo& info) {
    if (!cards_row_) {
        return;
    }

    // Remove old card widgets
    lv_obj_clean(cards_row_);
    unit_cards_.clear();

    int current_slot = lv_subject_get_int(AmsState::instance().get_current_slot_subject());

    for (int i = 0; i < static_cast<int>(info.units.size()); ++i) {
        const AmsUnit& unit = info.units[i];
        UnitCard uc;
        uc.unit_index = i;

        // Create card from XML component — all static styling is declarative
        uc.card = static_cast<lv_obj_t*>(lv_xml_create(cards_row_, "ams_unit_card", nullptr));
        if (!uc.card) {
            spdlog::error("[{}] Failed to create ams_unit_card XML for unit {}", get_name(), i);
            continue;
        }

        // Flex grow so cards share available width equally
        lv_obj_set_flex_grow(uc.card, 1);

        // Store unit index for click handler
        // NOTE: lv_obj_add_event_cb used here (not XML event_cb) because each dynamically
        // created card needs per-instance user_data (unit index) that XML bindings can't provide.
        lv_obj_set_user_data(uc.card, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_add_event_cb(uc.card, on_unit_card_clicked, LV_EVENT_CLICKED, this);

        // Find child widgets declared in XML
        uc.logo_image = lv_obj_find_by_name(uc.card, "unit_logo");
        uc.name_label = lv_obj_find_by_name(uc.card, "unit_name");
        uc.bars_container = lv_obj_find_by_name(uc.card, "bars_container");
        uc.slot_count_label = lv_obj_find_by_name(uc.card, "slot_count");

        // Set logo image based on AMS system type
        if (uc.logo_image) {
            // Try unit name first (e.g., "Box Turtle 1", "Night Owl"),
            // fall back to system type name (e.g., "AFC", "Happy Hare")
            const char* logo_path = AmsState::get_logo_path(unit.name);
            if (!logo_path || !logo_path[0]) {
                logo_path = AmsState::get_logo_path(info.type_name);
            }
            if (logo_path && logo_path[0]) {
                lv_image_set_src(uc.logo_image, logo_path);
            } else {
                lv_obj_add_flag(uc.logo_image, LV_OBJ_FLAG_HIDDEN);
            }
        }

        // Set dynamic content only — unit name and slot count vary per unit
        if (uc.name_label) {
            lv_label_set_text(uc.name_label, get_unit_display_name(unit, i).c_str());
        }

        set_slot_count_label(uc.slot_count_label, unit.slot_count);

        // Create the mini bars for this unit (dynamic — slot count varies)
        create_mini_bars(uc, unit, current_slot);

        unit_cards_.push_back(uc);
    }

    spdlog::debug("[{}] Created {} unit cards from XML (bypass={})", get_name(),
                  static_cast<int>(unit_cards_.size()), info.supports_bypass);
}

void AmsOverviewPanel::update_unit_card(UnitCard& card, const AmsUnit& unit, int current_slot) {
    if (!card.card) {
        return;
    }

    // Update name label
    if (card.name_label) {
        lv_label_set_text(card.name_label, get_unit_display_name(unit, card.unit_index).c_str());
    }

    // Rebuild mini bars (slot colors/status may have changed)
    if (card.bars_container) {
        lv_obj_clean(card.bars_container);
        create_mini_bars(card, unit, current_slot);
    }

    // Update slot count
    set_slot_count_label(card.slot_count_label, unit.slot_count);
}

void AmsOverviewPanel::create_mini_bars(UnitCard& card, const AmsUnit& unit, int current_slot) {
    if (!card.bars_container) {
        return;
    }

    int slot_count = static_cast<int>(unit.slots.size());
    if (slot_count <= 0) {
        return;
    }

    // Calculate bar width to fit within bars_container
    // Force layout to get actual container width, then divide among slots
    lv_obj_update_layout(card.bars_container);
    int32_t container_width = lv_obj_get_content_width(card.bars_container);
    if (container_width <= 0) {
        container_width = 80; // Fallback if layout not yet calculated
    }
    int32_t gap = theme_manager_get_spacing("space_xxs");
    int32_t total_gaps = (slot_count > 1) ? (slot_count - 1) * gap : 0;
    int32_t bar_width = (container_width - total_gaps) / std::max(1, slot_count);
    bar_width = std::clamp(bar_width, MINI_BAR_MIN_WIDTH_PX, MINI_BAR_MAX_WIDTH_PX);

    for (int s = 0; s < slot_count; ++s) {
        const SlotInfo& slot = unit.slots[s];
        int global_idx = unit.first_slot_global_index + s;
        bool is_loaded = (global_idx == current_slot);
        bool is_present =
            (slot.status == SlotStatus::AVAILABLE || slot.status == SlotStatus::LOADED ||
             slot.status == SlotStatus::FROM_BUFFER);
        bool has_error = (slot.status == SlotStatus::BLOCKED);

        // Slot column container (bar + status line)
        lv_obj_t* slot_col = lv_obj_create(card.bars_container);
        lv_obj_remove_flag(slot_col, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(slot_col, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_size(slot_col, bar_width,
                        MINI_BAR_HEIGHT_PX + STATUS_LINE_HEIGHT_PX + STATUS_LINE_GAP_PX);
        lv_obj_set_flex_flow(slot_col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(slot_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(slot_col, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_row(slot_col, STATUS_LINE_GAP_PX, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(slot_col, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(slot_col, 0, LV_PART_MAIN);

        // Bar background (outline container)
        lv_obj_t* bar_bg = lv_obj_create(slot_col);
        lv_obj_remove_flag(bar_bg, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(bar_bg, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_size(bar_bg, bar_width, MINI_BAR_HEIGHT_PX);
        lv_obj_set_style_radius(bar_bg, MINI_BAR_RADIUS_PX, LV_PART_MAIN);
        lv_obj_set_style_pad_all(bar_bg, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar_bg, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(bar_bg, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(bar_bg, theme_manager_get_color("text_muted"), LV_PART_MAIN);
        lv_obj_set_style_border_opa(bar_bg, is_present ? LV_OPA_50 : LV_OPA_20, LV_PART_MAIN);

        // Fill portion (colored, anchored to bottom)
        if (is_present) {
            lv_obj_t* bar_fill = lv_obj_create(bar_bg);
            lv_obj_remove_flag(bar_fill, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(bar_fill, LV_OBJ_FLAG_EVENT_BUBBLE);
            lv_obj_set_width(bar_fill, LV_PCT(100));
            lv_obj_set_style_border_width(bar_fill, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(bar_fill, 0, LV_PART_MAIN);
            lv_obj_set_style_radius(bar_fill, MINI_BAR_RADIUS_PX, LV_PART_MAIN);

            // Color gradient (lighter at top, darker at bottom)
            lv_color_t base_color = lv_color_hex(slot.color_rgb);
            lv_color_t light_color = lv_color_make(std::min(255, base_color.red + 50),
                                                   std::min(255, base_color.green + 50),
                                                   std::min(255, base_color.blue + 50));
            lv_obj_set_style_bg_color(bar_fill, light_color, LV_PART_MAIN);
            lv_obj_set_style_bg_grad_color(bar_fill, base_color, LV_PART_MAIN);
            lv_obj_set_style_bg_grad_dir(bar_fill, LV_GRAD_DIR_VER, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(bar_fill, LV_OPA_COVER, LV_PART_MAIN);

            // Fill height based on weight percentage (default 100% if unknown)
            float pct = slot.get_remaining_percent();
            int fill_pct = (pct >= 0) ? static_cast<int>(pct) : 100;
            fill_pct = std::clamp(fill_pct, 5, 100); // Minimum 5% so bar is visible
            lv_obj_set_height(bar_fill, LV_PCT(fill_pct));
            lv_obj_align(bar_fill, LV_ALIGN_BOTTOM_MID, 0, 0);
        }

        // Status line below bar (green=loaded, red=error)
        lv_obj_t* status_line = lv_obj_create(slot_col);
        lv_obj_remove_flag(status_line, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(status_line, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_size(status_line, bar_width, STATUS_LINE_HEIGHT_PX);
        lv_obj_set_style_border_width(status_line, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(status_line, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(status_line, MINI_BAR_RADIUS_PX / 2, LV_PART_MAIN);

        if (has_error) {
            lv_obj_set_style_bg_color(status_line, theme_manager_get_color("danger"), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(status_line, LV_OPA_COVER, LV_PART_MAIN);
        } else if (is_loaded) {
            lv_obj_set_style_bg_color(status_line, theme_manager_get_color("success"),
                                      LV_PART_MAIN);
            lv_obj_set_style_bg_opa(status_line, LV_OPA_COVER, LV_PART_MAIN);
        } else {
            lv_obj_add_flag(status_line, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// ============================================================================
// System Path
// ============================================================================

void AmsOverviewPanel::refresh_system_path(const AmsSystemInfo& info, int current_slot) {
    if (!system_path_)
        return;

    int unit_count = static_cast<int>(info.units.size());
    ui_system_path_canvas_set_unit_count(system_path_, unit_count);

    // Calculate and set X positions based on unit card positions
    // Force layout so we can get accurate card positions
    if (cards_row_) {
        lv_obj_update_layout(cards_row_);
    }

    for (int i = 0; i < unit_count && i < static_cast<int>(unit_cards_.size()); ++i) {
        if (unit_cards_[i].card) {
            // Get card center X relative to the system path widget
            lv_obj_update_layout(unit_cards_[i].card);
            lv_area_t card_coords;
            lv_obj_get_coords(unit_cards_[i].card, &card_coords);

            lv_area_t path_coords;
            lv_obj_get_coords(system_path_, &path_coords);
            int32_t card_center_x = (card_coords.x1 + card_coords.x2) / 2 - path_coords.x1;
            ui_system_path_canvas_set_unit_x(system_path_, i, card_center_x);
        }
    }

    // Set active unit based on current slot
    int active_unit = info.get_active_unit_index();
    ui_system_path_canvas_set_active_unit(system_path_, active_unit);

    // Set filament color from active slot
    if (current_slot >= 0) {
        const SlotInfo* slot = info.get_slot_global(current_slot);
        if (slot) {
            ui_system_path_canvas_set_active_color(system_path_, slot->color_rgb);
        }
    }

    // Set whether filament is fully loaded
    ui_system_path_canvas_set_filament_loaded(system_path_, info.filament_loaded);

    // Set bypass path state (bypass is drawn inside the canvas, no card needed)
    bool bypass_active = info.supports_bypass && (current_slot == -2);
    ui_system_path_canvas_set_bypass(system_path_, info.supports_bypass, bypass_active, 0x888888);

    // Set per-unit hub sensor states
    for (int i = 0; i < unit_count && i < static_cast<int>(info.units.size()); ++i) {
        ui_system_path_canvas_set_unit_hub_sensor(system_path_, i, info.units[i].has_hub_sensor,
                                                  info.units[i].hub_sensor_triggered);
    }

    // Set toolhead sensor state
    {
        auto segment = static_cast<PathSegment>(
            lv_subject_get_int(AmsState::instance().get_path_filament_segment_subject()));
        bool toolhead_triggered = (segment >= PathSegment::TOOLHEAD);

        bool has_toolhead = std::any_of(info.units.begin(), info.units.end(),
                                        [](const AmsUnit& u) { return u.has_toolhead_sensor; });
        ui_system_path_canvas_set_toolhead_sensor(system_path_, has_toolhead, toolhead_triggered);
    }

    // Update currently loaded swatch color (imperative — color subject is int, not CSS)
    if (panel_) {
        lv_obj_t* swatch = lv_obj_find_by_name(panel_, "overview_swatch");
        if (swatch) {
            lv_color_t color = lv_color_hex(static_cast<uint32_t>(
                lv_subject_get_int(AmsState::instance().get_current_color_subject())));
            lv_obj_set_style_bg_color(swatch, color, 0);
            lv_obj_set_style_border_color(swatch, color, 0);
        }
    }

    // Set status text from action detail subject (drawn to left of nozzle)
    lv_subject_t* action_subject = AmsState::instance().get_ams_action_detail_subject();
    if (action_subject) {
        const char* action_text = lv_subject_get_string(action_subject);
        ui_system_path_canvas_set_status_text(system_path_, action_text);
    }

    ui_system_path_canvas_refresh(system_path_);
}

// ============================================================================
// Event Handling
// ============================================================================

void AmsOverviewPanel::on_unit_card_clicked(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[AMS Overview] on_unit_card_clicked");

    auto* self = static_cast<AmsOverviewPanel*>(lv_event_get_user_data(e));
    if (!self) {
        spdlog::warn("[AMS Overview] Card clicked but panel instance is null");
        return;
    }

    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    int unit_index = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(target)));

    spdlog::info("[AMS Overview] Unit {} clicked - showing inline detail", unit_index);

    // Show detail view inline (swaps left column content, no overlay push)
    self->show_unit_detail(unit_index);

    LVGL_SAFE_EVENT_CB_END();
}

// ============================================================================
// Detail View (inline unit zoom)
// ============================================================================

void AmsOverviewPanel::show_unit_detail(int unit_index) {
    if (!panel_ || !detail_container_ || !cards_row_)
        return;

    // Cancel any in-flight zoom animations to prevent race conditions
    lv_anim_delete(detail_container_, nullptr);

    auto* backend = AmsState::instance().get_backend();
    if (!backend)
        return;

    AmsSystemInfo info = backend->get_system_info();
    if (unit_index < 0 || unit_index >= static_cast<int>(info.units.size()))
        return;

    // Capture clicked card's screen center BEFORE hiding overview elements
    lv_area_t card_coords = {};
    if (unit_index < static_cast<int>(unit_cards_.size()) && unit_cards_[unit_index].card) {
        lv_obj_update_layout(unit_cards_[unit_index].card);
        lv_obj_get_coords(unit_cards_[unit_index].card, &card_coords);
    }

    detail_unit_index_ = unit_index;
    const AmsUnit& unit = info.units[unit_index];

    spdlog::info("[{}] Showing detail for unit {} ({})", get_name(), unit_index, unit.name);

    // Update detail header (logo + name)
    update_detail_header(unit, info);

    // Create slot widgets for this unit
    create_detail_slots(unit);

    // Configure path canvas for this unit's filament routing
    setup_detail_path_canvas(unit, info);

    // Swap visibility: hide overview elements, show detail
    lv_obj_add_flag(cards_row_, LV_OBJ_FLAG_HIDDEN);
    if (system_path_area_)
        lv_obj_add_flag(system_path_area_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(detail_container_, LV_OBJ_FLAG_HIDDEN);

    // Zoom-in animation (scale + fade) — gated on animations setting
    if (SettingsManager::instance().get_animations_enabled()) {
        // Set transform pivot to the clicked card's center relative to detail container
        lv_obj_update_layout(detail_container_);
        lv_area_t detail_coords;
        lv_obj_get_coords(detail_container_, &detail_coords);
        int32_t pivot_x = (card_coords.x1 + card_coords.x2) / 2 - detail_coords.x1;
        int32_t pivot_y = (card_coords.y1 + card_coords.y2) / 2 - detail_coords.y1;
        lv_obj_set_style_transform_pivot_x(detail_container_, pivot_x, LV_PART_MAIN);
        lv_obj_set_style_transform_pivot_y(detail_container_, pivot_y, LV_PART_MAIN);

        // Start small and transparent
        lv_obj_set_style_transform_scale(detail_container_, DETAIL_ZOOM_SCALE_MIN, LV_PART_MAIN);
        lv_obj_set_style_opa(detail_container_, LV_OPA_TRANSP, LV_PART_MAIN);

        // Scale animation
        lv_anim_t scale_anim;
        lv_anim_init(&scale_anim);
        lv_anim_set_var(&scale_anim, detail_container_);
        lv_anim_set_values(&scale_anim, DETAIL_ZOOM_SCALE_MIN, DETAIL_ZOOM_SCALE_MAX);
        lv_anim_set_duration(&scale_anim, DETAIL_ZOOM_DURATION_MS);
        lv_anim_set_path_cb(&scale_anim, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&scale_anim, [](void* obj, int32_t value) {
            lv_obj_set_style_transform_scale(static_cast<lv_obj_t*>(obj), value, LV_PART_MAIN);
        });
        lv_anim_start(&scale_anim);

        // Fade animation
        lv_anim_t fade_anim;
        lv_anim_init(&fade_anim);
        lv_anim_set_var(&fade_anim, detail_container_);
        lv_anim_set_values(&fade_anim, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_anim_set_duration(&fade_anim, DETAIL_ZOOM_DURATION_MS);
        lv_anim_set_path_cb(&fade_anim, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&fade_anim, [](void* obj, int32_t value) {
            lv_obj_set_style_opa(static_cast<lv_obj_t*>(obj), value, LV_PART_MAIN);
        });
        lv_anim_start(&fade_anim);
    } else {
        // No animation — ensure final state
        lv_obj_set_style_transform_scale(detail_container_, DETAIL_ZOOM_SCALE_MAX, LV_PART_MAIN);
        lv_obj_set_style_opa(detail_container_, LV_OPA_COVER, LV_PART_MAIN);
    }
}

void AmsOverviewPanel::show_overview() {
    if (!panel_ || !detail_container_ || !cards_row_)
        return;

    // Cancel any in-flight zoom animations to prevent race conditions
    lv_anim_delete(detail_container_, nullptr);

    spdlog::info("[{}] Returning to overview mode", get_name());

    detail_unit_index_ = -1;

    if (SettingsManager::instance().get_animations_enabled()) {
        // Zoom-out animation: scale down + fade out, then swap visibility
        // Transform pivot is still set from the zoom-in (card center position)
        lv_anim_t scale_anim;
        lv_anim_init(&scale_anim);
        lv_anim_set_var(&scale_anim, detail_container_);
        lv_anim_set_values(&scale_anim, DETAIL_ZOOM_SCALE_MAX, DETAIL_ZOOM_SCALE_MIN);
        lv_anim_set_duration(&scale_anim, DETAIL_ZOOM_DURATION_MS);
        lv_anim_set_path_cb(&scale_anim, lv_anim_path_ease_in);
        lv_anim_set_exec_cb(&scale_anim, [](void* obj, int32_t value) {
            lv_obj_set_style_transform_scale(static_cast<lv_obj_t*>(obj), value, LV_PART_MAIN);
        });
        // On complete: swap visibility and clean up
        lv_anim_set_completed_cb(&scale_anim, [](lv_anim_t* anim) {
            auto* container = static_cast<lv_obj_t*>(anim->var);
            lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
            // Reset transform properties for next use
            lv_obj_set_style_transform_scale(container, DETAIL_ZOOM_SCALE_MAX, LV_PART_MAIN);
            lv_obj_set_style_opa(container, LV_OPA_COVER, LV_PART_MAIN);

            // Show overview elements (use global instance since lambda has no 'this')
            AmsOverviewPanel* self = g_overview_panel_instance.load();
            if (self) {
                self->destroy_detail_slots();
                if (self->cards_row_)
                    lv_obj_remove_flag(self->cards_row_, LV_OBJ_FLAG_HIDDEN);
                if (self->system_path_area_)
                    lv_obj_remove_flag(self->system_path_area_, LV_OBJ_FLAG_HIDDEN);
                self->refresh_units();
            }
        });
        lv_anim_start(&scale_anim);

        // Fade animation
        lv_anim_t fade_anim;
        lv_anim_init(&fade_anim);
        lv_anim_set_var(&fade_anim, detail_container_);
        lv_anim_set_values(&fade_anim, LV_OPA_COVER, LV_OPA_TRANSP);
        lv_anim_set_duration(&fade_anim, DETAIL_ZOOM_DURATION_MS);
        lv_anim_set_path_cb(&fade_anim, lv_anim_path_ease_in);
        lv_anim_set_exec_cb(&fade_anim, [](void* obj, int32_t value) {
            lv_obj_set_style_opa(static_cast<lv_obj_t*>(obj), value, LV_PART_MAIN);
        });
        lv_anim_start(&fade_anim);
    } else {
        // No animation — instant swap
        destroy_detail_slots();
        lv_obj_remove_flag(cards_row_, LV_OBJ_FLAG_HIDDEN);
        if (system_path_area_)
            lv_obj_remove_flag(system_path_area_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(detail_container_, LV_OBJ_FLAG_HIDDEN);
        refresh_units();
    }
}

void AmsOverviewPanel::update_detail_header(const AmsUnit& unit, const AmsSystemInfo& info) {
    // Update logo
    lv_obj_t* logo = lv_obj_find_by_name(panel_, "detail_logo");
    if (logo) {
        const char* logo_path = AmsState::get_logo_path(unit.name);
        if (!logo_path || !logo_path[0]) {
            logo_path = AmsState::get_logo_path(info.type_name);
        }
        if (logo_path && logo_path[0]) {
            lv_image_set_src(logo, logo_path);
            lv_obj_remove_flag(logo, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(logo, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Update name
    lv_obj_t* name = lv_obj_find_by_name(panel_, "detail_unit_name");
    if (name) {
        lv_label_set_text(name, get_unit_display_name(unit, detail_unit_index_).c_str());
    }
}

void AmsOverviewPanel::create_detail_slots(const AmsUnit& unit) {
    if (!detail_slot_grid_)
        return;

    // Clear any existing detail slots
    destroy_detail_slots();

    int count = unit.slot_count;
    if (count <= 0 || count > MAX_DETAIL_SLOTS)
        return;

    int slot_offset = unit.first_slot_global_index;

    // Create slot widgets via XML
    for (int i = 0; i < count; ++i) {
        lv_obj_t* slot =
            static_cast<lv_obj_t*>(lv_xml_create(detail_slot_grid_, "ams_slot", nullptr));
        if (!slot) {
            spdlog::error("[{}] Failed to create ams_slot for detail index {}", get_name(), i);
            continue;
        }

        int global_index = i + slot_offset;
        ui_ams_slot_set_index(slot, global_index);
        ui_ams_slot_set_layout_info(slot, i, count);
        detail_slot_widgets_[i] = slot;
    }

    detail_slot_count_ = count;

    // Calculate slot sizing using shared layout helper
    lv_obj_t* slot_area = lv_obj_get_parent(detail_slot_grid_);
    lv_obj_update_layout(slot_area);
    int32_t available_width = lv_obj_get_content_width(slot_area);
    auto layout = calculate_ams_slot_layout(available_width, count);

    lv_obj_set_style_pad_column(detail_slot_grid_, layout.overlap > 0 ? -layout.overlap : 0,
                                LV_PART_MAIN);

    for (int i = 0; i < count; ++i) {
        if (detail_slot_widgets_[i]) {
            lv_obj_set_width(detail_slot_widgets_[i], layout.slot_width);
        }
    }

    // Update tray height to ~1/3 of slot height
    if (detail_slot_tray_ && count > 0 && detail_slot_widgets_[0]) {
        lv_obj_update_layout(detail_slot_widgets_[0]);
        int32_t slot_height = lv_obj_get_height(detail_slot_widgets_[0]);
        int32_t tray_height = slot_height / 3;
        if (tray_height < 20)
            tray_height = 20;
        lv_obj_set_height(detail_slot_tray_, tray_height);
    }

    // Move labels to overlay layer so they render on top of overlapping slots.
    // Only needed when slots overlap (5+ slots use negative column padding).
    if (detail_labels_layer_ && count > 4) {
        lv_obj_clean(detail_labels_layer_);

        int32_t slot_spacing = layout.slot_width - layout.overlap;
        for (int i = 0; i < count; ++i) {
            if (detail_slot_widgets_[i]) {
                int32_t slot_center_x = layout.slot_width / 2 + i * slot_spacing;
                ui_ams_slot_move_label_to_layer(detail_slot_widgets_[i], detail_labels_layer_,
                                                slot_center_x);
            }
        }
        spdlog::debug("[{}] Moved {} detail labels to overlay layer", get_name(), count);
    }

    spdlog::debug("[{}] Created {} detail slots (offset={}, width={})", get_name(), count,
                  slot_offset, layout.slot_width);
}

void AmsOverviewPanel::destroy_detail_slots() {
    if (detail_slot_grid_) {
        lv_obj_clean(detail_slot_grid_);
    }
    std::fill(std::begin(detail_slot_widgets_), std::end(detail_slot_widgets_), nullptr);
    detail_slot_count_ = 0;
}

void AmsOverviewPanel::setup_detail_path_canvas(const AmsUnit& unit, const AmsSystemInfo& info) {
    if (!detail_path_canvas_)
        return;

    auto* backend = AmsState::instance().get_backend();
    if (!backend)
        return;

    // Hub-only mode: only draw slots → hub, skip downstream (shown by system_path_canvas)
    ui_filament_path_canvas_set_hub_only(detail_path_canvas_, true);

    // Configure canvas for this unit's local slot count
    ui_filament_path_canvas_set_slot_count(detail_path_canvas_, unit.slot_count);
    ui_filament_path_canvas_set_topology(detail_path_canvas_,
                                         static_cast<int>(backend->get_topology()));

    // Sync slot sizing with the detail slot grid layout
    if (detail_slot_grid_) {
        lv_obj_t* slot_area = lv_obj_get_parent(detail_slot_grid_);
        lv_obj_update_layout(slot_area);
        int32_t available_width = lv_obj_get_content_width(slot_area);
        auto layout = calculate_ams_slot_layout(available_width, unit.slot_count);

        ui_filament_path_canvas_set_slot_width(detail_path_canvas_, layout.slot_width);
        ui_filament_path_canvas_set_slot_overlap(detail_path_canvas_, layout.overlap);
    }

    // Map global active slot to local index for this unit
    int local_active = info.current_slot - unit.first_slot_global_index;
    if (local_active >= 0 && local_active < unit.slot_count) {
        ui_filament_path_canvas_set_active_slot(detail_path_canvas_, local_active);

        // Set filament color from the active slot
        SlotInfo slot_info = backend->get_slot_info(info.current_slot);
        ui_filament_path_canvas_set_filament_color(detail_path_canvas_, slot_info.color_rgb);
    } else {
        ui_filament_path_canvas_set_active_slot(detail_path_canvas_, -1);
    }

    // Set filament segment position
    PathSegment segment = backend->get_filament_segment();
    ui_filament_path_canvas_set_filament_segment(detail_path_canvas_, static_cast<int>(segment));

    // Set error segment if any
    PathSegment error_seg = backend->infer_error_segment();
    ui_filament_path_canvas_set_error_segment(detail_path_canvas_, static_cast<int>(error_seg));

    // Use Stealthburner toolhead for Voron printers
    if (PrinterDetector::is_voron_printer()) {
        ui_filament_path_canvas_set_faceted_toolhead(detail_path_canvas_, true);
    }

    // Set per-slot filament states using LOCAL indices (0..unit.slot_count-1)
    ui_filament_path_canvas_clear_slot_filaments(detail_path_canvas_);
    for (int i = 0; i < unit.slot_count; ++i) {
        int global_idx = i + unit.first_slot_global_index;
        PathSegment slot_seg = backend->get_slot_filament_segment(global_idx);
        if (slot_seg != PathSegment::NONE) {
            SlotInfo si = backend->get_slot_info(global_idx);
            ui_filament_path_canvas_set_slot_filament(detail_path_canvas_, i,
                                                      static_cast<int>(slot_seg), si.color_rgb);
        }
    }

    ui_filament_path_canvas_refresh(detail_path_canvas_);

    spdlog::debug("[{}] Detail path canvas configured: slots={}, topology={}", get_name(),
                  unit.slot_count, static_cast<int>(backend->get_topology()));
}

// ============================================================================
// Cleanup
// ============================================================================

void AmsOverviewPanel::clear_panel_reference() {
    // Clear observer guards before clearing widget pointers
    slots_version_observer_.reset();

    // Clear global instance pointer
    g_overview_panel_instance.store(nullptr);

    // Clear widget references
    system_path_ = nullptr;
    system_path_area_ = nullptr;
    panel_ = nullptr;
    parent_screen_ = nullptr;
    cards_row_ = nullptr;
    unit_cards_.clear();

    // Cancel any in-flight animations before clearing pointers (prevents use-after-free)
    if (detail_container_) {
        lv_anim_delete(detail_container_, nullptr);
    }

    // Clear detail view state
    detail_container_ = nullptr;
    detail_slot_grid_ = nullptr;
    detail_labels_layer_ = nullptr;
    detail_slot_tray_ = nullptr;
    detail_path_canvas_ = nullptr;
    detail_unit_index_ = -1;
    detail_slot_count_ = 0;
    std::fill(std::begin(detail_slot_widgets_), std::end(detail_slot_widgets_), nullptr);

    // Reset subjects_initialized_ so observers are recreated on next access
    subjects_initialized_ = false;
}

// ============================================================================
// Global Instance
// ============================================================================

static std::unique_ptr<AmsOverviewPanel> g_ams_overview_panel;
static lv_obj_t* s_ams_overview_panel_obj = nullptr;

// Lazy registration flag for XML component
static bool s_overview_registered = false;

static void ensure_overview_registered() {
    if (s_overview_registered) {
        return;
    }

    spdlog::info("[AMS Overview] Lazy-registering XML component");

    // Register XML event callbacks before component registration
    lv_xml_register_event_cb(nullptr, "on_ams_overview_settings_clicked", on_settings_clicked_xml);
    lv_xml_register_event_cb(nullptr, "on_ams_overview_unload_clicked", on_unload_clicked_xml);
    lv_xml_register_event_cb(nullptr, "on_ams_overview_reset_clicked", on_reset_clicked_xml);
    lv_xml_register_event_cb(nullptr, "on_ams_overview_back_clicked", [](lv_event_t* e) {
        LV_UNUSED(e);
        AmsOverviewPanel* panel = g_overview_panel_instance.load();
        if (panel) {
            panel->show_overview();
        }
    });

    // Register canvas widgets
    ui_system_path_canvas_register();
    ui_filament_path_canvas_register();

    // Register AMS slot widgets for inline detail view
    // (safe to call multiple times — each register function has an internal guard)
    ui_spool_canvas_register();
    ui_ams_slot_register();

    // Register the XML components (unit card must be registered before overview panel)
    lv_xml_register_component_from_file("A:ui_xml/ams_unit_card.xml");
    lv_xml_register_component_from_file("A:ui_xml/ams_overview_panel.xml");

    s_overview_registered = true;
    spdlog::debug("[AMS Overview] XML registration complete");
}

void destroy_ams_overview_panel_ui() {
    if (s_ams_overview_panel_obj) {
        spdlog::info("[AMS Overview] Destroying panel UI to free memory");

        NavigationManager::instance().unregister_overlay_close_callback(s_ams_overview_panel_obj);

        if (g_ams_overview_panel) {
            g_ams_overview_panel->clear_panel_reference();
        }

        lv_obj_safe_delete(s_ams_overview_panel_obj);
    }
}

AmsOverviewPanel& get_global_ams_overview_panel() {
    if (!g_ams_overview_panel) {
        g_ams_overview_panel =
            std::make_unique<AmsOverviewPanel>(get_printer_state(), get_moonraker_api());
        StaticPanelRegistry::instance().register_destroy("AmsOverviewPanel",
                                                         []() { g_ams_overview_panel.reset(); });
    }

    // Lazy create the panel UI if not yet created
    if (!s_ams_overview_panel_obj && g_ams_overview_panel) {
        ensure_overview_registered();

        // Initialize AmsState subjects BEFORE XML creation so bindings work
        AmsState::instance().init_subjects(true);

        // Create the panel on the active screen
        lv_obj_t* screen = lv_scr_act();
        s_ams_overview_panel_obj =
            static_cast<lv_obj_t*>(lv_xml_create(screen, "ams_overview_panel", nullptr));

        if (s_ams_overview_panel_obj) {
            // Initialize panel observers
            if (!g_ams_overview_panel->are_subjects_initialized()) {
                g_ams_overview_panel->init_subjects();
            }

            // Setup the panel
            g_ams_overview_panel->setup(s_ams_overview_panel_obj, screen);
            lv_obj_add_flag(s_ams_overview_panel_obj, LV_OBJ_FLAG_HIDDEN);

            // Register overlay instance for lifecycle management
            NavigationManager::instance().register_overlay_instance(s_ams_overview_panel_obj,
                                                                    g_ams_overview_panel.get());

            // Register close callback to destroy UI when overlay is closed
            NavigationManager::instance().register_overlay_close_callback(
                s_ams_overview_panel_obj, []() { destroy_ams_overview_panel_ui(); });

            spdlog::info("[AMS Overview] Lazy-created panel UI with close callback");
        } else {
            spdlog::error("[AMS Overview] Failed to create panel from XML");
        }
    }

    return *g_ams_overview_panel;
}

// ============================================================================
// Multi-unit Navigation
// ============================================================================

void navigate_to_ams_panel() {
    auto* backend = AmsState::instance().get_backend();
    if (!backend) {
        spdlog::warn("[AMS] navigate_to_ams_panel called with no backend");
        return;
    }

    AmsSystemInfo info = backend->get_system_info();

    if (info.is_multi_unit()) {
        // Multi-unit: show overview panel
        spdlog::info("[AMS] Multi-unit setup ({} units) - showing overview", info.unit_count());
        auto& overview = get_global_ams_overview_panel();
        lv_obj_t* panel = overview.get_panel();
        if (panel) {
            NavigationManager::instance().push_overlay(panel);
        }
    } else {
        // Single-unit (or no units): go directly to detail panel
        spdlog::info("[AMS] Single-unit setup - showing detail panel directly");
        auto& detail = get_global_ams_panel();
        lv_obj_t* panel = detail.get_panel();
        if (panel) {
            NavigationManager::instance().push_overlay(panel);
        }
    }
}
