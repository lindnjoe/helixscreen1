#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Module: permissions
# Permission checks and sudo handling
#
# Reads: (platform passed as argument)
# Writes: SUDO

# Source guard
[ -n "${_HELIX_PERMISSIONS_SOURCED:-}" ] && return 0
_HELIX_PERMISSIONS_SOURCED=1

# Initialize SUDO (will be set by check_permissions)
SUDO=""

# Check if running as root (required for AD5M/K1, optional for Pi)
# Sets: SUDO variable ("sudo" or "")
check_permissions() {
    local platform=$1

    if [ "$platform" = "ad5m" ] || [ "$platform" = "k1" ]; then
        if [ "$(id -u)" != "0" ]; then
            log_error "Installation on $platform requires root privileges."
            log_error "Please run: sudo $0 $*"
            exit 1
        fi
        SUDO=""
    else
        # Pi: warn if not root but allow sudo
        if [ "$(id -u)" != "0" ]; then
            if ! command -v sudo >/dev/null 2>&1; then
                log_error "Not running as root and sudo is not available."
                log_error "Please run as root or install sudo."
                exit 1
            fi
            log_info "Not running as root. Will use sudo for privileged operations."
            SUDO="sudo"
        else
            SUDO=""
        fi
    fi
}
