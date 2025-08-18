#include <WiFi.h>
#include <RTClib.h>
#include <Wire.h>

RTC_DS3231 rtc;

// Pin definitions
// Reed switches (input pins with pull-up)
const int reedPins[] = {2, 4, 34, 35, 36, 39};

// Red LEDs (alarm indicators)
const int redLEDPins[] = {12, 13, 14, 25, 26, 27};

// Green LEDs (confirmation indicators)
const int greenLEDPins[] = {32, 33, 15, 16, 17, 23};

// Buzzer
const int buzzerPin = 19;

// Medicine schedule structure
struct MedicineSchedule {
  int hour;
  int minute;
  int drawerIndex;
  bool isActive;
  bool isTaken;
  unsigned long greenLEDStartTime;
};

// Define medicine times (24-hour format)
MedicineSchedule schedule[] = {
  {0, 7 , 0, false, false, 0},   // Morning before breakfast - Drawer 0
  {0, 8, 1, false, false, 0},    // Morning after breakfast - Drawer 1
  {0, 9, 2, false, false, 0},  // Noon before lunch - Drawer 2
  {0, 10, 3, false, false, 0},   // Noon after lunch - Drawer 3
  {0, 11, 4, false, false, 0},  // Night before dinner - Drawer 4
  {0, 12, 5, false, false, 0}    // Night after dinner - Drawer 5
};

const int numSchedules = sizeof(schedule) / sizeof(schedule[0]);
const unsigned long greenLEDDuration = 10 * 60 * 1000; // 10 minutes in milliseconds
const int buzzerFrequency = 1000; // 1kHz tone

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C for RTC
  Wire.begin();
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  
  // If RTC lost power, set the time (uncomment and modify as needed)
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  // Initialize reed switch pins as input with pull-up
  for (int i = 0; i < 6; i++) {
    int p = reedPins[i];
    if (p >= 34 && p <= 39) {
      pinMode(p, INPUT);            // external 10k pull-up present
    } else {
      pinMode(p, INPUT_PULLUP);     // internal pull-up OK on “normal” G
    }
}

  
  // Initialize LED pins as output
  for (int i = 0; i < 6; i++) {
    pinMode(redLEDPins[i], OUTPUT);
    pinMode(greenLEDPins[i], OUTPUT);
    digitalWrite(redLEDPins[i], LOW);
    digitalWrite(greenLEDPins[i], LOW);
  }
  
  // Initialize buzzer pin
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  
  Serial.println("Smart Medicine Box Initialized!");
  Serial.println("Schedule:");
  for (int i = 0; i < numSchedules; i++) {
    Serial.printf("Drawer %d: %02d:%02d\n", 
                  schedule[i].drawerIndex, 
                  schedule[i].hour, 
                  schedule[i].minute);
  }
}

void loop() {
  DateTime now = rtc.now();
  
  // Check each scheduled medicine time
  for (int i = 0; i < numSchedules; i++) {
    checkMedicineTime(now, i);
    handleDrawerOpen(i);
    handleGreenLED(i);
  }
  
  // Print current time every minute (optional for debugging)
  static unsigned long lastTimeDisplay = 0;
  if (millis() - lastTimeDisplay > 60000) {
    Serial.printf("Current time: %02d:%02d:%02d\n", 
                  now.hour(), now.minute(), now.second());
    lastTimeDisplay = millis();
  }
  
  delay(1000); // Check every second
}

void checkMedicineTime(DateTime now, int scheduleIndex) {
  MedicineSchedule &med = schedule[scheduleIndex];
  
  // Check if it's time for medicine
  if (now.hour() == med.hour && now.minute() == med.minute && !med.isTaken) {
    if (!med.isActive) {
      med.isActive = true;
      Serial.printf("Medicine time! Drawer %d\n", med.drawerIndex);
    }
    
    // Activate red LED and buzzer
    digitalWrite(redLEDPins[med.drawerIndex], HIGH);
    tone(buzzerPin, buzzerFrequency, 500); // 500ms tone
  }
  
  // Reset at midnight for next day
  if (now.hour() == 0 && now.minute() == 0 && now.second() == 0) {
    resetDailySchedule();
  }
}

void handleDrawerOpen(int scheduleIndex) {
  MedicineSchedule &med = schedule[scheduleIndex];
  
  // Check if drawer is opened (reed switch activated - LOW when magnet is away)
  if (digitalRead(reedPins[med.drawerIndex]) == LOW && med.isActive && !med.isTaken) {
    // Drawer opened during medicine time
    med.isTaken = true;
    med.isActive = false;
    
    // Turn off red LED and buzzer
    digitalWrite(redLEDPins[med.drawerIndex], LOW);
    noTone(buzzerPin);
    
    // Turn on green LED
    digitalWrite(greenLEDPins[med.drawerIndex], HIGH);
    med.greenLEDStartTime = millis();
    
    Serial.printf("Medicine taken from drawer %d at %02d:%02d\n", 
                  med.drawerIndex, rtc.now().hour(), rtc.now().minute());
  }
}

void handleGreenLED(int scheduleIndex) {
  MedicineSchedule &med = schedule[scheduleIndex];
  
  // Turn off green LED after 10 minutes
  if (digitalRead(greenLEDPins[med.drawerIndex]) == HIGH && 
      millis() - med.greenLEDStartTime > greenLEDDuration) {
    digitalWrite(greenLEDPins[med.drawerIndex], LOW);
    Serial.printf("Green LED turned off for drawer %d\n", med.drawerIndex);
  }
}

void resetDailySchedule() {
  Serial.println("Resetting daily schedule...");
  for (int i = 0; i < numSchedules; i++) {
    schedule[i].isActive = false;
    schedule[i].isTaken = false;
    schedule[i].greenLEDStartTime = 0;
    
    // Turn off all LEDs
    digitalWrite(redLEDPins[i], LOW);
    digitalWrite(greenLEDPins[i], LOW);
  }
  noTone(buzzerPin);
}

// Function to manually test a specific drawer (call from Serial monitor)
void testDrawer(int drawerIndex) {
  if (drawerIndex >= 0 && drawerIndex < 6) {
    Serial.printf("Testing drawer %d\n", drawerIndex);
    digitalWrite(redLEDPins[drawerIndex], HIGH);
    tone(buzzerPin, buzzerFrequency, 2000); // 2 second test tone
    
    // Test sequence
    delay(3000);
    digitalWrite(redLEDPins[drawerIndex], LOW);
    digitalWrite(greenLEDPins[drawerIndex], HIGH);
    delay(3000);
    digitalWrite(greenLEDPins[drawerIndex], LOW);
    noTone(buzzerPin);
    Serial.println("Test completed");
  }
}

// Function to check serial commands
void serialEvent() {
  if (Serial.available()) {
    String command = Serial.readString();
    command.trim();
    
    if (command.startsWith("test")) {
      int drawer = command.substring(4).toInt();
      testDrawer(drawer);
    } else if (command == "time") {
      DateTime now = rtc.now();
      Serial.printf("Current time: %02d:%02d:%02d %02d/%02d/%04d\n", 
                    now.hour(), now.minute(), now.second(),
                    now.day(), now.month(), now.year());
    } else if (command == "reset") {
      resetDailySchedule();
    }
  }
}


