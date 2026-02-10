# SPDX-License-Identifier: GPL-3.0-or-later
# HelixScreen KIAUH Extension
#
# Provides install/update/remove functionality for HelixScreen through KIAUH.
# This extension wraps the bundled installer script for consistent behavior.

from __future__ import annotations

import subprocess
from pathlib import Path
from typing import TYPE_CHECKING, Dict

from extensions.base_extension import BaseExtension
from core.logger import Logger
from utils.input_utils import get_confirm

if TYPE_CHECKING:
    pass

from . import (
    HELIXSCREEN_INSTALLER_URL,
    HELIXSCREEN_SERVICE_NAME,
    find_install_dir,
)


class HelixscreenExtension(BaseExtension):
    """KIAUH extension for HelixScreen touchscreen UI."""

    def __init__(self, metadata: Dict[str, str]) -> None:
        super().__init__(metadata)
        self.logger = Logger()

    def install_extension(self, **kwargs) -> None:
        """Install HelixScreen using the bundled installer."""
        self.logger.print_status("Installing HelixScreen...")

        # Check if already installed
        install_dir = find_install_dir()
        if install_dir:
            self.logger.print_warn(
                "HelixScreen appears to be already installed at "
                f"{install_dir}"
            )
            if not get_confirm("Reinstall HelixScreen?", default=False):
                return

        # Download and run the installer
        try:
            self._run_installer()
            self.logger.print_ok("HelixScreen installed successfully!")
            self._print_post_install_info()
        except subprocess.CalledProcessError as e:
            self.logger.print_error(f"Installation failed with exit code {e.returncode}")
            raise
        except Exception as e:
            self.logger.print_error(f"Installation failed: {e}")
            raise

    def update_extension(self, **kwargs) -> None:
        """Update HelixScreen to the latest version."""
        self.logger.print_status("Updating HelixScreen...")

        if not self._is_installed():
            self.logger.print_error(
                "HelixScreen is not installed. Please install it first."
            )
            return

        # Use the installer with --update flag
        try:
            self._run_installer(update=True)
            self.logger.print_ok("HelixScreen updated successfully!")
        except subprocess.CalledProcessError as e:
            self.logger.print_error(f"Update failed with exit code {e.returncode}")
            raise
        except Exception as e:
            self.logger.print_error(f"Update failed: {e}")
            raise

    def remove_extension(self, **kwargs) -> None:
        """Remove HelixScreen."""
        self.logger.print_status("Removing HelixScreen...")

        if not self._is_installed():
            self.logger.print_warn("HelixScreen does not appear to be installed.")
            return

        if not get_confirm(
            "This will remove HelixScreen and restore your previous screen UI. Continue?",
            default=False,
        ):
            return

        # Use the installer with --uninstall flag
        try:
            self._run_installer(uninstall=True)
            self.logger.print_ok("HelixScreen removed successfully!")
            self.logger.print_info("Your previous screen UI has been restored.")
            self.logger.print_info("A reboot may be required for changes to take effect.")
        except subprocess.CalledProcessError as e:
            self.logger.print_error(f"Removal failed with exit code {e.returncode}")
            raise
        except Exception as e:
            self.logger.print_error(f"Removal failed: {e}")
            raise

    def _is_installed(self) -> bool:
        """Check if HelixScreen is installed."""
        return find_install_dir() is not None

    def _run_installer(
        self, update: bool = False, uninstall: bool = False
    ) -> None:
        """
        Download and run the HelixScreen bundled installer.

        Args:
            update: If True, pass --update flag
            uninstall: If True, pass --uninstall flag
        """
        # Build the command - pipe curl to sh
        curl_cmd = ["curl", "-sSL", HELIXSCREEN_INSTALLER_URL]
        sh_cmd = ["sh"]

        # Add flags as needed
        if update:
            sh_cmd.extend(["-s", "--", "--update"])
        elif uninstall:
            sh_cmd.extend(["-s", "--", "--uninstall"])

        # Download installer
        self.logger.print_status("Downloading installer...")
        curl_result = subprocess.run(
            curl_cmd,
            capture_output=True,
            check=True,
        )

        # Run installer
        self.logger.print_status(
            "Running installer" + (" (update)" if update else " (uninstall)" if uninstall else "") + "..."
        )
        subprocess.run(
            sh_cmd,
            input=curl_result.stdout,
            check=True,
        )

    def _print_post_install_info(self) -> None:
        """Print helpful information after installation."""
        self.logger.print_info("")
        self.logger.print_info("HelixScreen is now running on your touchscreen!")
        self.logger.print_info("")
        self.logger.print_info("Useful commands:")
        # Check if systemd is available (Pi) or SysV init (K1/AD5M)
        if Path("/run/systemd/system").exists():
            self.logger.print_info(f"  systemctl status {HELIXSCREEN_SERVICE_NAME}")
            self.logger.print_info(f"  journalctl -u {HELIXSCREEN_SERVICE_NAME} -f")
        else:
            self.logger.print_info("  cat /tmp/helixscreen.log")
            self.logger.print_info("  /etc/init.d/S*helixscreen status")
        self.logger.print_info("")
        self.logger.print_info("For more information, visit:")
        self.logger.print_info("  https://github.com/prestonbrown/helixscreen")
