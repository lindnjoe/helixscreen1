# Component State Theming Architecture

**Created**: 2026-01-24
**Status**: Planned
**Priority**: Medium
**Prerequisite**: Phase 5-6 dual-palette theme system (complete)

## Overview

This plan addresses architectural improvements to component state handling identified during the Phase 6 theme token audit. The goal is to ensure all interactive components properly use semantic theme tokens for all visual states (default, hover, pressed, disabled, focus).

## Current State

The dual-palette theme system (Phases 5-6) provides 16 semantic color tokens:
- Backgrounds: `app_bg`, `panel_bg`, `card_bg`, `card_alt`, `border`
- Text: `text`, `text_muted`, `text_subtle`
- Accents: `primary`, `secondary`, `tertiary`
- States: `info`, `success`, `warning`, `danger`, `focus`

Legacy tokens are aliased in `theme_manager.cpp` for backward compatibility.

## Problems Identified

### P1: No Focus Ring Support
- The `#focus` token exists but is never applied
- Touch-first UI, but keyboard/accessibility needs focus indicators
- No `LV_STATE_FOCUSED` styling in theme_core.c

### P2: Button Variant System Missing
- All buttons use same `surface_control` background
- Variants applied via inline XML styling (inconsistent)
- No primary/secondary/ghost/danger button styles

### P3: Slider Theming Incomplete
- Uses LVGL default styling, not theme tokens
- Track should use `#border`, indicator `#primary`, knob `#card_bg`
- No disabled state styling

### P4: Dropdown Theming Incomplete
- Missing selected item highlight styling
- No focus ring on dropdown button
- Disabled state only uses opacity

### P5: Setting Row Disabled States
- `setting_toggle_row`, `setting_dropdown_row`, `setting_action_row`
- When control is disabled, label text should use `#text_subtle`
- Currently labels stay `#text_primary` regardless of state

### P6: Switch Legacy Tokens
- Uses `surface_elevated`/`surface_dim` (aliased but not semantic)
- Unchecked track should use `#border`
- Knob should use `#card_bg`

## Solution Design

### Phase 7A: Focus Ring System

Add focus state styling to theme_core.c:

```c
// In style_init()
lv_style_init(&helix->focus_ring_style);
lv_color_t focus_color = theme_manager_get_color("focus");
lv_style_set_outline_color(&helix->focus_ring_style, focus_color);
lv_style_set_outline_width(&helix->focus_ring_style, 2);
lv_style_set_outline_opa(&helix->focus_ring_style, LV_OPA_COVER);
lv_style_set_outline_pad(&helix->focus_ring_style, 2);

// Apply to focusable widgets
lv_obj_add_style(obj, &helix->focus_ring_style, LV_STATE_FOCUSED);
```

Apply to: `lv_button`, `lv_switch`, `lv_slider`, `lv_dropdown`, `lv_textarea`

### Phase 7B: Button Variant System

Define button variants in globals.xml:

```xml
<!-- Button variant styles -->
<style name="btn_primary">
  <prop name="bg_color" value="#primary"/>
  <prop name="text_color" value="#text"/>
</style>
<style name="btn_secondary">
  <prop name="bg_color" value="#card_alt"/>
  <prop name="text_color" value="#text"/>
</style>
<style name="btn_ghost">
  <prop name="bg_opa" value="0"/>
  <prop name="text_color" value="#primary"/>
</style>
<style name="btn_danger">
  <prop name="bg_color" value="#danger"/>
  <prop name="text_color" value="#text"/>
</style>
```

Or implement in theme_core.c with runtime variant detection.

**Usage in XML:**
```xml
<lv_button variant="primary">...</lv_button>
<lv_button variant="ghost">...</lv_button>
```

### Phase 7C: Slider Theme Styling

Add slider styling to theme_core.c:

```c
// Track background (unfilled portion)
lv_style_init(&helix->slider_track_style);
lv_style_set_bg_color(&helix->slider_track_style, theme_manager_get_color("border"));
lv_style_set_bg_opa(&helix->slider_track_style, LV_OPA_COVER);
lv_style_set_radius(&helix->slider_track_style, LV_RADIUS_CIRCLE);

// Indicator (filled portion)
lv_style_init(&helix->slider_indicator_style);
lv_style_set_bg_color(&helix->slider_indicator_style, theme_manager_get_color("primary"));
lv_style_set_bg_opa(&helix->slider_indicator_style, LV_OPA_COVER);

// Knob
lv_style_init(&helix->slider_knob_style);
lv_style_set_bg_color(&helix->slider_knob_style, theme_manager_get_color("card_bg"));
lv_style_set_shadow_width(&helix->slider_knob_style, 4);
lv_style_set_shadow_color(&helix->slider_knob_style, theme_manager_get_color("app_bg"));

// Disabled states
lv_style_init(&helix->slider_disabled_style);
lv_style_set_bg_color(&helix->slider_disabled_style, theme_manager_get_color("card_alt"));
lv_style_set_bg_opa(&helix->slider_disabled_style, LV_OPA_50);
```

### Phase 7D: Dropdown Theme Styling

Add dropdown styling to theme_core.c:

