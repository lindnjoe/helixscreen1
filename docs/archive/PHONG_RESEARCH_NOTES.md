# Phong Shading Research Notes

**Date**: 2025-11-19
**Goal**: Understand current Gouraud implementation to enable Phong (per-pixel) shading

---

## Current Gouraud Shading Pipeline

### 1. Vertex Lighting (`light.c:gl_shade_vertex()`, lines 228-388)

**When**: Called during vertex transformation (BEFORE rasterization)

**Input**: `GLVertex* v` with:
- `v->normal` - 3D surface normal at vertex
- `v->ec` - eye coordinates (for specular calculations)

**Process**:
- Loops over all enabled lights (`c->first_light`)
- For each light, calculates:
  - **Ambient**: `l->ambient * m->ambient`
  - **Diffuse**: `dot(normal, light_dir) * l->diffuse * m->diffuse`
  - **Specular** (if enabled): `pow(dot(normal, halfway), shininess) * l->specular * m->specular`
- Accumulates final RGB color

**Output**: Stores final color in `v->color.v[0-3]` (RGBA)

**Key Point**: Lighting is calculated **once per vertex**, not per pixel.

---

### 2. Triangle Setup (`ztriangle.h`, lines 104-123)

**When**: Triangle rasterization setup

**Process** (when `INTERP_RGB` is defined):
```c
// Calculate RGB gradients across triangle
d1 = p1->r - p0->r;
d2 = p2->r - p0->r;
drdx = (GLint)(fdy2 * d1 - fdy1 * d2);  // Red gradient in X
drdy = (GLint)(fdx1 * d2 - fdx2 * d1);  // Red gradient in Y
// Similar for green (g) and blue (b)
```

**Output**: Gradients used to interpolate colors across triangle:
- `drdx, drdy` - red gradients
- `dgdx, dgdy` - green gradients
- `dbdx, dbdy` - blue gradients

---

### 3. Scan-line Rasterization (`ztriangle.h`, lines 242-251, 284-403)

**Process**:
- For each triangle edge (left/right):
  - Initialize RGB at edge: `r1 = l1->r`, `g1 = l1->g`, `b1 = l1->b`
  - Calculate per-scanline deltas: `drdl_min`, `dgdl_min`, `dbdl_min`
- For each horizontal scan-line:
  - Initialize RGB at line start: `or1 = r1`, `og1 = g1`, `ob1 = b1`
  - For each pixel across the line, call `PUT_PIXEL(n)`
  - Update RGB per pixel: `or1 += drdx`, `og1 += dgdx`, `ob1 += dbdx`

---

### 4. Pixel Writing (`ztriangle.c:ZB_fillTriangleSmooth()`, lines 154-170)

**PUT_PIXEL macro**:
```c
#define PUT_PIXEL(_a) \
{ \
    register GLuint zz = z >> ZB_POINT_Z_FRAC_BITS; \
    if (ZCMPSIMP(zz, pz[_a], _a, 0)) { \
        TGL_BLEND_FUNC_RGB(or1, og1, ob1, (pp[_a])); \  // Convert RGB to pixel
        if (zbdw) pz[_a] = zz; \                         // Update depth buffer
    } \
    z += dzdx; \       // Update depth
    og1 += dgdx; \     // Update green
    or1 += drdx; \     // Update red
    ob1 += dbdx; \     // Update blue
}
```

**Key**: Uses interpolated RGB values (`or1, og1, ob1`) directly, no lighting calculation.

---

## Data Flow Summary

```
Vertex Transform
    ↓
gl_shade_vertex() → Calculates lighting → Stores RGB in vertex
    ↓
Triangle Setup → Calculates RGB gradients (drdx, dgdx, dbdx)
    ↓
Scan-line Loop → Interpolates RGB across triangle
    ↓
PUT_PIXEL → Converts RGB to pixel color → Writes to framebuffer
```

