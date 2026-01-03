#!/usr/bin/env bash
# Copyright (C) 2025-2026 356C LLC
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Trim blank/transparent space around printer images and optionally
# adjust to approximately 4:3 aspect ratio by adding padding.
#
# Usage:
#   ./scripts/trim_printer_images.sh [--dry-run] [--no-backup] [--target-ratio 1.33]
#
# Options:
#   --dry-run       Show what would be done without modifying files
#   --no-backup     Don't create backup files
#   --target-ratio  Target width:height ratio (default: 1.33 for 4:3)
#   --padding       Extra padding in pixels to add around trimmed content (default: 10)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRINTER_IMAGES_DIR="${SCRIPT_DIR}/../assets/images/printers"

# Default options
DRY_RUN=false
CREATE_BACKUP=true
TARGET_RATIO=1.33  # 4:3 aspect ratio (width/height)
RATIO_TOLERANCE=0.15  # Allow ratios between ~1.13 and ~1.53
PADDING=10  # Extra padding around content

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[OK]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --no-backup)
            CREATE_BACKUP=false
            shift
            ;;
        --target-ratio)
            TARGET_RATIO="$2"
            shift 2
            ;;
        --padding)
            PADDING="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [--dry-run] [--no-backup] [--target-ratio 1.33] [--padding 10]"
            echo ""
            echo "Trim blank space around printer images and adjust aspect ratio."
            echo ""
            echo "Options:"
            echo "  --dry-run       Show what would be done without modifying files"
            echo "  --no-backup     Don't create .bak backup files"
            echo "  --target-ratio  Target width:height ratio (default: 1.33 for 4:3)"
            echo "  --padding       Extra padding around trimmed content (default: 10)"
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Check for ImageMagick
if ! command -v magick &> /dev/null; then
    log_error "ImageMagick (magick) not found. Please install it first."
    exit 1
fi

# Process each PNG image
process_image() {
    local img="$1"
    local filename
    filename=$(basename "$img")

    # Skip README
    if [[ "$filename" == "README.md" ]]; then
        return 0
    fi

    log_info "Processing: $filename"

    # Get original dimensions
    local orig_dims
    orig_dims=$(magick "$img" -format "%wx%h" info:)
    local orig_w orig_h
    orig_w=$(echo "$orig_dims" | cut -dx -f1)
    orig_h=$(echo "$orig_dims" | cut -dx -f2)

    # Get trim geometry (what would remain after trimming)
    # Format: WxH+X+Y where X,Y is offset of content from top-left
    local trim_info
    trim_info=$(magick "$img" -trim -format "%wx%h+%X+%Y" info: 2>/dev/null || echo "")

    if [[ -z "$trim_info" ]]; then
        log_warn "  Could not analyze image, skipping"
        return 0
    fi

    # Parse trim dimensions
    local trim_w trim_h
    trim_w=$(echo "$trim_info" | sed 's/\([0-9]*\)x.*/\1/')
    trim_h=$(echo "$trim_info" | sed 's/[0-9]*x\([0-9]*\)+.*/\1/')

    echo "  Original: ${orig_w}x${orig_h}"
    echo "  Content:  ${trim_w}x${trim_h} (after trim)"

    # Add padding to trimmed dimensions
    local padded_w=$((trim_w + PADDING * 2))
    local padded_h=$((trim_h + PADDING * 2))

    # Calculate current aspect ratio of trimmed content
    local current_ratio
    current_ratio=$(echo "scale=3; $padded_w / $padded_h" | bc)

    # Determine final dimensions
    # If ratio is too tall (< target - tolerance), add width padding
    # If ratio is too wide (> target + tolerance), add height padding
    # Otherwise keep the trimmed+padded dimensions
    local final_w=$padded_w
    local final_h=$padded_h

    local ratio_low ratio_high
    ratio_low=$(echo "scale=3; $TARGET_RATIO - $RATIO_TOLERANCE" | bc)
    ratio_high=$(echo "scale=3; $TARGET_RATIO + $RATIO_TOLERANCE" | bc)

    # Check if we need to adjust aspect ratio
    if (( $(echo "$current_ratio < $ratio_low" | bc -l) )); then
        # Too tall, need to add width
        final_w=$(echo "scale=0; $padded_h * $TARGET_RATIO / 1" | bc)
        echo "  Adjusting: too tall (ratio=${current_ratio}), adding horizontal padding"
    elif (( $(echo "$current_ratio > $ratio_high" | bc -l) )); then
        # Too wide, need to add height
        final_h=$(echo "scale=0; $padded_w / $TARGET_RATIO / 1" | bc)
        echo "  Adjusting: too wide (ratio=${current_ratio}), adding vertical padding"
    else
        echo "  Ratio OK: ${current_ratio} (target: ${TARGET_RATIO}±${RATIO_TOLERANCE})"
    fi

    # Calculate final ratio
    local final_ratio
    final_ratio=$(echo "scale=3; $final_w / $final_h" | bc)

    echo "  Final:    ${final_w}x${final_h} (ratio: ${final_ratio})"

    # Check if any change is needed
    if [[ "$final_w" == "$orig_w" && "$final_h" == "$orig_h" ]]; then
        log_success "  No change needed"
        return 0
    fi

    if [[ "$DRY_RUN" == "true" ]]; then
        log_warn "  [DRY RUN] Would trim and resize"
        return 0
    fi

    # Create backup if requested
    if [[ "$CREATE_BACKUP" == "true" ]]; then
        cp "$img" "${img}.bak"
        echo "  Backup created: ${filename}.bak"
    fi

    # Process the image:
    # 1. Trim transparent/blank edges
    # 2. Add padding and center on new canvas with target dimensions
    # Using -background none to preserve transparency
    magick "$img" \
        -trim \
        -gravity center \
        -background none \
        -extent "${final_w}x${final_h}" \
        "$img"

    log_success "  Processed successfully"
}

# Main
echo "========================================"
echo "Printer Image Trimmer"
echo "========================================"
echo "Target ratio: ${TARGET_RATIO} (±${RATIO_TOLERANCE})"
echo "Padding: ${PADDING}px"
echo "Dry run: ${DRY_RUN}"
echo "Create backups: ${CREATE_BACKUP}"
echo "========================================"
echo ""

# Count images
image_count=$(find "$PRINTER_IMAGES_DIR" -maxdepth 1 -name "*.png" | wc -l | tr -d ' ')
log_info "Found ${image_count} PNG images to process"
echo ""

# Process each image
for img in "$PRINTER_IMAGES_DIR"/*.png; do
    if [[ -f "$img" ]]; then
        process_image "$img"
        echo ""
    fi
done

log_success "Done!"

if [[ "$DRY_RUN" == "true" ]]; then
    echo ""
    log_warn "This was a dry run. No files were modified."
    echo "Run without --dry-run to apply changes."
fi

if [[ "$CREATE_BACKUP" == "true" && "$DRY_RUN" != "true" ]]; then
    echo ""
    echo "Backups were created with .bak extension."
    echo "To remove backups: rm ${PRINTER_IMAGES_DIR}/*.bak"
fi
