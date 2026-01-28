// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_color_sensor_manager.cpp
 * @brief Unit tests for ColorSensorManager
 *
 * Tests cover:
 * - Type helpers (role string conversion)
 * - Sensor discovery from device IDs (td1_lane0, td1_lane1)
 * - Role assignment (FILAMENT_COLOR)
 * - State updates from Moonraker TD-1 status JSON
 * - Subject value correctness for UI binding
 * - Config persistence
 */

#include "../ui_test_utils.h"
#include "color_sensor_manager.h"
#include "color_sensor_types.h"

#include <spdlog/spdlog.h>

#include <vector>

#include "../catch_amalgamated.hpp"

using namespace helix::sensors;
using json = nlohmann::json;

// ============================================================================
// Test Fixture
// ============================================================================

class ColorSensorTestFixture {
  public:
    ColorSensorTestFixture() {
        // Initialize LVGL (safe version avoids "already initialized" warnings)
        lv_init_safe();

        // Create a headless display for testing
        if (!display_created_) {
            display_ = lv_display_create(480, 320);
            alignas(64) static lv_color_t buf[480 * 10];
            lv_display_set_buffers(display_, buf, nullptr, sizeof(buf),
                                   LV_DISPLAY_RENDER_MODE_PARTIAL);
            lv_display_set_flush_cb(display_,
                                    [](lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
                                        lv_display_flush_ready(disp);
                                    });
            display_created_ = true;
        }

        // Initialize subjects (idempotent)
        mgr().init_subjects();

        // Reset state for test isolation
        mgr().reset_for_testing();
    }

    ~ColorSensorTestFixture() {
        // Reset after each test
        mgr().reset_for_testing();
    }

  protected:
    ColorSensorManager& mgr() {
        return ColorSensorManager::instance();
    }

    // Helper to discover standard test sensors
    void discover_test_sensors() {
        std::vector<std::string> devices = {"td1_lane0", "td1_lane1"};
        mgr().discover(devices);
    }

    // Helper to simulate Moonraker status update
    void update_sensor_state(const std::string& device_id, const std::string& color_hex,
                             float transmission_distance) {
        json status;
        status[device_id]["color"] = color_hex;
        status[device_id]["td"] = transmission_distance;
        mgr().update_from_status(status);
    }

  private:
    static lv_display_t* display_;
    static bool display_created_;
};

// Static members
lv_display_t* ColorSensorTestFixture::display_ = nullptr;
bool ColorSensorTestFixture::display_created_ = false;

// ============================================================================
// Type Helper Tests (color_sensor_types.h)
// ============================================================================

TEST_CASE("ColorSensorTypes - role string conversion", "[color][types]") {
    SECTION("color_role_to_string") {
        REQUIRE(color_role_to_string(ColorSensorRole::NONE) == "none");
        REQUIRE(color_role_to_string(ColorSensorRole::FILAMENT_COLOR) == "filament_color");
    }

    SECTION("color_role_from_string") {
        REQUIRE(color_role_from_string("none") == ColorSensorRole::NONE);
        REQUIRE(color_role_from_string("filament_color") == ColorSensorRole::FILAMENT_COLOR);
        REQUIRE(color_role_from_string("invalid") == ColorSensorRole::NONE);
        REQUIRE(color_role_from_string("") == ColorSensorRole::NONE);
    }

    SECTION("color_role_to_display_string") {
        REQUIRE(color_role_to_display_string(ColorSensorRole::NONE) == "Unassigned");
        REQUIRE(color_role_to_display_string(ColorSensorRole::FILAMENT_COLOR) == "Filament Color");
    }
}

// ============================================================================
// Sensor Discovery Tests
// ============================================================================