**Critical Insight**: Lighting happens **once per vertex** in `gl_shade_vertex()`, then colors are linearly interpolated. This creates visible "lighting bands" on low-poly curved surfaces.

---

## What Needs to Change for Phong Shading

### Goal
Calculate lighting **per pixel** instead of per vertex, by interpolating normals instead of colors.

### Required Changes

#### 1. Data Structure Changes
**File**: `tinygl/include/zbuffer.h` (likely)

Add normal storage to `ZBufferPoint`:
```c
typedef struct {
    GLint x, y, z;      // Existing
    GLint r, g, b;      // Existing (will keep for Gouraud compatibility)
    // NEW: Add normals for Phong
    GLfloat nx, ny, nz; // Interpolated normal
} ZBufferPoint;
```

#### 2. Triangle Setup Changes
**File**: `tinygl/src/ztriangle.h`

Add normal gradient calculation (similar to RGB gradients):
```c
#ifdef INTERP_NORMAL
{
    d1 = p1->nx - p0->nx;
    d2 = p2->nx - p0->nx;
    dnxdx = (GLfloat)(fdy2 * d1 - fdy1 * d2);
    dnxdy = (GLfloat)(fdx1 * d2 - fdx2 * d1);
}
// Similar for ny, nz
#endif
```

Add scan-line edge normal interpolation:
```c
#ifdef INTERP_NORMAL
    nx1 = l1->nx;
    dnxdl_min = (dnxdy + dnxdx * dxdy_min);
    dnxdl_max = dnxdl_min + dnxdx;
    // Similar for ny, nz
#endif
```

#### 3. Pixel Shader Changes
**File**: `tinygl/src/ztriangle.c` (new function: `ZB_fillTrianglePhong`)

Create new `PUT_PIXEL` macro that:
1. Interpolates normal (not color)
2. Normalizes interpolated normal
3. Calls lighting function per-pixel
4. Converts lighting result to pixel color

```c
#define PUT_PIXEL(_a) \
{ \
    register GLuint zz = z >> ZB_POINT_Z_FRAC_BITS; \
    if (ZCMPSIMP(zz, pz[_a], _a, 0)) { \
        // Normalize interpolated normal
        V3 normal = {onx1, ony1, onz1}; \
        gl_V3_Norm_Fast(&normal); \
        \
        // Calculate lighting per-pixel (NEW!)
        GLfloat R, G, B; \
        gl_calculate_pixel_lighting(&R, &G, &B, &normal, pixel_pos, context); \
        \
        // Convert to pixel and write
        TGL_BLEND_FUNC_RGB((GLint)R, (GLint)G, (GLint)B, (pp[_a])); \
        if (zbdw) pz[_a] = zz; \
    } \
    z += dzdx; \
    onx1 += dnxdx;  // Update normal X
    ony1 += dnydx;  // Update normal Y
    onz1 += dnzdx;  // Update normal Z
}
```

#### 4. Lighting Function Refactor
**File**: `tinygl/src/light.c`

Extract lighting calculation from `gl_shade_vertex()` into reusable function:
```c
// NEW: Per-pixel lighting (extracted from gl_shade_vertex)
void gl_calculate_pixel_lighting(
    GLfloat* R, GLfloat* G, GLfloat* B,  // Output
    V3* normal,                           // Interpolated normal
    V3* position,                         // World/eye position
    GLContext* c                          // Context with lights/materials
) {
    // Same logic as gl_shade_vertex(), but works on arbitrary position/normal
    // Loop over lights, calculate ambient/diffuse/specular
}
```

Keep `gl_shade_vertex()` for Gouraud mode (backward compatibility).

#### 5. Runtime Toggle
**File**: `tinygl/include/GL/gl.h`

Add new API:
```c
void glPhongShading(GLboolean enable);
```

**File**: `tinygl/src/api.c` (or similar)

```c
void glPhongShading(GLboolean enable) {
    GLContext* c = gl_get_context();
    c->use_phong_shading = enable;
}
```

