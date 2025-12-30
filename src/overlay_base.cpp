// SPDX-License-Identifier: GPL-3.0-or-later

#include "overlay_base.h"

#include <spdlog/spdlog.h>

OverlayBase::~OverlayBase() {
    // Note: cleanup() should be called before destruction if there are
    // pending async operations. The destructor doesn't call cleanup()
    // automatically because derived classes may need to handle cleanup
    // in a specific order.
    if (!cleanup_called_) {
        spdlog::trace("[OverlayBase] Destructor called without prior cleanup()");
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
