// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui_modal.h"

#include <functional>

/**
 * @file ui_runout_guidance_modal.h
 * @brief Runout guidance modal with 6 action buttons
 *
 * Shown when filament runout is detected during a print pause.
 * Provides buttons for: Load Filament, Unload Filament, Purge,
 * Resume Print, Cancel Print, and OK (dismiss when idle).
 *
 * Button-to-callback mapping:
 * - btn_load_filament   -> on_ok()        (primary action)
 * - btn_unload_filament -> on_quaternary() (unload before loading new)
 * - btn_purge           -> on_quinary()   (purge after loading)
 * - btn_resume          -> on_cancel()    (resume paused print)
 * - btn_cancel_print    -> on_tertiary()  (cancel print)
 * - btn_ok              -> on_senary()    (dismiss when idle)
 *
 * @example
 *   runout_modal_.set_on_load_filament([this]() { start_load(); });
 *   runout_modal_.set_on_resume([this]() { resume_print(); });
 *   runout_modal_.show(lv_screen_active());
 */

/**
 * @brief Runout guidance modal with 6 action buttons
 *
 * Derives from Modal base class for RAII lifecycle management.
 */
class RunoutGuidanceModal : public Modal {
  public:
    using Callback = std::function<void()>;

    /**
     * @brief Get human-readable name for logging
     * @return "Runout Guidance"
     */
    const char* get_name() const override {
        return "Runout Guidance";
    }

    /**
     * @brief Get XML component name for lv_xml_create()
     * @return "runout_guidance_modal"
     */
    const char* component_name() const override {
        return "runout_guidance_modal";
    }

    /**
     * @brief Set callback for Load Filament button (btn_load_filament -> on_ok)
     * @param cb Callback function
     */
    void set_on_load_filament(Callback cb) {
        on_load_filament_ = std::move(cb);
    }

    /**
     * @brief Set callback for Unload Filament button (btn_unload_filament -> on_quaternary)
     * @param cb Callback function
     */
    void set_on_unload_filament(Callback cb) {
        on_unload_filament_ = std::move(cb);
    }

    /**
     * @brief Set callback for Purge button (btn_purge -> on_quinary)
     * @param cb Callback function
     */
    void set_on_purge(Callback cb) {
        on_purge_ = std::move(cb);
    }

    /**
     * @brief Set callback for Resume button (btn_resume -> on_cancel)
     * @param cb Callback function
     */
    void set_on_resume(Callback cb) {
        on_resume_ = std::move(cb);
    }

    /**
     * @brief Set callback for Cancel Print button (btn_cancel_print -> on_tertiary)
     * @param cb Callback function
     */
    void set_on_cancel_print(Callback cb) {
        on_cancel_print_ = std::move(cb);
    }

    /**
     * @brief Set callback for OK button when idle (btn_ok -> on_senary)
     * @param cb Callback function
     */
    void set_on_ok_dismiss(Callback cb) {
        on_ok_dismiss_ = std::move(cb);
    }

  protected:
    /**
     * @brief Called after modal is created and visible
     *
     * Wires up all 6 buttons to their respective handlers.
     */
    void on_show() override;

    /**
     * @brief Called when user clicks Load Filament button
     *
     * Invokes the load filament callback if set, then hides the modal.
     */
    void on_ok() override {
        if (on_load_filament_) {
            on_load_filament_();
        }
        hide();
    }

    /**
     * @brief Called when user clicks Resume button
     *
     * Invokes the resume callback if set, then hides the modal.
     */
    void on_cancel() override {
        if (on_resume_) {
            on_resume_();
        }
        hide();
    }

    /**
     * @brief Called when user clicks Cancel Print button
     *
     * Invokes the cancel print callback if set, then hides the modal.
     */
    void on_tertiary() override {
        if (on_cancel_print_) {
            on_cancel_print_();
        }
        hide();
    }

    /**
     * @brief Called when user clicks Unload Filament button
     *
     * Invokes the unload callback if set. Does not hide modal
     * since user may want to load after unload.
     */
    void on_quaternary() override {
        if (on_unload_filament_) {
            on_unload_filament_();
        }
        // Don't hide - user may want to load after unload
    }

    /**
     * @brief Called when user clicks Purge button
     *
     * Invokes the purge callback if set. Does not hide modal
     * since user may want to purge multiple times.
     */
    void on_quinary() override {
        if (on_purge_) {
            on_purge_();
        }
        // Don't hide - user may want to purge multiple times
    }

    /**
     * @brief Called when user clicks OK button (dismiss when idle)
     *
     * Invokes the ok dismiss callback if set, then hides the modal.
     */
    void on_senary() override {
        if (on_ok_dismiss_) {
            on_ok_dismiss_();
        }
        hide();
    }

  private:
    Callback on_load_filament_;
    Callback on_unload_filament_;
    Callback on_purge_;
    Callback on_resume_;
    Callback on_cancel_print_;
    Callback on_ok_dismiss_;
};
