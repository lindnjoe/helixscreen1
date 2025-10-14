# FontAwesome → Material Design Icon Mappings

**Created:** 2025-10-14
**Purpose:** Reference document for completing FontAwesome to Material Design icon migration

## Icon Mapping Table

### ✅ Verified Mappings (Already Implemented)

| FontAwesome Icon | Material Design Icon | Usage | Status |
|-----------------|---------------------|--------|--------|
| `#icon_home` | `mat_home` | Navigation, motion panel home button | ✅ Complete |
| `#icon_print` | `mat_print` | Navigation | ✅ Complete |
| `#icon_controls` | `mat_move` | Navigation, controls card | ✅ Complete |
| `#icon_filament` | `mat_extrude` | Navigation, extrusion card | ✅ Complete |
| `#icon_settings` | `mat_fan` | Fan control card | ✅ Complete |
| `#icon_advanced` | `mat_motor_off` | Motors disable card | ✅ Complete |
| `#icon_fire` | `mat_heater` (nozzle) / `mat_bed` (bed) | Temperature panels, control cards | ✅ Complete |
| `#icon_temperature` | `mat_heater` | Home panel temperature display | ✅ Complete |
| `#icon_wifi` | `mat_network` | Home panel network status | ✅ Complete |
| `#icon_lightbulb` | `mat_light` | Home panel light control | ✅ Complete |
| `#icon_chevron_up` | `mat_arrow_up` | Motion panel Z-axis up | ✅ Complete |
| `#icon_chevron_down` | `mat_arrow_down` | Motion panel Z-axis down | ✅ Complete |
| `#icon_chevron_left` | `mat_back` | Header bar back button | ✅ Complete |
| `#icon_arrow_up_line` | `mat_extrude` | Extrusion panel icon | ✅ Complete |

### 🔄 Pending Mappings (Need Verification & Implementation)

| FontAwesome Icon | Recommended Material Icon | Rationale | Files Affected |
|-----------------|--------------------------|-----------|----------------|
| `#icon_backspace` | `mat_delete` | Delete/backspace semantic match | `numeric_keypad_modal.xml` |
| `#icon_clock` | `mat_clock` | Exact semantic match (print time) | 5× print file cards, `print_file_detail.xml`, `test_card.xml` |
| `#icon_leaf` | `mat_filament` | Material/filament indicator (eco → material) | 5× print file cards, `print_file_detail.xml`, `test_card.xml` |
| `#icon_list` | `mat_layers` **OR** `mat_sd` | List view indicator (layers = list structure, sd = file list) | `print_select_panel.xml` |
| `#icon_chevron_left` | `mat_back` | Back navigation (already verified) | `numeric_keypad_modal.xml`, `print_status_panel.xml` |

### ⚠️ Icons to Keep (Not Replacing)

| Icon | Font | Reason |
|------|------|--------|
| Arrow directions (8-way) | `arrows_32` | Custom diagonal arrows font, no Material equivalent |

## Material Design Icon Availability

**Total Icons:** 56
**Source:** `/Users/pbrown/Code/Printing/guppyscreen/assets/material_svg/`
**Format:** 64x64 SVG → RGB565A8 LVGL 9 C arrays

### Available Icons (Full List)

```
Navigation & Movement:
- mat_home, mat_back, mat_move
- mat_arrow_up, mat_arrow_down, mat_arrow_left, mat_arrow_right
- mat_z_closer, mat_z_farther, mat_home_z

Printing:
- mat_print, mat_pause, mat_resume, mat_cancel
- mat_sd, mat_refresh, mat_layers, mat_chart

Temperature & Heating:
- mat_heater, mat_bed, mat_cooldown

Extrusion & Filament:
- mat_extruder, mat_extrude, mat_extrude_img
- mat_filament, mat_load_filament, mat_unload_filament, mat_retract

Fans & Cooling:
- mat_fan, mat_fan_on, mat_fan_off

Controls & Settings:
- mat_light, mat_light_off
- mat_network
- mat_fine_tune, mat_flow_down, mat_flow_up
- mat_speed_down, mat_speed_up
- mat_pa_minus, mat_pa_plus

Advanced:
- mat_bedmesh, mat_belts_calibration, mat_inputshaper
- mat_limit, mat_info, mat_sysinfo
- mat_power_devices, mat_motor, mat_motor_off
- mat_update, mat_emergency, mat_delete

Timing & Progress:
- mat_clock, mat_hourglass

Other:
- mat_spoolman
```

