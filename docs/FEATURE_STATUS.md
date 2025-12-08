# Feature Implementation Status

**Last Updated:** 2025-12-08

This document tracks the implementation status of all features identified in the feature parity analysis. It serves as the **single source of truth** for what's done, in progress, and remaining.

---

## Status Legend

| Icon | Status | Description |
|------|--------|-------------|
| ✅ | Complete | Fully implemented and tested |
| 🟡 | In Progress | Partially implemented, work ongoing |
| 🚧 | Stub Only | UI exists with "Coming Soon" overlay, not functional |
| ⬜ | Not Started | No work done yet |
| ❌ | Blocked | Cannot proceed (dependency, decision needed) |
| 🔴 | Deprecated | Removed from scope |

---

## Quick Stats

| Category | Complete | In Progress | Stub | Not Started | Total |
|----------|----------|-------------|------|-------------|-------|
| CRITICAL (Tier 1) | 0 | 0 | 0 | 7 | 7 |
| HIGH (Tier 2) | 0 | 0 | 0 | 7 | 7 |
| MEDIUM (Tier 3) | 0 | 0 | 0 | 7 | 7 |
| DIFFERENTIATOR (Tier 4) | 0 | 0 | 0 | 5 | 5 |
| **TOTAL** | **0** | **0** | **0** | **26** | **26** |

---

## TIER 1: CRITICAL Features

These features ALL major competitors have. Required for feature parity.

| Feature | Status | Files | Notes |
|---------|--------|-------|-------|
| **Temperature Presets** | ⬜ | - | PLA/PETG/ABS/etc preset buttons |
| **Macro Panel** | ⬜ | - | List/execute Klipper macros |
| **Console Panel** | ⬜ | - | G-code console with keyboard |
| **Screws Tilt Adjust** | ⬜ | - | Visual bed leveling with rotation indicators |
| **Camera/Webcam** | ⬜ | - | MJPEG viewer, Crowsnest integration |
| **Print History** | ⬜ | - | Past jobs list, statistics |
| **Power Device Control** | ⬜ | - | Moonraker power devices on/off |

### Detailed Status

#### Temperature Presets
- **Status:** ⬜ Not Started
- **Priority:** CRITICAL
- **Complexity:** MEDIUM
- **Depends On:** None (existing temp panels work)
- **Files to Create:**
  - [ ] `ui_xml/temp_preset_modal.xml`
  - [ ] `include/temperature_presets.h`
  - [ ] `src/temperature_presets.cpp`
- **Files to Modify:**
  - [ ] `ui_xml/nozzle_temp_panel.xml`
  - [ ] `ui_xml/bed_temp_panel.xml`
  - [ ] `config/helixconfig.json.template`
- **API:** None (just heater control)
- **Checklist:**
  - [ ] Default presets (PLA, PETG, ABS, TPU, ASA)
  - [ ] Custom preset creation
  - [ ] Preset editing/deletion
  - [ ] Quick-apply from home screen
  - [ ] Persist in config

#### Macro Panel
- **Status:** ⬜ Not Started
- **Priority:** CRITICAL
- **Complexity:** MEDIUM
- **Depends On:** None
- **Files to Create:**
  - [ ] `ui_xml/macro_panel.xml`
  - [ ] `ui_xml/macro_card.xml`
  - [ ] `include/ui_panel_macros.h`
  - [ ] `src/ui_panel_macros.cpp`
- **Files to Modify:**
  - [ ] `ui_xml/navigation_bar.xml`
  - [ ] `src/main.cpp`
- **API:** `printer.objects.query` for `gcode_macro *`
- **Checklist:**
  - [ ] List all macros from Klipper
  - [ ] Categorization (user, system, calibration)
  - [ ] Execute macro (no params)
  - [ ] Execute macro with params (on-screen keyboard)
  - [ ] Favorites/quick access
  - [ ] Hide system macros toggle

#### Console Panel
- **Status:** ⬜ Not Started
- **Priority:** CRITICAL
- **Complexity:** HIGH
- **Depends On:** On-screen keyboard (exists)
- **Files to Create:**
  - [ ] `ui_xml/console_panel.xml`
  - [ ] `include/ui_panel_console.h`
  - [ ] `src/ui_panel_console.cpp`
- **Files to Modify:**
  - [ ] `ui_xml/navigation_bar.xml`
  - [ ] `src/main.cpp`
  - [ ] `include/moonraker_api.h`
  - [ ] `src/moonraker_api.cpp`
- **API:** `/server/gcode_store`, `/printer/gcode/script`, `notify_gcode_response`
- **Checklist:**
  - [ ] Scrollable command history
  - [ ] G-code input with keyboard
  - [ ] Color-coded output (errors red)
  - [ ] Temperature message filtering
  - [ ] Command history (up/down)
  - [ ] Clear button

