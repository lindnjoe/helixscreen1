// SPDX-License-Identifier: GPL-3.0-or-later

#include "config.h"
#include "led/led_controller.h"
#include "printer_discovery.h"

#include "../catch_amalgamated.hpp"
#include "hv/json.hpp"

TEST_CASE("PrinterDiscovery detects led_effect objects", "[led][discovery]") {
    helix::PrinterDiscovery discovery;
    nlohmann::json objects =
        nlohmann::json::array({"led_effect breathing", "led_effect fire_comet",
                               "led_effect rainbow", "neopixel chamber_light", "extruder"});
    discovery.parse_objects(objects);

    REQUIRE(discovery.has_led_effects());
    REQUIRE(discovery.led_effects().size() == 3);
    REQUIRE(discovery.led_effects()[0] == "led_effect breathing");
    REQUIRE(discovery.led_effects()[1] == "led_effect fire_comet");
    REQUIRE(discovery.led_effects()[2] == "led_effect rainbow");

    // Verify native LEDs still detected
    REQUIRE(discovery.has_led());
    REQUIRE(discovery.leds().size() == 1);
    REQUIRE(discovery.leds()[0] == "neopixel chamber_light");
}

TEST_CASE("PrinterDiscovery: led_effect does not get caught by led prefix", "[led][discovery]") {
    helix::PrinterDiscovery discovery;
    nlohmann::json objects = nlohmann::json::array({"led_effect status_effect", "led case_light"});
    discovery.parse_objects(objects);

    REQUIRE(discovery.has_led_effects());
    REQUIRE(discovery.led_effects().size() == 1);
    REQUIRE(discovery.led_effects()[0] == "led_effect status_effect");

    // "led case_light" should be in leds, not effects
    REQUIRE(discovery.has_led());
    REQUIRE(discovery.leds().size() == 1);
    REQUIRE(discovery.leds()[0] == "led case_light");
}

TEST_CASE("PrinterDiscovery detects LED-related macros", "[led][discovery]") {
    helix::PrinterDiscovery discovery;
    nlohmann::json objects = nlohmann::json::array(
        {"gcode_macro LIGHTS_ON", "gcode_macro LIGHTS_OFF", "gcode_macro LED_PARTY",
         "gcode_macro LAMP_TOGGLE", "gcode_macro BACKLIGHT_SET", "gcode_macro PRINT_START",
         "gcode_macro PRINT_END", "gcode_macro M600", "gcode_macro BED_MESH_CALIBRATE",
         "gcode_macro HOME_ALL"});
    discovery.parse_objects(objects);

    REQUIRE(discovery.has_led_macros());
    auto& led_macros = discovery.led_macros();

    // Should include LED-related macros
    REQUIRE(std::find(led_macros.begin(), led_macros.end(), "LIGHTS_ON") != led_macros.end());
    REQUIRE(std::find(led_macros.begin(), led_macros.end(), "LIGHTS_OFF") != led_macros.end());
    REQUIRE(std::find(led_macros.begin(), led_macros.end(), "LED_PARTY") != led_macros.end());
    REQUIRE(std::find(led_macros.begin(), led_macros.end(), "LAMP_TOGGLE") != led_macros.end());
    REQUIRE(std::find(led_macros.begin(), led_macros.end(), "BACKLIGHT_SET") != led_macros.end());

    // Should NOT include excluded macros
    REQUIRE(std::find(led_macros.begin(), led_macros.end(), "PRINT_START") == led_macros.end());
    REQUIRE(std::find(led_macros.begin(), led_macros.end(), "PRINT_END") == led_macros.end());
    REQUIRE(std::find(led_macros.begin(), led_macros.end(), "M600") == led_macros.end());
    REQUIRE(std::find(led_macros.begin(), led_macros.end(), "BED_MESH_CALIBRATE") ==
            led_macros.end());
    REQUIRE(std::find(led_macros.begin(), led_macros.end(), "HOME_ALL") == led_macros.end());
}