Update triangle rasterizer selection to choose:
- `ZB_fillTriangleSmooth()` if Gouraud (existing)
- `ZB_fillTrianglePhong()` if Phong (new)

---

## Files to Modify

| File | Changes | Complexity |
|------|---------|------------|
| `tinygl/include/zbuffer.h` | Add `nx, ny, nz` to `ZBufferPoint` | Low |
| `tinygl/src/ztriangle.h` | Add `INTERP_NORMAL`, normal gradients | Medium |
| `tinygl/src/ztriangle.c` | Create `ZB_fillTrianglePhong()` | High |
| `tinygl/src/light.c` | Extract `gl_calculate_pixel_lighting()` | Medium |
| `tinygl/include/GL/gl.h` | Add `glPhongShading()` API | Low |
| `tinygl/src/api.c` | Implement `glPhongShading()` | Low |
| `tinygl/src/vertex.c` (likely) | Pass normals to rasterizer | Low |

---

## Performance Considerations

### Current (Gouraud):
- Lighting: **3 times per triangle** (once per vertex)
- Interpolation: **RGB only** (3 values per pixel)
- Per-pixel work: Depth test + RGB→pixel conversion

### Proposed (Phong):
- Lighting: **~1000 times per triangle** (once per pixel, for typical triangle)
- Interpolation: **Normal (3 values)** + depth (1 value)
- Per-pixel work: Normal normalization + full lighting equation + depth test + RGB→pixel conversion

**Expected Impact**: 30-50% slower (consistent with plan estimate)

**Optimizations to Consider**:
1. Fast normal normalization (lookup table or FISR)
2. Hybrid mode: Phong for curved surfaces, Gouraud for flat
3. Optional specular (Phong-Blinn without specular is cheaper)

---

## Key Macros and Conventions

### Interpolation Flags (from `ztriangle.h`)
- `INTERP_Z` - Interpolate depth
- `INTERP_RGB` - Interpolate color (Gouraud)
- `INTERP_ST` - Interpolate texture coordinates (not used)
- `INTERP_STZ` - Interpolate perspective-correct texture coords
- **NEW**: `INTERP_NORMAL` - Interpolate normals (Phong)

### Macro Pattern
Functions like `ZB_fillTriangleSmooth()`:
1. Define interpolation flags (`#define INTERP_RGB`)
2. Define `DRAW_INIT()` macro (setup code)
3. Define `PUT_PIXEL(_a)` macro (per-pixel logic)
4. `#include "ztriangle.h"` (scan-line loop template)

**Important**: Each combination of flags generates a separate specialized function.

---

## Testing Strategy

### Test Scenes (add to `tests/tinygl/`)
1. **Phong vs Gouraud Comparison**:
   - Low-poly sphere (tessellation 1) - shows Phong advantage
   - High-poly sphere (tessellation 3) - both should look similar
2. **Performance Benchmark**:
   - Measure FPS with Phong enabled vs disabled
   - Target: <50% slowdown
3. **Visual Quality**:
   - Cylinder with gradient lighting (catches lighting bands)
   - Should see smooth gradient with Phong, bands with Gouraud

### Success Criteria
- ✅ Phong produces smooth lighting on low-poly curved surfaces
- ✅ Gouraud still works (backward compatibility)
- ✅ Can toggle at runtime
- ✅ Performance acceptable (<50% slower)
- ✅ All existing tests still pass

---

## Next Steps

### Stage 2: Add Normal Interpolation Infrastructure
1. Read `zbuffer.h` to understand `ZBufferPoint` structure
2. Add `nx, ny, nz` fields to `ZBufferPoint`
3. Add `INTERP_NORMAL` flag and gradient calculations to `ztriangle.h`
4. **DO NOT** implement per-pixel lighting yet - just infrastructure
5. Verify code compiles and existing tests pass (Gouraud unchanged)

**Philosophy**: Build infrastructure incrementally, test at each stage.
