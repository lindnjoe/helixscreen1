// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file printer_versions_state.cpp
 * @brief Software version state management extracted from PrinterState
 *
 * Manages Klipper and Moonraker version subjects for UI display in the
 * Settings panel About section.
 *
 * Extracted from PrinterState as part of god class decomposition.
 */

#include "printer_versions_state.h"

#include <spdlog/spdlog.h>

#include <cstring>

namespace helix {

void PrinterVersionsState::init_subjects(bool register_xml) {
    if (subjects_initialized_) {
        spdlog::debug("[PrinterVersionsState] Subjects already initialized, skipping");
        return;
    }

    spdlog::debug("[PrinterVersionsState] Initializing subjects (register_xml={})", register_xml);

    // Initialize string buffers with default em dash value
    std::memset(klipper_version_buf_, 0, sizeof(klipper_version_buf_));
    std::memset(moonraker_version_buf_, 0, sizeof(moonraker_version_buf_));
    std::strcpy(klipper_version_buf_, "—");
    std::strcpy(moonraker_version_buf_, "—");

    // Initialize string subjects with buffers
    lv_subject_init_string(&klipper_version_, klipper_version_buf_, nullptr,
                           sizeof(klipper_version_buf_), "—");
    lv_subject_init_string(&moonraker_version_, moonraker_version_buf_, nullptr,
                           sizeof(moonraker_version_buf_), "—");

    // Register with SubjectManager for automatic cleanup
    subjects_.register_subject(&klipper_version_);
    subjects_.register_subject(&moonraker_version_);

    // Register with LVGL XML system for XML bindings
    if (register_xml) {
        spdlog::debug("[PrinterVersionsState] Registering subjects with XML system");
        lv_xml_register_subject(NULL, "klipper_version", &klipper_version_);
        lv_xml_register_subject(NULL, "moonraker_version", &moonraker_version_);
    } else {
        spdlog::debug("[PrinterVersionsState] Skipping XML registration (tests mode)");
    }

    subjects_initialized_ = true;
    spdlog::debug("[PrinterVersionsState] Subjects initialized successfully");
}

void PrinterVersionsState::deinit_subjects() {
    if (!subjects_initialized_) {
        return;
    }

    spdlog::debug("[PrinterVersionsState] Deinitializing subjects");
    subjects_.deinit_all();
    subjects_initialized_ = false;
}

void PrinterVersionsState::reset_for_testing() {
    if (!subjects_initialized_) {
        spdlog::debug("[PrinterVersionsState] reset_for_testing: subjects not initialized, "
                      "nothing to reset");
        return;
    }

    spdlog::info(
        "[PrinterVersionsState] reset_for_testing: Deinitializing subjects to clear observers");

    // Use SubjectManager for automatic subject cleanup
    subjects_.deinit_all();
    subjects_initialized_ = false;
}

void PrinterVersionsState::set_klipper_version_internal(const std::string& version) {
    lv_subject_copy_string(&klipper_version_, version.c_str());
    spdlog::debug("[PrinterVersionsState] Klipper version set: {}", version);
}

void PrinterVersionsState::set_moonraker_version_internal(const std::string& version) {
    lv_subject_copy_string(&moonraker_version_, version.c_str());
    spdlog::debug("[PrinterVersionsState] Moonraker version set: {}", version);
}

} // namespace helix
