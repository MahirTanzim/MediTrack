// Blynk credentials (replace with your actual credentials)
#define BLYNK_TEMPLATE_ID "TMPL6niMtcd8O"
#define BLYNK_TEMPLATE_NAME "Meditrack"
#define BLYNK_AUTH_TOKEN "Vt51zA-XOoeH0eUm3IZsOuVdxwwHJvrj"

// Header Files
#include <WiFi.h>
#include <RTClib.h>
#include <Wire.h>
#include <BlynkSimpleEsp32.h>



// WiFi credentials
const char* ssid = "Mahir";
const char* password = "1.2.3.4.5.6";

// Blynk Virtual Pins
#define VPIN_DRAWER0_TIME V0   // Time input for drawer 0
#define VPIN_DRAWER1_TIME V1   // Time input for drawer 1
#define VPIN_DRAWER2_TIME V2   // Time input for drawer 2
#define VPIN_DRAWER3_TIME V3   // Time input for drawer 3
#define VPIN_DRAWER4_TIME V4   // Time input for drawer 4
#define VPIN_DRAWER5_TIME V5   // Time input for drawer 5
#define VPIN_CURRENT_TIME V6   // Display current time
#define VPIN_INTAKE_LOG V7     // Medicine intake log
#define VPIN_STATUS V8         // System status
#define VPIN_RESET_LOG V9      // Button to reset daily log
#define VPIN_MANUAL_TEST V10   // Manual test dropdown
#define VPIN_DRAWER0_STATUS V11 // Status LEDs for each drawer
#define VPIN_DRAWER1_STATUS V12
#define VPIN_DRAWER2_STATUS V13
#define VPIN_DRAWER3_STATUS V14
#define VPIN_DRAWER4_STATUS V15
#define VPIN_DRAWER5_STATUS V16

RTC_DS3231 rtc;
BlynkTimer timer;

// Pin definitions
const int reedPins[] = {2, 4, 34, 35, 36, 39};
const int redLEDPins[] = {12, 13, 14, 25, 26, 27};
const int greenLEDPins[] = {32, 33, 15, 16, 17, 23};
const int buzzerPin = 19;

// Medicine schedule structure
struct MedicineSchedule {
  int hour;
  int minute;
  int drawerIndex;
  bool isActive;
  bool isTaken;
  bool missedNotificationSent;
  unsigned long alarmStartTime;
  unsigned long greenLEDStartTime;
  String drawerName;
};

// Define medicine times and names
MedicineSchedule schedule[] = {
  {8, 0, 0, false, false, false, 0, 0, "Morning Before Breakfast"},
  {8, 30, 1, false, false, false, 0, 0, "Morning After Breakfast"},
  {12, 0, 2, false, false, false, 0, 0, "Noon Before Lunch"},
  {12, 30, 3, false, false, false, 0, 0, "Noon After Lunch"},
  {18, 0, 4, false, false, false, 0, 0, "Evening Before Dinner"},
  {20, 0, 5, false, false, false, 0, 0, "Night After Dinner"}
};

const int numSchedules = sizeof(schedule) / sizeof(schedule[0]);
const unsigned long greenLEDDuration = 10 * 60 * 1000; // 10 minutes
const unsigned long missedMedicineTimeout = 30 * 60 * 1000; // 30 minutes
const int buzzerFrequency = 1000;

String dailyLog = "";
int virtualPinTimeInputs[] = {VPIN_DRAWER0_TIME, VPIN_DRAWER1_TIME, VPIN_DRAWER2_TIME, 
                              VPIN_DRAWER3_TIME, VPIN_DRAWER4_TIME, VPIN_DRAWER5_TIME};
int virtualPinStatusLEDs[] = {VPIN_DRAWER0_STATUS, VPIN_DRAWER1_STATUS, VPIN_DRAWER2_STATUS,
                              VPIN_DRAWER3_STATUS, VPIN_DRAWER4_STATUS, VPIN_DRAWER5_STATUS};

