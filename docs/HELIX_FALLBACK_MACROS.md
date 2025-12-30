# HELIX_* Fallback Macros

This document describes HelixScreen's fallback macro system - helper macros that can be installed on printers that don't have their own equivalents.

## Overview

The StandardMacros system resolves macros in priority order:
1. **User configured** - Explicit choice in Settings
2. **Auto-detected** - Pattern matching finds printer's existing macros
3. **HELIX fallback** - HelixScreen's helper macro (if installed)
4. **Empty** - Slot disabled

HELIX_* fallback macros provide a safety net when a printer doesn't have common utility macros.

## Available HELIX_* Macros

All macros are defined in `src/printer/macro_manager.cpp` and installed together as `helix_macros.cfg`.

### HELIX_BED_LEVEL_IF_NEEDED

**Purpose:** Conditional bed mesh - only runs if mesh is stale or missing.

**Behavior:**
- Checks if a bed mesh profile exists
- Checks mesh age against `MESH_MAX_AGE` threshold (default: 1 hour)
- Only runs `BED_MESH_CALIBRATE` if needed
- Saves profile automatically

**Use Case:** Prevents redundant meshing on every print while ensuring mesh is fresh.

### HELIX_CLEAN_NOZZLE

**Purpose:** Standardized nozzle wipe routine.

**Parameters:**
- `BRUSH_X`, `BRUSH_Y` - Brush position
- `WIPE_COUNT` - Number of wipe passes

**Use Case:** Pre-print nozzle cleaning for printers with a brush.

### HELIX_START_PRINT

**Purpose:** Unified print-start orchestrator.

**Features:**
- Heats bed and nozzle
- Runs QGL or Z_TILT_ADJUST if available
- Runs conditional bed mesh
- Cleans nozzle if brush configured

**Note:** Not currently assigned to a StandardMacro slot - the Print Preparation Manager handles start-print orchestration differently.

### HELIX_VERSION

**Purpose:** Reports installed macro package version.

## Current Wiring Status

| StandardMacro Slot | Spec Says | Code Implements | Status |
|--------------------|-----------|-----------------|--------|
| LoadFilament | — | "" | ✓ |
| UnloadFilament | — | "" | ✓ |
| Purge | — | "" | ✓ |
| Pause | — | "" | ✓ |
| Resume | — | "" | ✓ |
| Cancel | — | "" | ✓ |
| BedMesh | — | "" | ✓ |
| **BedLevel** | **HELIX_BED_LEVEL_IF_NEEDED** | **""** | **❌ Not wired** |
| CleanNozzle | HELIX_CLEAN_NOZZLE | HELIX_CLEAN_NOZZLE | ✓ |
| HeatSoak | — | "" | ✓ |

**Gap:** `HELIX_BED_LEVEL_IF_NEEDED` is defined but not connected to the BedLevel slot.

## Installation

Via Settings → Advanced → Install Helix Macros:

1. Uploads `helix_macros.cfg` to printer's config directory
2. Adds `[include helix_macros.cfg]` to `printer.cfg`
3. Triggers Klipper restart
4. Re-discovers capabilities to confirm installation

## Design Decisions

### Why Not More Fallbacks?

Some operations are too printer-specific for safe generic implementations:

- **LoadFilament/UnloadFilament**: Extruder type, runout sensors, and filament path vary wildly
- **Pause/Resume/Cancel**: Klipper provides built-in versions; we auto-detect those
- **Purge**: Flow rate and nozzle size make this highly variable
- **HeatSoak**: Could be added - simple "wait for chamber temp" is fairly universal

### HELIX_START_PRINT Integration

This macro exists but isn't part of StandardMacros. The Print Preparation Manager provides more granular control over print-start sequencing through the UI rather than a monolithic macro.

## Future Considerations

1. **Wire up BedLevel fallback** - Match the spec, connect `HELIX_BED_LEVEL_IF_NEEDED`
2. **Add HeatSoak fallback** - Simple chamber temperature wait
3. **Consider HELIX_START_PRINT role** - Does it complement or conflict with Print Preparation?

## Related Files

- `include/standard_macros.h` - Slot definitions and resolution logic
- `src/printer/standard_macros.cpp` - FALLBACK_MACROS map
- `src/printer/macro_manager.cpp` - HELIX_MACROS_CFG content
- `docs/STANDARD_MACROS_SPEC.md` - Original specification
