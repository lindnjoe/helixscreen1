# MoonrakerClientMock Implementation Plan

## Current Status (Complete)

### Temperature & Heating
- Extruder temperature simulation with realistic heating/cooling rates
- Bed temperature simulation
- Temperature targets update via M-code (M104, M109, M140, M190) and Klipper commands
- Automatic heating/cooling toward targets in background thread

### Motion & Positioning
- Position tracking (X, Y, Z) via G0/G1 commands
- Absolute (G90) and relative (G91) positioning modes
- Homing state tracking (G28 with axis selection)
- Position persistence across multiple moves

### Print State Machine
- States: standby, printing, paused, complete, cancelled, error
- Print file tracking (SDCARD_PRINT_FILE)
- Print progress simulation (0.0 to 1.0 over ~50 seconds during print)
- State transitions: pause/resume/cancel/emergency stop (M112)
- Delayed transition from cancelled→standby (2 ticks = 1 second)

### Hardware Discovery
- 7 printer type presets (Voron 2.4, Trident, Creality K1, FlashForge, Generic CoreXY/Bedslinger, Multi-extruder)
- Heater, sensor, fan, and LED lists populated per printer type
- Discovery callback invoked immediately

### Bed Mesh Simulation
- Initial synthetic 7×7 dome-shaped mesh (0-0.3mm Z range)
- BED_MESH_CALIBRATE: regenerate mesh with deterministic variation per profile
- BED_MESH_PROFILE: load/save/remove profiles
- BED_MESH_CLEAR: clear active mesh
- Profile-based mesh variation

### Speed & Flow Factors
- Speed factor oscillation (90-110%) simulated via sine wave
- Flow factor oscillation (95-105%) simulated via cosine wave
- Fan ramp simulation (0→255 over 60 ticks = 30 seconds)

### Bed Mesh File APIs (In Progress)
- `server.files.list`: Mock file list from `assets/test_gcodes/` directory
  - Returns filename, path, size, modified timestamp, permissions
- `server.files.metadata`: Extract metadata from G-code file headers
  - Slicer name, version, estimated time, filament weight/length
  - Thumbnail extraction to local cache directory
  - Layer count, first layer temperatures

---

## Future Work (Prioritized)

### High Priority (UI Testing Blocker)

#### 1. Implement JSON-RPC Response Callbacks
**Goal**: Invoke callbacks for `send_jsonrpc()` methods with file/metadata responses

Currently, the mock builds response JSON but never invokes callbacks. Need to:
- Detect method in `send_jsonrpc(method, params, callback)`
- Build appropriate mock response JSON
- Invoke callback with response JSON immediately
- Handle both single-callback and success/error-callback variants

**Methods to Implement**:
- `server.files.list` → invoke with mock file list JSON
- `server.files.metadata` → invoke with extracted metadata JSON
- `printer.objects.query` → invoke with current printer state snapshot

**Tests**: Verify callbacks are invoked with correct JSON structure

---

### Medium Priority (Feature Completeness)

#### 2. Fan Control Simulation
**Goal**: Track fan speed state from G-code commands

**G-codes to implement**:
- M106 P0 S128 - Set fan 0 to 50% (S is 0-255)
- M107 P0 - Turn off fan 0
- SET_FAN_SPEED FAN=fan SPEED=0.5 - Klipper fan control

**Implementation**:
- Parse S value and fan index from M106/M107
- Parse FAN and SPEED from SET_FAN_SPEED
- Update `fan_speed_` atomic and dispatch status update
- Support multiple fans with independent speeds

**Tests**: Verify fan speed state changes, edge cases (S=0, S=255, invalid fan)

---

#### 3. Extrusion Position Tracking
**Goal**: Simulate extruder position (E axis) state

**G-codes to implement**:
- G92 E0 - Set extruder position to 0
- G0/G1 with E parameter - Extrude during moves

**Implementation**:
- Parse E value from G92 and G0/G1 commands
- Maintain `extruder_position_` atomic variable
- Track cumulative extrusion distance
- Update in status notifications

**Tests**: Verify E position persists, relative vs absolute modes, move + extrusion combinations

---

#### 4. Z Offset (GCODE_OFFSET) Simulation
**Goal**: Track Z offset state from SET_GCODE_OFFSET commands

**Implementation**:
- Parse Z parameter from SET_GCODE_OFFSET Z0.2
- Store in `gcode_offset_z_` variable
- Include in status updates under `gcode_move.gcode_position`

