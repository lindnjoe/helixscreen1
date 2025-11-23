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

#include "ui_notification.h"
#include "ui_toast.h"
#include "ui_modal.h"
#include "ui_status_bar.h"
#include "ui_notification_history.h"
#include "app_globals.h"
#include <spdlog/spdlog.h>
#include <cstring>

// Forward declarations
static void notification_observer_cb(lv_observer_t* observer, lv_subject_t* subject);
static void modal_ok_btn_clicked(lv_event_t* e);

// Helper function to add notification to history
static void add_to_history(ToastSeverity severity, const char* title, const char* message, bool was_modal) {
    NotificationHistoryEntry entry = {};
    entry.timestamp_ms = lv_tick_get();
    entry.severity = severity;
    entry.was_modal = was_modal;
    entry.was_read = false;

    // Copy title
    if (title) {
        strncpy(entry.title, title, sizeof(entry.title) - 1);
        entry.title[sizeof(entry.title) - 1] = '\0';
    } else {
        entry.title[0] = '\0';
    }

    // Copy message
    strncpy(entry.message, message, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';

    NotificationHistory::instance().add(entry);

    // Update unread count badge in status bar
    ui_status_bar_update_notification_count(NotificationHistory::instance().get_unread_count());
}

void ui_notification_init() {
    // Add observer to handle notification emissions
    // (Subject itself is initialized in app_globals_init_subjects())
    lv_subject_add_observer(&get_notification_subject(), notification_observer_cb, nullptr);

    spdlog::debug("Notification system initialized");
}

void ui_notification_info(const char* message) {
    if (!message) {
        spdlog::warn("Attempted to show info notification with null message");
        return;
    }

    ui_toast_show(ToastSeverity::INFO, message, 4000);
    add_to_history(ToastSeverity::INFO, nullptr, message, false);
}

void ui_notification_success(const char* message) {
    if (!message) {
        spdlog::warn("Attempted to show success notification with null message");
        return;
    }

    ui_toast_show(ToastSeverity::SUCCESS, message, 4000);
    add_to_history(ToastSeverity::SUCCESS, nullptr, message, false);
}

void ui_notification_warning(const char* message) {
    if (!message) {
        spdlog::warn("Attempted to show warning notification with null message");
        return;
    }

    ui_toast_show(ToastSeverity::WARNING, message, 5000);
    add_to_history(ToastSeverity::WARNING, nullptr, message, false);
}

void ui_notification_error(const char* title, const char* message, bool modal) {
    if (!message) {
        spdlog::warn("Attempted to show error notification with null message");
        return;
    }

    if (modal && title) {
        // Show modal dialog for critical errors
        ui_modal_config_t config = {
            .position = {.use_alignment = true, .alignment = LV_ALIGN_CENTER},
            .backdrop_opa = 180,
            .keyboard = nullptr,
            .persistent = false
        };

        const char* attrs[] = {
            "title", title,
            "message", message,
            nullptr
        };

        lv_obj_t* modal = ui_modal_show("error_dialog", &config, attrs);

        if (modal) {
            // Wire up OK button callback
            lv_obj_t* ok_btn = lv_obj_find_by_name(modal, "dialog_ok_btn");
            if (ok_btn) {
                lv_obj_add_event_cb(ok_btn, modal_ok_btn_clicked, LV_EVENT_CLICKED, modal);
            }
        }

        // Update status bar to show error
        ui_status_bar_update_notification(NotificationStatus::ERROR);
    } else {
        // Show toast for non-critical errors
        ui_toast_show(ToastSeverity::ERROR, message, 6000);
    }

    // Add to history
    add_to_history(ToastSeverity::ERROR, title, message, modal);
}

// Subject observer callback - routes notifications to appropriate display
static void notification_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    (void)observer;

    // Get notification data from subject
    NotificationData* data = (NotificationData*)lv_subject_get_pointer(subject);
    if (!data || !data->message) {
        spdlog::warn("Notification observer received invalid data");
        return;
    }

    // Route to modal or toast based on show_modal flag
    if (data->show_modal) {
        ui_notification_error(data->title, data->message, true);
    } else {
        // Route to toast based on severity
        switch (data->severity) {
            case ToastSeverity::INFO:
                ui_notification_info(data->message);
                break;
            case ToastSeverity::SUCCESS:
                ui_notification_success(data->message);
                break;
            case ToastSeverity::WARNING:
                ui_notification_warning(data->message);
                break;
            case ToastSeverity::ERROR:
                ui_notification_error(nullptr, data->message, false);
                break;
        }
    }

    spdlog::debug("Notification routed: modal={}, severity={}, msg={}",
                  data->show_modal, static_cast<int>(data->severity), data->message);
}

// Modal OK button callback
static void modal_ok_btn_clicked(lv_event_t* e) {
    lv_obj_t* modal = (lv_obj_t*)lv_event_get_user_data(e);
    if (modal) {
        ui_modal_hide(modal);
    }
}
