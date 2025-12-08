# HelixScreen Development Roadmap

**Last Updated:** 2025-12-10

---

## Project Status: Beta → Feature Parity Push

HelixScreen is a production-quality Klipper touchscreen UI. Core functionality is complete; **now pushing for feature parity with KlipperScreen, Mainsail, and Fluidd**.

| Area | Status | Details |
|------|--------|---------|
| **UI Panels** | ✅ Complete | 14 production panels + 6 overlays |
| **Settings** | ✅ Complete | 18 settings across 8 categories |
| **First-Run Wizard** | ✅ Complete | 7-step guided setup |
| **Moonraker API** | 🔄 Expanding | 30+ methods, adding ~25 more |
| **Build System** | ✅ Complete | macOS, Linux, Pi, AD5M |
| **Test Suite** | ✅ Complete | 51 unit tests |
| **Feature Parity** | 🔄 In Progress | 47 gaps identified, prioritized |

---

## Current Priorities

### 1. AMS/Multi-Material Support
**Status:** In Progress (branch: `feature/ams`)

Support for Happy Hare and AFC-Klipper multi-filament systems with Bambu-inspired UI:
- [x] Phase 0: Foundation - Detection, state management, mock backend
- [x] Phase 1: Core UI - AMS panel with slot grid visualization
- [x] Phase 2: Basic Operations - Load/unload/select with real backends
- [x] Phase 2.5: Spool Visualization - Pseudo-3D spool canvas with gradients
- [ ] Phase 2.6: Configurable visualization (in progress)
- [ ] Phase 3: Spoolman integration for material/color info
- [ ] Phase 4: Rich feedback - Filament path animations
- [ ] Phase 5: Print integration - Color requirements display
- [ ] Phase 6: Error recovery wizard

See `docs/AMS_IMPLEMENTATION_PLAN.md` for detailed specification.
Branch: `feature/ams-support` in `helixscreen-ams-feature` worktree.

### 3. Production Hardening
**Status:** In Progress

- [x] **Connection-aware navigation** - Disable Controls/Filament when disconnected, auto-navigate to home
- [x] **Reconnection flow UX** - Toast notifications for disconnect/reconnect/klippy states
- [ ] **Structured logging** - Log levels, rotation, remote debugging
- [ ] **Edge case testing** - Print failures, filesystem errors

---

## Feature Parity Quick Reference

### Moonraker API Additions Needed (~25 endpoints)
```
Job Queue:     /server/job_queue/*
Print History: /server/history/*
Webcams:       /server/webcams/*
Power Devices: /machine/device_power/*
Updates:       /machine/update/*
Spoolman:      /server/spoolman/*
GCode Store:   /server/gcode_store
```

### New Panels to Create
```
macro_panel.xml         - Execute Klipper macros
console_panel.xml       - G-code console with keyboard
camera_panel.xml        - Webcam viewer (MJPEG)
history_panel.xml       - Print job history
power_panel.xml         - Power device control
screws_tilt_panel.xml   - Visual bed leveling
input_shaper_panel.xml  - Resonance calibration
```

### Strategy: Breadth First
1. Create ALL panel stubs with "Coming Soon" overlays
2. Implement quick wins (layer display, presets, power)
3. Build out core features incrementally
4. Each feature clearly marked as complete/in-progress/stub

---

## Documentation

| Document | Purpose |
|----------|---------|
| `docs/FEATURE_PARITY_RESEARCH.md` | Complete competitive analysis, API reference |
| `docs/FEATURE_STATUS.md` | Live implementation tracking |
| `docs/AMS_IMPLEMENTATION_PLAN.md` | Multi-material support spec |

---

## Backlog (Lower Priority)

| Feature | Priority | Notes |
|---------|----------|-------|
| **mDNS discovery** | Low | Auto-find Moonraker (manual IP works) |
| **OTA updates** | Future | Currently requires manual binary update |
| **User manual** | Future | End-user documentation |

---

## Completed Features

