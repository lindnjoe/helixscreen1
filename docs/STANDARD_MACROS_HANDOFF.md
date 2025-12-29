# Standard Macros System - Session Handoff

**Copy the prompt below into a fresh Claude Code session to continue implementation.**

---

## Progress Summary

| Stage | Description | Status | Commit |
|-------|-------------|--------|--------|
| 1 | StandardMacros Core Class | ✅ Complete | `5888124d` |
| 2 | Hardware Discovery Integration | ✅ Complete | `5888124d` |
| 3 | Settings Overlay UI | ✅ Complete | `26dfae60` |
| 4 | Settings Panel Handlers | ✅ Complete | `26dfae60` |
| 5 | Controls Panel Integration | ✅ Complete | `26dfae60` |
| 6 | Filament Panel Integration | 🔲 Next | — |
| 7 | Print Status Panel Integration | 🔲 Pending | — |
| 8 | Testing & Polish | 🔲 Pending | — |

---

## What Was Built (Stages 1-5)

### Files Created
- `include/standard_macros.h` - Header with `StandardMacros` singleton, `StandardMacroSlot` enum, `StandardMacroInfo` struct
- `src/standard_macros.cpp` - Full implementation
- `ui_xml/macro_buttons_overlay.xml` - Settings overlay with Quick Buttons + Standard Macros dropdowns

### Files Modified
- `src/application/application.cpp` - Added `StandardMacros::instance().init()` in `on_hardware_discovered` callback
- `src/xml_registration.cpp` - Registered `macro_buttons_overlay.xml` component
- `ui_xml/settings_panel.xml` - Added "Macro Buttons" action row in PRINTER section
- `include/ui_panel_settings.h` - Added `macro_buttons_overlay_`, handler declarations
- `src/ui_panel_settings.cpp` - Added overlay creation, dropdown population, 11 change handlers
- `src/moonraker_api_advanced.cpp` - Implemented `execute_macro()` (was stub)
- `include/ui_panel_controls.h` - Added StandardMacros integration, `refresh_macro_buttons()`
- `src/ui_panel_controls.cpp` - Quick buttons now use StandardMacros slots instead of hardcoded G-code

### Key Implementation Details

**9 Semantic Slots:**
```cpp
enum class StandardMacroSlot {
    LoadFilament, UnloadFilament, Purge,
    Pause, Resume, Cancel,
    BedLevel, CleanNozzle, HeatSoak
};
```

**Priority Resolution:** `configured_macro` → `detected_macro` → `fallback_macro`

**Auto-Detection Patterns:**
| Slot | Patterns |
|------|----------|
| LoadFilament | LOAD_FILAMENT, M701 |
| UnloadFilament | UNLOAD_FILAMENT, M702 |
| Purge | PURGE, PURGE_LINE, PRIME_LINE |
| Pause | PAUSE, M601 |
| Resume | RESUME, M602 |
| Cancel | CANCEL_PRINT |
| BedLevel | BED_MESH_CALIBRATE, Z_TILT_ADJUST, QUAD_GANTRY_LEVEL, QGL |
| CleanNozzle | CLEAN_NOZZLE, NOZZLE_WIPE, WIPE_NOZZLE |
| HeatSoak | HEAT_SOAK, CHAMBER_SOAK, SOAK |

**HELIX Fallbacks:** Only `BedLevel` (HELIX_BED_LEVEL_IF_NEEDED) and `CleanNozzle` (HELIX_CLEAN_NOZZLE) have fallbacks.

**Config Path:** `/standard_macros/<slot_name>` (e.g., `/standard_macros/load_filament`)

### Issues Resolved During Implementation
1. **Missing `#include <optional>`** in header - Fixed
2. **Fallback macros not restored on re-init** - Fixed by restoring from static table at start of `init()`
3. **`class` vs `struct` forward declaration mismatch** for MoonrakerError - Fixed

### Stage 5 Implementation Details
- Implemented `MoonrakerAPI::execute_macro()` (was a stub)
- Quick buttons read slot names from config: `/standard_macros/quick_button_1`, `/standard_macros/quick_button_2`
- `refresh_macro_buttons()` updates labels and hides empty slots
- `on_activate()` refreshes buttons after StandardMacros initialization

---

## Handoff Prompt for Stage 6

