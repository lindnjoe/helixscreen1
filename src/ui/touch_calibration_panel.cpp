// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 356C LLC

/**
 * @file touch_calibration_panel.cpp
 * @brief Touch calibration panel state machine implementation
 */

#include "touch_calibration_panel.h"

namespace helix {

TouchCalibrationPanel::TouchCalibrationPanel() = default;

TouchCalibrationPanel::~TouchCalibrationPanel() = default;

void TouchCalibrationPanel::set_completion_callback(CompletionCallback cb) {
    callback_ = std::move(cb);
}

void TouchCalibrationPanel::set_screen_size(int width, int height) {
    screen_width_ = width;
    screen_height_ = height;
}

void TouchCalibrationPanel::start() {
    state_ = State::POINT_1;
    calibration_.valid = false;

    // Calculate screen target positions with 15% margin inset
    screen_points_[0] = {static_cast<int>(screen_width_ * 0.15f),
                         static_cast<int>(screen_height_ * 0.30f)};
    screen_points_[1] = {static_cast<int>(screen_width_ * 0.50f),
                         static_cast<int>(screen_height_ * 0.85f)};
    screen_points_[2] = {static_cast<int>(screen_width_ * 0.85f),
                         static_cast<int>(screen_height_ * 0.15f)};
}

void TouchCalibrationPanel::capture_point(Point raw) {
    switch (state_) {
    case State::POINT_1:
        touch_points_[0] = raw;
        state_ = State::POINT_2;
        break;
    case State::POINT_2:
        touch_points_[1] = raw;
        state_ = State::POINT_3;
        break;
    case State::POINT_3:
        touch_points_[2] = raw;
        state_ = State::VERIFY;
        compute_calibration(screen_points_, touch_points_, calibration_);
        break;
    default:
        // No-op in IDLE, VERIFY, COMPLETE states
        break;
    }
}

void TouchCalibrationPanel::accept() {
    if (state_ != State::VERIFY) {
        return;
    }

    state_ = State::COMPLETE;
    if (callback_) {
        callback_(&calibration_);
    }
}

void TouchCalibrationPanel::retry() {
    if (state_ != State::VERIFY) {
        return;
    }

    state_ = State::POINT_1;
    calibration_.valid = false;

    // Recalculate screen points in case screen size changed during VERIFY
    screen_points_[0] = {static_cast<int>(screen_width_ * 0.15f),
                         static_cast<int>(screen_height_ * 0.30f)};
    screen_points_[1] = {static_cast<int>(screen_width_ * 0.50f),
                         static_cast<int>(screen_height_ * 0.85f)};
    screen_points_[2] = {static_cast<int>(screen_width_ * 0.85f),
                         static_cast<int>(screen_height_ * 0.15f)};
}

void TouchCalibrationPanel::cancel() {
    state_ = State::IDLE;
    calibration_.valid = false;
    if (callback_) {
        callback_(nullptr);
    }
}

TouchCalibrationPanel::State TouchCalibrationPanel::get_state() const {
    return state_;
}

Point TouchCalibrationPanel::get_target_position(int step) const {
    if (step < 0 || step > 2) {
        return Point{0, 0};
    }

    // Calculate target positions dynamically based on current screen size
    switch (step) {
    case 0:
        return {static_cast<int>(screen_width_ * 0.15f), static_cast<int>(screen_height_ * 0.30f)};
    case 1:
        return {static_cast<int>(screen_width_ * 0.50f), static_cast<int>(screen_height_ * 0.85f)};
    case 2:
        return {static_cast<int>(screen_width_ * 0.85f), static_cast<int>(screen_height_ * 0.15f)};
    default:
        return Point{0, 0};
    }
}

const TouchCalibration* TouchCalibrationPanel::get_calibration() const {
    if ((state_ == State::VERIFY || state_ == State::COMPLETE) && calibration_.valid) {
        return &calibration_;
    }
    return nullptr;
}

} // namespace helix
