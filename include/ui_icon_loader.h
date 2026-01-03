// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set the window icon for the LVGL SDL display.
 * Loads assets/images/helix-icon-128.png and applies it to the window.
 *
 * @param disp  The LVGL display (from lv_sdl_window_create)
 * @return true if icon was loaded successfully, false otherwise
 */
bool ui_set_window_icon(lv_display_t* disp);

#ifdef __cplusplus
}
#endif
