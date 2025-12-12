// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_spinner.h"

#include "ui_theme.h"

#include "lvgl/lvgl.h"
#include "lvgl/src/xml/lv_xml.h"
#include "lvgl/src/xml/lv_xml_parser.h"
#include "lvgl/src/xml/lv_xml_utils.h"
#include "lvgl/src/xml/lv_xml_widget.h"
#include "lvgl/src/xml/parsers/lv_xml_obj_parser.h"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <cstring>

/**
 * @brief Get integer value from a responsive token
 *
 * The responsive spacing system auto-registers base tokens (e.g., "spinner_lg")
 * from globals.xml triplets (spinner_lg_small/medium/large) based on breakpoint.
 *
 * @param token_name Token name without size suffix
 * @param fallback Default value if token not found
 * @return Pixel value for current breakpoint
 */
static int32_t get_responsive_px(const char* token_name, int32_t fallback) {
    const char* val = lv_xml_get_const(nullptr, token_name);
    if (val) {
        return static_cast<int32_t>(atoi(val));
    }
    spdlog::warn("[ui_spinner] Token '{}' not found, using fallback {}", token_name, fallback);
    return fallback;
}

/**
 * @brief XML create callback for <spinner> widget
 *
 * Creates a spinner with:
 * - Responsive size based on "size" attribute (xs, sm, md, lg)
 * - Primary color indicator arc
 * - Hidden background track for clean modern look
 *
 * @param state XML parser state
 * @param attrs XML attributes
 * @return Created spinner object
 */
static void* ui_spinner_create(lv_xml_parser_state_t* state, const char** attrs) {
    lv_obj_t* parent = static_cast<lv_obj_t*>(lv_xml_state_get_parent(state));
    lv_obj_t* spinner = lv_spinner_create(parent);

    // Parse size attribute (default: lg)
    const char* size_str = lv_xml_get_value_of(attrs, "size");
    if (!size_str) {
        size_str = "lg";
    }

    // Get responsive size and arc width from tokens
    int32_t size = 64;
    int32_t arc_width = 4;

    if (strcmp(size_str, "xs") == 0) {
        size = get_responsive_px("spinner_xs", 16);
        arc_width = get_responsive_px("spinner_arc_xs", 2);
    } else if (strcmp(size_str, "sm") == 0) {
        size = get_responsive_px("spinner_sm", 20);
        arc_width = get_responsive_px("spinner_arc_sm", 2);
    } else if (strcmp(size_str, "md") == 0) {
        size = get_responsive_px("spinner_md", 32);
        arc_width = get_responsive_px("spinner_arc_md", 3);
    } else { // lg (default)
        size = get_responsive_px("spinner_lg", 64);
        arc_width = get_responsive_px("spinner_arc_lg", 4);
    }

    // Apply size
    lv_obj_set_size(spinner, size, size);

    // Apply consistent styling - primary color indicator
    // Use ui_theme_get_color() for token lookup (not ui_theme_parse_color which expects hex)
    lv_color_t primary = ui_theme_get_color("primary_color");
    lv_obj_set_style_arc_color(spinner, primary, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, arc_width, LV_PART_INDICATOR);

    // Hide background track for clean modern look
    lv_obj_set_style_arc_opa(spinner, LV_OPA_0, LV_PART_MAIN);

    spdlog::trace("[ui_spinner] Created spinner size='{}' ({}px, arc={}px)", size_str, size,
                  arc_width);

    return spinner;
}

/**
 * @brief XML apply callback for <spinner> widget
 *
 * Delegates to standard object parser for base properties (align, hidden, etc.)
 *
 * @param state XML parser state
 * @param attrs XML attributes
 */
static void ui_spinner_apply(lv_xml_parser_state_t* state, const char** attrs) {
    lv_xml_obj_apply(state, attrs);
}

void ui_spinner_init() {
    lv_xml_register_widget("spinner", ui_spinner_create, ui_spinner_apply);
    spdlog::debug("[ui_spinner] Registered spinner widget with responsive sizing");
}
