# MoonrakerClientMock - Remaining Work

**Last Updated:** 2025-11-29

## Overview

The mock client is functional for all current UI testing needs. These are optional enhancements for completeness.

---

## Low Priority (Nice-to-Have)

### 1. Fan Control Simulation
**Status:** STUB - logged but not tracked

**G-codes:**
- `M106 P0 S128` - Set fan 0 to 50% (S is 0-255)
- `M107 P0` - Turn off fan 0
- `SET_FAN_SPEED FAN=fan SPEED=0.5` - Klipper fan control

**Implementation needed:**
- Parse S value and fan index from M106/M107
- Update `fan_speed_` atomic and dispatch status update

---

### 2. Z Offset Tracking
**Status:** STUB - logged but not tracked

**G-code:** `SET_GCODE_OFFSET Z=0.2`

**Implementation needed:**
- Parse Z parameter
- Store in `gcode_offset_z_` variable
- Include in status updates

---

### 3. Extrusion Position Tracking
**Status:** Not implemented

**G-codes:**
- `G92 E0` - Set extruder position to 0
- `G0/G1` with E parameter - Extrude during moves

**Implementation needed:**
- Parse E value from G92 and G0/G1 commands
- Maintain `extruder_position_` atomic variable

---

### 4. Input Shaper Configuration
**Status:** STUB - logged but not tracked

**G-code:** `SET_INPUT_SHAPER AXIS=X FREQ=60 DAMPING=0.1`

**Implementation needed:**
- Parse AXIS, FREQ, DAMPING parameters
- Store per-axis configuration

---

### 5. Pressure Advance Simulation
**Status:** STUB - logged but not tracked

**G-code:** `SET_PRESSURE_ADVANCE ADVANCE=0.05 EXTRUDER=extruder`

**Implementation needed:**
- Parse ADVANCE and EXTRUDER parameters
- Store per-extruder values

---

### 6. printer.objects.query Callback
**Status:** Not implemented

Currently unimplemented - would return current printer state snapshot.

---

## Already Implemented (Reference)

These features are fully working in the mock client:

- ✅ Temperature simulation (extruder + bed with realistic heating/cooling)
- ✅ Motion & positioning (X/Y/Z, G90/G91, G28 homing)
- ✅ Print state machine (standby/printing/paused/complete/cancelled/error)
- ✅ Hardware discovery (7 printer presets)
- ✅ Bed mesh simulation (BED_MESH_CALIBRATE, profiles)
- ✅ Speed/flow factor oscillation
- ✅ `server.files.list` with callback
- ✅ `server.files.metadata` with callback and thumbnail extraction
- ✅ SET_LED control with RGB/HSV parsing

---

## Not Planned

These are unlikely to be needed:

- **FIRMWARE_RESTART/RESTART** - UI doesn't handle firmware restart
- **PROBE/QGL/Z_TILT_ADJUST** - Complex state machines, rarely used in UI
