// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bed_mesh_coordinate_transform.h"

namespace helix {
namespace mesh {

double mesh_col_to_world_x(int col, int cols, double scale) {
    return (col - (cols - 1) / 2.0) * scale;
}

double mesh_row_to_world_y(int row, int rows, double scale) {
    return ((rows - 1 - row) - (rows - 1) / 2.0) * scale;
}

double mesh_z_to_world_z(double z_height, double z_center, double z_scale) {
    return (z_height - z_center) * z_scale;
}

double compute_mesh_z_center(double mesh_min_z, double mesh_max_z) {
    return (mesh_min_z + mesh_max_z) / 2.0;
}

double compute_grid_z(double z_center, double z_scale) {
    // This function is deprecated - grid_z should be computed from mesh_min_z directly
    // Return 0 as a fallback, but callers should use mesh_z_to_world_z(mesh_min_z, ...) instead
    (void)z_center;
    (void)z_scale;
    return 0.0;
}

// ============================================================================
// Printer coordinate transforms (origin-agnostic)
// ============================================================================

double printer_x_to_world_x(double x_mm, double bed_center_x, double scale_factor) {
    // Simply center around the bed center - works for any origin convention
    return (x_mm - bed_center_x) * scale_factor;
}

double printer_y_to_world_y(double y_mm, double bed_center_y, double scale_factor) {
    // Center around bed center, but invert Y so that mesh[0][*] (front row) appears
    // in front (positive Y in world space, toward the viewer in 3D view)
    // The inversion is about display convention, not printer coordinate system
    return -(y_mm - bed_center_y) * scale_factor;
}

double compute_bed_scale_factor(double bed_size_mm, double target_world_size) {
    if (bed_size_mm <= 0.0) {
        return 1.0; // Fallback to avoid division by zero
    }
    return target_world_size / bed_size_mm;
}

} // namespace mesh
} // namespace helix