void setup() {
  Serial.begin(115200);
  
  // Initialize WiFi and Blynk
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected!");
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  
  // Initialize I2C for RTC
  Wire.begin();
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // If RTC lost power, set the time
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  // Initialize pins
  setupPins();
  
  // Setup Blynk timers
  timer.setInterval(1000L, updateBlynkData);    // Update every second
  timer.setInterval(1000L, checkMedicineSchedule); // Check schedule every second
  timer.setInterval(30000L, checkMissedMedicine);  // Check missed medicine every 30 seconds
  
  Serial.println("Smart Medicine Box with Blynk Initialized!");
  
  // Send initial data to Blynk
  sendInitialDataToBlynk();
  
  // Clear daily log
  dailyLog = "=== Daily Medicine Log ===\n";
  Blynk.virtualWrite(VPIN_INTAKE_LOG, dailyLog);
}

void loop() {
  Blynk.run();
  timer.run();
  
  // Handle drawer opening detection
  for (int i = 0; i < numSchedules; i++) {
    handleDrawerOpen(i);
    handleGreenLED(i);
  }
  
  delay(100);
}

void setupPins() {
  // Initialize reed switch pins
  for (int i = 0; i < 6; i++) {
    int p = reedPins[i];
    if (p >= 34 && p <= 39) {
      pinMode(p, INPUT);
    } else {
      pinMode(p, INPUT_PULLUP);
    }
  }
  
  // Initialize LED pins
  for (int i = 0; i < 6; i++) {
    pinMode(redLEDPins[i], OUTPUT);
    pinMode(greenLEDPins[i], OUTPUT);
    digitalWrite(redLEDPins[i], LOW);
    digitalWrite(greenLEDPins[i], LOW);
  }
  
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
}

void sendInitialDataToBlynk() {
  // Send current schedule times to Blynk
  for (int i = 0; i < numSchedules; i++) {
    long timeInSeconds = schedule[i].hour * 3600 + schedule[i].minute * 60;
    Blynk.virtualWrite(virtualPinTimeInputs[i], timeInSeconds);
  }
  
  // Update status
  Blynk.virtualWrite(VPIN_STATUS, "System Online - Ready");
  
  // Update all drawer status LEDs to off initially
  for (int i = 0; i < 6; i++) {
    Blynk.virtualWrite(virtualPinStatusLEDs[i], 0);
  }
}

void updateBlynkData() {
  DateTime now = rtc.now();
  
  // Update current time display
  String currentTime = String(now.hour()) + ":" + 
                      (now.minute() < 10 ? "0" : "") + String(now.minute()) + ":" +
                      (now.second() < 10 ? "0" : "") + String(now.second());
  Blynk.virtualWrite(VPIN_CURRENT_TIME, currentTime);
  
  // Update drawer status LEDs in Blynk app
  for (int i = 0; i < numSchedules; i++) {
    int color;
    if (schedule[i].isActive && !schedule[i].isTaken) {
      color = 255; // Red - medicine time active
    } else if (digitalRead(greenLEDPins[i]) == HIGH) {
      color = 65280; // Green - medicine taken
    } else {
      color = 0; // Off - no activity
    }
    Blynk.virtualWrite(virtualPinStatusLEDs[i], color);
  }
}

void checkMedicineSchedule() {
  DateTime now = rtc.now();
  
  for (int i = 0; i < numSchedules; i++) {
    MedicineSchedule &med = schedule[i];
    
    // Check if it's time for medicine
    if (now.hour() == med.hour && now.minute() == med.minute && !med.isTaken) {
      if (!med.isActive) {
        med.isActive = true;
        med.alarmStartTime = millis();
        med.missedNotificationSent = false;
        
        Serial.printf("Medicine time! Drawer %d - %s\n", med.drawerIndex, med.drawerName.c_str());
        
        // Send notification to Blynk app
        String notification = "🕐 Medicine Time! Take medicine from: " + med.drawerName;
        Blynk.logEvent("medicine_time", notification);
        
        // Add to daily log
        String logEntry = String(now.hour()) + ":" + 
                         (now.minute() < 10 ? "0" : "") + String(now.minute()) + 
                         " - ⏰ " + med.drawerName + " - Medicine time started\n";
        dailyLog += logEntry;
        Blynk.virtualWrite(VPIN_INTAKE_LOG, dailyLog);
      }
      
      // Activate red LED and buzzer
      digitalWrite(redLEDPins[med.drawerIndex], HIGH);
      if (now.second() % 2 == 0) { // Buzzer every 2 seconds
        tone(buzzerPin, buzzerFrequency, 200);
      }
    }
  }
  
  // Reset at midnight
  if (now.hour() == 0 && now.minute() == 0 && now.second() == 0) {
    resetDailySchedule();
  }
}

