// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of HelixScreen.
 *
 * HelixScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HelixScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HelixScreen. If not, see <https://www.gnu.org/licenses/>.
 */

#include "ui_toast.h"
#include "ui_theme.h"
#include "ui_status_bar.h"
#include <spdlog/spdlog.h>

// Active toast state
static lv_obj_t* active_toast = nullptr;
static lv_timer_t* dismiss_timer = nullptr;

// Forward declarations
static void toast_dismiss_timer_cb(lv_timer_t* timer);
static void toast_close_btn_clicked(lv_event_t* e);

void ui_toast_init() {
    spdlog::debug("Toast notification system initialized");
}

static const char* severity_to_color_const(ToastSeverity severity) {
    switch (severity) {
        case ToastSeverity::INFO:
            return "info_color";
        case ToastSeverity::SUCCESS:
            return "success_color";
        case ToastSeverity::WARNING:
            return "warning_color";
        case ToastSeverity::ERROR:
            return "error_color";
        default:
            return "text_secondary";
    }
}

static const char* severity_to_icon(ToastSeverity severity) {
    switch (severity) {
        case ToastSeverity::INFO:
            return LV_SYMBOL_WARNING;  // Could use INFO icon if available
        case ToastSeverity::SUCCESS:
            return LV_SYMBOL_OK;
        case ToastSeverity::WARNING:
            return LV_SYMBOL_WARNING;
        case ToastSeverity::ERROR:
            return LV_SYMBOL_CLOSE;
        default:
            return LV_SYMBOL_WARNING;
    }
}

static NotificationStatus severity_to_notification_status(ToastSeverity severity) {
    switch (severity) {
        case ToastSeverity::INFO:
            return NotificationStatus::INFO;
        case ToastSeverity::SUCCESS:
            return NotificationStatus::INFO;  // Treat success as info in status bar
        case ToastSeverity::WARNING:
            return NotificationStatus::WARNING;
        case ToastSeverity::ERROR:
            return NotificationStatus::ERROR;
        default:
            return NotificationStatus::NONE;
    }
}

void ui_toast_show(ToastSeverity severity, const char* message, uint32_t duration_ms) {
    if (!message) {
        spdlog::warn("Attempted to show toast with null message");
        return;
    }

    // Hide existing toast if any
    if (active_toast) {
        ui_toast_hide();
    }

    // Get color constant for this severity
    const char* color_const = severity_to_color_const(severity);
    lv_color_t border_color = ui_theme_parse_color(lv_xml_get_const(NULL, color_const));

    // Convert color to string for XML attribute
    char color_str[16];
    snprintf(color_str, sizeof(color_str), "#%06X", lv_color_to_u32(border_color) & 0xFFFFFF);

    // Create toast via XML component
    const char* attrs[] = {
        "message", message,
        "border_color", color_str,
        nullptr
    };

    lv_obj_t* screen = lv_screen_active();
    lv_xml_create(screen, "toast_notification", attrs);

    // Find the created toast (should be last child of screen)
    uint32_t child_cnt = lv_obj_get_child_count(screen);
    active_toast = (child_cnt > 0) ? lv_obj_get_child(screen, child_cnt - 1) : nullptr;

    if (!active_toast) {
        spdlog::error("Failed to create toast notification widget");
        return;
    }

    // Position at top-center below navigation
    const int32_t top_margin = 80;  // Below nav/header area
    lv_obj_align(active_toast, LV_ALIGN_TOP_MID, 0, top_margin);

    // Update icon
    lv_obj_t* icon = lv_obj_find_by_name(active_toast, "toast_icon");
    if (icon) {
        lv_label_set_text(icon, severity_to_icon(severity));
    }

    // Wire up close button callback
    lv_obj_t* close_btn = lv_obj_find_by_name(active_toast, "toast_close_btn");
    if (close_btn) {
        lv_obj_add_event_cb(close_btn, toast_close_btn_clicked, LV_EVENT_CLICKED, nullptr);
    }

    // Create auto-dismiss timer
    dismiss_timer = lv_timer_create(toast_dismiss_timer_cb, duration_ms, nullptr);
    lv_timer_set_repeat_count(dismiss_timer, 1);  // Run once then stop

    // Update status bar notification icon
    ui_status_bar_update_notification(severity_to_notification_status(severity));

    spdlog::debug("Toast shown: {} ({}ms)", message, duration_ms);
}

void ui_toast_hide() {
    if (!active_toast) {
        return;
    }

    // Cancel dismiss timer if active
    if (dismiss_timer) {
        lv_timer_delete(dismiss_timer);
        dismiss_timer = nullptr;
    }

    // Delete toast widget
    lv_obj_delete(active_toast);
    active_toast = nullptr;

    // Clear status bar notification icon
    ui_status_bar_update_notification(NotificationStatus::NONE);

    spdlog::debug("Toast hidden");
}

bool ui_toast_is_visible() {
    return active_toast != nullptr;
}

// Timer callback for auto-dismiss
static void toast_dismiss_timer_cb(lv_timer_t* timer) {
    (void)timer;
    ui_toast_hide();
}

// Close button callback
static void toast_close_btn_clicked(lv_event_t* e) {
    (void)e;
    ui_toast_hide();
}
