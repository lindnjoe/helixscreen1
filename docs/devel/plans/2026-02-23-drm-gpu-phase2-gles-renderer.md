# Phase 2: G-code OpenGL ES 2.0 Renderer

**Date:** 2026-02-23
**Status:** Plan
**Prerequisite:** Phase 1 complete (DRM+EGL display backend merged)

## Goal

Replace the software TinyGL renderer with GPU-accelerated OpenGL ES 2.0 rendering for the G-code 3D viewer. Remove the TinyGL submodule entirely. Non-GPU platforms keep the 2D layer preview as fallback.

## Design Decisions (Resolved)

### 1. Shared EGL Context (Yes)

**Decision:** Share LVGL's EGL context via `eglCreateContext()` with `share_context` parameter.

**How it works:**
- LVGL's DRM+EGL driver stores its context in `lv_drm_ctx_t` (accessed via `lv_display_get_driver_data()`)
- The `lv_drm_ctx_t` contains an `lv_egl_ctx_t*` with `EGLDisplay`, `EGLContext`, `EGLSurface`
- We add ~20 lines of getter functions to LVGL (minimal patch) to expose these
- Our renderer calls `eglCreateContext(display, config, lvgl_context, attribs)` to create a shared context
- Shared contexts share texture/buffer namespaces — FBO rendered by our context is directly usable

**Pros:**
- Single GPU resource pool (no duplicate context overhead)
- Texture sharing is automatic (no `EGLImage` export/import dance)
- Simpler init — no separate GBM surface or DRM setup
- ~200 lines less code than separate context

**Cons:**
- Must save/restore GL state carefully (mitigated by FBO isolation)
- Tied to LVGL's EGL lifecycle (acceptable — renderer only exists when display does)

### 2. FBO Render Target (Yes)

**Decision:** Render to an offscreen FBO, blit the result into LVGL as an image.

**How it works:**
1. Create FBO with color renderbuffer (RGBA8) + depth renderbuffer (DEPTH16)
2. Bind FBO, render G-code geometry with GLSL shaders
3. `glReadPixels()` the result into an `lv_draw_buf_t`
4. Draw into LVGL via `lv_draw_image()` (same pattern as current TinyGL renderer)

**Why not render directly to display buffer:**
- LVGL uses partial refresh — direct rendering would conflict
- FBO gives clean isolation from LVGL's draw pipeline
- FBO can be cached (skip re-render if camera/data unchanged)
- Resolution can differ from widget size (interaction mode = 50% res)

### 3. Non-GPU Fallback: 2D Layer Preview

**Decision:** Keep the existing 2D layer renderer (`gcode_layer_renderer.cpp`) as the only rendering path on non-GPU platforms.

Platforms without GPU (AD5M, K1, CC1) never see the 3D option. The `ENABLE_GLES_3D` build flag gates compilation. The existing `HELIX_GCODE_MODE` env var controls runtime selection where both are available.

## Architecture

### LVGL EGL Access (Patch)

Add getter functions to LVGL's DRM EGL driver (submitted as patch):

```c
// In lv_linux_drm_egl.h (new public header or added to existing)
EGLDisplay lv_linux_drm_egl_get_display(lv_display_t *disp);
EGLContext lv_linux_drm_egl_get_context(lv_display_t *disp);
EGLConfig  lv_linux_drm_egl_get_config(lv_display_t *disp);
```

Implementation: cast `lv_display_get_driver_data()` to `lv_drm_ctx_t*`, return `ctx->egl_ctx->display` etc. ~20 lines total.

### Renderer Class

```
GCodeGLESRenderer (new)
├── EGL shared context (created once)
├── FBO (color + depth renderbuffers)
├── GLSL program (vertex + fragment shader)
├── VBO/IBO per geometry set (full + coarse LOD)
├── Uniform state (MVP matrix, lighting, colors)
└── lv_draw_buf_t output (same as TinyGL path)
```

