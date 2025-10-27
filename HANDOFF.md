# Session Handoff Document

**Last Updated:** 2025-10-26 Evening
**Current Focus:** Testing infrastructure established, wizard connection bugs fixed

---

## 🎯 Active Work & Next Priorities

**Status:** Testing framework complete with Catch2 v3, wizard connection refactored with reactive subjects and input validation

**Completed This Session:**
- ✅ Fixed wizard connection bugs (timeout, config update, websocket warning)
- ✅ Refactored wizard to use reactive subjects instead of direct widget manipulation
- ✅ Added comprehensive input validation (IP/hostname, port) with clear error messages
- ✅ Established testing infrastructure with Catch2 v3 (47 passing tests)
- ✅ Extracted validation logic to testable modules (`wizard_validation.h/.cpp`)
- ✅ Created test framework documentation in STATUS.md

**Next Steps (Priority Order):**
1. **Test wizard interactively:** `./build/bin/helix-ui-proto --wizard -s small`
   - Verify input validation (bad IP, invalid port, timeout)
   - **Fix Next button crash** (currently crashes, needs investigation)
   - Test successful connection flow and config save

2. **Write integration tests** for wizard flow:
   - Full wizard step progression
   - Button click simulation with `lv_event_send()`
   - State transition verification

3. **MoonrakerClient mocking** for connection tests:
   - Mock successful connection
   - Mock failed connection
   - Mock timeout scenarios

4. **Migrate old tests** to Catch2 v3:
   - `tests/unit/test_navigation.cpp` (navigation system)
   - `tests/unit/test_temp_graph.cpp` (temperature graph widget)

5. **Future wizard work:**
   - Hardware detection/mapping screens
   - Moonraker WebSocket integration for live printer communication

---

## 📋 Critical Architecture Patterns (Essential How-To Reference)

### Pattern #0: Navigation History Stack

**When to use:** Overlay panels (motion, temps, extrusion, keypad)

```cpp
// When showing overlay
ui_nav_push_overlay(motion_panel);  // Pushes current to history

// In back button callback
if (!ui_nav_go_back()) {
    // Fallback if history empty
}
```

**Files:** `ui_nav.h:54-62`, `ui_nav.cpp:250-327`

### Pattern #1: Global Keyboard for Textareas

**When to use:** Any textarea that needs text input

```cpp
// One-time init in main.cpp (already done)
ui_keyboard_init(lv_screen_active());

// For each textarea
ui_keyboard_register_textarea(my_textarea);
// That's it! Auto show/hide on focus
```

**Files:** `include/ui_keyboard.h`, `src/ui_keyboard.cpp`, `src/main.cpp:514`

### Pattern #2: Subject Initialization Order

**MUST initialize subjects BEFORE creating XML:**

```cpp
// CORRECT ORDER:
lv_xml_component_register_from_file("A:/ui_xml/globals.xml");
lv_xml_component_register_from_file("A:/ui_xml/my_panel.xml");

ui_my_panel_init_subjects();  // Initialize FIRST

lv_xml_create(screen, "my_panel", NULL);  // Create AFTER
```

### Pattern #3: Component Instantiation Names

**Always add explicit `name` attributes:**

```xml
<!-- WRONG: No name on instantiation -->
<app_layout>
  <my_panel/>
</app_layout>

<!-- CORRECT: Explicit name -->
<app_layout>
  <my_panel name="my_panel"/>
</app_layout>
```

**Why:** Component `<view name="...">` doesn't propagate to instantiation tags

### Pattern #4: Image Scaling in Flex Layouts

**When scaling images after layout changes, call `lv_obj_update_layout()` first:**

```cpp
lv_obj_update_layout(container);  // Force layout calculation
ui_image_scale_to_cover(img, container);  // Now works correctly
```

**Why:** LVGL uses deferred layout - containers report 0x0 until forced

**Files:** `ui_utils.cpp:213-276`, `ui_panel_print_status.cpp:249-314`

### Pattern #5: Copyright Headers

**ALL new files MUST include GPL v3 header**

**Reference:** `docs/COPYRIGHT_HEADERS.md` for C/C++ and XML templates

### Pattern #6: Logging Policy

**ALWAYS use spdlog, NEVER printf/cout/LV_LOG:**

```cpp
#include <spdlog/spdlog.h>

spdlog::debug("[Component] Debug info: {}", value);
spdlog::info("Operation complete");
spdlog::warn("Validation failed, using default");
spdlog::error("Critical failure: {}", error);
```

**Formatting:**
- Use fmt-style: `spdlog::info("Val: {}", x)` NOT printf-style
- Cast enums: `(int)panel_id`
- Cast pointers: `(void*)widget`

**Reference:** `CLAUDE.md` lines 77-134

### Pattern #7: LV_SIZE_CONTENT Pitfalls

**Problem:** Evaluates to 0 due to deferred layout calculation

**Solutions:**
1. Call `lv_obj_update_layout()` after creation (forces layout)
2. Use explicit pixel dimensions in XML (recommended)
3. Use `style_min_height`/`style_min_width` for flex containers

**Reference:** `docs/LVGL9_XML_GUIDE.md:1241-1329` (comprehensive guide)

### Pattern #8: Testing with Catch2 v3

**Extract logic to testable modules:**

```cpp
// include/my_module.h
bool validate_input(const std::string& input);

// src/my_module.cpp
bool validate_input(const std::string& input) {
    // Pure logic, no LVGL dependencies
    return !input.empty();
}

// tests/unit/test_my_module.cpp
#include "../catch_amalgamated.hpp"
#include "my_module.h"

TEST_CASE("Input validation", "[module][validation]") {
    REQUIRE(validate_input("valid") == true);
    REQUIRE(validate_input("") == false);
}
```

**Run tests:**
```bash
make test-wizard              # Run all unit tests
./build/bin/test_wizard_validation "[tag]"  # Run specific tests
```

**Best practices:**
- Keep logic separate from UI code (enables pure unit tests)
- Use descriptive test names and tags
- `REQUIRE()` for critical checks, `CHECK()` for non-fatal
- See STATUS.md for complete test framework documentation

**Files:** `tests/unit/test_wizard_validation.cpp`, `Makefile:350-384`, `STATUS.md:89-150`

---

## 🔧 Known Issues & Gotchas

### Wizard Next Button Crash ⚠️ ACTIVE BUG

**Problem:** Clicking "Next" button in wizard connection screen crashes the UI

**Status:** Needs investigation - likely accessing null widgets in next step

**Priority:** High - blocks wizard testing

**Next session:** Debug with `--wizard -s small`, add error handling for missing widgets

### LVGL 9 XML Flag Syntax ✅ FIXED

**NEVER use `flag_` prefix in XML attributes:**
- ❌ `flag_hidden="true"`, `flag_clickable="true"`
- ✅ `hidden="true"`, `clickable="true"`

**Status:** All 12 XML files fixed (2025-10-24)

### Icon Constants Rendering

Icon values appear empty in grep/terminal (FontAwesome uses Unicode Private Use Area U+F000-U+F8FF) but contain UTF-8 bytes.

**Fix:** `python3 scripts/generate-icon-consts.py` regenerates constants

**Verify:** `python3 -c "import re; f=open('ui_xml/globals.xml'); print([match.group(1).encode('utf-8').hex() for line in f for match in [re.search(r'icon_backspace.*value=\"([^\"]*)\"', line)] if match])"`

---

**Rule:** When work is complete, REMOVE it from HANDOFF and document in STATUS.md. Keep this document lean and current.
