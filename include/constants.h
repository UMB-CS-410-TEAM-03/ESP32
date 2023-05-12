#pragma once

// Wifi details
#define SSID "malav"
#define PASSWORD "bhaibhai"

// TOUCH SENSOR
#define TOUCH_SENSOR 15 // INPUT
// LCD
#define LCD_WIDTH 8
#define LCD_HEIGHT 2
#define RS 4 // OUTPUT
#define RW 0 // OUTPUT
#define ENABLE 16 // OUTPUT
#define D4 17 // OUTPUT
#define D5 5 // OUTPUT
#define D6 18 // OUTPUT
#define D7 19 // OUTPUT

// ROTARY ENCODER
#define RE_STEP_SIZE 20
#define RE_BUTTON 21 // INPUT PULL-UP RESISTOR
#define RE_CLK 22 // INPUT
#define RE_DAT 23 // INPUT
#define RE_VALUE_MAX 5
#define RE_VALUE_MIN 0

// MICRO SERVO
#define SERVO 13