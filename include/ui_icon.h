// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "lvgl/lvgl.h"

/**
 * Register the custom icon widget with LVGL's XML system.
 *
 * This enables the <icon> XML component to create font-based Material Design
 * Icons using lv_label with MDI font glyphs. The widget provides semantic
 * property handling for easy theming:
 *
 * Properties:
 *   - src: Icon short name (e.g., "home", "wifi", "settings")
 *   - size: Semantic size string - "xs", "sm", "md", "lg", "xl"
 *   - variant: Color variant - "primary", "secondary", "accent", "disabled",
 *              "warning", "error", "none"
 *   - color: Custom color override (e.g., "0xFF0000", "#FF0000")
 *
 * Size mapping (uses mdi_icons_* fonts):
 *   xs: 16px (mdi_icons_16)
 *   sm: 24px (mdi_icons_24)
 *   md: 32px (mdi_icons_32)
 *   lg: 48px (mdi_icons_48)
 *   xl: 64px (mdi_icons_64)
 *
 * Variant mapping (reads from globals.xml theme constants):
 *   primary:   Text color #text_primary (100% opacity)
 *   secondary: Text color #text_secondary (100% opacity)
 *   accent:    Text color #primary_color (100% opacity)
 *   disabled:  Text color #text_primary (50% opacity)
 *   warning:   Text color #warning_color (100% opacity)
 *   error:     Text color #error_color (100% opacity)
 *   none:      Text color #text_primary (default)
 *
 * Call once at application startup, BEFORE registering XML components.
 *
 * Example initialization order:
 *   ui_icon_register_widget();  // <-- Register before icon.xml
 *   lv_xml_register_component_from_file("A:ui_xml/icon.xml");
 */
void ui_icon_register_widget();

/**
 * Change the icon source at runtime.
 *
 * @param icon       Icon widget created by ui_icon_register_widget()
 * @param icon_name  Icon short name (e.g., "home", "wifi")
 *                   Legacy "mat_*_img" names are also supported for transition.
 */
void ui_icon_set_source(lv_obj_t* icon, const char* icon_name);

/**
 * Change the icon size at runtime.
 *
 * @param icon      Icon widget
 * @param size_str  Size string: "xs", "sm", "md", "lg", or "xl"
 */
void ui_icon_set_size(lv_obj_t* icon, const char* size_str);

/**
 * Change the icon color variant at runtime.
 *
 * @param icon         Icon widget
 * @param variant_str  Variant string: "primary", "secondary", "accent",
 *                     "disabled", "warning", "error", or "none"
 */
void ui_icon_set_variant(lv_obj_t* icon, const char* variant_str);

/**
 * Set custom color for icon at runtime.
 *
 * @param icon   Icon widget
 * @param color  LVGL color value
 * @param opa    Opacity (0-255, use LV_OPA_COVER for full opacity)
 */
void ui_icon_set_color(lv_obj_t* icon, lv_color_t color, lv_opa_t opa);
