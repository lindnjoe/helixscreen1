// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "moonraker_api_mock.h"

#include "../tests/mocks/mock_printer_state.h"
#include "gcode_parser.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <sstream>

// Static initialization of path prefixes for fallback search
const std::vector<std::string> MoonrakerAPIMock::PATH_PREFIXES = {
    "",      // From project root: assets/test_gcodes/
    "../",   // From build/: ../assets/test_gcodes/
    "../../" // From build/bin/: ../../assets/test_gcodes/
};

MoonrakerAPIMock::MoonrakerAPIMock(MoonrakerClient& client, PrinterState& state)
    : MoonrakerAPI(client, state) {
    spdlog::info("[MoonrakerAPIMock] Created - HTTP methods will use local test files");
}

std::string MoonrakerAPIMock::find_test_file(const std::string& filename) const {
    namespace fs = std::filesystem;

    for (const auto& prefix : PATH_PREFIXES) {
        std::string path = prefix + std::string(TEST_GCODE_DIR) + "/" + filename;

        if (fs::exists(path)) {
            spdlog::debug("[MoonrakerAPIMock] Found test file at: {}", path);
            return path;
        }
    }

    // File not found in any location
    spdlog::debug("[MoonrakerAPIMock] Test file not found in any search path: {}", filename);
    return "";
}

void MoonrakerAPIMock::download_file(const std::string& root, const std::string& path,
                                     StringCallback on_success, ErrorCallback on_error) {
    // Strip any leading directory components to get just the filename
    std::string filename = path;
    size_t last_slash = path.rfind('/');
    if (last_slash != std::string::npos) {
        filename = path.substr(last_slash + 1);
    }

    spdlog::debug("[MoonrakerAPIMock] download_file: root='{}', path='{}' -> filename='{}'", root,
                  path, filename);

    // Find the test file using fallback path search
    std::string local_path = find_test_file(filename);

    if (local_path.empty()) {
        // File not found in test directory
        spdlog::warn("[MoonrakerAPIMock] File not found in test directories: {}", filename);

        if (on_error) {
            MoonrakerError err;
            err.type = MoonrakerErrorType::FILE_NOT_FOUND;
            err.message = "Mock file not found: " + filename;
            err.method = "download_file";
            on_error(err);
        }
        return;
    }

    // Try to read the local file
    std::ifstream file(local_path, std::ios::binary);
    if (file) {
        std::ostringstream content;
        content << file.rdbuf();
        file.close();

        spdlog::info("[MoonrakerAPIMock] Downloaded {} ({} bytes)", filename, content.str().size());

        if (on_success) {
            on_success(content.str());
        }
    } else {
        // Shouldn't happen if find_test_file succeeded, but handle gracefully
        spdlog::error("[MoonrakerAPIMock] Failed to read file that exists: {}", local_path);

        if (on_error) {
            MoonrakerError err;
            err.type = MoonrakerErrorType::FILE_NOT_FOUND;
            err.message = "Failed to read test file: " + filename;
            err.method = "download_file";
            on_error(err);
        }
    }
}

void MoonrakerAPIMock::upload_file(const std::string& root, const std::string& path,
                                   const std::string& content, SuccessCallback on_success,
                                   ErrorCallback on_error) {
    (void)on_error; // Unused - mock always succeeds

    spdlog::info("[MoonrakerAPIMock] Mock upload_file: root='{}', path='{}', size={} bytes", root,
                 path, content.size());

    // Mock always succeeds
    if (on_success) {
        on_success();
    }
}

void MoonrakerAPIMock::upload_file_with_name(const std::string& root, const std::string& path,
                                             const std::string& filename,
                                             const std::string& content, SuccessCallback on_success,
                                             ErrorCallback on_error) {
    (void)on_error; // Unused - mock always succeeds

    spdlog::info(
        "[MoonrakerAPIMock] Mock upload_file_with_name: root='{}', path='{}', filename='{}', "
        "size={} bytes",
        root, path, filename, content.size());

    // Mock always succeeds
    if (on_success) {
        on_success();
    }
}

