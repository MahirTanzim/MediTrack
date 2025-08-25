#define BLYNK_TEMPLATE_ID "Template_ID"
#define BLYNK_TEMPLATE_NAME "Tamplate_name"
#define BLYNK_AUTH_TOKEN "Auth_token"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <RTClib.h>
#include <Wire.h>

// WiFi credentials
char ssid[] = "wifi_name";
char pass[] = "wifi_passward";

// RTC object
RTC_DS3231 rtc;


// Pin definitions
#define BUZZER_PIN 19

// Drawer pins (Red LED, Green LED, Reed Switch)
const int RED_LED_PINS[6] = {12, 13, 14, 25, 26, 27};
const int GREEN_LED_PINS[6] = {32, 33, 15, 16, 17, 23};
const int REED_SWITCH_PINS[6] = {2, 4, 34, 35, 36, 39};

// Medicine schedule structure
struct MedicineSchedule {
  int hour;
  int minute;
  bool enabled;
  bool taken;
  unsigned long takenTime;
};

// Medicine schedules for 6 drawers
MedicineSchedule schedules[6];

// Drawer names for logging
String drawerNames[6] = {"Morning Before", "Morning After", "Noon Before", "Noon After", "Night Before", "Night After"};

// State variables
bool drawerOpen[6] = {false};
bool alarmActive[6] = {false};
unsigned long greenLedTimer[6] = {0};
bool greenLedActive[6] = {false};

// Daily log
String dailyLog = "";
int logCount = 0;

// Virtual pin assignments
#define VPIN_MORNING_BEFORE_TIME V0
#define VPIN_MORNING_AFTER_TIME V1
#define VPIN_NOON_BEFORE_TIME V2
#define VPIN_NOON_AFTER_TIME V3
#define VPIN_NIGHT_BEFORE_TIME V4
#define VPIN_NIGHT_AFTER_TIME V5
#define VPIN_LOG_DISPLAY V6
#define VPIN_RESET_LOG V7
#define VPIN_STATUS_DISPLAY V8
#define VPIN_CURRENT_TIME_BUTTON V9
#define VPIN_CURRENT_TIME_DISPLAY V10

