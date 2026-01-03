// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "overlay_base.h"

#include "ui_nav_manager.h"

#include <spdlog/spdlog.h>

OverlayBase::~OverlayBase() {
    // Fallback unregister in case cleanup() wasn't called.
    // Guard against Static Destruction Order Fiasco: during shutdown,
    // NavigationManager may already be destroyed.
    if (overlay_root_ && !NavigationManager::is_destroyed()) {
        NavigationManager::instance().unregister_overlay_instance(overlay_root_);
    }

    // Guard against Static Destruction Order Fiasco: spdlog may already be
    // destroyed if this overlay wasn't registered with StaticPanelRegistry.
    if (!NavigationManager::is_destroyed()) {
        spdlog::debug("[OverlayBase] Destroyed");
    }
}

void OverlayBase::on_activate() {
    spdlog::trace("[OverlayBase] on_activate() - {}", get_name());
    visible_ = true;
}

void OverlayBase::on_deactivate() {
    spdlog::trace("[OverlayBase] on_deactivate() - {}", get_name());
    visible_ = false;
}

void OverlayBase::cleanup() {
    spdlog::trace("[OverlayBase] cleanup() - {}", get_name());
    cleanup_called_ = true;
    visible_ = false;
}
