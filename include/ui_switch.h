// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/**
 * Register responsive constants for switch sizing based on screen dimensions
 * Must be called AFTER globals.xml is registered and BEFORE test_panel.xml
 */
void ui_switch_register_responsive_constants(void);

/**
 * Register the ui_switch component with the LVGL XML system
 * Must be called before any XML files using <ui_switch> are registered
 */
void ui_switch_register(void);

#ifdef __cplusplus
}
#endif
