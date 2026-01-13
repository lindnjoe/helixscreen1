// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file printer_excluded_objects_state.cpp
 * @brief Excluded objects state management extracted from PrinterState
 *
 * Manages the set of objects excluded from printing via Klipper's EXCLUDE_OBJECT
 * feature. Uses version-based notification since LVGL subjects don't support sets.
 *
 * Extracted from PrinterState as part of god class decomposition.
 */

#include "printer_excluded_objects_state.h"

#include <spdlog/spdlog.h>

namespace helix {

void PrinterExcludedObjectsState::init_subjects(bool register_xml) {
    if (subjects_initialized_) {
        spdlog::debug("[PrinterExcludedObjectsState] Subjects already initialized, skipping");
        return;
    }

    spdlog::debug("[PrinterExcludedObjectsState] Initializing subjects (register_xml={})",
                  register_xml);

    // Initialize version subject to 0 (no changes yet)
    lv_subject_init_int(&excluded_objects_version_, 0);

    // Register with SubjectManager for automatic cleanup
    subjects_.register_subject(&excluded_objects_version_);

    // Register with LVGL XML system for XML bindings
    if (register_xml) {
        spdlog::debug("[PrinterExcludedObjectsState] Registering subjects with XML system");
        lv_xml_register_subject(NULL, "excluded_objects_version", &excluded_objects_version_);
    } else {
        spdlog::debug("[PrinterExcludedObjectsState] Skipping XML registration (tests mode)");
    }

    subjects_initialized_ = true;
    spdlog::debug("[PrinterExcludedObjectsState] Subjects initialized successfully");
}

void PrinterExcludedObjectsState::deinit_subjects() {
    if (!subjects_initialized_) {
        return;
    }

    spdlog::debug("[PrinterExcludedObjectsState] Deinitializing subjects");
    subjects_.deinit_all();
    subjects_initialized_ = false;
}

void PrinterExcludedObjectsState::reset_for_testing() {
    if (!subjects_initialized_) {
        spdlog::debug("[PrinterExcludedObjectsState] reset_for_testing: subjects not initialized, "
                      "nothing to reset");
        return;
    }

    spdlog::info("[PrinterExcludedObjectsState] reset_for_testing: Deinitializing subjects to "
                 "clear observers");

    // Clear the excluded objects set
    excluded_objects_.clear();

    // Use SubjectManager for automatic subject cleanup
    subjects_.deinit_all();
    subjects_initialized_ = false;
}

void PrinterExcludedObjectsState::set_excluded_objects(
    const std::unordered_set<std::string>& objects) {
    // Only update if the set actually changed
    if (excluded_objects_ != objects) {
        excluded_objects_ = objects;

        // Increment version to notify observers
        int version = lv_subject_get_int(&excluded_objects_version_);
        lv_subject_set_int(&excluded_objects_version_, version + 1);

        spdlog::debug("[PrinterExcludedObjectsState] Excluded objects updated: {} objects "
                      "(version {})",
                      excluded_objects_.size(), version + 1);
    }
}

} // namespace helix
