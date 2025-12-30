# Overlay Migration to OverlayBase - Implementation Plan

## Background

We've implemented a lifecycle management system for overlays:
- **OverlayBase class**: Abstract base with `on_activate()`/`on_deactivate()` hooks
- **NavigationManager integration**: Dispatches lifecycle when overlays push/pop
- **NetworkSettingsOverlay**: Already migrated as proof-of-concept

This plan covers migrating remaining overlays to use OverlayBase.

---

## Pre-Work: Sync Branches

Before starting, ensure the worktree is up to date:

```bash
# 1. In main repo, check current state
git status
git log --oneline -5

# 2. If there's an existing worktree, remove it
git worktree list
git worktree remove --force /Users/pbrown/code/helixscreen-overlay-migration

# 3. Create fresh worktree from main
git worktree add -b feature/overlay-migration /Users/pbrown/code/helixscreen-overlay-migration main

# 4. Initialize worktree
./scripts/init-worktree.sh /Users/pbrown/code/helixscreen-overlay-migration
```

---

## Phase 1: PrintStatusPanel (High Priority)

### Why First
- Already has `on_activate()`/`on_deactivate()` that pause/resume G-code viewer
- Currently NOT wired to NavigationManager - lifecycle hooks never called
- High value: fixes CPU waste when overlay is hidden

### Files
- `include/ui_panel_print_status.h`
- `src/ui_panel_print_status.cpp`
- Push sites:
  - `src/application.cpp` (~line 756)
  - `src/print_start_navigation.cpp` (~line 57)
  - `src/ui_panel_print_select.cpp` (~line 1876)

### Changes
1. Change inheritance: `PanelBase` → `OverlayBase`
2. Add: `#include "overlay_base.h"`
3. Add: `const char* get_name() const override { return "Print Status"; }`
4. Convert: `setup(panel, parent)` → `create(parent)` returning `overlay_root_`
5. Keep: existing `on_activate()` / `on_deactivate()` implementations
6. Add: `cleanup()` override calling `OverlayBase::cleanup()`
7. Update all 3 push sites to call `register_overlay_instance()` before push

### Special Considerations
- Singleton via `get_global_print_status_panel()` - keep this pattern
- Nested overlays (temp panels, tune panel) - remain as-is
- 4 modal dialogs - unchanged
- Observer cleanup via ObserverGuard - unchanged

### Agent Prompt Template
```
Migrate PrintStatusPanel from PanelBase to OverlayBase in worktree at
/Users/pbrown/code/helixscreen-overlay-migration

See docs/OVERLAY_MIGRATION_PLAN.md for details.

Key changes:
1. Change base class to OverlayBase
2. Implement create() instead of setup()
3. Add get_name() override
4. Update 3 push sites with register_overlay_instance()
5. Keep existing on_activate()/on_deactivate()

Build with make -j and verify no errors.
```

### Code Review Checklist
- [ ] Inherits from OverlayBase correctly
- [ ] `create()` returns `overlay_root_`
- [ ] All 3 push sites register overlay instance
- [ ] Existing lifecycle methods preserved
- [ ] Build succeeds
- [ ] No runtime errors on open/close

---

## Phase 2: TimelapseSettingsOverlay

### Files
- `include/ui_timelapse_settings.h`
- `src/ui_timelapse_settings.cpp`

### Changes
Same pattern as PrintStatusPanel:
1. Change inheritance to OverlayBase
2. Implement `create()` from existing `setup()`
3. Add `get_name()` returning "Timelapse Settings"
4. Update push site(s) with `register_overlay_instance()`

### Agent Prompt Template
```
Migrate TimelapseSettingsOverlay from PanelBase to OverlayBase in worktree.
Follow same pattern as PrintStatusPanel migration.
See docs/OVERLAY_MIGRATION_PLAN.md for details.
```

---

## Phase 3: RetractionSettingsOverlay

### Files
- `include/ui_retraction_settings.h`
- `src/ui_retraction_settings.cpp`

### Changes
Same pattern as TimelapseSettingsOverlay.

---

## Migration Pattern Reference

### Before (PanelBase)
```cpp
class MyOverlay : public PanelBase {
public:
    void init_subjects() override;
    void setup(lv_obj_t* panel, lv_obj_t* parent) override;
    void on_activate() override;
    void on_deactivate() override;
private:
    lv_obj_t* panel_ = nullptr;
};
```

### After (OverlayBase)
```cpp
class MyOverlay : public OverlayBase {
public:
    void init_subjects() override;
    void register_callbacks() override;  // if needed
    lv_obj_t* create(lv_obj_t* parent) override;
    const char* get_name() const override { return "My Overlay"; }
    void on_activate() override;
    void on_deactivate() override;
    void cleanup() override;
    // overlay_root_ inherited from OverlayBase
};
```

### Push Site Pattern
```cpp
// Before
ui_nav_push_overlay(overlay_widget);

// After
NavigationManager::instance().register_overlay_instance(overlay_widget, &overlay_instance);
ui_nav_push_overlay(overlay_widget);
```

---

## Workflow Per Overlay

1. **Create worktree** (if not exists)
2. **Delegate to agent** with specific migration instructions
3. **Build verify**: `make -j` in worktree
4. **Code review with agent**: Use checklist above
5. **Manual test**: Open/close overlay, check logs with `-vv`
6. **Commit**: Conventional commit format
7. **Repeat** for next overlay
8. **Final merge** to main when all complete

---

## Commit Message Format

```
refactor(overlay): migrate <OverlayName> to OverlayBase

Change inheritance from PanelBase to OverlayBase for proper lifecycle
management. Existing on_activate()/on_deactivate() hooks are now called
automatically by NavigationManager when overlay is pushed/popped.
```

---

## Future Work (Phase 4 - Calibration Panels)

Not included in this session, but documented for later:

| Panel | Current State | Notes |
|-------|---------------|-------|
| ZOffsetCalibrationPanel | No base class | Has state machine |
| PIDCalibrationPanel | No base class | Has state machine |
| InputShaperPanel | No base class | Async measurements |
| ScrewsTiltPanel | No base class | Probing workflow |

These would benefit from `on_activate()` to reset state and `on_deactivate()` to cancel operations.

---

## Reference Files

- OverlayBase definition: `include/overlay_base.h`
- OverlayBase implementation: `src/overlay_base.cpp`
- Example migration: `include/network_settings_overlay.h`
- NavigationManager: `include/ui_nav_manager.h`