#### Screws Tilt Adjust
- **Status:** ⬜ Not Started
- **Priority:** CRITICAL
- **Complexity:** HIGH
- **Depends On:** None
- **Files to Create:**
  - [ ] `ui_xml/screws_tilt_panel.xml`
  - [ ] `ui_xml/screw_indicator.xml`
  - [ ] `include/ui_panel_screws_tilt.h`
  - [ ] `src/ui_panel_screws_tilt.cpp`
- **Files to Modify:**
  - [ ] `ui_xml/controls_panel.xml` (add card)
- **API:** `SCREWS_TILT_CALCULATE` command, parse response
- **Checklist:**
  - [ ] Visual bed diagram with screw positions
  - [ ] 3x3, 4-corner support
  - [ ] Rotation indicators ("CW 1/4 turn")
  - [ ] Re-probe button
  - [ ] Different bed shapes

#### Camera/Webcam
- **Status:** ⬜ Not Started
- **Priority:** CRITICAL
- **Complexity:** HIGH
- **Depends On:** Crowsnest/webcam configured
- **Files to Create:**
  - [ ] `ui_xml/camera_panel.xml`
  - [ ] `ui_xml/camera_pip.xml`
  - [ ] `include/ui_panel_camera.h`
  - [ ] `src/ui_panel_camera.cpp`
  - [ ] `include/webcam_client.h`
  - [ ] `src/webcam_client.cpp`
- **API:** `/server/webcams/list`, `/server/webcams/item`
- **Checklist:**
  - [ ] Single MJPEG stream display
  - [ ] Multi-camera selector
  - [ ] PiP during print
  - [ ] Snapshot button
  - [ ] Rotation/flip settings

#### Print History
- **Status:** ⬜ Not Started
- **Priority:** CRITICAL
- **Complexity:** MEDIUM
- **Depends On:** None
- **Files to Create:**
  - [ ] `ui_xml/history_panel.xml`
  - [ ] `ui_xml/history_item.xml`
  - [ ] `include/ui_panel_history.h`
  - [ ] `src/ui_panel_history.cpp`
- **Files to Modify:**
  - [ ] `ui_xml/navigation_bar.xml` or settings
- **API:** `/server/history/list`, `/server/history/totals`, `/server/history/job`
- **Checklist:**
  - [ ] List past print jobs
  - [ ] Success/failure indicators
  - [ ] Print time, filament used
  - [ ] Reprint from history
  - [ ] Statistics dashboard
  - [ ] Delete entries

#### Power Device Control
- **Status:** ⬜ Not Started
- **Priority:** HIGH
- **Complexity:** LOW
- **Depends On:** Power devices configured in Moonraker
- **Files to Create:**
  - [ ] `ui_xml/power_panel.xml`
  - [ ] `ui_xml/power_device_row.xml`
  - [ ] `include/ui_panel_power.h`
  - [ ] `src/ui_panel_power.cpp`
- **API:** `/machine/device_power/devices`, `/machine/device_power/device`
- **Checklist:**
  - [ ] List all power devices
  - [ ] On/Off/Toggle controls
  - [ ] Status indicators
  - [ ] Lock critical devices during print
  - [ ] Quick access from home

---

## TIER 2: HIGH Priority Features

Most competitors have these. Should implement for competitive parity.

| Feature | Status | Files | Notes |
|---------|--------|-------|-------|
| **Input Shaper Panel** | ⬜ | - | Resonance calibration UI |
| **Firmware Retraction** | ⬜ | - | View/adjust retraction settings |
| **Spoolman Integration** | ⬜ | - | Filament tracking, QR scanner |
| **Job Queue** | ⬜ | - | Batch printing queue |
| **Update Manager** | ⬜ | - | Software updates |
| **Timelapse Controls** | ⬜ | - | Moonraker-timelapse settings |
| **Layer Display** | ⬜ | - | Current/total layer on print status |

### Detailed Status

#### Input Shaper Panel
- **Status:** ⬜ Not Started
- **Priority:** HIGH
- **Complexity:** HIGH
- **API:** `SHAPER_CALIBRATE`, `MEASURE_AXES_NOISE`, result parsing
- **Checklist:**
  - [ ] Run calibration buttons
  - [ ] Progress indicator
  - [ ] Display recommended settings
  - [ ] Graph viewer for resonance results

#### Firmware Retraction
- **Status:** ⬜ Not Started
- **Priority:** HIGH
- **Complexity:** LOW
- **API:** `firmware_retraction` printer object
- **Checklist:**
  - [ ] View current settings
  - [ ] Adjust retract_length, retract_speed
  - [ ] Adjust unretract settings
  - [ ] Apply changes