TEST_CASE_METHOD(ColorSensorTestFixture, "ColorSensorManager - discovery", "[color][discovery]") {
    SECTION("Discovers TD-1 device") {
        std::vector<std::string> devices = {"td1_lane0"};
        mgr().discover(devices);

        REQUIRE(mgr().has_sensors());
        REQUIRE(mgr().sensor_count() == 1);

        auto configs = mgr().get_sensors();
        REQUIRE(configs.size() == 1);
        REQUIRE(configs[0].device_id == "td1_lane0");
        REQUIRE(configs[0].sensor_name == "TD-1 Lane 0");
        REQUIRE(configs[0].enabled == true);
        REQUIRE(configs[0].role == ColorSensorRole::NONE);
    }

    SECTION("Discovers multiple TD-1 devices") {
        discover_test_sensors();

        REQUIRE(mgr().sensor_count() == 2);

        auto configs = mgr().get_sensors();
        REQUIRE(configs[0].device_id == "td1_lane0");
        REQUIRE(configs[1].device_id == "td1_lane1");
    }

    SECTION("Empty device list clears previous sensors") {
        discover_test_sensors();
        REQUIRE(mgr().sensor_count() == 2);

        mgr().discover({});
        REQUIRE(mgr().sensor_count() == 0);
        REQUIRE_FALSE(mgr().has_sensors());
    }

    SECTION("Re-discovery replaces sensor list") {
        std::vector<std::string> devices1 = {"td1_lane0"};
        mgr().discover(devices1);
        REQUIRE(mgr().get_sensors()[0].device_id == "td1_lane0");

        std::vector<std::string> devices2 = {"td1_lane1"};
        mgr().discover(devices2);
        REQUIRE(mgr().sensor_count() == 1);
        REQUIRE(mgr().get_sensors()[0].device_id == "td1_lane1");
    }

    SECTION("Sensor count subject is updated") {
        lv_subject_t* count_subject = mgr().get_sensor_count_subject();
        REQUIRE(lv_subject_get_int(count_subject) == 0);

        discover_test_sensors();
        REQUIRE(lv_subject_get_int(count_subject) == 2);

        mgr().discover({});
        REQUIRE(lv_subject_get_int(count_subject) == 0);
    }
}

// ============================================================================
// Role Assignment Tests
// ============================================================================

TEST_CASE_METHOD(ColorSensorTestFixture, "ColorSensorManager - role assignment", "[color][roles]") {
    discover_test_sensors();

    SECTION("Can set FILAMENT_COLOR role") {
        mgr().set_sensor_role("td1_lane0", ColorSensorRole::FILAMENT_COLOR);

        auto configs = mgr().get_sensors();
        auto it = std::find_if(configs.begin(), configs.end(),
                               [](const auto& c) { return c.device_id == "td1_lane0"; });
        REQUIRE(it != configs.end());
        REQUIRE(it->role == ColorSensorRole::FILAMENT_COLOR);
    }

    SECTION("Role assignment is unique - assigning same role clears previous") {
        mgr().set_sensor_role("td1_lane0", ColorSensorRole::FILAMENT_COLOR);
        mgr().set_sensor_role("td1_lane1", ColorSensorRole::FILAMENT_COLOR);

        auto configs = mgr().get_sensors();

        auto lane0_it = std::find_if(configs.begin(), configs.end(),
                                     [](const auto& c) { return c.device_id == "td1_lane0"; });
        REQUIRE(lane0_it->role == ColorSensorRole::NONE);

        auto lane1_it = std::find_if(configs.begin(), configs.end(),
                                     [](const auto& c) { return c.device_id == "td1_lane1"; });
        REQUIRE(lane1_it->role == ColorSensorRole::FILAMENT_COLOR);
    }

    SECTION("Can assign NONE without affecting other sensors") {
        mgr().set_sensor_role("td1_lane0", ColorSensorRole::FILAMENT_COLOR);

        mgr().set_sensor_role("td1_lane0", ColorSensorRole::NONE);

        auto configs = mgr().get_sensors();
        auto it = std::find_if(configs.begin(), configs.end(),
                               [](const auto& c) { return c.device_id == "td1_lane0"; });
        REQUIRE(it->role == ColorSensorRole::NONE);
    }

    SECTION("Assigning role to unknown device does nothing") {
        mgr().set_sensor_role("nonexistent_device", ColorSensorRole::FILAMENT_COLOR);

        for (const auto& config : mgr().get_sensors()) {
            REQUIRE(config.role == ColorSensorRole::NONE);
        }
    }
}

