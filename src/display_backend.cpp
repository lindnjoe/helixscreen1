// SPDX-License-Identifier: GPL-3.0-or-later
//
// HelixScreen - Display Backend Factory Implementation

#include "display_backend.h"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <cstring>

// Platform-specific includes for availability checks
#include <sys/stat.h>
#include <unistd.h>

std::unique_ptr<DisplayBackend> DisplayBackend::create(DisplayBackendType type) {
    switch (type) {
#ifdef HELIX_DISPLAY_SDL
    case DisplayBackendType::SDL:
        return std::make_unique<DisplayBackendSDL>();
#endif

#ifdef HELIX_DISPLAY_FBDEV
    case DisplayBackendType::FBDEV:
        return std::make_unique<DisplayBackendFbdev>();
#endif

#ifdef HELIX_DISPLAY_DRM
    case DisplayBackendType::DRM:
        return std::make_unique<DisplayBackendDRM>();
#endif

    case DisplayBackendType::AUTO:
        return create_auto();

    default:
        spdlog::error("[DisplayBackend] Type {} not compiled in",
                      display_backend_type_to_string(type));
        return nullptr;
    }
}

std::unique_ptr<DisplayBackend> DisplayBackend::create_auto() {
    // Check environment variable override first
    const char* backend_env = std::getenv("HELIX_DISPLAY_BACKEND");
    if (backend_env != nullptr) {
        spdlog::info("[DisplayBackend] HELIX_DISPLAY_BACKEND={} - using forced backend",
                     backend_env);

        if (strcmp(backend_env, "drm") == 0) {
#ifdef HELIX_DISPLAY_DRM
            auto backend = std::make_unique<DisplayBackendDRM>();
            if (backend->is_available()) {
                return backend;
            }
            spdlog::warn("[DisplayBackend] DRM backend forced but not available");
#else
            spdlog::warn("[DisplayBackend] DRM backend forced but not compiled in");
#endif
        } else if (strcmp(backend_env, "fbdev") == 0 || strcmp(backend_env, "fb") == 0) {
#ifdef HELIX_DISPLAY_FBDEV
            auto backend = std::make_unique<DisplayBackendFbdev>();
            if (backend->is_available()) {
                return backend;
            }
            spdlog::warn("[DisplayBackend] Framebuffer backend forced but not available");
#else
            spdlog::warn("[DisplayBackend] Framebuffer backend forced but not compiled in");
#endif
        } else if (strcmp(backend_env, "sdl") == 0) {
#ifdef HELIX_DISPLAY_SDL
            auto backend = std::make_unique<DisplayBackendSDL>();
            if (backend->is_available()) {
                return backend;
            }
            spdlog::warn("[DisplayBackend] SDL backend forced but not available");
#else
            spdlog::warn("[DisplayBackend] SDL backend forced but not compiled in");
#endif
        } else {
            spdlog::warn("[DisplayBackend] Unknown HELIX_DISPLAY_BACKEND value: {}", backend_env);
        }
        // Fall through to auto-detection if forced backend unavailable
    }

    // Auto-detection: try backends in order of preference

    // 1. Try DRM first (best performance on modern Linux with GPU)
#ifdef HELIX_DISPLAY_DRM
    {
        auto backend = std::make_unique<DisplayBackendDRM>();
        if (backend->is_available()) {
            spdlog::info("[DisplayBackend] Auto-detected: DRM/KMS");
            return backend;
        }
        spdlog::debug("[DisplayBackend] DRM backend not available");
    }
#endif

    // 2. Try framebuffer (works on most embedded Linux)
#ifdef HELIX_DISPLAY_FBDEV
    {
        auto backend = std::make_unique<DisplayBackendFbdev>();
        if (backend->is_available()) {
            spdlog::info("[DisplayBackend] Auto-detected: Framebuffer");
            return backend;
        }
        spdlog::debug("[DisplayBackend] Framebuffer backend not available");
    }
#endif

    // 3. Fall back to SDL (desktop development)
#ifdef HELIX_DISPLAY_SDL
    {
        auto backend = std::make_unique<DisplayBackendSDL>();
        if (backend->is_available()) {
            spdlog::info("[DisplayBackend] Auto-detected: SDL");
            return backend;
        }
        spdlog::debug("[DisplayBackend] SDL backend not available");
    }
#endif

    spdlog::error("[DisplayBackend] No display backend available!");
    spdlog::error("[DisplayBackend] Compiled backends: "
#ifdef HELIX_DISPLAY_SDL
                  "SDL "
#endif
#ifdef HELIX_DISPLAY_FBDEV
                  "FBDEV "
#endif
#ifdef HELIX_DISPLAY_DRM
                  "DRM "
#endif
    );

    return nullptr;
}
