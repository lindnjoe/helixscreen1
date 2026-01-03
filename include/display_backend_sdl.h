// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later
//
// HelixScreen - SDL Display Backend
//
// Desktop development backend using SDL2 for window management
// and input handling. Used for macOS/Linux desktop builds.

#pragma once

#ifdef HELIX_DISPLAY_SDL

#include "display_backend.h"

/**
 * @brief SDL2-based display backend for desktop development
 *
 * Uses LVGL's built-in SDL driver (lv_sdl_window_create) to create
 * a window with hardware-accelerated rendering.
 *
 * Features:
 * - Hardware-accelerated rendering via SDL_Renderer
 * - Mouse and keyboard input
 * - Window resize support
 * - Screenshot support (press 'S' key)
 */
class DisplayBackendSDL : public DisplayBackend {
  public:
    DisplayBackendSDL() = default;
    ~DisplayBackendSDL() override = default;

    // Display creation
    lv_display_t* create_display(int width, int height) override;

    // Input device creation
    lv_indev_t* create_input_pointer() override;
    lv_indev_t* create_input_keyboard() override;

    // Backend info
    DisplayBackendType type() const override {
        return DisplayBackendType::SDL;
    }
    const char* name() const override {
        return "SDL2";
    }
    bool is_available() const override;

  private:
    lv_display_t* display_ = nullptr;
    lv_indev_t* mouse_ = nullptr;
    lv_indev_t* keyboard_ = nullptr;
};

#endif // HELIX_DISPLAY_SDL
