#pragma once

#include <stdint.h>

/* Minimal HID constants for UI compilation */
#define HID_ITF_PROTOCOL_KEYBOARD 1
#define HID_ITF_PROTOCOL_MOUSE    2
#define HID_ITF_PROTOCOL_DIAL     3
#define HID_ITF_PROTOCOL_MEDIA    4

#define KEYBOARD_MODIFIER_LEFTCTRL 0x01

#define HID_KEY_Z            0x1D
#define HID_KEY_C            0x06
#define HID_KEY_V            0x19
#define HID_KEY_SPACE        0x2C
#define HID_KEY_ARROW_RIGHT  0x4F
#define HID_KEY_ARROW_LEFT   0x50

#define MOUSE_BUTTON_LEFT    0x01
#define MOUSE_BUTTON_RIGHT   0x02

#define HID_USAGE_CONSUMER_MUTE             0x00E2
#define HID_USAGE_CONSUMER_VOLUME_INCREMENT 0x00E9
#define HID_USAGE_CONSUMER_VOLUME_DECREMENT 0x00EA

#define DIAL_RELEASE 0
#define DIAL_PRESS   1
#define DIAL_R       2
#define DIAL_L       3
