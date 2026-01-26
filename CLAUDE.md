# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Super Dial 电机旋钮屏 - An ESP32-S3 based smart haptic feedback rotary knob controller with circular LCD display. Inspired by Microsoft Surface Dial and Smart Knob. Features motor-driven haptic feedback, USB/Bluetooth HID, WiFi web configuration, and LVGL-based UI.

## Build Commands

This project uses ESP-IDF (v5.0+). All commands run from the `idf_code/` directory:

```bash
idf.py build          # Build the project
idf.py flash          # Flash to device
idf.py monitor        # Monitor serial output
idf.py flash monitor  # Flash and monitor combined
```

Target chip: ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM)

## Architecture

### Hardware Stack
- **Display**: GC9A01 240x240 circular LCD (SPI)
- **Motor**: 3205B brushless motor with FOC control via EG2133 driver
- **Encoder**: MT6701 magnetic encoder (SPI mode)
- **3 PCBs**: Screen board → Motor driver board → Main control board (connected via 8-pin 0.8mm cable through motor center)

### Software Subsystems

1. **Motor Control (FOC)**: `components/foc_knob/` wraps `espressif__esp_simplefoc`. Provides configurable haptic detents using MT6701 encoder feedback and PID control.

2. **Display/UI (LVGL)**: LVGL v8.4 with SquareLine Studio exported UI in `main/ui/`. Uses `esp_lvgl_port` for ESP LCD integration. Physical input comes from motor rotation.

3. **USB HID (TinyUSB)**: Emulates Surface Dial, keyboard (volume/media), mouse scroll, and generic HID. HID stays initialized after first enable to avoid re-enumeration issues.

4. **Bluetooth HID**: Multi-device reporting (keyboard, mouse, Surface Dial, media). Has known complexity with 4-device simultaneous reporting - solved by using alternative BLE HID demo for Surface Dial descriptor.

5. **WiFi Web Server**: AP mode for initial setup, STA mode for local network. Web interface at 192.168.4.1 (AP) or assigned IP (STA) for custom key bindings, image upload, haptic customization, and MQTT integration.

6. **SPIFFS**: Stores UI assets and configuration. First boot takes ~30 seconds for filesystem initialization (shows white screen).

### Key Source Locations

- `idf_code/main/main.c` - Application entry point
- `idf_code/main/ui/` - LVGL UI files (SquareLine Studio export)
- `idf_code/components/foc_knob/` - Motor knob control logic
- `idf_code/components/espressif__esp_simplefoc/` - FOC motor control library

### Configuration

- `idf_code/sdkconfig` - ESP-IDF SDK configuration
- `idf_code/main/idf_component.yml` - Component dependencies
- LVGL requires: RGB565 color format, byte swap enabled, rotation configured in `lvgl_port_display_cfg_t`

## Technical Constraints

- WiFi only supports 2.4GHz (not 5GHz or dual-band networks)
- USB requires USB 3.0 Type-C cable, 5V 1A minimum
- Motor encoder upgraded from I2C to SPI mode for better performance
- WiFi advanced features require verification code from author (authorization system)
