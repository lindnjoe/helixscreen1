// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

/**
 * @file helix_timing.h
 * @brief Portable timing utilities for cross-platform builds
 *
 * Provides millisecond-precision timing functions that work across:
 * - Desktop (SDL2) builds
 * - Embedded Linux (POSIX) builds
 */

#include <cstdint>

#ifdef HELIX_DISPLAY_SDL
#include <SDL.h>
#else
#include <time.h>
#endif

namespace helix {
namespace timing {

#ifdef HELIX_DISPLAY_SDL

/**
 * @brief Get current time in milliseconds since application start
 * @return Milliseconds elapsed (wraps at ~49 days)
 */
inline uint32_t get_ticks() {
    return SDL_GetTicks();
}

/**
 * @brief Sleep for specified milliseconds
 * @param ms Milliseconds to sleep
 */
inline void delay(uint32_t ms) {
    SDL_Delay(ms);
}

#else
// POSIX fallback for embedded Linux (no SDL)

inline uint32_t get_ticks() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint32_t>(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

inline void delay(uint32_t ms) {
    struct timespec ts = {static_cast<time_t>(ms / 1000),
                          static_cast<long>((ms % 1000) * 1000000L)};
    nanosleep(&ts, nullptr);
}

#endif

} // namespace timing
} // namespace helix
