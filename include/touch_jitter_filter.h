// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 356C LLC
//
// Touch jitter filter — suppresses small coordinate changes during
// stationary taps to prevent noisy touch controllers (e.g., Goodix GT9xx)
// from triggering LVGL scroll detection.

#pragma once

#include <lvgl.h>

struct TouchJitterFilter {
    /// Threshold in screen pixels (squared for fast comparison). 0 = disabled.
    int threshold_sq = 0;
    int last_x = 0;
    int last_y = 0;
    bool tracking = false;

    /// Apply jitter filtering to touch coordinates.
    /// Modifies x/y in-place: suppresses movement within the dead zone,
    /// passes through intentional movement, snaps to last stable position on release.
    void apply(lv_indev_state_t state, int32_t& x, int32_t& y) {
        if (threshold_sq <= 0)
            return;

        if (state == LV_INDEV_STATE_PRESSED) {
            if (!tracking) {
                last_x = x;
                last_y = y;
                tracking = true;
            } else {
                int dx = x - last_x;
                int dy = y - last_y;
                if (dx * dx + dy * dy <= threshold_sq) {
                    x = last_x;
                    y = last_y;
                } else {
                    last_x = x;
                    last_y = y;
                }
            }
        } else {
            if (tracking) {
                x = last_x;
                y = last_y;
                tracking = false;
            }
        }
    }
};
