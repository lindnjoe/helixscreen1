# Pre-Print Options - Session Handoff

**Last Updated:** 2025-11-28
**Status:** Phase 13 - G-code Pre-Print Modification (IN PROGRESS)

---

## What's Been Completed

### Core Infrastructure (DONE)
- **GCodeOpsDetector** (`src/gcode_ops_detector.cpp`) - Scans G-code files and detects operations like `BED_MESH_CALIBRATE`, `QUAD_GANTRY_LEVEL`, `CLEAN_NOZZLE`, etc.
- **PrinterCapabilities** (`src/printer_capabilities.cpp`) - Detects printer features from Moonraker's `printer.objects.list` (QGL, Z-tilt, bed mesh, macros)
- **CommandSequencer** (`src/command_sequencer.cpp`) - Coordinates multi-step print preparation with state-based completion detection
- **MacroManager** (`src/helix_macro_manager.cpp`) - Manages HelixScreen helper macros (HELIX_START_PRINT, etc.)

### UI Integration (DONE)
- **Pre-print checkboxes** in file detail view (`ui_xml/print_file_detail.xml`)
  - Bed Leveling, QGL, Z Tilt, Nozzle Clean checkboxes
  - Visibility controlled by `printer_has_*` subjects
  - Wired to `start_print()` flow in `ui_panel_print_select.cpp`
- **Filament type dropdown** auto-populated from G-code metadata

### Test Coverage (DONE)
- `test_command_sequencer.cpp` - 10 test cases, 108 assertions
- `test_printer_capabilities.cpp` - Hardware and macro detection
- `test_gcode_ops_detector.cpp` - Operation detection + robustness edge cases
- `test_helix_macro_manager.cpp` - Macro content and installation
- Validation tests fixed with RFC 1035 compliance

---

## What's Remaining (Priority Order)

### 1. G-code File Modification (Stage 3 from plan)
**Files:** `src/gcode_file_modifier.cpp` (NEW)

The detector finds operations, but we need to **comment them out** when user disables an option that's already in the G-code:

```cpp
class GCodeFileModifier {
public:
    // Comment out lines between start_line and end_line
    std::string disable_operation(const std::string& content,
                                   size_t start_line, size_t end_line);

    // Inject G-code at specific position
    std::string inject_gcode(const std::string& content,
                             size_t after_line,
                             const std::string& gcode_to_inject);

    // Create modified temp file for printing
    std::string create_modified_file(const std::string& original_path,
                                      const std::vector<Modification>& mods);
};
```

**Key consideration:** The current approach uses G-code injection (sending commands before `start_print()`), NOT file modification. File modification is the **secondary approach** for cases where injection isn't sufficient.

### 2. HTTP File Upload (Stage 4 from plan)
**Files:** `src/moonraker_api.cpp` (MODIFY)

MacroManager needs HTTP file upload to install `helix_macros.cfg`:
- POST to `/server/files/upload`
- Multipart form data with `file` and `root=config`
- Uses libhv HttpClient (already in project)

Currently stubbed with `TODO` comment in `helix_macro_manager.cpp:293-307`.

### 3. Print Status "Preparing" Phase (Stage 6 from plan)
**Files:** `src/ui_panel_print_status.cpp` (MODIFY)

Add visual feedback during pre-print sequence:
- New `PrintPhase::PREPARING` state
- Shows "Running bed leveling..." with progress
- Transitions to `PRINTING` when sequence completes

### 4. Wizard Connection/Summary Screens (Phase 11)
**Files:** `ui_xml/wizard_*.xml`, `src/ui_wizard_*.cpp`

Connection test and summary screens exist but aren't fully wired.

---

## Key Files to Read

| File | Purpose |
|------|---------|
| `docs/PRINT_OPTIONS_IMPLEMENTATION_PLAN.md` | Full architecture and stages |
| `docs/TESTING_PRE_PRINT_OPTIONS.md` | Manual test plan and checklist |
| `src/ui_panel_print_select.cpp:954-993` | Current `start_print()` flow |
| `src/command_sequencer.cpp` | How operations are sequenced |
| `src/printer_capabilities.cpp` | How printer features are detected |
| `include/gcode_ops_detector.h` | Operation detection API |

---

## Architecture Summary

```
User clicks "Print" with options checked
         │
         ▼
┌─────────────────────────────────────┐
│  PrintSelectPanel::start_print()   │
│  1. Collect checkbox states        │
│  2. Build operation sequence       │
│  3. Execute via CommandSequencer   │
└─────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│      CommandSequencer               │
│  For each operation:                │
│  1. Send G-code via MoonrakerAPI   │
│  2. Poll printer state             │
│  3. Wait for completion condition  │
│  4. Move to next operation         │
└─────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│  MoonrakerAPI::start_print()       │
│  Actual print begins               │
└─────────────────────────────────────┘
```

---

## Recent Commits (for context)

```
83902ab fix(cli): require --test mode for --gcode-file option
e66c1fa fix(validation): improve wizard IP/hostname/port validation
ae92452 feat(gcode-viewer): add ghost layer visualization
836bcd6 feat(macros): add HelixMacroManager for helper macro installation
907e8b5 docs(testing): add comprehensive pre-print options test plan
2c6a22d feat(pre-print): integrate CommandSequencer with start_print()
8c76c4f feat(pre-print): add Bambu-style pre-print options to file detail
```

---

## Known Issues

1. **Geometry builder SIGSEGV** - `test_gcode_geometry_builder.cpp:163` - RibbonGeometry move semantics bug (unrelated to pre-print)
2. **Wizard UI XML loading** - `wizard_container` returns nullptr in test environment (infrastructure issue)

---

## Next Session Prompt

```
Continue implementing the Bambu-style pre-print options feature for HelixScreen.

COMPLETED:
- GCodeOpsDetector, PrinterCapabilities, CommandSequencer, MacroManager
- UI checkboxes in file detail view, wired to start_print() flow
- Comprehensive test coverage

NEXT STEPS (pick one):
A) Implement GCodeFileModifier to comment out detected ops when user disables them
B) Implement HTTP file upload in MoonrakerAPI for macro installation
C) Add "Preparing" phase to PrintStatusPanel with progress feedback
D) Wire up wizard connection test + summary screens

Read these files first:
- docs/PRINT_OPTIONS_IMPLEMENTATION_PLAN.md (full architecture)
- docs/TESTING_PRE_PRINT_OPTIONS.md (test checklist)
- src/ui_panel_print_select.cpp (current print flow)
- src/command_sequencer.cpp (operation sequencing)

The project uses LVGL 9.4 with XML-based UI, Moonraker WebSocket API, and Catch2 for testing.
Build with: make -j
Run with: ./build/bin/helix-ui-proto --test -p print-select
```

---

## Build & Test Commands

```bash
# Build
make -j

# Run with mock printer
./build/bin/helix-ui-proto --test -p print-select

# Auto-select file and show detail view
./build/bin/helix-ui-proto --test -p print-select --select-file "3DBenchy.gcode"

# Run pre-print related tests
./build/bin/run_tests "[sequencer]" "[capabilities]" "[gcode_ops]" "[helix_macros]"

# Verbose logging
./build/bin/helix-ui-proto --test -vv 2>&1 | grep -E "CommandSequencer|start_print"
```
