// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/**
 * Register the ui_dialog component with the LVGL XML system
 * Must be called before any XML files using <ui_dialog> are registered
 *
 * Dialog uses LVGL's built-in button grey color for background,
 * which automatically adapts to light/dark theme mode.
 * This matches the default lv_button appearance.
 */
void ui_dialog_register(void);

#ifdef __cplusplus
}
#endif
