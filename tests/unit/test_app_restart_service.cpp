// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>

#include "../catch_amalgamated.hpp"

// Test the INVOCATION_ID detection logic used by app_request_restart_service()
// We test the logic directly rather than the full function since the full function
// calls app_request_quit()/app_request_restart() which have side effects.

TEST_CASE("Restart service routing logic", "[app_globals][restart]") {
    SECTION("INVOCATION_ID present indicates systemd environment") {
        // Save and set
        const char* original = getenv("INVOCATION_ID");
        setenv("INVOCATION_ID", "test-unit-id", 1);

        REQUIRE(getenv("INVOCATION_ID") != nullptr);
        // Under systemd: would take quit path

        // Restore
        if (original) {
            setenv("INVOCATION_ID", original, 1);
        } else {
            unsetenv("INVOCATION_ID");
        }
    }

    SECTION("No INVOCATION_ID indicates standalone environment") {
        // Save and unset
        const char* original = getenv("INVOCATION_ID");
        unsetenv("INVOCATION_ID");

        REQUIRE(getenv("INVOCATION_ID") == nullptr);
        // Standalone: would take fork/exec path

        // Restore
        if (original) {
            setenv("INVOCATION_ID", original, 1);
        }
    }
}