void checkMissedMedicine() {
  for (int i = 0; i < numSchedules; i++) {
    MedicineSchedule &med = schedule[i];
    
    // Check if medicine was missed (30 minutes passed without taking)
    if (med.isActive && !med.isTaken && !med.missedNotificationSent) {
      if (millis() - med.alarmStartTime > missedMedicineTimeout) {
        med.missedNotificationSent = true;
        
        // Send missed medicine notification
        String notification = "⚠️ MISSED MEDICINE! " + med.drawerName + " not taken for 30+ minutes!";
        Blynk.logEvent("missed_medicine", notification);
        
        // Add to daily log
        DateTime now = rtc.now();
        String logEntry = String(now.hour()) + ":" + 
                         (now.minute() < 10 ? "0" : "") + String(now.minute()) + 
                         " - ❌ " + med.drawerName + " - MISSED (30+ min)\n";
        dailyLog += logEntry;
        Blynk.virtualWrite(VPIN_INTAKE_LOG, dailyLog);
        
        Serial.printf("MISSED: Medicine from drawer %d not taken for 30+ minutes!\n", med.drawerIndex);
      }
    }
  }
}

void handleDrawerOpen(int scheduleIndex) {
  MedicineSchedule &med = schedule[scheduleIndex];
  
  if (digitalRead(reedPins[med.drawerIndex]) == LOW && med.isActive && !med.isTaken) {
    med.isTaken = true;
    med.isActive = false;
    
    // Turn off red LED and buzzer
    digitalWrite(redLEDPins[med.drawerIndex], LOW);
    noTone(buzzerPin);
    
    // Turn on green LED
    digitalWrite(greenLEDPins[med.drawerIndex], HIGH);
    med.greenLEDStartTime = millis();
    
    DateTime now = rtc.now();
    Serial.printf("Medicine taken from drawer %d at %02d:%02d\n", 
                  med.drawerIndex, now.hour(), now.minute());
    
    // Send confirmation notification to Blynk
    String notification = "✅ Medicine taken from: " + med.drawerName;
    Blynk.logEvent("medicine_taken", notification);
    
    // Add to daily log
    String logEntry = String(now.hour()) + ":" + 
                     (now.minute() < 10 ? "0" : "") + String(now.minute()) + 
                     " - ✅ " + med.drawerName + " - Medicine taken\n";
    dailyLog += logEntry;
    Blynk.virtualWrite(VPIN_INTAKE_LOG, dailyLog);
  }
}

void handleGreenLED(int scheduleIndex) {
  MedicineSchedule &med = schedule[scheduleIndex];
  
  if (digitalRead(greenLEDPins[med.drawerIndex]) == HIGH && 
      millis() - med.greenLEDStartTime > greenLEDDuration) {
    digitalWrite(greenLEDPins[med.drawerIndex], LOW);
    Serial.printf("Green LED turned off for drawer %d\n", med.drawerIndex);
  }
}

