// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 356C LLC

// Tests for TouchJitterFilter — the shared jitter filter used by
// calibrated_read_cb in display_backend_fbdev.cpp. Tests exercise the
// exact same apply() method used in production, preventing divergence.

#include "touch_jitter_filter.h"

#include "../catch_amalgamated.hpp"

TEST_CASE("Jitter filter: disabled when threshold is 0", "[jitter-filter]") {
    TouchJitterFilter f{};

    int32_t x = 100, y = 200;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 100);
    REQUIRE(y == 200);

    x = 103;
    y = 202;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 103);
    REQUIRE(y == 202);
}

TEST_CASE("Jitter filter: first press records position", "[jitter-filter]") {
    TouchJitterFilter f{.threshold_sq = 15 * 15};

    int32_t x = 400, y = 300;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);

    REQUIRE(x == 400);
    REQUIRE(y == 300);
    REQUIRE(f.tracking == true);
    REQUIRE(f.last_x == 400);
    REQUIRE(f.last_y == 300);
}

TEST_CASE("Jitter filter: small movements suppressed", "[jitter-filter]") {
    TouchJitterFilter f{.threshold_sq = 15 * 15}; // 225

    int32_t x = 400, y = 300;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);

    // Jitter within threshold
    x = 405;
    y = 303;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 400);
    REQUIRE(y == 300);

    // Opposite direction jitter
    x = 395;
    y = 298;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 400);
    REQUIRE(y == 300);

    // Right at boundary: dx=10, dy=10, dist²=200 < 225
    x = 410;
    y = 310;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 400);
    REQUIRE(y == 300);
}

TEST_CASE("Jitter filter: intentional movement passes through", "[jitter-filter]") {
    TouchJitterFilter f{.threshold_sq = 15 * 15};

    int32_t x = 400, y = 300;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);

    // Large movement: dx=20, dist²=400 > 225
    x = 420;
    y = 300;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 420);
    REQUIRE(y == 300);
    REQUIRE(f.last_x == 420);
    REQUIRE(f.last_y == 300);

    // Jitter around new position is suppressed
    x = 423;
    y = 302;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 420);
    REQUIRE(y == 300);
}

TEST_CASE("Jitter filter: release snaps to last stable position", "[jitter-filter]") {
    TouchJitterFilter f{.threshold_sq = 15 * 15};

    int32_t x = 400, y = 300;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);

    x = 407;
    y = 304;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 400);

    // Release reports last stable position
    x = 408;
    y = 305;
    f.apply(LV_INDEV_STATE_RELEASED, x, y);
    REQUIRE(x == 400);
    REQUIRE(y == 300);
    REQUIRE(f.tracking == false);
}

TEST_CASE("Jitter filter: reset between taps", "[jitter-filter]") {
    TouchJitterFilter f{.threshold_sq = 15 * 15};

    // First tap
    int32_t x = 100, y = 100;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    f.apply(LV_INDEV_STATE_RELEASED, x, y);
    REQUIRE(f.tracking == false);

    // Second tap at different location
    x = 500;
    y = 400;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 500);
    REQUIRE(y == 400);
    REQUIRE(f.last_x == 500);
    REQUIRE(f.last_y == 400);
}

TEST_CASE("Jitter filter: diagonal movement threshold", "[jitter-filter]") {
    TouchJitterFilter f{.threshold_sq = 10 * 10}; // 100

    int32_t x = 200, y = 200;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);

    // dx=7, dy=7, dist²=98 < 100 → suppressed
    x = 207;
    y = 207;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 200);
    REQUIRE(y == 200);

    // dx=8, dy=8, dist²=128 > 100 → passes
    x = 208;
    y = 208;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 208);
    REQUIRE(y == 208);
}

TEST_CASE("Jitter filter: drag tracking updates correctly", "[jitter-filter]") {
    TouchJitterFilter f{.threshold_sq = 10 * 10};

    int32_t x = 100, y = 100;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);

    // Move to (120, 100) — exceeds threshold
    x = 120;
    y = 100;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 120);

    // Continue to (140, 100) — exceeds from new position
    x = 140;
    y = 100;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 140);

    // Jitter suppressed relative to (140, 100)
    x = 143;
    y = 102;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 140);
    REQUIRE(y == 100);
}

TEST_CASE("Jitter filter: exact threshold boundary", "[jitter-filter]") {
    TouchJitterFilter f{.threshold_sq = 10 * 10}; // 100

    int32_t x = 100, y = 100;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);

    // Exactly at threshold: dx=10, dy=0, dist²=100 == 100 → suppressed (<=)
    x = 110;
    y = 100;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 100);
    REQUIRE(y == 100);

    // One pixel past: dx=11, dy=0, dist²=121 > 100 → passes
    x = 111;
    y = 100;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 111);
    REQUIRE(y == 100);
}

TEST_CASE("Jitter filter: negative threshold_sq treated as disabled", "[jitter-filter]") {
    TouchJitterFilter f{.threshold_sq = -100};

    int32_t x = 100, y = 200;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 100);
    REQUIRE(y == 200);

    x = 101;
    y = 201;
    f.apply(LV_INDEV_STATE_PRESSED, x, y);
    REQUIRE(x == 101);
    REQUIRE(y == 201);
}

TEST_CASE("Jitter filter: release without prior press is no-op", "[jitter-filter]") {
    TouchJitterFilter f{.threshold_sq = 15 * 15};

    int32_t x = 300, y = 400;
    f.apply(LV_INDEV_STATE_RELEASED, x, y);
    REQUIRE(x == 300);
    REQUIRE(y == 400);
    REQUIRE(f.tracking == false);
}