## Replacement Pattern

### Before (FontAwesome Font):
```xml
<lv_label text="#icon_fire"
          style_text_font="fa_icons_64"
          style_text_color="#primary_color"
          align="center"/>
```

### After (Material Design Image):
```xml
<lv_image src="mat_heater"
          align="center"
          style_img_recolor="#primary_color"
          style_img_recolor_opa="100%"/>
```

### Scaling Icons (for smaller sizes):
```xml
<!-- 50% scale (32px from 64px source) -->
<lv_image src="mat_back"
          align="center"
          zoom="128"
          style_img_recolor="#text_primary"
          style_img_recolor_opa="100%"/>
```

## Files Remaining to Update

### High Priority (User-Facing UI):
1. ✅ `navigation_bar.xml` - COMPLETE
2. ✅ `home_panel.xml` - COMPLETE
3. ✅ `controls_panel.xml` - COMPLETE
4. ✅ `nozzle_temp_panel.xml` - COMPLETE
5. ✅ `bed_temp_panel.xml` - COMPLETE
6. ✅ `motion_panel.xml` - COMPLETE
7. ✅ `extrusion_panel.xml` - COMPLETE
8. ✅ `header_bar.xml` - COMPLETE
9. ⏳ `numeric_keypad_modal.xml` - PENDING (backspace, back chevron)
10. ⏳ `print_select_panel.xml` - PENDING (list icon)
11. ⏳ `print_status_panel.xml` - PENDING (back chevron)

### Medium Priority (Print File Display):
12. ⏳ `print_file_card.xml` - PENDING (clock, leaf)
13. ⏳ `print_file_card_3col.xml` - PENDING (clock, leaf)
14. ⏳ `print_file_card_4col.xml` - PENDING (clock, leaf)
15. ⏳ `print_file_card_5col.xml` - PENDING (clock, leaf)
16. ⏳ `print_file_detail.xml` - PENDING (clock, leaf)

### Low Priority (Testing):
17. ⏳ `test_card.xml` - PENDING (clock, leaf)

## Decision Points for Next Session

### 1. List Icon Choice
**Options:**
- `mat_layers` - Shows stacked layers (visual list metaphor)
- `mat_sd` - SD card/file context (semantically closer to file list)

**Recommendation:** `mat_sd` (better semantic match for "file list view")

### 2. Leaf Icon Replacement
**Original Meaning:** Eco-friendly / Material type indicator
**Recommendation:** `mat_filament` (represents material/filament type)

### 3. Backspace Icon
**Recommendation:** `mat_delete` (standard delete/backspace action)

## Next Steps

1. ✅ Document icon mappings (this file)
2. ⏳ Verify mapping decisions with user
3. ⏳ Replace remaining 10 XML files with verified mappings
4. ⏳ Remove FontAwesome fonts and registration code
5. ⏳ Update build system (Makefile, package.json)
6. ⏳ Test all panels across screen sizes
7. ⏳ Update final documentation

## References

- **Conversion Script:** `scripts/convert-material-icons-lvgl9.sh`
- **Icon Registration:** `src/material_icons.cpp`
- **Icon Declarations:** `include/material_icons.h`
- **Quick Reference:** `docs/QUICK_REFERENCE.md` (Icon & Image Assets section)
- **Handoff Doc:** `docs/HANDOFF.md` (Material Design Icons section)