### Core Architecture
- [x] LVGL 9.4 with declarative XML layouts
- [x] Reactive Subject-Observer data binding
- [x] Class-based panel architecture (PanelBase, ObserverGuard)
- [x] Theme system with dark/light modes
- [x] Responsive breakpoints (small/medium/large displays)
- [x] RAII lifecycle management throughout

### Navigation & Panels
- [x] **Home Panel** - Printer status, live temps, LED control, disconnect overlay
- [x] **Controls Panel** - 6-card launcher (Motion, Temps, Extrusion, Fan, Mesh, PID)
- [x] **Motion Panel** - Jog pad, Z-axis, distance selector, homing
- [x] **Temperature Panels** - Nozzle/bed presets, temp graphs, custom entry
- [x] **Extrusion Panel** - Extrude/retract, amount selector, safety checks
- [x] **Filament Panel** - Load/unload, filament profiles
- [x] **Print Select** - Card/list views, sorting, USB source tabs
- [x] **Print Status** - Progress, time remaining, pause/resume/cancel, exclude object
- [x] **Settings Panel** - 18 settings (theme, display, sound, network, safety, calibration)
- [x] **Advanced Panel** - Bed mesh visualization

### Settings Features (18 total)
- [x] Dark/Light theme toggle with restart dialog
- [x] Display brightness control (hardware sync)
- [x] Display sleep timeout
- [x] Scroll momentum and sensitivity
- [x] LED light toggle (capability-aware)
- [x] Sound toggle with M300 test beep
- [x] Print completion notifications (Off/Notification/Alert)
- [x] E-Stop confirmation toggle
- [x] Bed mesh 3D visualization
- [x] Z-offset calibration
- [x] PID tuning
- [x] WiFi settings overlay
- [x] Network settings
- [x] Factory reset

### First-Run Wizard (7 steps)
- [x] WiFi setup with scanning and hidden network support
- [x] Moonraker connection with validation
- [x] Printer identification with auto-detection (50+ printer database)
- [x] Heater selection (bed/hotend)
- [x] Fan selection (hotend/part cooling)
- [x] LED selection (optional)
- [x] Summary and confirmation

### Moonraker Integration
- [x] WebSocket client with auto-reconnection
- [x] JSON-RPC protocol with timeout management
- [x] File operations (list, metadata, delete, upload, start print)
- [x] Print control (pause, resume, cancel)
- [x] Motion control (homing, jog, positioning)
- [x] Heater/fan/LED control
- [x] System commands (E-stop, restart)
- [x] Exclude object with undo window
- [x] **Print History** - Dashboard with stats, time filtering, history list with search/filter/sort, detail overlay with Reprint/Delete

### G-code Features
- [x] Pre-print operation toggles (bed level, QGL, Z-tilt, nozzle clean)
- [x] G-code file modification (comment out embedded operations)
- [x] Command sequencer for pre-print ops
- [x] Printer capabilities detection
- [x] Memory-safe streaming for large files

### Build System
- [x] Makefile with parallel builds (`make -j`)
- [x] macOS native build (SDL2)
- [x] Linux native build (SDL2)
- [x] Raspberry Pi cross-compile (Docker, aarch64)
- [x] Adventurer 5M cross-compile (Docker, armv7-a, static linking)
- [x] CI/CD with GitHub Actions
- [x] Icon font generation with validation
- [x] Pre-commit hooks (clang-format, quality checks)

### Testing
- [x] Catch2 test framework
- [x] 51 unit tests covering core functionality
- [x] Mock Moonraker client for offline testing
- [x] Test fixtures for printer configurations

---

## Recent Work

