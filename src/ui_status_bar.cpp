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

#include "ui_status_bar.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "ui_panel_notification_history.h"
#include <spdlog/spdlog.h>

// Cached widget references
static lv_obj_t* network_icon = nullptr;
static lv_obj_t* printer_icon = nullptr;
static lv_obj_t* notification_icon = nullptr;
static lv_obj_t* notification_badge = nullptr;
static lv_obj_t* notification_badge_count = nullptr;

// Event callback for notification history button
static void status_notification_history_clicked(lv_event_t* e) {
    lv_obj_t* parent = lv_screen_active();
    lv_obj_t* panel = ui_panel_notification_history_create(parent);
    if (panel) {
        ui_nav_push_overlay(panel);
    }
}

void ui_status_bar_init() {
    // Register notification history callback
    lv_xml_register_event_cb(NULL, "status_notification_history_clicked", status_notification_history_clicked);

    // Find status bar icons by name
    network_icon = lv_obj_find_by_name(lv_screen_active(), "status_network_icon");
    printer_icon = lv_obj_find_by_name(lv_screen_active(), "status_printer_icon");
    notification_icon = lv_obj_find_by_name(lv_screen_active(), "status_notification_icon");
    notification_badge = lv_obj_find_by_name(lv_screen_active(), "notification_badge");
    notification_badge_count = lv_obj_find_by_name(lv_screen_active(), "notification_badge_count");

    if (!network_icon || !printer_icon || !notification_icon) {
        spdlog::error("Failed to find status bar icon widgets");
        return;
    }

    if (!notification_badge || !notification_badge_count) {
        spdlog::warn("Failed to find notification badge widgets");
    }

    spdlog::debug("Status bar initialized successfully");
}

void ui_status_bar_update_network(NetworkStatus status) {
    if (!network_icon) {
        spdlog::warn("Status bar not initialized, cannot update network icon");
        return;
    }

    const char* icon = nullptr;
    lv_color_t color;

    switch (status) {
        case NetworkStatus::CONNECTED:
            icon = LV_SYMBOL_WIFI;
            color = ui_theme_parse_color(lv_xml_get_const(NULL, "success_color"));
            break;
        case NetworkStatus::CONNECTING:
            icon = LV_SYMBOL_WIFI;
            color = ui_theme_parse_color(lv_xml_get_const(NULL, "warning_color"));
            break;
        case NetworkStatus::DISCONNECTED:
        default:
            icon = LV_SYMBOL_WIFI;
            color = ui_theme_parse_color(lv_xml_get_const(NULL, "text_secondary"));
            break;
    }

    lv_label_set_text(network_icon, icon);
    lv_obj_set_style_text_color(network_icon, color, 0);
}

void ui_status_bar_update_printer(PrinterStatus status) {
    if (!printer_icon) {
        spdlog::warn("Status bar not initialized, cannot update printer icon");
        return;
    }

    const char* icon = nullptr;
    lv_color_t color;

    switch (status) {
        case PrinterStatus::READY:
            icon = LV_SYMBOL_SETTINGS;  // Or use a printer-specific icon if available
            color = ui_theme_parse_color(lv_xml_get_const(NULL, "success_color"));
            break;
        case PrinterStatus::PRINTING:
            icon = LV_SYMBOL_PLAY;
            color = ui_theme_parse_color(lv_xml_get_const(NULL, "info_color"));
            break;
        case PrinterStatus::ERROR:
            icon = LV_SYMBOL_WARNING;
            color = ui_theme_parse_color(lv_xml_get_const(NULL, "error_color"));
            break;
        case PrinterStatus::DISCONNECTED:
        default:
            icon = LV_SYMBOL_SETTINGS;
            color = ui_theme_parse_color(lv_xml_get_const(NULL, "text_secondary"));
            break;
    }

    lv_label_set_text(printer_icon, icon);
    lv_obj_set_style_text_color(printer_icon, color, 0);
}

void ui_status_bar_update_notification(NotificationStatus status) {
    if (!notification_icon) {
        spdlog::warn("Status bar not initialized, cannot update notification icon");
        return;
    }

    if (status == NotificationStatus::NONE) {
        // Hide notification icon when no active notifications
        lv_obj_add_flag(notification_icon, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    // Show and update notification icon
    lv_obj_remove_flag(notification_icon, LV_OBJ_FLAG_HIDDEN);

    const char* icon = LV_SYMBOL_WARNING;
    lv_color_t color;

    switch (status) {
        case NotificationStatus::INFO:
            icon = LV_SYMBOL_WARNING;  // Could use different icon for info
            color = ui_theme_parse_color(lv_xml_get_const(NULL, "info_color"));
            break;
        case NotificationStatus::WARNING:
            icon = LV_SYMBOL_WARNING;
            color = ui_theme_parse_color(lv_xml_get_const(NULL, "warning_color"));
            break;
        case NotificationStatus::ERROR:
            icon = LV_SYMBOL_WARNING;
            color = ui_theme_parse_color(lv_xml_get_const(NULL, "error_color"));
            break;
        default:
            break;
    }

    lv_label_set_text(notification_icon, icon);
    lv_obj_set_style_text_color(notification_icon, color, 0);
}

void ui_status_bar_update_notification_count(size_t count) {
    if (!notification_badge || !notification_badge_count) {
        spdlog::trace("Notification badge widgets not available");
        return;
    }

    if (count == 0) {
        // Hide badge when no unread notifications
        lv_obj_add_flag(notification_badge, LV_OBJ_FLAG_HIDDEN);
    } else {
        // Show badge and update count
        lv_obj_remove_flag(notification_badge, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(notification_badge_count, "%zu", count);
    }

    spdlog::trace("Notification count updated: {}", count);
}