// ============================================================================
// State Update Tests
// ============================================================================

TEST_CASE_METHOD(ColorSensorTestFixture, "ColorSensorManager - state updates", "[color][state]") {
    discover_test_sensors();
    mgr().set_sensor_role("td1_lane0", ColorSensorRole::FILAMENT_COLOR);

    SECTION("Parses color_hex and transmission_distance from status JSON") {
        auto state = mgr().get_sensor_state(ColorSensorRole::FILAMENT_COLOR);
        REQUIRE(state.has_value());
        REQUIRE(state->color_hex.empty());
        REQUIRE(state->transmission_distance == 0.0f);

        json status;
        status["td1_lane0"]["color"] = "#FF5733";
        status["td1_lane0"]["td"] = 1.25f;
        mgr().update_from_status(status);

        state = mgr().get_sensor_state(ColorSensorRole::FILAMENT_COLOR);
        REQUIRE(state->color_hex == "#FF5733");
        REQUIRE(state->transmission_distance == Catch::Approx(1.25f));
    }

    SECTION("Status update for unknown device is ignored") {
        json status;
        status["unknown_device"]["color"] = "#FF5733";
        mgr().update_from_status(status);

        REQUIRE(mgr().sensor_count() == 2);
    }

    SECTION("Empty status update is handled") {
        json status = json::object();
        mgr().update_from_status(status);

        REQUIRE(mgr().has_sensors());
    }
}

// ============================================================================
// Subject Value Tests
// ============================================================================

TEST_CASE_METHOD(ColorSensorTestFixture, "ColorSensorManager - subject values",
                 "[color][subjects]") {
    discover_test_sensors();

    SECTION("Color hex subject shows empty when no sensor assigned to role") {
        REQUIRE(std::string(lv_subject_get_string(mgr().get_color_hex_subject())) == "");
    }

    SECTION("TD value subject shows -1 when no sensor assigned to role") {
        REQUIRE(lv_subject_get_int(mgr().get_td_value_subject()) == -1);
    }

    SECTION("Color hex subject updates correctly") {
        mgr().set_sensor_role("td1_lane0", ColorSensorRole::FILAMENT_COLOR);

        // After assignment, should show empty since state defaults to empty
        REQUIRE(std::string(lv_subject_get_string(mgr().get_color_hex_subject())) == "");

        // Update state with color
        update_sensor_state("td1_lane0", "#FF5733", 1.25f);
        REQUIRE(std::string(lv_subject_get_string(mgr().get_color_hex_subject())) == "#FF5733");

        // Update with different color
        update_sensor_state("td1_lane0", "#00FF00", 2.5f);
        REQUIRE(std::string(lv_subject_get_string(mgr().get_color_hex_subject())) == "#00FF00");
    }

    SECTION("TD value subject updates correctly (TD x 100)") {
        mgr().set_sensor_role("td1_lane0", ColorSensorRole::FILAMENT_COLOR);

        // After assignment, should show 0 since TD defaults to 0.0
        REQUIRE(lv_subject_get_int(mgr().get_td_value_subject()) == 0);

        // Update state with TD value 1.25
        update_sensor_state("td1_lane0", "#FF5733", 1.25f);
        REQUIRE(lv_subject_get_int(mgr().get_td_value_subject()) == 125);

        // Update with different TD value
        update_sensor_state("td1_lane0", "#00FF00", 2.75f);
        REQUIRE(lv_subject_get_int(mgr().get_td_value_subject()) == 275);
    }

    SECTION("Subjects show empty/-1 when sensor disabled") {
        mgr().set_sensor_role("td1_lane0", ColorSensorRole::FILAMENT_COLOR);
        update_sensor_state("td1_lane0", "#FF5733", 1.25f);

        mgr().set_sensor_enabled("td1_lane0", false);
        REQUIRE(std::string(lv_subject_get_string(mgr().get_color_hex_subject())) == "");
        REQUIRE(lv_subject_get_int(mgr().get_td_value_subject()) == -1);
    }
}