TEST_CASE("PrinterDiscovery: non-LED macros not detected", "[led][discovery]") {
    helix::PrinterDiscovery discovery;
    nlohmann::json objects = nlohmann::json::array(
        {"gcode_macro PARK_TOOLHEAD", "gcode_macro SET_VELOCITY", "gcode_macro START_PRINT"});
    discovery.parse_objects(objects);

    REQUIRE(!discovery.has_led_macros());
    REQUIRE(discovery.led_macros().empty());
}

TEST_CASE("LedController discover_from_hardware with effects and macros", "[led][discovery]") {
    helix::PrinterDiscovery discovery;
    nlohmann::json objects = nlohmann::json::array(
        {"neopixel chamber_light", "led_effect breathing", "led_effect fire_comet",
         "gcode_macro LIGHTS_ON", "gcode_macro LIGHTS_OFF", "gcode_macro LED_PARTY"});
    discovery.parse_objects(objects);

    auto& ctrl = helix::led::LedController::instance();
    ctrl.deinit();

    // Clear any macro_devices from previous tests so auto-creation kicks in
    auto* cfg = Config::get_instance();
    if (cfg) {
        cfg->set("/printer/leds/macro_devices", nlohmann::json::array());
        cfg->save();
    }

    ctrl.init(nullptr, nullptr);
    ctrl.discover_from_hardware(discovery);

    // Native backend
    REQUIRE(ctrl.native().is_available());
    REQUIRE(ctrl.native().strips().size() == 1);

    // Effects backend
    REQUIRE(ctrl.effects().is_available());
    REQUIRE(ctrl.effects().effects().size() == 2);
    REQUIRE(ctrl.effects().effects()[0].display_name == "Breathing");
    REQUIRE(ctrl.effects().effects()[0].icon_hint == "air");
    REQUIRE(ctrl.effects().effects()[1].display_name == "Fire Comet");

    // Discovered macros stored as candidates
    REQUIRE(ctrl.discovered_macros().size() == 3);
    REQUIRE(std::find(ctrl.discovered_macros().begin(), ctrl.discovered_macros().end(),
                      "LIGHTS_ON") != ctrl.discovered_macros().end());

    // Auto-created macro devices from candidates (no config loaded)
    REQUIRE(ctrl.macro().is_available());
    // Should have auto-created "Lights" (ON_OFF) and "LED Modes" (PRESET: LED_PARTY)
    REQUIRE(ctrl.macro().macros().size() == 2);

    // Verify auto-created devices
    const auto& macros = ctrl.macro().macros();
    bool found_lights = false, found_modes = false;
    for (const auto& m : macros) {
        if (m.display_name == "Lights") {
            found_lights = true;
            REQUIRE(m.type == helix::led::MacroLedType::ON_OFF);
            REQUIRE(m.on_macro == "LIGHTS_ON");
            REQUIRE(m.off_macro == "LIGHTS_OFF");
        }
        if (m.display_name == "LED Modes") {
            found_modes = true;
            REQUIRE(m.type == helix::led::MacroLedType::PRESET);
            REQUIRE(m.presets.size() == 1);
            REQUIRE(m.presets[0].second == "LED_PARTY");
        }
    }
    REQUIRE(found_lights);
    REQUIRE(found_modes);

    // All three backends available
    auto backends = ctrl.available_backends();
    REQUIRE(backends.size() == 3);

    ctrl.deinit();
}

TEST_CASE("PrinterDiscovery clear resets LED effects and macros", "[led][discovery]") {
    helix::PrinterDiscovery discovery;
    nlohmann::json objects = nlohmann::json::array({"led_effect test", "gcode_macro LIGHTS_ON"});
    discovery.parse_objects(objects);
    REQUIRE(discovery.has_led_effects());
    REQUIRE(discovery.has_led_macros());

    discovery.clear();
    REQUIRE(!discovery.has_led_effects());
    REQUIRE(!discovery.has_led_macros());
    REQUIRE(discovery.led_effects().empty());
    REQUIRE(discovery.led_macros().empty());
}
