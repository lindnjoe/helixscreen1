#!/bin/sh
# Copyright (C) 2025-2026 356C LLC
# SPDX-License-Identifier: GPL-3.0-or-later
#
# HelixScreen Uninstaller
# Removes HelixScreen and restores the previous screen UI
#
# Usage:
#   ./uninstall.sh              # Interactive uninstall
#   ./uninstall.sh --force      # Skip confirmation prompt
#
# This script:
#   1. Stops HelixScreen
#   2. Removes init script or systemd service
#   3. Removes installation directory
#   4. Re-enables previous UI (GuppyScreen, FeatherScreen, etc.)

set -e

# Colors (if terminal supports it)
if [ -t 1 ]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    CYAN='\033[0;36m'
    NC='\033[0m'
else
    RED=''
    GREEN=''
    YELLOW=''
    CYAN=''
    NC=''
fi

log_info() { echo "${CYAN}[INFO]${NC} $1"; }
log_success() { echo "${GREEN}[OK]${NC} $1"; }
log_warn() { echo "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo "${RED}[ERROR]${NC} $1" >&2; }

INSTALL_DIR="/opt/helixscreen"
INIT_SCRIPT="/etc/init.d/S90helixscreen"
SYSTEMD_SERVICE="/etc/systemd/system/helixscreen.service"

# Previous UIs we may need to re-enable
PREVIOUS_UIS="guppyscreen GuppyScreen featherscreen FeatherScreen"

# Detect init system
detect_init_system() {
    if command -v systemctl >/dev/null 2>&1 && [ -d /run/systemd/system ]; then
        echo "systemd"
    elif [ -d /etc/init.d ]; then
        echo "sysv"
    else
        echo "unknown"
    fi
}

# Check if running as root
check_root() {
    if [ "$(id -u)" != "0" ]; then
        log_error "This script must be run as root."
        log_error "Please run: sudo $0"
        exit 1
    fi
}

# Stop HelixScreen
stop_helixscreen() {
    log_info "Stopping HelixScreen..."

    INIT_SYSTEM=$(detect_init_system)

    if [ "$INIT_SYSTEM" = "systemd" ]; then
        systemctl stop helixscreen 2>/dev/null || true
        systemctl disable helixscreen 2>/dev/null || true
    fi

    if [ -x "$INIT_SCRIPT" ]; then
        "$INIT_SCRIPT" stop 2>/dev/null || true
    fi

    # Kill any remaining processes
    if command -v killall >/dev/null 2>&1; then
        killall helix-screen 2>/dev/null || true
        killall helix-splash 2>/dev/null || true
    elif command -v pidof >/dev/null 2>&1; then
        for proc in helix-screen helix-splash; do
            for pid in $(pidof "$proc" 2>/dev/null); do
                kill "$pid" 2>/dev/null || true
            done
        done
    fi

    log_success "HelixScreen stopped"
}

# Remove init script or systemd service
remove_service() {
    log_info "Removing service configuration..."

    INIT_SYSTEM=$(detect_init_system)

    if [ "$INIT_SYSTEM" = "systemd" ]; then
        if [ -f "$SYSTEMD_SERVICE" ]; then
            rm -f "$SYSTEMD_SERVICE"
            systemctl daemon-reload
            log_success "Removed systemd service"
        fi
    fi

    if [ -f "$INIT_SCRIPT" ]; then
        rm -f "$INIT_SCRIPT"
        log_success "Removed SysV init script"
    fi
}

# Remove installation directory
remove_installation() {
    log_info "Removing installation..."

    if [ -d "$INSTALL_DIR" ]; then
        rm -rf "$INSTALL_DIR"
        log_success "Removed $INSTALL_DIR"
    else
        log_warn "$INSTALL_DIR not found (already removed?)"
    fi

    # Clean up PID files
    rm -f /var/run/helixscreen.pid 2>/dev/null || true
    rm -f /var/run/helix-splash.pid 2>/dev/null || true

    # Clean up log file
    rm -f /tmp/helixscreen.log 2>/dev/null || true
}

# Re-enable previous UI
reenable_previous_ui() {
    log_info "Looking for previous screen UI to re-enable..."

    local found_ui=false

    for ui in $PREVIOUS_UIS; do
        # Check for init.d scripts
        for initscript in /etc/init.d/S*${ui}* /opt/config/mod/.root/S*${ui}*; do
            if [ -f "$initscript" ] 2>/dev/null; then
                log_info "Found previous UI: $initscript"
                # Re-enable by making executable
                chmod +x "$initscript" 2>/dev/null || true
                # Start it
                if "$initscript" start 2>/dev/null; then
                    log_success "Re-enabled and started: $initscript"
                    found_ui=true
                else
                    log_warn "Re-enabled but failed to start: $initscript"
                    log_warn "You may need to reboot"
                    found_ui=true
                fi
            fi
        done

        # Check for systemd services
        INIT_SYSTEM=$(detect_init_system)
        if [ "$INIT_SYSTEM" = "systemd" ]; then
            if systemctl list-unit-files "${ui}.service" >/dev/null 2>&1; then
                log_info "Found previous UI (systemd): $ui"
                systemctl enable "$ui" 2>/dev/null || true
                if systemctl start "$ui" 2>/dev/null; then
                    log_success "Re-enabled and started: $ui"
                    found_ui=true
                else
                    log_warn "Re-enabled but failed to start: $ui"
                    found_ui=true
                fi
            fi
        fi
    done

    if [ "$found_ui" = false ]; then
        log_info "No previous screen UI found to re-enable"
        log_info "If you had a stock UI, a reboot may restore it"
    fi
}

# Main uninstall
main() {
    local force=false

    # Parse arguments
    while [ $# -gt 0 ]; do
        case "$1" in
            --force|-f)
                force=true
                shift
                ;;
            --help|-h)
                echo "HelixScreen Uninstaller"
                echo ""
                echo "Usage: $0 [options]"
                echo ""
                echo "Options:"
                echo "  --force, -f   Skip confirmation prompt"
                echo "  --help, -h    Show this help message"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done

    echo ""
    echo "${CYAN}========================================${NC}"
    echo "${CYAN}     HelixScreen Uninstaller${NC}"
    echo "${CYAN}========================================${NC}"
    echo ""

    # Check for root
    check_root

    # Confirm unless --force
    if [ "$force" = false ]; then
        echo "This will:"
        echo "  - Stop HelixScreen"
        echo "  - Remove $INSTALL_DIR"
        echo "  - Remove service configuration"
        echo "  - Re-enable previous screen UI (if found)"
        echo ""
        printf "Are you sure you want to continue? [y/N] "
        read -r response
        case "$response" in
            [yY][eE][sS]|[yY])
                ;;
            *)
                log_info "Uninstall cancelled"
                exit 0
                ;;
        esac
        echo ""
    fi

    # Perform uninstall
    stop_helixscreen
    remove_service
    remove_installation
    reenable_previous_ui

    echo ""
    echo "${GREEN}========================================${NC}"
    echo "${GREEN}    Uninstall Complete!${NC}"
    echo "${GREEN}========================================${NC}"
    echo ""
    log_info "HelixScreen has been removed."
    log_info "A reboot is recommended to ensure clean state."
    echo ""
}

main "$@"
