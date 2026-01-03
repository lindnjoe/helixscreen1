// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "bed_mesh_renderer.h" // For bed_mesh_renderer_t, bed_mesh_quad_3d_t

#include <vector>

/**
 * @file bed_mesh_geometry.h
 * @brief 3D geometry generation for bed mesh visualization
 *
 * Provides functions for generating 3D mesh quads from height data and
 * sorting them for proper depth ordering (painter's algorithm).
 *
 * All functions operate on an existing bed_mesh_renderer_t instance in
 * the helix::mesh namespace.
 */

namespace helix {
namespace mesh {

/**
 * @brief Generate 3D quads from mesh height data
 *
 * Creates a quad (4 vertices) for each mesh cell with:
 * - World-space 3D positions computed from mesh indices and Z values
 * - Per-vertex colors mapped from height (via gradient module)
 * - Center color for fast solid rendering during drag
 *
 * Quads are stored in renderer->quads vector. Number of quads = (rows-1) × (cols-1).
 *
 * Quad vertex layout (view from above, looking down -Z axis):
 *
 *   mesh[row][col]         mesh[row][col+1]
 *        [2]TL ──────────────── [3]TR
 *         │                      │
 *         │       QUAD           │
 *         │     (row,col)        │
 *         │                      │
 *        [0]BL ──────────────── [1]BR
 *   mesh[row+1][col]       mesh[row+1][col+1]
 *
 * @param renderer Renderer instance with valid mesh data
 */
void generate_mesh_quads(bed_mesh_renderer_t* renderer);

/**
 * @brief Sort quads by average depth (painter's algorithm)
 *
 * Sorts quads in descending depth order (furthest first) to ensure
 * correct occlusion when rendering without a Z-buffer.
 *
 * Uses quad.avg_depth which must be computed during projection.
 *
 * @param quads Vector of quads to sort (modified in-place)
 */
void sort_quads_by_depth(std::vector<bed_mesh_quad_3d_t>& quads);

/**
 * @brief Interpolate coordinate from mesh index to printer coordinate
 *
 * Helper function to deduplicate coordinate interpolation logic used in
 * multiple places (vertex projection, quad generation).
 *
 * Maps mesh index [0, max_index] to printer coordinate [min_mm, max_mm].
 *
 * @param index Current mesh index (row or col)
 * @param max_index Maximum mesh index (rows-1 or cols-1)
 * @param min_mm Minimum coordinate in millimeters
 * @param max_mm Maximum coordinate in millimeters
 * @return Interpolated coordinate in millimeters
 */
inline double mesh_index_to_printer_coord(int index, int max_index, double min_mm, double max_mm) {
    return min_mm +
           (static_cast<double>(index) / static_cast<double>(max_index)) * (max_mm - min_mm);
}

} // namespace mesh
} // namespace helix
