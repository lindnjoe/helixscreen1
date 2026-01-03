// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "test_fixtures.h"

#include "spdlog/spdlog.h"

// ============================================================================
// MoonrakerTestFixture Implementation
// ============================================================================

MoonrakerTestFixture::MoonrakerTestFixture() {
    // Initialize printer state with subjects (skip XML registration for tests)
    m_state.init_subjects(false);

    // Create disconnected client - validation happens before network I/O
    m_client = std::make_unique<MoonrakerClient>();

    // Create API with client and state
    m_api = std::make_unique<MoonrakerAPI>(*m_client, m_state);

    spdlog::debug("[MoonrakerTestFixture] Initialized with disconnected client");
}

MoonrakerTestFixture::~MoonrakerTestFixture() {
    // Ensure API is destroyed before client (API holds reference to client)
    m_api.reset();
    m_client.reset();
    spdlog::debug("[MoonrakerTestFixture] Cleaned up");
}

// ============================================================================
// UITestFixture Implementation
// ============================================================================

UITestFixture::UITestFixture() {
    // Initialize UITest virtual input device
    UITest::init(test_screen());
    spdlog::debug("[UITestFixture] Initialized with virtual input device");
}

UITestFixture::~UITestFixture() {
    // Clean up virtual input device
    UITest::cleanup();
    spdlog::debug("[UITestFixture] Cleaned up virtual input device");
}

// ============================================================================
// FullMoonrakerTestFixture Implementation
// ============================================================================

FullMoonrakerTestFixture::FullMoonrakerTestFixture() {
    // Initialize UITest virtual input device
    // (MoonrakerTestFixture constructor already ran)
    UITest::init(test_screen());
    spdlog::debug("[FullMoonrakerTestFixture] Initialized with Moonraker + UITest");
}

FullMoonrakerTestFixture::~FullMoonrakerTestFixture() {
    // Clean up virtual input device
    UITest::cleanup();
    spdlog::debug("[FullMoonrakerTestFixture] Cleaned up");
}