```
I'm continuing the "Standard Macros" system for HelixScreen. Stages 1-5 are complete.

## Quick Context

HelixScreen is a C++/LVGL touchscreen UI for 3D printers. The StandardMacros system maps semantic operations (Load Filament, Pause, Clean Nozzle, etc.) to printer-specific G-code macros.

## What's Already Done

- `include/standard_macros.h` - StandardMacros singleton with 9 slots
- `src/standard_macros.cpp` - Full implementation with auto-detection, config, execute()
- `src/moonraker_api_advanced.cpp` - `execute_macro()` implemented
- Integration in `src/application/application.cpp` - init() called on hardware discovery
- `ui_xml/macro_buttons_overlay.xml` - UI overlay with Quick Buttons + Standard Macros dropdowns
- `ui_xml/settings_panel.xml` - "Macro Buttons" action row added
- `src/xml_registration.cpp` - Overlay component registered
- `src/ui_panel_settings.cpp` - Settings overlay handlers wired up with dropdown population
- `src/ui_panel_controls.cpp` - Quick buttons integrated with StandardMacros

## Read First

1. Read the spec: docs/STANDARD_MACROS_SPEC.md (especially Panel Integration section)
2. Read current filament panel: src/ui_panel_filament.cpp
3. Look for existing LOAD_FILAMENT/UNLOAD_FILAMENT handling

## Current Stage: 6 - Filament Panel Integration

Task: Integrate Filament Panel with StandardMacros

Implementation:
1. Find existing filament load/unload calls in filament panel
2. Replace hardcoded calls with `StandardMacros::instance().execute()`
3. Handle empty slots by showing warning toast

Example pattern from spec:
```cpp
void FilamentPanel::execute_load() {
    if (!StandardMacros::instance().execute(
            StandardMacroSlot::LoadFilament, api_,
            []() { NOTIFY_SUCCESS("Loading filament..."); },
            [](auto& err) { NOTIFY_ERROR("Load failed: {}", err.user_message()); })) {
        NOTIFY_WARNING("Load filament macro not configured");
    }
}
```

## Workflow

1. Read filament panel to understand current load/unload handling
2. Add StandardMacros integration
3. Run `make -j` to verify
4. Test with `./build/bin/helix-screen --test -p filament -vv`
5. Update docs/STANDARD_MACROS_HANDOFF.md
6. Commit when working

Begin with Stage 6.
```

---

## Stage Prompts (Stages 6-8)

### Stage 4: Settings Panel Handlers ✅ COMPLETE
Settings panel handlers are fully implemented:
- `handle_macro_buttons_clicked()` creates overlay lazily
- 11 dropdown change handlers registered (2 quick buttons + 9 slots)
- Dropdowns populate with "(Auto: X)" / "(Empty)" + sorted printer macros
- Config saved via `StandardMacros::set_macro()` and direct config writes

### Stage 5: Controls Panel Integration ✅ COMPLETE
Controls panel quick buttons now use StandardMacros:
- Implemented `MoonrakerAPI::execute_macro()` (builds G-code from name + params)
- Quick buttons read slot assignment from config (`/standard_macros/quick_button_1`, etc.)
- `refresh_macro_buttons()` updates labels from `StandardMacroInfo::display_name`
- Empty slots hide the button via `LV_OBJ_FLAG_HIDDEN`
- `on_activate()` refreshes after StandardMacros initialization

### Stage 6: Filament Panel Integration
```
Continue with Stage 6 of the Standard Macros system.

Task: Integrate Filament Panel with StandardMacros
- Replace hardcoded LOAD_FILAMENT/UNLOAD_FILAMENT calls with StandardMacros::execute()
- Handle empty slots (show warning toast if slot not configured)
- Example pattern from spec:
  if (!StandardMacros::instance().execute(StandardMacroSlot::LoadFilament, api_, ...)) {
      NOTIFY_WARNING("Load filament macro not configured");
  }

Files: src/ui_panel_filament.cpp
```

### Stage 7: Print Status Panel Integration
```
Continue with Stage 7 of the Standard Macros system.

Task: Integrate Print Status Panel with StandardMacros
- Pause/Resume/Cancel buttons use StandardMacros::execute()
- Disable (grey out) buttons if slot is empty
- Don't hide - user needs to see the button exists but is unavailable

Files: src/ui_panel_print_status.cpp
```

### Stage 8: Testing & Polish
```
Final stage of Standard Macros system.

Task: Testing & Polish
1. Run full test suite: make test-run
2. Test with --test flag (mock printer):
   - Verify auto-detection logs show discovered macros
   - Verify config persistence (set a macro, restart, verify it's restored)
   - Verify empty slot handling (buttons hidden/disabled appropriately)
3. Test UI flow:
   - Settings → Macro Buttons → Change a dropdown → Verify saved
   - Controls panel quick buttons update after config change
4. Consider adding unit tests for StandardMacros if time permits

When all tests pass, we're done!
```

---

## Reference Files

| Purpose | File |
|---------|------|
| Overlay pattern | `ui_xml/display_settings_overlay.xml` |
| Setting components | `ui_xml/setting_toggle_row.xml`, `setting_dropdown_row.xml` |
| XML registration | `src/xml_registration.cpp` |
| Settings panel | `ui_xml/settings_panel.xml`, `src/ui_panel_settings.cpp` |
| Macro detection | `include/printer_capabilities.h` |
| HELIX macros | `src/helix_macro_manager.cpp` |
| Config access | `include/config.h` |

---

## Notes

- Each stage should be a single commit with conventional commit format
- Run `make -j` after each stage to verify compilation
- Run agentic code review before committing
- The SPEC file (`docs/STANDARD_MACROS_SPEC.md`) is the source of truth for design decisions
- Use the approaches system to track multi-step work