```c
// Selected item in dropdown list
lv_style_init(&helix->dropdown_selected_style);
lv_style_set_bg_color(&helix->dropdown_selected_style, theme_manager_get_color("card_alt"));
lv_style_set_bg_opa(&helix->dropdown_selected_style, LV_OPA_COVER);

// Apply to LV_PART_SELECTED
lv_obj_add_style(list, &helix->dropdown_selected_style, LV_PART_SELECTED);
```

### Phase 7E: Setting Row Disabled States

Update setting row XML components to bind label colors to control state:

```xml
<!-- setting_toggle_row.xml -->
<text_body name="label" text="$label">
  <bind_style_if_state subject="$subject" state="disabled"
                       style_text_color="#text_subtle"/>
</text_body>
```

Or use a wrapper subject that tracks the control's disabled state.

### Phase 7F: Switch Token Migration

Update ui_switch.cpp to use semantic tokens:

| Current | New |
|---------|-----|
| `surface_elevated` | `card_bg` |
| `surface_dim` | `card_alt` |
| `text_light` | `text_subtle` |

The aliases handle this now, but direct usage is cleaner.

## Implementation Order

```
Phase 7A: Focus Ring System
    ├── Modify theme_core.c
    ├── Add focus_ring_style
    └── Apply to all focusable widgets

Phase 7B: Button Variants
    ├── Design variant attribute handling
    ├── Add variant styles to theme_core.c
    └── Update documentation

Phase 7C: Slider Theming
    ├── Add slider styles to theme_core.c
    └── Test all slider instances

Phase 7D: Dropdown Theming
    ├── Add dropdown styles to theme_core.c
    └── Test dropdown list appearance

Phase 7E: Setting Row States
    ├── Update setting_toggle_row.xml
    ├── Update setting_dropdown_row.xml
    └── Update setting_action_row.xml

Phase 7F: Switch Token Migration
    ├── Update ui_switch.cpp
    └── Remove legacy token usage
```

## Token Usage Matrix (Target State)

| Component | Part | Default | Hover | Pressed | Disabled | Focus |
|-----------|------|---------|-------|---------|----------|-------|
| **Button (primary)** | bg | `primary` | darken 10% | darken 20% | `card_alt` | `focus` ring |
| **Button (secondary)** | bg | `card_alt` | darken 5% | darken 10% | `card_alt` 50% | `focus` ring |
| **Button (ghost)** | bg | transparent | `card_alt` 30% | `card_alt` 50% | - | `focus` ring |
| **Switch (on)** | track | `primary` | - | - | `card_alt` | `focus` ring |
| **Switch (off)** | track | `border` | - | - | `card_alt` | `focus` ring |
| **Switch** | knob | `card_bg` | - | - | `text_subtle` | - |
| **Slider** | track | `border` | - | - | `card_alt` | - |
| **Slider** | indicator | `primary` | - | - | `text_subtle` | - |
| **Slider** | knob | `card_bg` | - | scale 1.1x | `card_alt` | `focus` ring |
| **Dropdown** | button | `card_alt` | darken 5% | darken 10% | 50% opa | `focus` ring |
| **Dropdown** | list | `card_bg` | - | - | - | - |
| **Dropdown** | selected | `card_alt` | - | - | - | - |

## Testing Strategy

1. **Visual Regression**: Screenshot each component in all states
2. **Theme Switching**: Verify appearance in Ayu, Nord, Dracula themes
3. **Dark/Light Mode**: Test dual-palette themes in both modes
4. **Keyboard Navigation**: Verify focus rings appear correctly
5. **Accessibility**: Ensure contrast ratios meet WCAG guidelines

## Files to Modify

| File | Changes |
|------|---------|
| `src/theme_core.c` | Add focus, slider, dropdown, button variant styles |
| `src/ui/ui_switch.cpp` | Migrate to semantic tokens |
| `ui_xml/setting_toggle_row.xml` | Add disabled state binding |
| `ui_xml/setting_dropdown_row.xml` | Add disabled state binding |
| `ui_xml/setting_action_row.xml` | Add disabled state binding |
| `ui_xml/globals.xml` | (Optional) Add button variant styles |

## Success Criteria

- [ ] Focus rings visible on all focusable widgets when focused
- [ ] Button variants (primary, secondary, ghost, danger) working
- [ ] Sliders use theme tokens for all parts
- [ ] Dropdowns have proper selected/disabled styling
- [ ] Setting row labels dim when control is disabled
- [ ] All tests pass
- [ ] No hardcoded colors in modified files
- [ ] Visual consistency across all themes

## Estimated Effort

| Phase | Effort | Dependencies |
|-------|--------|--------------|
| 7A Focus Ring | 2h | None |
| 7B Button Variants | 4h | None |
| 7C Slider Theming | 2h | None |
| 7D Dropdown Theming | 2h | None |
| 7E Setting Row States | 2h | None |
| 7F Switch Migration | 1h | None |
| **Total** | **~13h** | Can be parallelized |

## References

- Phase 5 commit: Theme system refactoring
- Phase 6 commit: `401dac5c` - Token audit
- Component State Audit: Agent report from 2026-01-24
- LVGL 9.4 styling documentation