// ============================================================================
// Config Persistence Tests
// ============================================================================

TEST_CASE_METHOD(ColorSensorTestFixture, "ColorSensorManager - config persistence",
                 "[color][config]") {
    discover_test_sensors();

    SECTION("save_config returns JSON with role assignments") {
        mgr().set_sensor_role("td1_lane0", ColorSensorRole::FILAMENT_COLOR);

        json config = mgr().save_config();

        REQUIRE(config.is_object());
        REQUIRE(config.contains("sensors"));
        REQUIRE(config["sensors"].is_array());
        REQUIRE(config["sensors"].size() == 2);

        bool found_lane0 = false;
        for (const auto& sensor : config["sensors"]) {
            if (sensor["device_id"] == "td1_lane0") {
                REQUIRE(sensor["role"] == "filament_color");
                found_lane0 = true;
            }
        }
        REQUIRE(found_lane0);
    }

    SECTION("load_config restores role assignments") {
        // Set up config JSON
        json config;
        json sensors_array = json::array();
        json sensor1;
        sensor1["device_id"] = "td1_lane0";
        sensor1["role"] = "filament_color";
        sensor1["enabled"] = true;
        sensors_array.push_back(sensor1);
        config["sensors"] = sensors_array;

        mgr().load_config(config);

        auto configs = mgr().get_sensors();
        auto it = std::find_if(configs.begin(), configs.end(),
                               [](const auto& c) { return c.device_id == "td1_lane0"; });
        REQUIRE(it != configs.end());
        REQUIRE(it->role == ColorSensorRole::FILAMENT_COLOR);
    }

    SECTION("load_config with unknown device is handled gracefully") {
        json config;
        json sensors_array = json::array();
        json sensor1;
        sensor1["device_id"] = "unknown_device";
        sensor1["role"] = "filament_color";
        sensors_array.push_back(sensor1);
        config["sensors"] = sensors_array;

        // Should not crash
        mgr().load_config(config);

        // Existing sensors should be unaffected
        for (const auto& sensor : mgr().get_sensors()) {
            REQUIRE(sensor.role == ColorSensorRole::NONE);
        }
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE_METHOD(ColorSensorTestFixture, "ColorSensorManager - edge cases", "[color][edge]") {
    SECTION("get_sensor_state returns nullopt for unassigned role") {
        discover_test_sensors();
        auto state = mgr().get_sensor_state(ColorSensorRole::FILAMENT_COLOR);
        REQUIRE_FALSE(state.has_value());
    }

    SECTION("get_sensor_state returns nullopt for NONE role") {
        discover_test_sensors();
        auto state = mgr().get_sensor_state(ColorSensorRole::NONE);
        REQUIRE_FALSE(state.has_value());
    }

    SECTION("is_sensor_available checks role assignment and enabled") {
        discover_test_sensors();
        REQUIRE_FALSE(mgr().is_sensor_available(ColorSensorRole::FILAMENT_COLOR));

        mgr().set_sensor_role("td1_lane0", ColorSensorRole::FILAMENT_COLOR);
        REQUIRE(mgr().is_sensor_available(ColorSensorRole::FILAMENT_COLOR));

        mgr().set_sensor_enabled("td1_lane0", false);
        REQUIRE_FALSE(mgr().is_sensor_available(ColorSensorRole::FILAMENT_COLOR));
    }

    SECTION("category_name returns 'color'") {
        REQUIRE(mgr().category_name() == "color");
    }
}