void setup() {
  Serial.begin(115200);
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    while (1);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // Check if RTC lost power and if so, set the time
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting the time!");
    // Set to compile time - you may want to set this manually
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  // Print current RTC time for debugging
  DateTime now = rtc.now();
  Serial.print("Current RTC time: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  
  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  for (int i = 0; i < 6; i++) {
    pinMode(RED_LED_PINS[i], OUTPUT);
    pinMode(GREEN_LED_PINS[i], OUTPUT);
    
    digitalWrite(RED_LED_PINS[i], LOW);
    digitalWrite(GREEN_LED_PINS[i], LOW);
    if(REED_SWITCH_PINS[i] >= 34 && REED_SWITCH_PINS[i] <= 39){
      pinMode(REED_SWITCH_PINS[i], INPUT);
    }else{
      pinMode(REED_SWITCH_PINS[i], INPUT_PULLUP);
    }
    
    // Initialize schedules
    schedules[i].hour = 0;
    schedules[i].minute = 0;
    schedules[i].enabled = false;
    schedules[i].taken = false;
    schedules[i].takenTime = 0;
  }
  
  // Connect to Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  Serial.println("Smart Medicine Box Started!");
}

void loop() {
  Blynk.run();
  
  DateTime now = rtc.now();
  
  // Debug: Print current time every 30 seconds
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 30000) {
    Serial.print("Current time: ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());
    lastDebugTime = millis();
  }
  
  // Check each drawer
  for (int i = 0; i < 6; i++) {
    checkMedicineTime(i, now);
    checkDrawerStatus(i);
    updateGreenLed(i);
  }
  
  delay(1000); // Check every second
}

void checkMedicineTime(int drawer, DateTime now) {
  if (!schedules[drawer].enabled) return;
  
  // Debug: Print schedule checking
  static unsigned long lastScheduleDebug[6] = {0};
  if (millis() - lastScheduleDebug[drawer] > 60000) { // Debug every minute
    Serial.println("Checking " + drawerNames[drawer] + " - Scheduled: " + 
                   String(schedules[drawer].hour) + ":" + 
                   (schedules[drawer].minute < 10 ? "0" : "") + String(schedules[drawer].minute) + 
                   ", Current: " + String(now.hour()) + ":" + 
                   (now.minute() < 10 ? "0" : "") + String(now.minute()) + 
                   ", Taken: " + String(schedules[drawer].taken ? "Yes" : "No"));
    lastScheduleDebug[drawer] = millis();
  }
  
  // Check if it's time for medicine
  if (now.hour() == schedules[drawer].hour && 
      now.minute() == schedules[drawer].minute && 
      !schedules[drawer].taken && 
      !alarmActive[drawer]) {
    
    // Activate alarm
    alarmActive[drawer] = true;
    digitalWrite(RED_LED_PINS[drawer], HIGH);
    Serial.println("MEDICINE TIME TRIGGERED for " + drawerNames[drawer]);
    
    // Send notification to Blynk
    Blynk.logEvent("medicine_reminder", "Time for " + drawerNames[drawer] + " medicine!");
  }
  
  // Reset daily status at midnight
  if (now.hour() == 0 && now.minute() == 0) {
    schedules[drawer].taken = false;
    schedules[drawer].takenTime = 0;
    Serial.println("Daily reset for " + drawerNames[drawer]);
  }
}

void checkDrawerStatus(int drawer) {
  bool currentState = digitalRead(REED_SWITCH_PINS[drawer]) == HIGH; // HIGH when open (magnet away)
  
  // Drawer opened
  if (digitalRead(REED_SWITCH_PINS[drawer]) == HIGH && !drawerOpen[drawer]) {
    drawerOpen[drawer] = true;
    
    if (alarmActive[drawer]) {
      // Medicine taken
      alarmActive[drawer] = false;
      schedules[drawer].taken = true;
      schedules[drawer].takenTime = millis();
      
      // Turn off red LED and buzzer
      digitalWrite(RED_LED_PINS[drawer], LOW);
      digitalWrite(BUZZER_PIN, LOW);
      
      // Turn on green LED
      digitalWrite(GREEN_LED_PINS[drawer], HIGH);
      greenLedActive[drawer] = true;
      greenLedTimer[drawer] = millis();
      
      // Log the event
      DateTime now = rtc.now();
      String logEntry = drawerNames[drawer] + " taken at " + 
                       String(now.hour()) + ":" + 
                       (now.minute() < 10 ? "0" : "") + String(now.minute());
      addToLog(logEntry);
      
      Serial.println(logEntry);
      
      // Update Blynk status
      updateBlynkStatus();
    }
  }
  
  // Drawer closed
  if (!currentState && drawerOpen[drawer]) {
    drawerOpen[drawer] = false;
  }
  
  // Buzzer control
  if (alarmActive[drawer]) {
    static unsigned long buzzerTimer = 0;
    static bool buzzerState = false;
    
    if (millis() - buzzerTimer > 1000) { // Beep every second
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
      buzzerTimer = millis();
    }
  }
}

void updateGreenLed(int drawer) {
  if (greenLedActive[drawer]) {
    if (millis() - greenLedTimer[drawer] > 600000) { // 10 minutes = 600000 ms
      digitalWrite(GREEN_LED_PINS[drawer], LOW);
      greenLedActive[drawer] = false;
    }
  }
}

void addToLog(String entry) {
  logCount++;
  dailyLog += String(logCount) + ". " + entry + "\n";
  
  // Keep only last 20 entries
  if (logCount > 20) {
    int firstNewline = dailyLog.indexOf('\n');
    if (firstNewline != -1) {
      dailyLog = dailyLog.substring(firstNewline + 1);
    }
  }
  
  // Update Blynk log display
  Blynk.virtualWrite(VPIN_LOG_DISPLAY, dailyLog);
}

void updateBlynkStatus() {
  String status = "Medicine Status:\n";
  DateTime now = rtc.now();
  
  for (int i = 0; i < 6; i++) {
    if (schedules[i].enabled) {
      status += drawerNames[i] + ": ";
      if (schedules[i].taken) {
        status += "✓ Taken\n";
      } else if (alarmActive[i]) {
        status += "⚠ Alarm Active\n";
      } else {
        status += "⏰ Scheduled " + String(schedules[i].hour) + ":" + 
                 (schedules[i].minute < 10 ? "0" : "") + String(schedules[i].minute) + "\n";
      }
    }
  }
  
  Blynk.virtualWrite(VPIN_STATUS_DISPLAY, status);
}

// Blynk virtual pin handlers for time input
BLYNK_WRITE(VPIN_MORNING_BEFORE_TIME) {
  parseTime(0, param.asLong());
}

BLYNK_WRITE(VPIN_MORNING_AFTER_TIME) {

  parseTime(1, param.asLong());
}

BLYNK_WRITE(VPIN_NOON_BEFORE_TIME) {
  String timeStr = param.asStr();
  parseTime(2, param.asLong());
}

BLYNK_WRITE(VPIN_NOON_AFTER_TIME) {
  String timeStr = param.asStr();
  parseTime(3, param.asLong());
}

BLYNK_WRITE(VPIN_NIGHT_BEFORE_TIME) {
  String timeStr = param.asStr();
  parseTime(4, param.asLong());
}

BLYNK_WRITE(VPIN_NIGHT_AFTER_TIME) {
  String timeStr = param.asStr();
  parseTime(5, param.asLong());
}

BLYNK_WRITE(VPIN_RESET_LOG) {
  if (param.asInt() == 1) {
    dailyLog = "";
    logCount = 0;
    
    // Reset all taken status
    for (int i = 0; i < 6; i++) {
      schedules[i].taken = false;
      schedules[i].takenTime = 0;
      alarmActive[i] = false;
      digitalWrite(RED_LED_PINS[i], LOW);
      digitalWrite(GREEN_LED_PINS[i], LOW);
      greenLedActive[i] = false;
    }
    
    digitalWrite(BUZZER_PIN, LOW);
    
    Blynk.virtualWrite(VPIN_LOG_DISPLAY, "Log cleared.");
    updateBlynkStatus();
    
    Serial.println("Daily log reset!");
  }
}

BLYNK_WRITE(VPIN_CURRENT_TIME_BUTTON) {
  if (param.asInt() == 1) {
    DateTime now = rtc.now();
    
    String currentTime = String(now.hour()) + ":" + 
                      (now.minute() < 10 ? "0" : "") + String(now.minute()) + ":" +
                      (now.second() < 10 ? "0" : "") + String(now.second());
    
    Blynk.virtualWrite(VPIN_CURRENT_TIME_DISPLAY, currentTime);
    Serial.println("Current time requested: " + currentTime);
  }
}

void parseTime(int drawer, long timeInSeconds) {
  // Expected format: "HH:MM" or empty to disable
  
    if (drawer < 0 || drawer >= 6  ) return;
  
  
    int hour = timeInSeconds / 3600;
    int minute = (timeInSeconds % 3600) / 60;
    
    if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59) {
      schedules[drawer].hour = hour;
      schedules[drawer].minute = minute;
      schedules[drawer].enabled = true;
      schedules[drawer].taken = false;
      
      Serial.println(drawerNames[drawer] + " set to " + 
                    String(hour) + ":" + 
                    (minute < 10 ? "0" : "") + String(minute));
      
      updateBlynkStatus();
    } else {
      Serial.println("Invalid time format for " + drawerNames[drawer]);
    }
  
}

// Blynk connected event
BLYNK_CONNECTED() {
  Serial.println("Connected to Blynk!");
  updateBlynkStatus();
  Blynk.virtualWrite(VPIN_LOG_DISPLAY, dailyLog);
  Blynk.virtualWrite(VPIN_CURRENT_TIME_DISPLAY, "Press button to see current time");
}

// Function to manually test a drawer (for debugging)
void testDrawer(int drawer) {
  alarmActive[drawer] = true;
  digitalWrite(RED_LED_PINS[drawer], HIGH);
  Serial.println("Testing " + drawerNames[drawer]);
}
