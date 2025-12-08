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

#pragma once

/**
 * @file ui_spinner.h
 * @brief Custom spinner component with responsive sizing
 *
 * Provides a semantic <spinner> XML widget with consistent styling and
 * responsive sizing that adapts to screen breakpoints.
 *
 * Usage in XML:
 *   <spinner size="lg"/>   - Large (modal overlays, major operations)
 *   <spinner size="md"/>   - Medium (loading states)
 *   <spinner size="sm"/>   - Small (inline status indicators)
 *   <spinner size="xs"/>   - Extra small (compact UI)
 *
 * Size values are defined in globals.xml with _small/_medium/_large variants
 * and are auto-registered by the responsive spacing system at runtime.
 */

/**
 * @brief Initialize the spinner custom widget
 *
 * Registers the <spinner> XML widget with LVGL's XML parser.
 * Must be called after lv_xml_init() and after globals.xml is loaded.
 */
void ui_spinner_init();
