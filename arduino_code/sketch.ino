#include <Keypad.h>
#include <Servo.h>
#include <ArduinoJson.h>
#include "config.h"  // Import settings from the external file

// --- KEYPAD SETUP ---
// (Rows/Cols layout remains here as it's specific to the library logic)
const byte ROWS = 4; 
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'}, 
  {'4','5','6','B'},
  {'7','8','9','C'}, 
  {'*','0','#','D'}
};

// Initialize Keypad using pins from config.h
// We cast to (byte*) because the Keypad library expects non-const arrays
Keypad keypad = Keypad(makeKeymap(keys), (byte*)ROW_PINS, (byte*)COL_PINS, ROWS, COLS);

Servo myservo;
String inputCode = "";

// State Variables
bool isLocked = true;
bool alarmActive = false;

// Logic Variables
int strikeCount = 0;            
unsigned long lastStrikeTime = 0; 
unsigned long lastUpdate = 0;
unsigned long lastAlarmBeep = 0;
int lastDoorState = -1; 

void setup() {
  Serial.begin(9600);
  myservo.attach(PIN_SERVO);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GRN, OUTPUT);
  
  lastDoorState = digitalRead(PIN_BTN);
  lockDoor("SYSTEM"); 
}

void loop() {
  // 1. KEYPAD
  char key = keypad.getKey();
  if (key) {
    if (!alarmActive) beep(50);
    if (key == '#') { checkCode(); inputCode = ""; }
    else if (key == '*') { inputCode = ""; beep(200); }
    else { inputCode += key; }
  }

  // 2. SENSORS
  checkSuspiciousVibration();
  checkDoorStatus();

  // 3. ALARM
  if (alarmActive) playWarningSound();

  // 4. REMOTE COMMANDS
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "UNLOCK") unlockDoor("REMOTE");
    if (cmd == "LOCK") lockDoor("REMOTE");
    if (cmd == "CLEAR_ALARM") clearAlarm("REMOTE");
  }

  // 5. HEARTBEAT
  if (millis() - lastUpdate > 3000) {
    sendJSON("STATUS", "UPDATE");
    lastUpdate = millis();
  }
}

// --- NEW FUNCTION: CLEAR ALARM ---
void clearAlarm(String source) {
  if (alarmActive) {
    alarmActive = false;
    strikeCount = 0;
    sendJSON("EVENT", "ALARM_CLEARED_" + source);
    beep(100); beep(100);
  }
}

void checkDoorStatus() {
  int currentDoorState = digitalRead(PIN_BTN);
  if (currentDoorState != lastDoorState) {
    delay(50); 
    if (digitalRead(PIN_BTN) == currentDoorState) {
      if (currentDoorState == HIGH) sendJSON("EVENT", "DOOR_OPEN");
      else sendJSON("EVENT", "DOOR_CLOSED");
      lastDoorState = currentDoorState; 
    }
  }
}

void checkSuspiciousVibration() {
  int vibReading = analogRead(PIN_VIB);
  unsigned long now = millis();

  if (vibReading > VIB_THRESHOLD && !alarmActive) {
    if (now - lastStrikeTime > COOLDOWN_MS) {
      strikeCount++;
      lastStrikeTime = now; 
      if (strikeCount >= ALARM_TRIGGER_COUNT) {
        alarmActive = true;
        sendJSON("EVENT", "ALARM_TRIGGERED");
        strikeCount = 0;
      }
    }
  }
  if (strikeCount > 0 && (now - lastStrikeTime > RESET_WINDOW_MS)) {
    strikeCount = 0;
  }
}

void playWarningSound() {
  if (millis() - lastAlarmBeep > 150) { 
    lastAlarmBeep = millis();
    tone(PIN_BUZZER, 3000, 100); 
  }
}

void checkCode() {
  if (inputCode == ALARM_RESET_CODE) {
    clearAlarm("KEYPAD");
    return;
  }

  if (inputCode == MASTER_CODE) {
    if (isLocked) unlockDoor("KEYPAD");
    else lockDoor("KEYPAD");
  } else { 
    beep(500); 
    sendJSON("EVENT", "AUTH_FAIL"); 
  }
}

void unlockDoor(String source) {
  myservo.write(90); 
  isLocked = false;
  digitalWrite(PIN_LED_RED, LOW); 
  digitalWrite(PIN_LED_GRN, HIGH); 
  beep(200);
  sendJSON("EVENT", "UNLOCKED_" + source);
}

void lockDoor(String source) {
  myservo.write(0);
  isLocked = true;
  digitalWrite(PIN_LED_RED, HIGH); 
  digitalWrite(PIN_LED_GRN, LOW);
  beep(100);
  sendJSON("EVENT", "LOCKED_" + source);
}

void beep(int d) { 
  tone(PIN_BUZZER, 2000, d); 
  delay(d);
}

void sendJSON(String type, String val) {
  StaticJsonDocument<200> doc;
  doc["type"] = type; 
  doc["val"] = val;   
  doc["door"] = digitalRead(PIN_BTN) == HIGH ? "OPEN" : "CLOSED";
  doc["vib"] = analogRead(PIN_VIB);
  serializeJson(doc, Serial);
  Serial.println();
}