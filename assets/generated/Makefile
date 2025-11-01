# Directory where all assets will go
ASSETS_DIR := .
IMG2WEBP ?= img2webp
CWEBP ?= cwebp
CONVERT ?= magick
WEBPMUX ?= webpmux

# Default target
all: $(ASSETS_DIR)/4x4_anim.webp \
     $(ASSETS_DIR)/1x1.webp \
     $(ASSETS_DIR)/valid_4x4.webp \
     $(ASSETS_DIR)/truncated.webp \
     $(ASSETS_DIR)/invalid_empty.webp \
     $(ASSETS_DIR)/invalid_garbage.webp

# --- Base frames for animation ---
$(ASSETS_DIR)/frame1.png:
	$(CONVERT) -size 4x4 xc:red $@

$(ASSETS_DIR)/frame2.png:
	$(CONVERT) -size 4x4 xc:green $@

$(ASSETS_DIR)/frame3.png:
	$(CONVERT) -size 4x4 xc:blue $@

# --- Animated WebP (3 frames) ---
$(ASSETS_DIR)/4x4_anim.webp: $(ASSETS_DIR)/frame1.png $(ASSETS_DIR)/frame2.png $(ASSETS_DIR)/frame3.png
	$(IMG2WEBP) -d 100 $(ASSETS_DIR)/frame1.png \
	            -d 250 $(ASSETS_DIR)/frame2.png \
	            -d 400 $(ASSETS_DIR)/frame3.png \
	            -o $@

# --- Valid still WebP (4x4 red) ---
$(ASSETS_DIR)/valid_4x4.webp: $(ASSETS_DIR)/valid_4x4.png
	$(CWEBP) $< -o $@

$(ASSETS_DIR)/valid_4x4.png:
	$(CONVERT) -size 4x4 xc:red $@

# --- 1x1 white WebP (dimension sanity) ---
$(ASSETS_DIR)/1x1.webp: $(ASSETS_DIR)/1x1.png
	$(CWEBP) $< -o $@

$(ASSETS_DIR)/1x1.png:
	$(CONVERT) -size 1x1 xc:white $@

# --- Truncated WebP (cut halfway) ---
$(ASSETS_DIR)/truncated.webp: $(ASSETS_DIR)/valid_4x4.webp
	head -c 50 $< > $@

# --- Invalid Inputs ---
$(ASSETS_DIR)/invalid_empty.webp:
	touch $@

$(ASSETS_DIR)/invalid_garbage.webp:
	head -c 128 /dev/urandom > $@

# --- Cleanup ---
clean:
	rm -f $(ASSETS_DIR)/*.png $(ASSETS_DIR)/*.webp

.PHONY: all clean