#### Spoolman Integration
- **Status:** ⬜ Not Started
- **Priority:** HIGH
- **Complexity:** MEDIUM
- **API:** `/server/spoolman/*` endpoints
- **Checklist:**
  - [ ] Spoolman panel with spool list
  - [ ] Active spool display
  - [ ] Spool selection at print start
  - [ ] QR code scanner (killer feature!)
  - [ ] Remaining filament gauge
  - [ ] Low filament warnings

#### Job Queue
- **Status:** ⬜ Not Started
- **Priority:** HIGH
- **Complexity:** MEDIUM
- **API:** `/server/job_queue/*` endpoints
- **Checklist:**
  - [ ] View queued jobs
  - [ ] Add files to queue
  - [ ] Reorder queue
  - [ ] Remove from queue
  - [ ] Start/pause queue

#### Update Manager
- **Status:** ⬜ Not Started
- **Priority:** HIGH
- **Complexity:** MEDIUM
- **API:** `/machine/update/*` endpoints
- **Checklist:**
  - [ ] Show available updates
  - [ ] Update status indicators
  - [ ] One-click update
  - [ ] Rollback option

#### Timelapse Controls
- **Status:** ⬜ Not Started
- **Priority:** HIGH
- **Complexity:** MEDIUM
- **API:** Moonraker-timelapse API
- **Checklist:**
  - [ ] Enable/disable toggle
  - [ ] Mode selector
  - [ ] Frame rate setting
  - [ ] Video library browser

#### Layer Display
- **Status:** ⬜ Not Started
- **Priority:** HIGH
- **Complexity:** LOW
- **API:** `print_stats.info.current_layer`, `print_stats.info.total_layer`
- **Checklist:**
  - [ ] Current/total layers on print status
  - [ ] Layer progress bar

---

## TIER 3: MEDIUM Priority Features

Some competitors have these. Nice to have for completeness.

| Feature | Status | Notes |
|---------|--------|-------|
| **Limits Panel** | ⬜ | Velocity/acceleration limits |
| **LED Effects** | ⬜ | StealthBurner LED control |
| **Probe Calibration** | ⬜ | Beacon/Cartographer/Eddy |
| **Temperature Graphs** | ⬜ | Multi-sensor historical graphs |
| **Filament Sensors** | ⬜ | Runout/motion sensor status |
| **System Info** | ⬜ | CPU/memory/network stats |
| **Adaptive Mesh** | ⬜ | Native Klipper 0.12+ feature |

---

## TIER 4: DIFFERENTIATOR Features

NO competitor does these well. Opportunity to lead.

| Feature | Status | Notes |
|---------|--------|-------|
| **PID Tuning UI** | ⬜ | UNIQUE - touchscreen PID calibration |
| **Pressure Advance UI** | ⬜ | Live PA adjustment |
| **First-Layer Wizard** | ⬜ | Guided Z-offset + mesh flow |
| **Material Database** | ⬜ | Built-in material profiles |
| **Maintenance Tracker** | ⬜ | Nozzle/belt reminders |

---

## Infrastructure Improvements

| Item | Status | Notes |
|------|--------|-------|
| **Coming Soon Component** | ⬜ | Reusable overlay for stubs |
| **Nav Bar Updates** | ⬜ | Icons for new panels |
| **Settings Reorganization** | ⬜ | Group new settings |
| **Moonraker API Additions** | ⬜ | ~25 new endpoints |

---

## Implementation Log

### 2025-12-08
- Created FEATURE_PARITY_RESEARCH.md with comprehensive analysis
- Created FEATURE_STATUS.md (this file)
- Identified 47 feature gaps across 4 priority tiers

---

## Next Actions

### Immediate (Today)
1. [ ] Create "Coming Soon" component in globals.xml
2. [ ] Add nav icons for new panels
3. [ ] Create stub panels with Coming Soon overlays:
   - [ ] macro_panel.xml
   - [ ] console_panel.xml
   - [ ] camera_panel.xml
   - [ ] history_panel.xml
   - [ ] power_panel.xml
   - [ ] screws_tilt_panel.xml
   - [ ] input_shaper_panel.xml

### Quick Wins (Next Session)
1. [ ] Layer display in print_status_panel
2. [ ] Temperature presets (basic)
3. [ ] Power device control

### Core Features (Following Sessions)
1. [ ] Macro panel - list and execute
2. [ ] Console panel - read-only history
3. [ ] Camera panel - single MJPEG stream
4. [ ] History panel - list past jobs

---

## Dependencies Map

```
Nothing depends on these (can start immediately):
├── Temperature Presets
├── Layer Display
├── Power Device Control
├── Firmware Retraction
└── Limits Panel

These depend on "Coming Soon" component:
├── Macro Panel (stub)
├── Console Panel (stub)
├── Camera Panel (stub)
├── History Panel (stub)
├── Screws Tilt Panel (stub)
└── Input Shaper Panel (stub)

These depend on completed features:
├── Spoolman → needs Camera for QR scanner
├── First-Layer Wizard → needs working Z-offset
└── PID Tuning UI → needs console to show progress
```

