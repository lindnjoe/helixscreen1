#!/usr/bin/env bats
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Tests for platform hook files.
# Verifies the hook contract (all required functions defined),
# shellcheck compliance, and basic syntax validity.

HOOKS_DIR="config/platform"
HOOK_FILES="hooks-ad5m-forgex.sh hooks-ad5m-kmod.sh hooks-pi.sh hooks-k1.sh"
REQUIRED_FUNCTIONS="platform_stop_competing_uis platform_enable_backlight platform_wait_for_services platform_pre_start platform_post_stop"

# --- Hook contract tests: every hook file must define all 5 functions ---

@test "forgex hooks define all required functions" {
    ( . "$HOOKS_DIR/hooks-ad5m-forgex.sh"
      for func in $REQUIRED_FUNCTIONS; do
          type "$func" >/dev/null 2>&1
      done )
}

@test "kmod hooks define all required functions" {
    ( . "$HOOKS_DIR/hooks-ad5m-kmod.sh"
      for func in $REQUIRED_FUNCTIONS; do
          type "$func" >/dev/null 2>&1
      done )
}

@test "pi hooks define all required functions" {
    ( . "$HOOKS_DIR/hooks-pi.sh"
      for func in $REQUIRED_FUNCTIONS; do
          type "$func" >/dev/null 2>&1
      done )
}

@test "k1 hooks define all required functions" {
    ( . "$HOOKS_DIR/hooks-k1.sh"
      for func in $REQUIRED_FUNCTIONS; do
          type "$func" >/dev/null 2>&1
      done )
}

# --- Shellcheck compliance ---

@test "forgex hooks pass shellcheck" {
    shellcheck -s sh "$HOOKS_DIR/hooks-ad5m-forgex.sh"
}

@test "kmod hooks pass shellcheck" {
    shellcheck -s sh "$HOOKS_DIR/hooks-ad5m-kmod.sh"
}

@test "pi hooks pass shellcheck" {
    shellcheck -s sh "$HOOKS_DIR/hooks-pi.sh"
}

@test "k1 hooks pass shellcheck" {
    shellcheck -s sh "$HOOKS_DIR/hooks-k1.sh"
}

# --- Syntax validity ---

@test "forgex hooks have valid sh syntax" {
    sh -n "$HOOKS_DIR/hooks-ad5m-forgex.sh"
}

@test "kmod hooks have valid sh syntax" {
    sh -n "$HOOKS_DIR/hooks-ad5m-kmod.sh"
}

@test "pi hooks have valid sh syntax" {
    sh -n "$HOOKS_DIR/hooks-pi.sh"
}

@test "k1 hooks have valid sh syntax" {
    sh -n "$HOOKS_DIR/hooks-k1.sh"
}
