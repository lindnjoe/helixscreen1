// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "subject_managed_panel.h"

#include <lvgl.h>
#include <string>
#include <unordered_set>

namespace helix {

/**
 * @brief Manages excluded objects state for Klipper's EXCLUDE_OBJECT feature
 *
 * Tracks which objects have been excluded from the current print job.
 * Uses a version-based notification pattern since LVGL subjects don't
 * natively support set types.
 *
 * Extracted from PrinterState as part of god class decomposition.
 *
 * Usage pattern:
 * 1. Observer subscribes to excluded_objects_version_ subject
 * 2. When notified, observer calls get_excluded_objects() for updated set
 *
 * @note set_excluded_objects() only increments version if set actually changed
 */
class PrinterExcludedObjectsState {
  public:
    PrinterExcludedObjectsState() = default;
    ~PrinterExcludedObjectsState() = default;

    // Non-copyable
    PrinterExcludedObjectsState(const PrinterExcludedObjectsState&) = delete;
    PrinterExcludedObjectsState& operator=(const PrinterExcludedObjectsState&) = delete;

    /**
     * @brief Initialize excluded objects subjects
     * @param register_xml If true, register subjects with LVGL XML system
     */
    void init_subjects(bool register_xml = true);

    /**
     * @brief Deinitialize subjects (called by SubjectManager automatically)
     */
    void deinit_subjects();

    /**
     * @brief Reset state for testing - clears subjects and reinitializes
     */
    void reset_for_testing();

    // ========================================================================
    // Setters
    // ========================================================================

    /**
     * @brief Update excluded objects from Moonraker status update
     *
     * Compares new set with current set and only updates if different.
     * Increments version subject to notify observers when set changes.
     *
     * @param objects Set of object names that are currently excluded
     */
    void set_excluded_objects(const std::unordered_set<std::string>& objects);

    // ========================================================================
    // Subject accessors
    // ========================================================================

    /**
     * @brief Get excluded objects version subject
     *
     * This subject is incremented whenever the excluded objects list changes.
     * Observers should watch this subject and call get_excluded_objects() to
     * get the updated list when notified.
     *
     * @return Subject pointer (integer, incremented on each change)
     */
    lv_subject_t* get_excluded_objects_version_subject() {
        return &excluded_objects_version_;
    }

    // ========================================================================
    // Query methods
    // ========================================================================

    /**
     * @brief Get the current set of excluded objects
     *
     * Returns object names that have been excluded from printing via Klipper's
     * EXCLUDE_OBJECT feature. Updated from Moonraker notify_status_update.
     *
     * @return Const reference to the set of excluded object names
     */
    const std::unordered_set<std::string>& get_excluded_objects() const {
        return excluded_objects_;
    }

  private:
    SubjectManager subjects_;
    bool subjects_initialized_ = false;

    // Excluded objects version subject (incremented when excluded_objects_ changes)
    lv_subject_t excluded_objects_version_{};

    // Set of excluded object names (NOT a subject - sets aren't natively supported)
    std::unordered_set<std::string> excluded_objects_;
};

} // namespace helix
