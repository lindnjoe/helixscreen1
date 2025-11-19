# G-Code Top Surface Rendering Fix

**Issue**: Top layer (Z=30.00mm) of rendered G-code test cube shows holes/transparency instead of solid diagonal infill like OrcaSlicer preview.

**Root Cause**: TBD (under investigation)

**Date Started**: 2025-11-19

---

## Stage 1: OrcaSlicer Baseline Analysis

**Goal**: Document expected rendering from OrcaSlicer reference screenshot

**Success Criteria**:
- ✅ Diagonal infill line count recorded
- ✅ Line spacing measured
- ✅ Pattern direction documented

**Tests**: Visual inspection of OrcaSlicer screenshot

**Status**: ✅ **COMPLETE**

**Results**:
- **Line count**: 21-23 diagonal infill lines visible on top surface
- **Pattern**: Unidirectional diagonal (~45° angle, lower-left to upper-right)
- **Spacing**: Approximately 0.8-1.0mm between parallel lines
- **Coverage**: Solid, continuous lines with no visible gaps
- **Surface quality**: Smooth top surface with embossed "Z" letter visible
- **Screenshot**: Provided by user 2025-11-19

**Notes**: This is the target rendering quality we need to achieve.

---

## Stage 2: Current Rendering Baseline Capture

**Goal**: Document our current rendering state and capture segment statistics from geometry builder

**Success Criteria**:
- [ ] Build completes successfully (BLOCKED: requires TinyGL fix first)
- [ ] Layer summary statistics captured for Z=30.00mm
- [ ] Total segment count recorded
- [ ] Connection statistics documented (connected % vs disconnected %)
- [ ] Gap statistics captured (min/avg/max)
- [ ] Screenshot taken of current rendering showing holes

**Tests**:
```bash
# After TinyGL is fixed:
make -j
./build/bin/helix-ui-proto -p gcode-test --test 2>&1 | tee /tmp/top_layer_debug.txt
grep -A 20 "LAYER Z=30.00mm SUMMARY" /tmp/top_layer_debug.txt

# Expected output format:
# ═══ LAYER Z=30.00mm SUMMARY ═══
#   Total segments: [N]
#   Connected: [N] ([%])
#   Disconnected: [N] ([%])
#   Gap stats: min=[X]mm avg=[X]mm max=[X]mm
```

**Status**: ⏳ **BLOCKED** - Waiting for user to fix TinyGL compilation errors

**Blockers**:
- TinyGL compilation errors in `ztriangle.c` (undeclared V3, normal identifiers)
- User is working on dithering implementation

**Expected Output**:
- Total segments for Z=30.00mm layer: ~20-30 expected
- Connected percentage: Should be high (>80%) with 1.0x width tolerance
- Disconnected segments: Should be minimal, mostly perimeter-to-infill transitions
- Gap statistics: Useful for diagnosing spacing issues

**Code Changes Already Applied**:
- `src/gcode_geometry_builder.cpp:401-414` - Added final layer summary output loop
- This ensures the last layer (Z=30.00mm) gets summarized even though there's no layer transition after it

---

## Stage 3: Quantitative Comparison

**Goal**: Compare OrcaSlicer segment count with our generated segment count and identify discrepancies

**Success Criteria**:
- [ ] OrcaSlicer count (21-23) vs our segment count comparison documented
- [ ] Discrepancy percentage calculated
- [ ] Hypothesis formed about root cause category

**Tests**:
```bash
# Manual comparison:
# OrcaSlicer: 21-23 diagonal lines
# Our output: [from Stage 2 Total segments value]
# Difference: Calculate and document
```

**Status**: ⏸️ **NOT STARTED** - Waiting for Stage 2 completion

**Analysis Questions**:
1. **If our count is similar (20-25 segments)**:
   - Segments exist but not rendering properly
   - → Check tube width (may be too narrow)
   - → Check end cap generation (may have gaps)
   - → Check for Z-fighting or depth issues

2. **If our count is higher (>30 segments)**:
   - May be duplicating segments
   - May be generating extra non-extrusion moves
   - → Review segment filtering logic
   - → Check for perimeter vs infill differentiation

3. **If our count is lower (<15 segments)**:
   - Missing segments due to over-simplification
   - → Review simplification tolerance
   - → Check if segments are being incorrectly filtered
   - → Verify all top layer segments are being processed