**Tests**: Verify offset applies to subsequent moves, zero offset works

---

#### 5. Input Shaper Configuration
**Goal**: Store and report input shaper state

**G-code**: SET_INPUT_SHAPER AXIS=X FREQ=60 DAMPING=0.1

**Implementation**:
- Parse AXIS, FREQ, DAMPING parameters
- Store per-axis (X, Y, Z) configuration in mock state
- Include in `printer.objects.query` response

**Tests**: Verify per-axis config persists, invalid axes ignored

---

#### 6. Pressure Advance Simulation
**Goal**: Store and report pressure advance tuning

**G-code**: SET_PRESSURE_ADVANCE ADVANCE=0.05 EXTRUDER=extruder

**Implementation**:
- Parse ADVANCE and EXTRUDER parameters
- Store per-extruder pressure advance values
- Include in status updates

**Tests**: Verify multi-extruder handling, default extruder fallback

---

### Low Priority (Completeness/Polish)

#### 7. LED Control (SET_LED)
**Status**: Logged but not simulated
**Why**: UI doesn't display LED state; nice-to-have for completeness

**Implementation**:
- Parse LED name and RGB/HSV parameters
- Store LED state in map: `std::map<std::string, LEDState>`
- Include in hardware discovery queries

---

#### 8. Firmware Restart (FIRMWARE_RESTART, RESTART)
**Status**: Logged but not simulated
**Why**: UI doesn't handle firmware restart; would need connection re-establishment

**Implementation** (Optional):
- Could simulate brief connection loss and reconnection
- Currently low priority since UI doesn't interact with it

---

#### 9. Probe & QGL/Z-Tilt Leveling
**Status**: Logged but not simulated
**Why**: Rarely used in UI; would require complex state machine

**Note**: These would need:
- PROBE: Single-point probing result
- QUAD_GANTRY_LEVEL: Multi-point leveling state
- Z_TILT_ADJUST: Multi-point Z-tilt state

Defer unless UI explicitly needs them.

---

## Not Implemented (By Design)

### send_jsonrpc() without Callback
- `send_jsonrpc(method)` - No response expected
- `send_jsonrpc(method, params)` - No response expected

These are fire-and-forget commands (firmware restarts, firmware upgrades, etc.) and don't need mock responses.

---

## Architecture Notes

### File APIs Design
Helper functions added at top of `src/moonraker_client_mock.cpp`:
- `scan_mock_gcode_files()` - Returns vector of filenames from `assets/test_gcodes/`
- `build_mock_file_list_response()` - Builds Moonraker-format JSON with file metadata
- `build_mock_file_metadata_response(filename)` - Extracts G-code metadata and thumbnails

### Callback Invocation Pattern
When implementing callback-based responses, follow this pattern:

```cpp
int MoonrakerClientMock::send_jsonrpc(const std::string& method,
                                      const json& params,
                                      std::function<void(json)> cb) {
    spdlog::debug("[MoonrakerClientMock] send_jsonrpc: {}", method);

    // Detect method and build response
    if (method == "server.files.list") {
        json response = build_mock_file_list_response();
        if (cb) {
            cb(response);  // Invoke callback immediately with mock data
        }
    } else if (method == "server.files.metadata") {
        // Parse params for filename
        std::string filename = params.value("filename", "");
        json response = build_mock_file_metadata_response(filename);
        if (cb) {
            cb(response);
        }
    }
    // ... other methods
    else {
        spdlog::warn("[MoonrakerClientMock] send_jsonrpc: {} not implemented", method);
    }

    return 0;
}
```

---

## Testing Strategy

### Unit Tests
Each implementation should include:
1. **Happy path**: Valid command, correct state update, callback invoked
2. **Edge cases**: Empty parameters, invalid values, out-of-range
3. **Thread safety**: State updates visible in parallel simulation loop
4. **Callback verification**: Ensure response JSON matches expected format

### Integration Tests
- Print panel displays file list correctly
- Metadata panel shows extracted slicer info
- Temperature graph updates realistically
- Print progress bar advances smoothly

---

## Commit Strategy

Implement in small, testable increments:
1. ✅ File APIs (in progress) - Single PR with list + metadata
2. Fan control simulation - Small PR
3. Extrusion tracking - Small PR
4. Z offset + pressure advance - Combined small PR
5. Input shaper - Optional, can batch with #4
6. LED control - Optional, polish-only
7. Firmware restart - Very low priority

Each PR should:
- Have passing tests
- Include only related changes
- Reference implementation plan in commit message
