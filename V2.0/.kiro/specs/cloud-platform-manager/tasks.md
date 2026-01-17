# Implementation Tasks

## Task 1: QR Code Panel Layout Redesign ✅
- [x] Add layout constants (QR_CODE_SIZE=110, QR_CODE_SPACING=15, etc.)
- [x] Create flex container for QR codes with proper spacing
- [x] Set consistent QR code container size (110x110)
- [x] Add 15px spacing between QR codes using flex layout
- [x] Center QR codes horizontally in panel

## Task 2: QR Code Labels Alignment ✅
- [x] Create label row container with flex layout
- [x] Align labels with corresponding QR codes
- [x] Set consistent label width matching QR code width
- [x] Center text within each label

## Task 3: Button Row Redesign ✅
- [x] Create button row container aligned with QR code total width
- [x] Start Server button spans 2 QR codes width + spacing
- [x] Help button matches single QR code width
- [x] Use flex layout for proper spacing

## Task 4: Help Popup Redesign ✅
- [x] Add semi-transparent background overlay (50% opacity)
- [x] Create popup card with fixed size (380x320) fitting 800x480 screen
- [x] Apply SD card manager style (border radius 16px, white background)
- [x] Add red circular close button (32x32) at top right
- [x] Create scrollable content area with vertical scrollbar
- [x] Center title at top of popup card

## Task 5: Background Color Consistency ✅
- [x] Set panel background to COLOR_BG (0xF5F5F5)
- [x] Set background opacity to LV_OPA_COVER
- [x] Add COLOR_DIVIDER constant for border styling

## Task 6: Cleanup and Destroy ✅
- [x] Add help popup destruction in cloud_manager_ui_destroy()
- [x] Add help_popup_close_cb callback function

## Summary
All UI redesign tasks completed:
- QR codes now have 15px spacing with flex layout
- Buttons aligned with QR code boundaries
- Help popup follows SD card manager style with scrollable content
- Consistent gray background across entire panel