**Public API matches `GCodeTinyGLRenderer`** — same `render()`, `set_viewport_size()`, `set_interaction_mode()`, `set_filament_color()`, etc. The UI wrapper (`ui_gcode_viewer.cpp`) switches renderer class based on build flag.

### GLSL Shaders

**Vertex shader** (Gouraud lighting, matching TinyGL's current look):
```glsl
// gcode_gles.vert
uniform mat4 u_mvp;
uniform mat3 u_normal_matrix;
uniform vec3 u_light_dir[2];
uniform vec3 u_light_color[2];
uniform vec3 u_ambient;
uniform vec4 u_base_color;
uniform float u_specular_intensity;
uniform float u_specular_shininess;

attribute vec3 a_position;
attribute vec3 a_normal;

varying vec4 v_color;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    vec3 n = normalize(u_normal_matrix * a_normal);
    vec3 diffuse = u_ambient;
    for (int i = 0; i < 2; i++) {
        float NdotL = max(dot(n, u_light_dir[i]), 0.0);
        diffuse += u_light_color[i] * NdotL;
    }
    // Specular (Blinn-Phong, view-space)
    vec3 view_dir = vec3(0.0, 0.0, 1.0);
    float spec = 0.0;
    for (int i = 0; i < 2; i++) {
        vec3 half_dir = normalize(u_light_dir[i] + view_dir);
        spec += pow(max(dot(n, half_dir), 0.0), u_specular_shininess);
    }
    v_color = vec4(u_base_color.rgb * diffuse + vec3(spec * u_specular_intensity), u_base_color.a);
}
```

**Fragment shader:**
```glsl
// gcode_gles.frag
precision mediump float;
varying vec4 v_color;
uniform float u_ghost_alpha; // 1.0 = solid, <1.0 = ghost layer

void main() {
    // Stipple emulation for ghost mode (screen-door transparency)
    if (u_ghost_alpha < 1.0) {
        ivec2 fc = ivec2(gl_FragCoord.xy);
        if (mod(float(fc.x + fc.y), 2.0) > 0.5) discard;
    }
    gl_FragColor = v_color;
}
```

### Geometry Pipeline

Current TinyGL path: `GeometryBuilder` → `PrebuiltGeometry` (interleaved `pos+normal+color` arrays) → `glBegin/glEnd/glVertex3f` per triangle.

New path: `GeometryBuilder` → `PrebuiltGeometry` → **VBO upload** (one-time `glBufferData`) → `glDrawArrays` per draw call.

The `PrebuiltGeometry` struct already stores flat arrays of positions and normals. The port is straightforward:

1. Create VBO from `PrebuiltGeometry::positions` and `PrebuiltGeometry::normals`
2. One VBO per layer group (for layer range filtering)
3. `glDrawArrays(GL_TRIANGLES, ...)` replaces the per-vertex `glBegin/glEnd` loop
4. Coarse LOD geometry gets its own VBO set

### Render Flow

```
1. ui_gcode_viewer.cpp draw event fires
2. GCodeGLESRenderer::render() called
3. Check cached state — skip if camera/data/layer unchanged
4. eglMakeCurrent(shared_context)
5. glBindFramebuffer(GL_FRAMEBUFFER, fbo_)
6. glViewport(0, 0, render_width, render_height)
7. glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
8. Set uniforms (MVP, lighting, colors)
9. For each visible layer range:
   a. Bind layer VBO
   b. Set u_base_color (filament color or ghost color)
   c. Set u_ghost_alpha (1.0 for printed, <1.0 for ghost)
   d. glDrawArrays(GL_TRIANGLES, ...)
10. glReadPixels() → lv_draw_buf_t
11. lv_draw_image() into LVGL layer
12. eglMakeCurrent(EGL_NO_CONTEXT) — release
```

### Threading Model

**Single-threaded** — all GL calls happen on the LVGL thread during the draw event callback. This matches the current TinyGL approach and avoids all multi-threaded GL complexity.

Geometry building (CPU-heavy) stays on background threads as today. Only VBO upload and rendering happen on the LVGL thread.

## File Changes

| File | Change |
|------|--------|
| `src/rendering/gcode_gles_renderer.cpp` | **NEW** — OpenGL ES 2.0 renderer |
| `include/gcode_gles_renderer.h` | **NEW** — Public API (mirrors TinyGL API) |
| `src/rendering/gcode_gles_shaders.h` | **NEW** — Inline GLSL shader source strings |
| `src/ui/ui_gcode_viewer.cpp` | Switch `#ifdef ENABLE_TINYGL_3D` → `#ifdef ENABLE_GLES_3D` |
| `lib/lvgl/` | Patch: add EGL context getters (~20 lines) |
| `lv_conf.h` | No changes needed (EGL already gated) |
| `Makefile` | Add `ENABLE_GLES_3D` flag, new source files |
| `mk/cross.mk` | Set `ENABLE_GLES_3D=yes` for Pi targets with `ENABLE_OPENGLES=yes` |
| `mk/deps.mk` | Remove TinyGL build rules |
| `lib/tinygl/` | Remove submodule |
| `src/rendering/gcode_tinygl_renderer.cpp` | Remove |
| `include/gcode_tinygl_renderer.h` | Remove |

## Task Breakdown

### Task 1: LVGL EGL Getter Patch

Add public getter functions to access EGL context from `lv_linux_drm_egl.c`.

**Files:** `lib/lvgl/src/drivers/display/drm/lv_linux_drm_egl.c`, new header or existing header
**Output:** Patch file in `patches/`
**Estimate:** ~30 lines

### Task 2: GCodeGLESRenderer — Core Rendering

Create the renderer with:
- EGL shared context creation/teardown
- FBO creation/resize
- GLSL shader compilation + program linking
- Basic `render()` with clear + single-color triangle draw
- `glReadPixels()` → `lv_draw_buf_t` → `lv_draw_image()`

**Test:** Render a solid-color triangle visible in the G-code viewer widget.

### Task 3: Geometry Upload + Drawing

Port geometry from `glBegin/glEnd` to VBOs:
- Upload `PrebuiltGeometry` positions + normals to VBOs
- Per-layer-group VBOs for layer range filtering
- `glDrawArrays(GL_TRIANGLES, ...)`
- Coarse LOD VBO set for interaction mode

**Test:** Full G-code model visible with correct geometry.

### Task 4: Lighting + Materials

Wire up GLSL uniforms:
- Two-point studio lighting (matching TinyGL's `setup_lighting()`)
- Filament color
- Specular highlights (Blinn-Phong)
- Ambient term

**Test:** Visual parity with TinyGL renderer.

### Task 5: Ghost Layers + Print Progress

Port ghost/progress visualization:
- Ghost alpha uniform for stipple discard pattern
- Ghost color dimming
- Layer range filtering (min/max layer)
- Print progress layer tracking

**Test:** Ghost layers render with stipple pattern matching TinyGL output.

### Task 6: Interactive Features

Port remaining features:
- Interaction mode (50% resolution during drag)
- Object highlighting (per-object color override)
- Excluded object dimming
- Frustum culling (can reuse existing CPU-side culling)
- Frame skip optimization (cached render state comparison)
- `pick_object()` — ray-based click detection (CPU-side, no GL needed)

### Task 7: Build System + UI Integration

- Add `ENABLE_GLES_3D` build flag
- Wire into `mk/cross.mk` for Pi targets
- Update `ui_gcode_viewer.cpp` to use `GCodeGLESRenderer`
- Remove `ENABLE_TINYGL_3D` flag and TinyGL build rules
- Remove `lib/tinygl/` submodule

### Task 8: Testing + Deployment

- Build with `make pi-docker`
- Deploy to Pi 3/4/5, BTT CB1
- Compare visual output with TinyGL screenshots
- Measure FPS improvement
- Test non-GPU platform build (no 3D, 2D fallback only)
- Test DRM disabled (fbdev) — 3D viewer should gracefully show 2D

## Performance Expectations

| Metric | TinyGL (CPU) | OpenGL ES (GPU) | Notes |
|--------|-------------|-----------------|-------|
| 3M triangles | 3-4 FPS | 30+ FPS | Main win |
| Interaction mode | 8-10 FPS | 60 FPS | Half-res |
| VBO upload | N/A | ~50ms one-time | Geometry change only |
| Memory | ~20MB (framebuffer + zbuffer) | ~5MB (VBOs) + GPU VRAM | Lower CPU memory |
| `glReadPixels` | N/A | ~2ms at 800x480 | Per-frame cost |

## Risks

- **`glReadPixels` latency:** Synchronous GPU→CPU readback. At 800x480 RGBA this is ~1.5MB per frame. Measured ~2ms on Pi 4 which is acceptable. If too slow, can use PBO (pixel buffer object) for async readback.
- **ES 2.0 on Pi 3:** VideoCore IV only supports ES 2.0. Our shaders must stay ES 2.0 compatible (no `layout` qualifiers, no `in`/`out`, use `attribute`/`varying`).
- **State leaks:** Shared EGL context means any GL state we leave dirty could affect LVGL. Mitigated by: bind FBO before all draws, unbind after, use `eglMakeCurrent(EGL_NO_CONTEXT)` when done.
- **Geometry memory:** Large G-code files produce millions of vertices. VBO upload is one-time but could spike memory during the copy. Same as TinyGL — not a regression.

## Phase 2b: Visual Quality Improvements

**Date:** 2026-02-24
**Status:** Plan
**Prerequisite:** Phase 2 core renderer working (SDL GL context, VBO rendering, basic Gouraud shading)

### Motivation

Current renderer is optimized for embedded performance but looks flat compared to Mainsail's Sindarius/Babylon.js-based viewer. Key differences identified through comparison:

| Aspect | HelixScreen (current) | Sindarius/Mainsail |
|--------|----------------------|-------------------|
| Shading | Gouraud (per-vertex) | Per-pixel Phong via Babylon StandardMaterial |
| Lighting | 2 fixed directional + ambient | **Camera-following point light** |
| Normals | Flat per-face | Smooth (averaged at shared vertices) |
| Specular | Muted (shininess=20, intensity=0.075) | Configurable, more prominent |
| Ghost layers | Screen-door stipple (checkerboard discard) | Real alpha blending with depth write |
| Tube sides | 4/8/16 configurable | Parametric tube with adjustable tessellation |

### Design: What to Change

#### 1. Camera-Following Light (Biggest Visual Win)

**Current:** Two fixed directional lights (top + front fill) matched from OrcaSlicer.
**Problem:** Surfaces facing away from the fixed lights look dark/flat regardless of camera angle.

**Change:** Replace one fixed light with a camera-tracking directional light. Compute light direction from camera position each frame and pass as uniform.

```cpp
// In render(), before setting uniforms:
glm::vec3 cam_light_dir = glm::normalize(camera_.get_position() - camera_.get_target());
// Pass as u_light_dir[0] instead of fixed kLightTopDir
```

Keep the second light as a fixed ambient fill to prevent completely dark areas when looking from below. Adjust intensities:
- Camera light: 0.6 intensity (primary)
- Fixed fill: 0.2 intensity (prevents black shadows)
- Ambient: 0.25 (slightly reduced since camera light provides more even coverage)

#### 2. Per-Pixel (Phong) Shading

**Current:** Lighting computed per-vertex in vertex shader (Gouraud). Causes visible faceting on low-polygon tubes.
**Change:** Move lighting to fragment shader. Pass normal and position as varyings, compute diffuse + specular per-fragment.

**Vertex shader changes:**
```glsl
// Remove lighting computation, just pass through
varying vec3 v_normal;
varying vec3 v_position;
varying vec3 v_base_color;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_normal = normalize(u_normal_matrix * a_normal);
    v_position = (u_view * vec4(a_position, 1.0)).xyz;  // view-space position
    v_base_color = mix(u_base_color.rgb, a_color, u_use_vertex_color) * u_color_scale;
}
```

**Fragment shader changes:**
```glsl
precision mediump float;
varying vec3 v_normal;
varying vec3 v_position;
varying vec3 v_base_color;

uniform vec3 u_light_dir[2];
uniform vec3 u_light_color[2];
uniform vec3 u_ambient;
uniform float u_specular_intensity;
uniform float u_specular_shininess;
uniform float u_base_alpha;
uniform float u_ghost_alpha;

void main() {
    if (u_ghost_alpha < 1.0) {
        vec2 fc = floor(gl_FragCoord.xy);
        if (mod(fc.x + fc.y, 2.0) < 0.5) discard;
    }

    vec3 n = normalize(v_normal);  // Re-normalize after interpolation
    vec3 view_dir = normalize(-v_position);  // View direction in view space

    // Diffuse
    vec3 diffuse = u_ambient;
    for (int i = 0; i < 2; i++) {
        float NdotL = max(dot(n, u_light_dir[i]), 0.0);
        diffuse += u_light_color[i] * NdotL;
    }

    // Specular (Blinn-Phong, both lights)
    float spec = 0.0;
    for (int i = 0; i < 2; i++) {
        vec3 half_dir = normalize(u_light_dir[i] + view_dir);
        spec += pow(max(dot(n, half_dir), 0.0), u_specular_shininess);
    }

    vec3 color = v_base_color * diffuse + vec3(spec * u_specular_intensity);
    gl_FragColor = vec4(color, u_base_alpha);
}
```

**New uniform needed:** `u_view` (view matrix, for computing view-space position). Already available from camera — just not currently passed.

**Performance note:** Per-pixel Phong adds ~1 normalize + 2 dot products + 1 pow per fragment. On Pi 4's VideoCore VI this is negligible. On Pi 3's VideoCore IV it may cost ~10% — still acceptable since we're GPU-bound on readback, not fragment shading.

#### 3. Smooth Normals

**Current:** Each tube face gets a single flat normal pointing radially outward. Adjacent faces have discontinuous normals → visible hard edges.
**Change:** Compute per-vertex normals by averaging the normals of the two faces sharing each vertex.

```
Current (flat):          Smooth (averaged):
  n1→ |n2→              n_avg↗  n_avg→
  ______|______          ______/______
  face1 | face2          face1  face2
```

**Implementation in `generate_ribbon_vertices()`:**
- For each vertex at angle θ, compute the vertex normal as the radial direction at that angle (not the face normal at θ+half_step)
- This is actually simpler than face normals: `vertex_normal[i] = normalize(cos(θ_i) * right + sin(θ_i) * perp_up)`
- Each vertex already sits at angle `i * 2π/N`, so the radial direction IS the smooth normal
- No averaging needed — just use the vertex position's radial direction directly

**Change scope:** ~10 lines in `generate_ribbon_vertices()`. Replace face_normal indexing with per-vertex radial normals. Two triangles per face share their corner vertex normals (already the case with current vertex layout).

#### 4. Increase Specular for Plastic Look

**Current:** `shininess=20, intensity=0.075` — extremely muted, almost invisible.
**Change:** `shininess=48, intensity=0.25` — visible glossy highlight, like PLA/PETG plastic.

These are just constant changes — 2 lines. Can fine-tune interactively.

#### 5. Real Alpha Blending for Ghost Layers (Optional/Later)

**Current:** Screen-door stipple (checkerboard fragment discard). Looks pixelated.
**Change:** Proper alpha blending with `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)`.

**Complexity:** Requires depth-sorted rendering (back-to-front) for correct transparency. Ghost layers are already rendered after solid layers, so the sort order is: solid layers (opaque, depth write on) → ghost layers (translucent, depth write off, blend on).

**Risk:** Alpha blending on mobile GPUs can be slow due to overdraw. Screen-door stipple is zero-cost. May want to keep stipple as a performance fallback. Defer to after the first three improvements are validated.

### Implementation Sequence

#### Step 1: Camera-Following Light
1. Add camera position → light direction computation in `render()`
2. Replace `kLightTopDir` uniform with computed camera light direction
3. Adjust light intensities (camera=0.6, fill=0.2, ambient=0.25)
4. Visual test: rotate model, confirm all angles are well-lit

#### Step 2: Smooth Normals
1. In `generate_ribbon_vertices()`, compute per-vertex radial normals instead of per-face normals
2. Each vertex gets `normalize(cos(θ) * right + sin(θ) * up)` as its normal
3. Both triangles of each face share the corner vertex normals
4. Visual test: tube surfaces appear smooth, no hard edges between faces

#### Step 3: Per-Pixel Phong Shading
1. Add `u_view` uniform for view matrix
2. Move vertex shader lighting to fragment shader
3. Pass `v_normal`, `v_position`, `v_base_color` as varyings
4. Re-normalize interpolated normals in fragment shader
5. Compute diffuse + specular per-fragment
6. Visual test: smooth lighting gradients across tube faces, specular highlights track camera

#### Step 4: Specular Tuning
1. Bump `SPECULAR_SHININESS` to 48, `SPECULAR_INTENSITY` to 0.25
2. Both lights contribute specular (not just top light)
3. Visual test: filament has visible plastic sheen

#### Step 5: Alpha Blending for Ghost Layers (Optional)
1. Enable `GL_BLEND` for ghost layer pass
2. Set `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)`
3. Disable depth writes for ghost pass (`glDepthMask(GL_FALSE)`)
4. Set ghost alpha to 0.15-0.25
5. Visual test: ghost layers are smoothly translucent, no checkerboard artifacts

### Files Modified

| File | Changes |
|------|---------|
| `src/rendering/gcode_gles_renderer.cpp` | Shaders rewritten (vertex→passthrough, fragment→Phong), camera light uniform, `u_view` uniform, specular constants, optional alpha blend pass |
| `src/rendering/gcode_geometry_builder.cpp` | ~10 lines: per-vertex radial normals instead of per-face flat normals |
| `include/gcode_gles_renderer.h` | Add `u_view` uniform location member |

### Performance Budget

| Change | Fragment cost | Vertex cost | Notes |
|--------|-------------|-------------|-------|
| Camera light | None | None | Just different uniform value |
| Smooth normals | None | None | Different normal values, same count |
| Per-pixel Phong | +1 normalize, 2 dot, 1 pow | -lighting (moved) | Net ~10% fragment cost on VideoCore IV |
| Higher specular | None | None | Just different constants |
| Alpha blend | Overdraw cost | None | Only for ghost layers, defer if slow |

Total expected impact: <15% on Pi 3 (worst case), negligible on Pi 4/5. We're bottlenecked by `glReadPixels` latency, not shading.

## Non-Goals

- Anti-aliasing (MSAA) — adds FBO complexity, not needed at 800x480
- GPU-accelerated bed mesh — already 30+ FPS in software, port later if needed
- Texture mapping on geometry — not needed for G-code visualization
- Vulkan — overkill for 2D UI + simple 3D
- PBR materials — StandardMaterial-level Phong is sufficient for filament visualization
- Ambient occlusion — cost/benefit not worth it on embedded
- Shadow mapping — too expensive for mobile GPUs, camera-following light reduces the need
