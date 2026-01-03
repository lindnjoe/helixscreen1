// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <spdlog/spdlog.h>

/**
 * @brief Safe logging for destructors and cleanup paths
 *
 * During static destruction, even checking spdlog::default_logger() can crash
 * because spdlog's internal mutexes may be destroyed.
 *
 * **SOLUTION:** Use fprintf(stderr, ...) in destructors and stop() methods.
 * fprintf has no static dependencies and is always safe.
 *
 * Example:
 *   MyClass::~MyClass() {
 *       fprintf(stderr, "[MyClass] Destructor called\n");
 *       cleanup();
 *   }
 *
 * For normal operation (not in destructors), use regular spdlog::* functions.
 */

// NOTE: These macros are NOT safe during static destruction - use fprintf instead
#define SAFE_LOG_DEBUG(...)                                                                        \
    do {                                                                                           \
        if (spdlog::default_logger()) {                                                            \
            spdlog::debug(__VA_ARGS__);                                                            \
        }                                                                                          \
    } while (0)

#define SAFE_LOG_INFO(...)                                                                         \
    do {                                                                                           \
        if (spdlog::default_logger()) {                                                            \
            spdlog::info(__VA_ARGS__);                                                             \
        }                                                                                          \
    } while (0)

#define SAFE_LOG_WARN(...)                                                                         \
    do {                                                                                           \
        if (spdlog::default_logger()) {                                                            \
            spdlog::warn(__VA_ARGS__);                                                             \
        }                                                                                          \
    } while (0)

#define SAFE_LOG_ERROR(...)                                                                        \
    do {                                                                                           \
        if (spdlog::default_logger()) {                                                            \
            spdlog::error(__VA_ARGS__);                                                            \
        }                                                                                          \
    } while (0)
