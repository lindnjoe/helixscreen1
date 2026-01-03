// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "lvgl.h"

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Initialize header_bar component system
 * Registers global resize handler
 * Call this once during app initialization
 */
void ui_component_header_bar_init();

/**
 * Setup a header_bar instance for responsive height management
 * Call this in panel setup functions after finding the header widget
 * @param header The header_bar widget instance
 * @param screen The parent screen for measuring height
 */
void ui_component_header_bar_setup(lv_obj_t* header, lv_obj_t* screen);

// ============================================================================
// HEADER BAR API
// ============================================================================

/**
 * Show the action button in a header_bar component
 * @param header_bar_widget Pointer to the header_bar component root
 * @return true if button was found and shown, false otherwise
 */
bool ui_header_bar_show_action_button(lv_obj_t* header_bar_widget);

/**
 * Hide the action button in a header_bar component
 * @param header_bar_widget Pointer to the header_bar component root
 * @return true if button was found and hidden, false otherwise
 */
bool ui_header_bar_hide_action_button(lv_obj_t* header_bar_widget);

/**
 * Set the text of the action button in a header_bar component
 * Note: This does NOT automatically show the button - call ui_header_bar_show_action_button()
 * separately
 * @param header_bar_widget Pointer to the header_bar component root
 * @param text New button text
 * @return true if button was found and updated, false otherwise
 */
bool ui_header_bar_set_action_button_text(lv_obj_t* header_bar_widget, const char* text);
