#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate synthetic telemetry test data for HelixScreen analytics.

Produces realistic-looking telemetry events matching the HelixScreen schema
and writes them as batched JSON files to a specified output directory,
organized as YYYY/MM/DD/{timestamp}-{random}.json to match R2 storage format.

Usage:
    python generate-test-data.py --output-dir .telemetry-data/events
"""

import argparse
import hashlib
import json
import os
import random
import uuid
from datetime import datetime, timedelta, timezone

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

NUM_DEVICES = 50
DATE_RANGE_DAYS = 30
START_DATE = datetime(2026, 1, 10, tzinfo=timezone.utc)

APP_VERSIONS = ["0.9.6", "0.9.7", "0.9.8"]
APP_VERSION_WEIGHTS = [0.15, 0.35, 0.50]

PLATFORMS = [
    {"platform": "rpi4", "arch": "armv7l", "cpu_model": "BCM2711", "cpu_cores": 4, "ram_total_mb": 4096},
    {"platform": "rpi4", "arch": "aarch64", "cpu_model": "BCM2711", "cpu_cores": 4, "ram_total_mb": 8192},
    {"platform": "rpi5", "arch": "aarch64", "cpu_model": "BCM2712", "cpu_cores": 4, "ram_total_mb": 8192},
    {"platform": "x86_64", "arch": "x86_64", "cpu_model": "N100", "cpu_cores": 4, "ram_total_mb": 16384},
    {"platform": "x86_64", "arch": "x86_64", "cpu_model": "i5-1235U", "cpu_cores": 12, "ram_total_mb": 32768},
]
PLATFORM_WEIGHTS = [0.35, 0.20, 0.20, 0.15, 0.10]

PRINTER_PROFILES = [
    {
        "detected_model": "Voron 2.4",
        "kinematics": "corexy",
        "build_volume": "350x350x340",
        "mcu": "stm32f446",
        "mcu_count": 2,
        "extruder_count": 1,
        "has_heated_bed": True,
        "has_chamber": True,
        "features": ["bed_mesh", "qgl", "probe", "heated_bed", "firmware_retraction",
                      "exclude_object", "chamber_heater"],
    },
    {
        "detected_model": "Voron 2.4",
        "kinematics": "corexy",
        "build_volume": "250x250x230",
        "mcu": "stm32f446",
        "mcu_count": 1,
        "extruder_count": 1,
        "has_heated_bed": True,
        "has_chamber": False,
        "features": ["bed_mesh", "qgl", "probe", "heated_bed", "firmware_retraction",
                      "exclude_object"],
    },
    {
        "detected_model": "Voron Trident",
        "kinematics": "corexy",
        "build_volume": "300x300x250",
        "mcu": "stm32f446",
        "mcu_count": 1,
        "extruder_count": 1,
        "has_heated_bed": True,
        "has_chamber": False,
        "features": ["bed_mesh", "z_tilt", "probe", "heated_bed", "firmware_retraction",
                      "exclude_object"],
    },
    {
        "detected_model": "Ender 3 V2",
        "kinematics": "cartesian",
        "build_volume": "220x220x250",
        "mcu": "stm32f103",
        "mcu_count": 1,
        "extruder_count": 1,
        "has_heated_bed": True,
        "has_chamber": False,
        "features": ["bed_mesh", "probe", "heated_bed"],
    },
    {
        "detected_model": "Prusa MK3S",
        "kinematics": "cartesian",
        "build_volume": "250x210x210",
        "mcu": "atmega2560",
        "mcu_count": 1,
        "extruder_count": 1,
        "has_heated_bed": True,
        "has_chamber": False,
        "features": ["bed_mesh", "probe", "heated_bed", "firmware_retraction"],
    },
    {
        "detected_model": "Bambu X1C",
        "kinematics": "corexy",
        "build_volume": "256x256x256",
        "mcu": "stm32f407",
        "mcu_count": 1,
        "extruder_count": 1,
        "has_heated_bed": True,
        "has_chamber": True,
        "features": ["bed_mesh", "probe", "heated_bed", "firmware_retraction",
                      "exclude_object", "chamber_heater"],
    },
    {
        "detected_model": "Ratrig VCore 3",
        "kinematics": "corexy",
        "build_volume": "300x300x300",
        "mcu": "stm32f446",
        "mcu_count": 1,
        "extruder_count": 1,
        "has_heated_bed": True,
        "has_chamber": False,
        "features": ["bed_mesh", "z_tilt", "probe", "heated_bed", "firmware_retraction",
                      "exclude_object"],
    },
    {
        "detected_model": "Creality K1",
        "kinematics": "corexy",
        "build_volume": "220x220x250",
        "mcu": "stm32f402",
        "mcu_count": 1,
        "extruder_count": 1,
        "has_heated_bed": True,
        "has_chamber": False,
        "features": ["bed_mesh", "probe", "heated_bed", "firmware_retraction",
                      "exclude_object", "input_shaper"],
    },
]
PRINTER_WEIGHTS = [0.15, 0.10, 0.12, 0.15, 0.10, 0.12, 0.10, 0.16]

DISPLAYS = [
    {"display": "800x480", "display_backend": "DRM", "input_type": "touch", "has_backlight": True, "has_hw_blank": False},
    {"display": "1024x600", "display_backend": "DRM", "input_type": "touch", "has_backlight": True, "has_hw_blank": True},
    {"display": "1920x1080", "display_backend": "X11", "input_type": "mouse", "has_backlight": False, "has_hw_blank": False},
    {"display": "480x320", "display_backend": "fbdev", "input_type": "touch", "has_backlight": True, "has_hw_blank": False},
    {"display": "800x480", "display_backend": "fbdev", "input_type": "touch", "has_backlight": True, "has_hw_blank": False},
]
DISPLAY_WEIGHTS = [0.40, 0.20, 0.15, 0.10, 0.15]

LOCALES = ["en", "de", "fr", "es", "zh", "ja", "ko", "pt", "it", "nl"]
LOCALE_WEIGHTS = [0.50, 0.12, 0.08, 0.07, 0.06, 0.04, 0.03, 0.04, 0.03, 0.03]

THEMES = ["dark", "light"]
THEME_WEIGHTS = [0.80, 0.20]

KLIPPER_VERSIONS = ["v0.12.0", "v0.12.0-120-gabcdef1", "v0.11.0-300-g1234567"]
KLIPPER_WEIGHTS = [0.50, 0.35, 0.15]

MOONRAKER_VERSIONS = ["0.8.0", "0.9.0-dev", "0.8.0-134-gabcd"]
MOONRAKER_WEIGHTS = [0.45, 0.30, 0.25]

OS_OPTIONS = ["Raspberry Pi OS 12", "Raspberry Pi OS 11", "Armbian 24.2", "Debian 12", "Ubuntu 24.04"]
OS_WEIGHTS_RPI = [0.60, 0.25, 0.10, 0.03, 0.02]
OS_WEIGHTS_X86 = [0.02, 0.02, 0.06, 0.50, 0.40]

FILAMENT_TYPES = ["PLA", "PETG", "ABS", "ASA", "TPU", "PLA+", "Nylon"]
FILAMENT_WEIGHTS = [0.35, 0.25, 0.15, 0.10, 0.05, 0.07, 0.03]

# Nozzle / bed temps per filament
FILAMENT_TEMPS = {
    "PLA":   {"nozzle": (195, 215), "bed": (55, 65)},
    "PETG":  {"nozzle": (225, 245), "bed": (70, 85)},
    "ABS":   {"nozzle": (240, 260), "bed": (95, 110)},
    "ASA":   {"nozzle": (240, 260), "bed": (95, 110)},
    "TPU":   {"nozzle": (220, 240), "bed": (40, 60)},
    "PLA+":  {"nozzle": (200, 220), "bed": (55, 65)},
    "Nylon": {"nozzle": (250, 270), "bed": (70, 90)},
}

CRASH_SIGNALS = [
    {"signal": 11, "signal_name": "SIGSEGV"},
    {"signal": 6, "signal_name": "SIGABRT"},
]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def weighted_choice(options, weights):
    """Pick a random item using weights."""
    return random.choices(options, weights=weights, k=1)[0]


def make_device_id(seed: int) -> str:
    """Generate a deterministic 64-char hex device ID from a seed."""
    return hashlib.sha256(f"helixscreen-device-{seed}".encode()).hexdigest()


def random_timestamp(base: datetime, max_offset_sec: int) -> datetime:
    """Return a random timestamp within max_offset_sec of base."""
    offset = random.randint(0, max_offset_sec)
    return base + timedelta(seconds=offset)


def random_backtrace() -> list:
    """Generate a realistic-looking backtrace of 4-12 hex addresses."""
    length = random.randint(4, 12)
    base = random.randint(0x400000, 0x500000)
    return [f"0x{base + i * random.randint(16, 512):x}" for i in range(length)]


# ---------------------------------------------------------------------------
# Device profile generation
# ---------------------------------------------------------------------------

def create_device(seed: int) -> dict:
    """Create a stable device profile that stays consistent across sessions."""
    rng = random.Random(seed)

    device_id = make_device_id(seed)
    platform_info = rng.choices(PLATFORMS, weights=PLATFORM_WEIGHTS, k=1)[0]
    printer = rng.choices(PRINTER_PROFILES, weights=PRINTER_WEIGHTS, k=1)[0]
    display_info = rng.choices(DISPLAYS, weights=DISPLAY_WEIGHTS, k=1)[0]

    is_rpi = platform_info["platform"].startswith("rpi")
    os_weights = OS_WEIGHTS_RPI if is_rpi else OS_WEIGHTS_X86
    os_name = rng.choices(OS_OPTIONS, weights=os_weights, k=1)[0]

    locale = rng.choices(LOCALES, weights=LOCALE_WEIGHTS, k=1)[0]
    theme = rng.choices(THEMES, weights=THEME_WEIGHTS, k=1)[0]

    # Device starts on an app version and may upgrade during the period
    initial_version_idx = rng.choices(range(len(APP_VERSIONS)),
                                       weights=APP_VERSION_WEIGHTS, k=1)[0]

    return {
        "device_id": device_id,
        "platform": platform_info,
        "printer": printer,
        "display": display_info,
        "os": os_name,
        "locale": locale,
        "theme": theme,
        "initial_version_idx": initial_version_idx,
        "klipper_version": rng.choices(KLIPPER_VERSIONS, weights=KLIPPER_WEIGHTS, k=1)[0],
        "moonraker_version": rng.choices(MOONRAKER_VERSIONS, weights=MOONRAKER_WEIGHTS, k=1)[0],
        # How active this device is (sessions per 30 days)
        "activity_level": rng.choice([1, 2, 3, 4, 5, 8, 12, 20, 30]),
    }


def get_app_version(device: dict, day_offset: int) -> str:
    """Determine app version for a device at a given day offset.

    Devices may upgrade partway through the 30-day window.
    """
    idx = device["initial_version_idx"]
    # Some devices upgrade around day 15
    if day_offset > 15 and idx < len(APP_VERSIONS) - 1 and random.random() < 0.4:
        idx += 1
    return APP_VERSIONS[min(idx, len(APP_VERSIONS) - 1)]


# ---------------------------------------------------------------------------
# Event builders
# ---------------------------------------------------------------------------

def build_session_event(device: dict, timestamp: datetime) -> dict:
    """Build a session telemetry event."""
    day_offset = (timestamp - START_DATE).days
    version = get_app_version(device, day_offset)
    p = device["platform"]
    pr = device["printer"]
    d = device["display"]

    return {
        "schema_version": 2,
        "event": "session",
        "device_id": device["device_id"],
        "timestamp": timestamp.strftime("%Y-%m-%dT%H:%M:%SZ"),
        "app": {
            "version": version,
            "platform": p["platform"],
            "display": d["display"],
            "display_backend": d["display_backend"],
            "input_type": d["input_type"],
            "locale": device["locale"],
            "theme": device["theme"],
            "has_backlight": d["has_backlight"],
            "has_hw_blank": d["has_hw_blank"],
        },
        "host": {
            "arch": p["arch"],
            "cpu_model": p["cpu_model"],
            "cpu_cores": p["cpu_cores"],
            "ram_total_mb": p["ram_total_mb"],
            "os": device["os"],
        },
        "printer": {
            "kinematics": pr["kinematics"],
            "build_volume": pr["build_volume"],
            "mcu": pr["mcu"],
            "mcu_count": pr["mcu_count"],
            "extruder_count": pr["extruder_count"],
            "has_heated_bed": pr["has_heated_bed"],
            "has_chamber": pr["has_chamber"],
            "klipper_version": device["klipper_version"],
            "moonraker_version": device["moonraker_version"],
            "detected_model": pr["detected_model"],
        },
        "features": list(pr["features"]),
    }


def build_print_outcome_event(device: dict, timestamp: datetime) -> dict:
    """Build a print outcome telemetry event."""
    # Determine outcome with ~77% success rate
    roll = random.random()
    if roll < 0.77:
        outcome = "success"
        phases_completed = 10
    elif roll < 0.90:
        outcome = "failed"
        phases_completed = random.randint(3, 8)
    else:
        outcome = "cancelled"
        phases_completed = random.randint(1, 7)

    filament = weighted_choice(FILAMENT_TYPES, FILAMENT_WEIGHTS)
    temps = FILAMENT_TEMPS[filament]

    # Duration: 15 min to 24 hours, with most prints 1-6 hours
    if outcome == "success":
        duration = random.gauss(10800, 5400)  # mean 3h, stddev 1.5h
        duration = max(900, min(86400, duration))
    else:
        # Failed/cancelled prints tend to be shorter
        duration = random.gauss(3600, 2400)
        duration = max(300, min(43200, duration))

    # Filament used correlates with duration
    filament_mm = duration * random.uniform(1.5, 3.0)

    return {
        "schema_version": 2,
        "event": "print_outcome",
        "device_id": device["device_id"],
        "timestamp": timestamp.strftime("%Y-%m-%dT%H:%M:%SZ"),
        "outcome": outcome,
        "duration_sec": round(duration, 1),
        "phases_completed": phases_completed,
        "filament_used_mm": round(filament_mm, 1),
        "filament_type": filament,
        "nozzle_temp": round(random.uniform(*temps["nozzle"]), 1),
        "bed_temp": round(random.uniform(*temps["bed"]), 1),
    }


def build_crash_event(device: dict, timestamp: datetime) -> dict:
    """Build a crash telemetry event."""
    day_offset = (timestamp - START_DATE).days
    version = get_app_version(device, day_offset)

    # SIGSEGV is more common than SIGABRT
    sig = random.choices(CRASH_SIGNALS, weights=[0.75, 0.25], k=1)[0]

    uptime = random.uniform(60, 86400)  # 1 minute to 24 hours

    return {
        "schema_version": 2,
        "event": "crash",
        "device_id": device["device_id"],
        "timestamp": timestamp.strftime("%Y-%m-%dT%H:%M:%SZ"),
        "signal": sig["signal"],
        "signal_name": sig["signal_name"],
        "app_version": version,
        "uptime_sec": round(uptime, 1),
        "backtrace": random_backtrace(),
    }


# ---------------------------------------------------------------------------
# Main generation logic
# ---------------------------------------------------------------------------

def generate_all_events(devices: list) -> list:
    """Generate all telemetry events for the device fleet over 30 days."""
    all_events = []
    total_seconds = DATE_RANGE_DAYS * 86400

    for device in devices:
        num_sessions = device["activity_level"]

        # Generate session events spread across the 30-day window
        for _ in range(num_sessions):
            ts = random_timestamp(START_DATE, total_seconds)
            all_events.append(build_session_event(device, ts))

            # Each session likely has some prints (0-5)
            num_prints = random.choices([0, 1, 2, 3, 4, 5],
                                         weights=[0.10, 0.30, 0.30, 0.15, 0.10, 0.05],
                                         k=1)[0]
            for p in range(num_prints):
                # Print happens some time after session start
                print_offset = random.randint(600, 28800)  # 10 min to 8 hours
                print_ts = ts + timedelta(seconds=print_offset)
                # Clamp to date range
                end_date = START_DATE + timedelta(days=DATE_RANGE_DAYS)
                if print_ts < end_date:
                    all_events.append(build_print_outcome_event(device, print_ts))

    # Generate 5-10 crash events across random devices
    num_crashes = random.randint(5, 10)
    crash_devices = random.choices(devices, k=num_crashes)
    for device in crash_devices:
        ts = random_timestamp(START_DATE, total_seconds)
        all_events.append(build_crash_event(device, ts))

    # Sort all events by timestamp
    all_events.sort(key=lambda e: e["timestamp"])
    return all_events


def write_batched_files(events: list, output_dir: str) -> int:
    """Write events as batched JSON files in YYYY/MM/DD/ directory structure.

    Returns the number of files written.
    """
    file_count = 0
    idx = 0

    while idx < len(events):
        # Batch size: 1-20 events
        batch_size = random.randint(1, 20)
        batch = events[idx:idx + batch_size]
        idx += batch_size

        # Use the first event's timestamp for the directory path
        ts_str = batch[0]["timestamp"]
        ts = datetime.strptime(ts_str, "%Y-%m-%dT%H:%M:%SZ")

        date_dir = os.path.join(output_dir, ts.strftime("%Y/%m/%d"))
        os.makedirs(date_dir, exist_ok=True)

        # Filename: {epoch}-{random}.json
        epoch_ms = int(ts.timestamp() * 1000)
        rand_suffix = uuid.uuid4().hex[:8]
        filename = f"{epoch_ms}-{rand_suffix}.json"
        filepath = os.path.join(date_dir, filename)

        with open(filepath, "w") as f:
            json.dump(batch, f, indent=2)
            f.write("\n")

        file_count += 1

    return file_count


def main():
    parser = argparse.ArgumentParser(
        description="Generate synthetic HelixScreen telemetry test data"
    )
    parser.add_argument(
        "--output-dir",
        default=".telemetry-data/events",
        help="Output directory for generated JSON files (default: .telemetry-data/events)",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Random seed for reproducible output (default: 42)",
    )
    args = parser.parse_args()

    random.seed(args.seed)

    print(f"Generating telemetry data for {NUM_DEVICES} devices over {DATE_RANGE_DAYS} days...")
    print(f"Date range: {START_DATE.date()} to {(START_DATE + timedelta(days=DATE_RANGE_DAYS)).date()}")

    # Create device fleet
    devices = [create_device(i) for i in range(NUM_DEVICES)]

    # Show fleet summary
    models = {}
    for d in devices:
        model = d["printer"]["detected_model"]
        models[model] = models.get(model, 0) + 1
    print(f"\nDevice fleet:")
    for model, count in sorted(models.items(), key=lambda x: -x[1]):
        print(f"  {model}: {count}")

    # Generate events
    events = generate_all_events(devices)

    # Count event types
    event_counts = {}
    for e in events:
        t = e["event"]
        event_counts[t] = event_counts.get(t, 0) + 1

    print(f"\nGenerated events:")
    for etype, count in sorted(event_counts.items()):
        print(f"  {etype}: {count}")
    print(f"  total: {len(events)}")

    # Write to disk
    output_dir = os.path.abspath(args.output_dir)
    file_count = write_batched_files(events, output_dir)

    print(f"\nWrote {file_count} JSON files to {output_dir}")


if __name__ == "__main__":
    main()
