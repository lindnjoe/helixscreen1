/*
 * HelixScreen - 3D Printer Touch Interface
 * Copyright (C) 2025 Preston Brown
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "lvgl.h"

/**
 * @brief Create notification history panel
 *
 * @param parent Parent object
 * @return Created panel object
 */
lv_obj_t* ui_panel_notification_history_create(lv_obj_t* parent);

/**
 * @brief Refresh notification list from history
 *
 * Called when panel is shown or filter changes.
 */
void ui_panel_notification_history_refresh();