---

## Files Created/Modified Tracking

### New Files (Planned)
```
ui_xml/
├── coming_soon_overlay.xml      [ ] Created  [ ] Tested
├── temp_preset_modal.xml        [ ] Created  [ ] Tested
├── macro_panel.xml              [ ] Created  [ ] Tested
├── macro_card.xml               [ ] Created  [ ] Tested
├── console_panel.xml            [ ] Created  [ ] Tested
├── screws_tilt_panel.xml        [ ] Created  [ ] Tested
├── screw_indicator.xml          [ ] Created  [ ] Tested
├── camera_panel.xml             [ ] Created  [ ] Tested
├── camera_pip.xml               [ ] Created  [ ] Tested
├── history_panel.xml            [ ] Created  [ ] Tested
├── history_item.xml             [ ] Created  [ ] Tested
├── power_panel.xml              [ ] Created  [ ] Tested
├── power_device_row.xml         [ ] Created  [ ] Tested
├── input_shaper_panel.xml       [ ] Created  [ ] Tested
├── retraction_panel.xml         [ ] Created  [ ] Tested
├── spoolman_panel.xml           [ ] Created  [ ] Tested
├── job_queue_panel.xml          [ ] Created  [ ] Tested
├── update_panel.xml             [ ] Created  [ ] Tested
└── timelapse_panel.xml          [ ] Created  [ ] Tested

include/
├── temperature_presets.h        [ ] Created  [ ] Tested
├── ui_panel_macros.h            [ ] Created  [ ] Tested
├── ui_panel_console.h           [ ] Created  [ ] Tested
├── ui_panel_screws_tilt.h       [ ] Created  [ ] Tested
├── ui_panel_camera.h            [ ] Created  [ ] Tested
├── webcam_client.h              [ ] Created  [ ] Tested
├── ui_panel_history.h           [ ] Created  [ ] Tested
├── ui_panel_power.h             [ ] Created  [ ] Tested
├── ui_panel_input_shaper.h      [ ] Created  [ ] Tested
├── ui_panel_retraction.h        [ ] Created  [ ] Tested
├── spoolman_client.h            [ ] Created  [ ] Tested
├── ui_panel_job_queue.h         [ ] Created  [ ] Tested
└── ui_panel_updates.h           [ ] Created  [ ] Tested

src/
├── temperature_presets.cpp      [ ] Created  [ ] Tested
├── ui_panel_macros.cpp          [ ] Created  [ ] Tested
├── ui_panel_console.cpp         [ ] Created  [ ] Tested
├── ui_panel_screws_tilt.cpp     [ ] Created  [ ] Tested
├── ui_panel_camera.cpp          [ ] Created  [ ] Tested
├── webcam_client.cpp            [ ] Created  [ ] Tested
├── ui_panel_history.cpp         [ ] Created  [ ] Tested
├── ui_panel_power.cpp           [ ] Created  [ ] Tested
├── ui_panel_input_shaper.cpp    [ ] Created  [ ] Tested
├── ui_panel_retraction.cpp      [ ] Created  [ ] Tested
├── spoolman_client.cpp          [ ] Created  [ ] Tested
├── ui_panel_job_queue.cpp       [ ] Created  [ ] Tested
└── ui_panel_updates.cpp         [ ] Created  [ ] Tested
```

### Modified Files (Planned)
```
ui_xml/
├── globals.xml                  [ ] Updated (Coming Soon component)
├── navigation_bar.xml           [ ] Updated (new icons)
├── nozzle_temp_panel.xml        [ ] Updated (presets)
├── bed_temp_panel.xml           [ ] Updated (presets)
├── print_status_panel.xml       [ ] Updated (layer display)
├── controls_panel.xml           [ ] Updated (new cards)
└── home_panel.xml               [ ] Updated (quick access)

include/
├── moonraker_api.h              [ ] Updated (~25 new methods)
└── moonraker_client.h           [ ] Updated (new subscriptions)

src/
├── main.cpp                     [ ] Updated (panel registration)
├── moonraker_api.cpp            [ ] Updated (~25 new methods)
└── ui_panel_print_status.cpp    [ ] Updated (layer display)

config/
└── helixconfig.json.template    [ ] Updated (presets, settings)
```

---

## Session Notes

Use this section to track progress across sessions.

### Session 1 (2025-12-08)
- **Goal:** Research and documentation
- **Completed:**
  - Created FEATURE_PARITY_RESEARCH.md
  - Created FEATURE_STATUS.md
  - Updated ROADMAP.md
- **Next:** Create Coming Soon component, panel stubs