void MoonrakerAPIMock::download_thumbnail(const std::string& thumbnail_path,
                                          const std::string& cache_path, StringCallback on_success,
                                          ErrorCallback on_error) {
    (void)on_error; // Unused - mock falls back to placeholder on failure

    spdlog::debug("[MoonrakerAPIMock] download_thumbnail: path='{}' -> cache='{}'", thumbnail_path,
                  cache_path);

    namespace fs = std::filesystem;

    // Moonraker thumbnail paths look like: ".thumbnails/filename-NNxNN.png"
    // Try to find the corresponding G-code file and extract the thumbnail
    std::string gcode_filename;

    // Extract the G-code filename from the thumbnail path
    // e.g., ".thumbnails/3DBenchy-300x300.png" -> "3DBenchy.gcode"
    size_t thumb_start = thumbnail_path.find(".thumbnails/");
    if (thumb_start != std::string::npos) {
        std::string thumb_name = thumbnail_path.substr(thumb_start + 12);
        // Remove resolution suffix like "-300x300.png" or "_300x300.png"
        size_t dash = thumb_name.rfind('-');
        size_t underscore = thumb_name.rfind('_');
        size_t sep = (dash != std::string::npos) ? dash : underscore;
        if (sep != std::string::npos) {
            gcode_filename = thumb_name.substr(0, sep) + ".gcode";
        }
    }

    // Try to find and extract thumbnail from the G-code file
    if (!gcode_filename.empty()) {
        std::string gcode_path = find_test_file(gcode_filename);
        if (!gcode_path.empty()) {
            // Extract thumbnails from the G-code file
            auto thumbnails = gcode::extract_thumbnails(gcode_path);
            if (!thumbnails.empty()) {
                // Find the largest thumbnail (best quality)
                const gcode::GCodeThumbnail* best = &thumbnails[0];
                for (const auto& thumb : thumbnails) {
                    if (thumb.pixel_count() > best->pixel_count()) {
                        best = &thumb;
                    }
                }

                // Write the thumbnail to the cache path
                std::ofstream file(cache_path, std::ios::binary);
                if (file) {
                    file.write(reinterpret_cast<const char*>(best->png_data.data()),
                               static_cast<std::streamsize>(best->png_data.size()));
                    file.close();

                    spdlog::info(
                        "[MoonrakerAPIMock] Extracted thumbnail {}x{} ({} bytes) from {} -> {}",
                        best->width, best->height, best->png_data.size(), gcode_filename,
                        cache_path);

                    if (on_success) {
                        on_success(cache_path);
                    }
                    return;
                }
            } else {
                spdlog::debug("[MoonrakerAPIMock] No thumbnails found in {}", gcode_path);
            }
        } else {
            spdlog::debug("[MoonrakerAPIMock] G-code file not found: {}", gcode_filename);
        }
    }

    // Fallback to placeholder if extraction failed
    spdlog::debug("[MoonrakerAPIMock] Falling back to placeholder thumbnail");

    std::string placeholder_path;
    for (const auto& prefix : PATH_PREFIXES) {
        std::string test_path = prefix + "assets/images/benchy_thumbnail_white.png";
        if (fs::exists(test_path)) {
            placeholder_path = "A:" + test_path;
            break;
        }
    }

    if (placeholder_path.empty()) {
        placeholder_path = "A:assets/images/placeholder_thumbnail.png";
    }

    if (on_success) {
        on_success(placeholder_path);
    }
}

// ============================================================================
// Shared State Methods
// ============================================================================

void MoonrakerAPIMock::set_mock_state(std::shared_ptr<MockPrinterState> state) {
    mock_state_ = state;
    if (state) {
        spdlog::debug("[MoonrakerAPIMock] Shared mock state attached");
    } else {
        spdlog::debug("[MoonrakerAPIMock] Shared mock state detached");
    }
}

std::set<std::string> MoonrakerAPIMock::get_excluded_objects_from_mock() const {
    if (mock_state_) {
        return mock_state_->get_excluded_objects();
    }
    return {};
}

std::vector<std::string> MoonrakerAPIMock::get_available_objects_from_mock() const {
    if (mock_state_) {
        return mock_state_->get_available_objects();
    }
    return {};
}
