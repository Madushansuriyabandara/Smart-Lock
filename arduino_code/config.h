#ifndef CONFIG_H
#define CONFIG_H

// --- HARDWARE PINS ---
// Change these here if you change your wiring
const int PIN_SERVO   = 10;
const int PIN_BUZZER  = 11;
const int PIN_BTN     = 12; // Door Sensor (Reed Switch)
const int PIN_LED_RED = 13;
const int PIN_LED_GRN = A1; 
const int PIN_VIB     = A0; // Vibration Sensor

// --- KEYPAD CONFIGURATION ---
const byte ROW_PINS[4] = {9, 8, 7, 6}; 
const byte COL_PINS[4] = {5, 4, 3, 2}; 

// --- SECURITY SECRETS ---
// Update these codes to secure your physical lock
const String MASTER_CODE      = "1234"; // Code to Unlock
const String ALARM_RESET_CODE = "A";    // Code to Silence Alarm

// --- TUNING PARAMETERS ---
const int VIB_THRESHOLD       = 900;    // Sensitivity (0-1023)
const int COOLDOWN_MS         = 300;    // Debounce time
const int RESET_WINDOW_MS     = 10000;  // Time to reset strike count
const int ALARM_TRIGGER_COUNT = 5;      // Strikes needed to trigger

#endif