### December 2025
| Feature | Commit |
|---------|--------|
| **Print History Feature Complete** | `2025-12-10` |
| Print History - Thumbnail caching + UI polish | `46889b1` |
| Print History - Dashboard with charts and reactive bindings | `9f0154b` |
| Print History - Detail overlay with Reprint/Delete | `2d1de9f` |
| Print History - Search, filter, sort for list | `0ba7937` |
| Print History - Dashboard and list panels | `8aef45e` |
| Print History - Moonraker API integration | `258c30a` |
| Reconnection flow UX (toast notifications) | `9844ead` |
| Connection-aware navigation gating | `a7eb28f` |
| Print completion notifications (Off/Notification/Alert) | `80a3199` |
| WiFi settings overlay with reactive architecture | `9037d81` |
| AD5M static build infrastructure (glibc 2.25) | `cdffc63` |
| Display sleep and hardware brightness sync | `74cb36f` |
| Sound toggle with speaker detection and M300 beep | `ccffb61` |
| Hardware backlight control with timeout highlighting | `62d7c99` |
| Animated heating progress indicator | `9d4b058` |
| Temperature icon clicks open temp panels | `7ee6c72` |
| Background temperature data collection | `065ddd5` |
| Motion panel responsive layout with kinematics | `0c96790` |
| Extrusion panel overhaul with new features | `a47f27c` |
| Icon font validation system | `49cc359` |
| MDI icon font unification | `7fc39c5` |

### November 2025 Highlights
| Feature | Date |
|---------|------|
| First-Run Wizard (all 7 steps) | 2025-11-30 |
| Fan Control sub-screen | 2025-11-30 |
| Exclude Object with undo | 2025-11-29 |
| ObserverGuard RAII pattern | 2025-11-29 |
| G-code memory-safe streaming | 2025-11-29 |
| Reactive UI refactoring | 2025-11-29 |
| Toast redesign (floating, top-right) | 2025-11-27 |
| Printer database v2.0 (50+ printers) | 2025-11-22 |
| Print Status with live Moonraker data | 2025-11-18 |
| Class-based panel architecture | 2025-11-17 |

---

## Architecture Principles

- **Reactive Pattern:** All UI state via Subject-Observer bindings
- **XML First:** Layout in XML, logic in C++
- **RAII Lifecycle:** PanelBase, ObserverGuard, ModalBase for safe cleanup
- **Design Tokens:** Colors, spacing, typography from globals.xml
- **Capability Detection:** Features shown/hidden based on printer capabilities
- **Mock Testing:** `--test` flag enables offline development

---

## Phase History

<details>
<summary>Expand to see original 15-phase development history</summary>

| Phase | Name | Status |
|-------|------|--------|
| 1 | Foundation (LVGL 9.3, XML, navigation) | ✅ Complete |
| 2 | Navigation & Blank Panels | ✅ Complete |
| 3 | Print Select Core | ✅ Complete |
| 4 | Print Select Polish | ✅ Complete |
| 5 | Controls Panel (6 sub-screens) | ✅ Complete |
| 6 | Additional Panel Content | ✅ Complete |
| 7 | Panel Transitions & Polish | ✅ Complete |
| 8 | Backend Integration (Moonraker) | ✅ Complete |
| 9 | Theming & Accessibility | ✅ Complete |
| 10 | Testing & Optimization | ✅ Complete |
| 11 | First-Run Wizard | ✅ Complete |
| 12 | Production Readiness | 🔄 In Progress |
| 13 | G-code Pre-Print Modification | ✅ Complete |
| 14 | Class-Based Panel Architecture | ✅ Complete |
| 15 | Reactive UI Architecture | ✅ Complete |

See `docs/archive/` for detailed phase documentation.

</details>

---

## Target Platforms

| Platform | Architecture | Status | Notes |
|----------|--------------|--------|-------|
| macOS | x86_64/ARM64 | ✅ Ready | Development with SDL2 |
| Linux | x86_64 | ✅ Ready | CI/CD tested |
| Raspberry Pi 4/5 | aarch64 | ✅ Ready | Docker cross-compile |
| BTT Pad | aarch64 | ✅ Ready | Same as Pi |
| Adventurer 5M | armv7-a | ✅ Ready | Static linking |

---

## Contributing

See `docs/CONTRIBUTING.md` for code standards and workflow.

Key files:
- `CLAUDE.md` - Project instructions and patterns
- `docs/LVGL9_XML_GUIDE.md` - XML layout reference
- `docs/ARCHITECTURE.md` - System design
- `docs/DEVELOPMENT.md` - Build and workflow