void resetDailySchedule() {
  Serial.println("Resetting daily schedule...");
  
  // Add reset entry to log
  DateTime now = rtc.now();
  dailyLog += "\n=== NEW DAY - " + String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + " ===\n";
  
  for (int i = 0; i < numSchedules; i++) {
    schedule[i].isActive = false;
    schedule[i].isTaken = false;
    schedule[i].missedNotificationSent = false;
    schedule[i].alarmStartTime = 0;
    schedule[i].greenLEDStartTime = 0;
    
    digitalWrite(redLEDPins[i], LOW);
    digitalWrite(greenLEDPins[i], LOW);
  }
  
  noTone(buzzerPin);
  Blynk.virtualWrite(VPIN_INTAKE_LOG, dailyLog);
  Blynk.virtualWrite(VPIN_STATUS, "Daily schedule reset - New day started");
}

// Blynk Virtual Pin Handlers

// Time input handlers for each drawer
BLYNK_WRITE(VPIN_DRAWER0_TIME) { updateScheduleTime(0, param.asLong()); }
BLYNK_WRITE(VPIN_DRAWER1_TIME) { updateScheduleTime(1, param.asLong()); }
BLYNK_WRITE(VPIN_DRAWER2_TIME) { updateScheduleTime(2, param.asLong()); }
BLYNK_WRITE(VPIN_DRAWER3_TIME) { updateScheduleTime(3, param.asLong()); }
BLYNK_WRITE(VPIN_DRAWER4_TIME) { updateScheduleTime(4, param.asLong()); }
BLYNK_WRITE(VPIN_DRAWER5_TIME) { updateScheduleTime(5, param.asLong()); }

void updateScheduleTime(int drawerIndex, long timeInSeconds) {
  int hours = timeInSeconds / 3600;
  int minutes = (timeInSeconds % 3600) / 60;
  
  schedule[drawerIndex].hour = hours;
  schedule[drawerIndex].minute = minutes;
  
  Serial.printf("Updated drawer %d time to %02d:%02d\n", drawerIndex, hours, minutes);
  
  String statusMsg = "Updated " + schedule[drawerIndex].drawerName + " time to " + 
                     String(hours) + ":" + (minutes < 10 ? "0" : "") + String(minutes);
  Blynk.virtualWrite(VPIN_STATUS, statusMsg);
}

// Reset daily log button
BLYNK_WRITE(VPIN_RESET_LOG) {
  if (param.asInt() == 1) {
    dailyLog = "=== Daily Medicine Log Reset ===\n";
    Blynk.virtualWrite(VPIN_INTAKE_LOG, dailyLog);
    Blynk.virtualWrite(VPIN_STATUS, "Daily log cleared");
    Serial.println("Daily log reset via Blynk");
  }
}

// Manual test dropdown
BLYNK_WRITE(VPIN_MANUAL_TEST) {
  int drawerToTest = param.asInt() - 1; // Dropdown values 1-6, convert to 0-5
  if (drawerToTest >= 0 && drawerToTest < 6) {
    testDrawer(drawerToTest);
  }
}

void testDrawer(int drawerIndex) {
  if (drawerIndex >= 0 && drawerIndex < 6) {
    Serial.printf("Testing drawer %d via Blynk\n", drawerIndex);
    
    String statusMsg = "Testing " + schedule[drawerIndex].drawerName;
    Blynk.virtualWrite(VPIN_STATUS, statusMsg);
    
    digitalWrite(redLEDPins[drawerIndex], HIGH);
    tone(buzzerPin, buzzerFrequency, 2000);
    
    delay(3000);
    digitalWrite(redLEDPins[drawerIndex], LOW);
    digitalWrite(greenLEDPins[drawerIndex], HIGH);
    delay(3000);
    digitalWrite(greenLEDPins[drawerIndex], LOW);
    noTone(buzzerPin);
    
    Blynk.virtualWrite(VPIN_STATUS, "Test completed for " + schedule[drawerIndex].drawerName);
    Serial.println("Test completed");
  }
}

// App connection event
BLYNK_CONNECTED() {
  Serial.println("Blynk Connected!");
  Blynk.virtualWrite(VPIN_STATUS, "Connected to Blynk - System Online");
  sendInitialDataToBlynk();
}

// App disconnection event  
BLYNK_DISCONNECTED() {
  Serial.println("Blynk Disconnected!");
}
