/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of GuppyScreen.
 *
 * GuppyScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GuppyScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GuppyScreen. If not, see <https://www.gnu.org/licenses/>.
 */

#include "ui_utils.h"
#include <cstdio>
#include <ctime>

std::string format_print_time(int minutes) {
    char buf[32];
    if (minutes < 60) {
        snprintf(buf, sizeof(buf), "%dm", minutes);
    } else {
        int hours = minutes / 60;
        int mins = minutes % 60;
        if (mins == 0) {
            snprintf(buf, sizeof(buf), "%dh", hours);
        } else {
            snprintf(buf, sizeof(buf), "%dh%dm", hours, mins);
        }
    }
    return std::string(buf);
}

std::string format_filament_weight(float grams) {
    char buf[32];
    if (grams < 1.0f) {
        snprintf(buf, sizeof(buf), "%.1fg", grams);
    } else if (grams < 10.0f) {
        snprintf(buf, sizeof(buf), "%.1fg", grams);
    } else {
        snprintf(buf, sizeof(buf), "%.0fg", grams);
    }
    return std::string(buf);
}

std::string format_file_size(size_t bytes) {
    char buf[32];
    if (bytes < 1024) {
        snprintf(buf, sizeof(buf), "%zu B", bytes);
    } else if (bytes < 1024 * 1024) {
        double kb = bytes / 1024.0;
        snprintf(buf, sizeof(buf), "%.1f KB", kb);
    } else if (bytes < 1024 * 1024 * 1024) {
        double mb = bytes / (1024.0 * 1024.0);
        snprintf(buf, sizeof(buf), "%.1f MB", mb);
    } else {
        double gb = bytes / (1024.0 * 1024.0 * 1024.0);
        snprintf(buf, sizeof(buf), "%.2f GB", gb);
    }
    return std::string(buf);
}

std::string format_modified_date(time_t timestamp) {
    char buf[64];
    struct tm* timeinfo = localtime(&timestamp);
    if (timeinfo) {
        // Format: "Jan 15 14:30"
        strftime(buf, sizeof(buf), "%b %d %H:%M", timeinfo);
    } else {
        snprintf(buf, sizeof(buf), "Unknown");
    }
    return std::string(buf);
}

// ============================================================================
// Header Bar Component Helpers
// ============================================================================

bool ui_header_bar_show_right_button(lv_obj_t* header_bar_widget) {
    if (!header_bar_widget) {
        return false;
    }

    lv_obj_t* button = lv_obj_find_by_name(header_bar_widget, "right_button");
    if (button) {
        lv_obj_remove_flag(button, LV_OBJ_FLAG_HIDDEN);
        return true;
    }

    return false;
}

bool ui_header_bar_hide_right_button(lv_obj_t* header_bar_widget) {
    if (!header_bar_widget) {
        return false;
    }

    lv_obj_t* button = lv_obj_find_by_name(header_bar_widget, "right_button");
    if (button) {
        lv_obj_add_flag(button, LV_OBJ_FLAG_HIDDEN);
        return true;
    }

    return false;
}

bool ui_header_bar_set_right_button_text(lv_obj_t* header_bar_widget, const char* text) {
    if (!header_bar_widget || !text) {
        return false;
    }

    lv_obj_t* button = lv_obj_find_by_name(header_bar_widget, "right_button");
    if (!button) {
        return false;
    }

    // Find the label child of the button
    lv_obj_t* label = lv_obj_find_by_name(button, "right_button_label");
    if (label) {
        lv_label_set_text(label, text);
        return true;
    }

    return false;
}
