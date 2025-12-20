# SPDX-License-Identifier: GPL-3.0-or-later
#
# HelixScreen - Pre-rendered Image Generation Module
#
# Pre-renders PNG images to LVGL binary format for each supported screen size.
# This eliminates runtime PNG decoding, dramatically improving startup performance
# on embedded devices (2 FPS → 116 FPS on AD5M).
#
# Images are generated to the BUILD directory (not committed to repo).
# They are created automatically during embedded builds and deployments.
#
# Platform-specific targets:
#   gen-images-ad5m  - Generate only 800x480 (AD5M fixed display)
#   gen-images-pi    - Generate all sizes (Pi variable displays)
#   gen-images       - Generate all sizes (generic)
#
# Output: build/assets/images/prerendered/*.bin
#
# See: docs/PRE_RENDERED_IMAGES.md

PRERENDERED_DIR := $(BUILD_DIR)/assets/images/prerendered
REGEN_IMAGES_SCRIPT := scripts/regen_images.sh

# Pre-rendered image files (build artifacts, not in repo)
# AD5M only needs 'small' (800x480)
PRERENDERED_IMAGES_AD5M := $(PRERENDERED_DIR)/splash-logo-small.bin

# Pi needs all sizes (unknown display at build time)
PRERENDERED_IMAGES_ALL := \
    $(PRERENDERED_DIR)/splash-logo-tiny.bin \
    $(PRERENDERED_DIR)/splash-logo-small.bin \
    $(PRERENDERED_DIR)/splash-logo-medium.bin \
    $(PRERENDERED_DIR)/splash-logo-large.bin

# Generate images for AD5M (800x480 fixed display only)
.PHONY: gen-images-ad5m
gen-images-ad5m: | $(BUILD_DIR)
	$(ECHO) "$(CYAN)Generating pre-rendered images for AD5M (800x480)...$(RESET)"
	$(Q)OUTPUT_DIR=$(PRERENDERED_DIR) TARGET_SIZES=small ./$(REGEN_IMAGES_SCRIPT)
	$(ECHO) "$(GREEN)✓ AD5M images generated$(RESET)"

# Generate images for Pi (all sizes for variable displays)
.PHONY: gen-images-pi
gen-images-pi: | $(BUILD_DIR)
	$(ECHO) "$(CYAN)Generating pre-rendered images for Pi (all sizes)...$(RESET)"
	$(Q)OUTPUT_DIR=$(PRERENDERED_DIR) ./$(REGEN_IMAGES_SCRIPT)
	$(ECHO) "$(GREEN)✓ Pi images generated$(RESET)"

# Generate all pre-rendered images (generic)
.PHONY: gen-images
gen-images: | $(BUILD_DIR)
	$(ECHO) "$(CYAN)Generating pre-rendered images (all sizes)...$(RESET)"
	$(Q)OUTPUT_DIR=$(PRERENDERED_DIR) ./$(REGEN_IMAGES_SCRIPT)
	$(ECHO) "$(GREEN)✓ Pre-rendered images generated in $(PRERENDERED_DIR)/$(RESET)"

# Legacy alias
.PHONY: regen-images
regen-images: gen-images

# Clean generated images
.PHONY: clean-images
clean-images:
	$(ECHO) "$(CYAN)Cleaning pre-rendered images...$(RESET)"
	$(Q)rm -rf $(PRERENDERED_DIR)
	$(ECHO) "$(GREEN)✓ Cleaned $(PRERENDERED_DIR)$(RESET)"

# List what would be generated
.PHONY: list-images
list-images:
	$(Q)./$(REGEN_IMAGES_SCRIPT) --list

# Check if pre-rendered images exist in build directory
.PHONY: check-images
check-images:
	$(ECHO) "$(CYAN)Checking pre-rendered images...$(RESET)"
	$(Q)missing=0; \
	for img in $(PRERENDERED_IMAGES); do \
		if [ ! -f "$$img" ]; then \
			echo "$(RED)✗ Missing: $$img$(RESET)"; \
			missing=1; \
		fi; \
	done; \
	if [ $$missing -eq 1 ]; then \
		echo "$(RED)Run 'make gen-images' to generate missing files$(RESET)"; \
		exit 1; \
	else \
		echo "$(GREEN)✓ All pre-rendered images present$(RESET)"; \
	fi

# Help text for image targets
.PHONY: help-images
help-images:
	@echo "Pre-rendered image targets:"
	@echo "  gen-images    - Generate pre-rendered .bin files to build/"
	@echo "  clean-images  - Remove generated .bin files"
	@echo "  list-images   - Show what images would be generated"
	@echo "  check-images  - Verify all pre-rendered images exist"
	@echo ""
	@echo "Output directory: $(PRERENDERED_DIR)/"
	@echo ""
	@echo "Note: Generated images are build artifacts, not committed to repo."
	@echo "      They are created during deploy-* and release builds."
	@echo ""
	@echo "Performance impact:"
	@echo "  PNG decoding:    ~2 FPS during splash (AD5M)"
	@echo "  Pre-rendered:    ~116 FPS (instant display)"