**Expected Outcome**: Clear hypothesis about whether this is a:
- Geometry generation issue (wrong segment count)
- Geometry sizing issue (segments too narrow)
- Rendering issue (segments exist but don't render)

---

## Stage 4: Visual Pattern Analysis

**Goal**: Identify specific patterns and locations of holes through visual comparison

**Success Criteria**:
- [ ] Side-by-side screenshot comparison completed
- [ ] Hole locations mapped (perimeter? center? random?)
- [ ] Pattern identified (systematic vs random)
- [ ] Correlation with disconnected segments verified

**Tests**:
```bash
# Capture our rendering:
./build/bin/helix-ui-proto -p gcode-test --test
# Press 'S' in the UI to save screenshot
# Or use automated script:
./scripts/screenshot.sh helix-ui-proto top_layer_current gcode-test

# Compare screenshots side-by-side
```

**Status**: ⏸️ **NOT STARTED** - Waiting for Stage 2 completion

**Analysis Checklist**:
- [ ] Are holes evenly distributed or clustered?
- [ ] Do holes appear between specific segments (e.g., where debug shows "connect=NO")?
- [ ] Are holes in the infill area, perimeter, or both?
- [ ] Is the hole size consistent with the extrusion width?
- [ ] Do the holes correspond to the gaps reported in statistics?

**Visual Comparison Points**:
1. **OrcaSlicer**: Solid continuous diagonal lines, no gaps
2. **Our rendering**: Document specific appearance:
   - Where are the holes? (perimeter, infill, transitions)
   - What shape are the holes? (circular, rectangular, irregular)
   - How large are the holes? (in mm or relative to line width)

**Expected Outcome**: Clear understanding of WHERE the problem manifests visually, which will guide the fix implementation.

---

## Stage 5: Root Cause Fix & Verification

**Goal**: Implement targeted fix based on diagnosis from Stages 3-4 and verify solid top surface rendering

**Success Criteria**:
- [ ] Fix implemented based on root cause
- [ ] Build succeeds with fix
- [ ] Top surface renders solid like OrcaSlicer (no holes/transparency)
- [ ] Segment count matches expected range (20-25)
- [ ] Visual comparison confirms match with OrcaSlicer quality

**Tests**:
```bash
make -j
./build/bin/helix-ui-proto -p gcode-test --test

# Verification checklist:
# 1. No visible holes in top surface
# 2. Diagonal pattern matches OrcaSlicer
# 3. Solid coverage across entire top face
# 4. "Z" embossing still visible
```

**Status**: ⏸️ **NOT STARTED** - Waiting for root cause diagnosis

**Potential Fixes** (based on diagnosis):

### Fix Option A: Tube Width Too Narrow
**If**: Segments exist but have visible gaps between them
**Change**: `src/gcode_geometry_builder.cpp`
```cpp
// Current: width from G-code or default
// Fix: Add width multiplier for better coverage
float effective_width = width * 1.1f; // 10% wider for overlap
```

### Fix Option B: Connection Tolerance Too Conservative
**If**: Too many disconnected segments causing end cap gaps
**Change**: `src/gcode_geometry_builder.cpp:318`
```cpp
// Current: 1.0x width tolerance
float connection_tolerance = width * 1.0f;
// Fix: Increase tolerance
float connection_tolerance = width * 1.2f; // More forgiving
```

### Fix Option C: End Cap Geometry Issues
**If**: End caps not covering tube ends properly
**Change**: Review `generate_ribbon_vertices()` end cap generation
- Verify cap vertices align with tube end
- Check cap normal direction (should face outward)
- Ensure cap uses same width as tube

### Fix Option D: Missing Segments
**If**: Segment count is too low
**Change**: Review simplification or filtering logic
- Check simplification tolerance (may be too aggressive)
- Verify all top layer segments are included
- Check for accidental filtering of valid segments

**Expected Outcome**: Top surface renders as a solid diagonal infill pattern matching OrcaSlicer quality, with no visible holes or transparency.

**Final Verification**:
- [ ] Side-by-side visual match with OrcaSlicer
- [ ] No holes visible at any zoom level
- [ ] Segment statistics look reasonable
- [ ] No performance regression
- [ ] Code is clean and documented

---

## Progress Log

### 2025-11-19
- **Stage 1 COMPLETE**: Analyzed OrcaSlicer screenshot, counted 21-23 diagonal infill lines
- **Code Added**: Final layer summary output in `gcode_geometry_builder.cpp:401-414`
  - Fixes issue where last layer wasn't getting summarized (no layer transition after it)
  - Outputs stats for all tracked top layers at end of geometry building
- **Status**: Blocked on TinyGL compilation errors (user fixing dithering work)

---

## Technical Context

### Current Implementation Details

**Geometry Builder** (`src/gcode_geometry_builder.cpp`):
- Tracks top 3 layers by Z-height (descending order)
- Uses width-based connection tolerance: `width * 1.0f`
- Generates end caps when segments don't connect
- Added final summary output for remaining top layers (lines 401-414)

**Debug Output** (first 50 segments per top layer):
```
Seg   0: [x0,y0,z0] → [x1,y1,z1] width=Wmm dist=Dmm tol=Tmm connect=YES/NO
```

**Triangle Generation**:
- 8 triangles per segment (4 side faces × 2 triangles each)
- +2 triangles for start cap (when segment doesn't connect to previous)
- +2 triangles for end cap (when gap detected to next segment)

### Files Involved
- `src/gcode_geometry_builder.cpp` - Main geometry generation (modified: added final summary)
- `src/gcode_tinygl_renderer.cpp` - TinyGL rendering (unchanged)
- `assets/OrcaCube AD5M.gcode` - Test G-code file (150 layers, Z=30.00mm max)

### Known Issues
- TinyGL has compilation errors (user working on dithering)
- Can't build until TinyGL is fixed

---

## Completion Criteria

This plan is complete when:
1. ✅ All 5 stages marked as COMPLETE
2. ✅ Top surface renders solid without holes
3. ✅ Visual quality matches OrcaSlicer reference
4. ✅ All tests pass
5. ✅ Code changes committed with clear commit message
6. ✅ This file is deleted (plan complete)
