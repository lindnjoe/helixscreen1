// Copyright (c) 2025 HelixScreen Project
// SPDX-License-Identifier: GPL-3.0-or-later
//
// TinyGL Test Framework - Core utilities for quality and performance testing

#pragma once

extern "C" {
#include <GL/gl.h>
#include <zbuffer.h>
}
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tinygl_test {

// Image comparison metrics
struct ImageMetrics {
    double mse;       // Mean Squared Error
    double psnr;      // Peak Signal-to-Noise Ratio (dB)
    double ssim;      // Structural Similarity Index
    double max_diff;  // Maximum pixel difference
    int diff_pixels;  // Number of pixels that differ
};

// Performance metrics
struct PerfMetrics {
    double frame_time_ms;
    double vertices_per_second;
    double triangles_per_second;
    double pixels_per_second;
    size_t memory_usage_bytes;
    double cache_miss_rate;  // If available from perf counters
};

// Test scene configuration
struct SceneConfig {
    int width = 800;
    int height = 600;
    bool enable_depth = true;
    bool enable_lighting = true;
    bool enable_textures = false;
    bool enable_smooth_shading = true;
    int num_lights = 2;
    float ambient_intensity = 0.3f;
    float specular_intensity = 0.05f;
    float specular_shininess = 32.0f;
};

// Base class for test scenes
class TestScene {
public:
    virtual ~TestScene() = default;

    // Setup the scene (called once)
    virtual void setup(const SceneConfig& config) = 0;

    // Render the scene (called per frame)
    virtual void render() = 0;

    // Get scene complexity metrics
    virtual size_t get_vertex_count() const = 0;
    virtual size_t get_triangle_count() const = 0;

    // Scene name for reporting
    virtual std::string get_name() const = 0;
};

// Test framework main class
class TinyGLTestFramework {
public:
    TinyGLTestFramework(int width = 800, int height = 600);
    ~TinyGLTestFramework();

    // Initialize TinyGL context
    bool initialize();

    // Render a test scene
    void render_scene(TestScene* scene, const SceneConfig& config = {});

    // Capture current framebuffer to image
    std::vector<uint8_t> capture_framebuffer_rgb();

    // Save framebuffer to PPM file
    bool save_screenshot(const std::string& filename);

    // Compare two images
    static ImageMetrics compare_images(
        const std::vector<uint8_t>& img1,
        const std::vector<uint8_t>& img2,
        int width, int height
    );

    // Benchmark a scene
    PerfMetrics benchmark_scene(
        TestScene* scene,
        const SceneConfig& config,
        int num_frames = 100
    );

    // Load reference image from PPM
    static std::vector<uint8_t> load_ppm(
        const std::string& filename,
        int& width, int& height
    );

    // Get TinyGL context for direct manipulation
    ZBuffer* get_zbuffer() { return zb_; }

    // Enable/disable Phong shading
    void set_phong_shading(bool enable);

private:
    int width_;
    int height_;
    ZBuffer* zb_;
    std::vector<unsigned int> framebuffer_;

    // Helper to setup standard lighting
    void setup_standard_lighting(const SceneConfig& config);

    // Helper to clear buffers
    void clear_buffers();

    // Performance timing utilities
    double measure_frame_time(std::function<void()> render_func);
};

// Predefined test scenes
class SphereTesselationScene : public TestScene {
public:
    SphereTesselationScene(int subdivisions = 3);
    void setup(const SceneConfig& config) override;
    void render() override;
    size_t get_vertex_count() const override { return vertices_.size() / 3; }
    size_t get_triangle_count() const override { return vertices_.size() / 9; }
    std::string get_name() const override { return "Sphere Tesselation"; }

private:
    int subdivisions_;
    std::vector<float> vertices_;
    std::vector<float> normals_;
    std::vector<float> colors_;

    void generate_sphere();
    void subdivide_triangle(
        float* v1, float* v2, float* v3,
        int depth
    );
};

class CubeGridScene : public TestScene {
public:
    CubeGridScene(int grid_size = 10);
    void setup(const SceneConfig& config) override;
    void render() override;
    size_t get_vertex_count() const override { return grid_size_ * grid_size_ * grid_size_ * 24; }
    size_t get_triangle_count() const override { return grid_size_ * grid_size_ * grid_size_ * 12; }
    std::string get_name() const override { return "Cube Grid"; }

private:
    int grid_size_;
    float rotation_;

    void render_cube(float x, float y, float z, float size);
};

class GouraudArtifactScene : public TestScene {
public:
    void setup(const SceneConfig& config) override;
    void render() override;
    size_t get_vertex_count() const override { return 360 * 2; }  // Cylinder vertices
    size_t get_triangle_count() const override { return 360 * 2; }
    std::string get_name() const override { return "Gouraud Artifacts"; }

private:
    void render_cylinder(float radius, float height, int segments);
    void render_large_triangles();
};

class ColorBandingScene : public TestScene {
public:
    void setup(const SceneConfig& config) override;
    void render() override;
    size_t get_vertex_count() const override { return 4; }  // Gradient quad
    size_t get_triangle_count() const override { return 2; }
    std::string get_name() const override { return "Color Banding"; }

private:
    void render_gradient_quad();
    void render_smooth_sphere();
};

// Utility functions
namespace utils {
    // Generate test G-code for rendering
    std::string generate_test_gcode(const std::string& pattern);

    // Create a reference lighting setup matching OrcaSlicer
    void setup_orcaslicer_lighting();

    // Performance counter access (if available)
    bool init_perf_counters();
    double get_cache_miss_rate();

    // Image difference visualization
    std::vector<uint8_t> create_diff_image(
        const std::vector<uint8_t>& img1,
        const std::vector<uint8_t>& img2,
        int width, int height,
        float amplification = 10.0f
    );

    // Calculate SSIM (Structural Similarity Index)
    double calculate_ssim(
        const std::vector<uint8_t>& img1,
        const std::vector<uint8_t>& img2,
        int width, int height
    );
}

} // namespace tinygl_test