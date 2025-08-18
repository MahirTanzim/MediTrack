# 💊 MediTrack: Smart medicine box with cloud integration and mobille monitoring system

A complete IoT solution for medication management using ESP32, designed to help patients and caregivers track medicine intake with real-time notifications and comprehensive logging.

![Smart Medicine Box](https://img.shields.io/badge/ESP32-IoT-blue) ![Blynk](https://img.shields.io/badge/Blynk-IoT%20Platform-green) ![Arduino](https://img.shields.io/badge/Arduino-IDE-orange)

## 🌟 Features

### 📱 Mobile App Control
- **Remote Time Setting**: Configure medicine schedules from anywhere
- **Real-time Status Monitoring**: Visual LED indicators for each drawer
- **Push Notifications**: Instant alerts for medicine time and missed doses
- **Complete Activity Log**: Detailed history with timestamps
- **Manual Testing**: Remote drawer testing functionality

### ⏰ Smart Scheduling
- **6 Customizable Time Slots**: Perfect for multiple daily medications
- **Automatic Reminders**: Red LED + buzzer alerts at scheduled times
- **Missed Dose Detection**: Alerts if medicine not taken within 30 minutes
- **Daily Reset**: Automatic schedule reset at midnight

### 🔔 Multi-Level Notifications
- 🕐 **Medicine Time Alert**: When it's time to take medication
- ✅ **Confirmation Alert**: When medicine is successfully taken
- ⚠️ **Missed Medicine Warning**: If dose is missed for 30+ minutes

### 📊 Health Tracking
- **Daily Intake Logging**: Complete record of all medication activities
- **Compliance Monitoring**: Track adherence to medication schedule
- **Historical Data**: Perfect for sharing with healthcare providers

## 🔧 Hardware Requirements

### Main Components
| Component | Quantity | Purpose |
|-----------|----------|---------|
| ESP32 Development Board | 1 | Main controller with WiFi |
| DS3231 RTC Module | 1 | Real-time clock for accurate timing |
| Reed Switches | 6 | Detect drawer opening (magnetic sensors) |
| Red LEDs | 6 | Medicine time indicators |
| Green LEDs | 6 | Medicine taken confirmation |
| Buzzer | 1 | Audio alerts |
| Resistors (220Ω) | 12 | LED current limiting |
| Pull-up Resistors (10kΩ) | 4 | For input-only GPIO pins |

### Pin Connections

```
ESP32 Pin Assignments:
├── Reed Switches (INPUT): 2, 4, 34, 35, 36, 39
├── Red LEDs (OUTPUT): 12, 13, 14, 25, 26, 27  
├── Green LEDs (OUTPUT): 32, 33, 15, 16, 17, 23
├── Buzzer (OUTPUT): 19
└── I2C (RTC): SDA-21, SCL-22
```

### Circuit Notes
- **GPIO 34-39**: Input-only pins, require external 10kΩ pull-up resistors
- **GPIO 2, 4**: Can use internal pull-up resistors
- **RTC Module**: Connect VCC to 3.3V, GND to GND

## 📚 Software Dependencies

### Arduino Libraries
Install these libraries through Arduino IDE Library Manager:

```cpp
// Core Libraries
#include <WiFi.h>           // ESP32 WiFi (built-in)
#include <Wire.h>           // I2C communication (built-in)

// External Libraries (install via Library Manager)
#include <RTClib.h>         // by Adafruit
#include <BlynkSimpleEsp32.h> // by Volodymyr Shymanskyy
```

### Installation Commands
```bash
# In Arduino IDE:
Tools → Manage Libraries → Search and Install:
- "RTClib" by Adafruit
- "Blynk" by Volodymyr Shymanskyy
```

## ⚙️ Setup Instructions

### 1. Hardware Assembly
1. Connect components according to pin diagram
2. Install ESP32 in breadboard/PCB
3. Connect RTC module via I2C
4. Wire reed switches with pull-up resistors for GPIO 34-39
5. Connect LEDs with 220Ω current-limiting resistors
6. Add buzzer with appropriate driver circuit

### 2. Blynk Configuration

#### Create Blynk Template
1. Go to [console.blynk.io](https://console.blynk.io)
2. Create new template: **"Smart Medicine Box"**
3. Hardware: **ESP32**, Connection: **WiFi**
4. Note your **Template ID**

#### Configure Datastreams
Add these Virtual Pins in your template:

| Pin | Name | Type | Purpose |
|-----|------|------|---------|
| V0-V5 | Medicine Times | String | Schedule input |
| V6 | Current Time | String | Time display |
| V7 | Medicine Log | String | Activity history |
| V8 | System Status | String | Status updates |
| V9 | Reset Log | Integer | Manual reset |
| V10 | Manual Test | Integer | Remote testing |
| V11-V16 | Drawer Status | Integer | LED indicators |

#### Create Device
1. Go to **Devices** tab
2. Create **"New Device"** from template
3. Copy **Auth Token** for Arduino code

#### Setup Events
Create these events for notifications:
- `medicine_time` - Medicine Time Alert
- `medicine_taken` - Medicine Taken Confirmation  
- `missed_medicine` - Missed Medicine Warning

### 3. Mobile App Setup

#### Add Widgets
| Widget Type | Virtual Pin | Configuration |
|-------------|-------------|---------------|
| Time Input | V0-V5 | Medicine schedule times |
| Label | V6, V8 | Current time & status |
| Terminal | V7 | Medicine intake log |
| Button | V9 | Reset daily log |
| Menu | V10 | Manual drawer test |
| LED | V11-V16 | Drawer status indicators |

### 4. Code Configuration

Update these credentials in the code:

```cpp
// Blynk Credentials (from Blynk Console)
#define BLYNK_TEMPLATE_ID "TMPL6niMtcd8O"        // Your Template ID
#define BLYNK_TEMPLATE_NAME "Meditrack"          // Your Template Name  
#define BLYNK_AUTH_TOKEN "Your_Auth_Token_Here"  // Your Device Auth Token

// WiFi Credentials
const char* ssid = "Your_WiFi_Name";             // Your WiFi SSID
const char* password = "Your_WiFi_Password";     // Your WiFi Password
```

### 5. Upload and Test
1. Select **ESP32 Dev Module** in Arduino IDE
2. Upload code to ESP32
3. Open Serial Monitor (115200 baud)
4. Verify WiFi and Blynk connection
5. Test each drawer using mobile app

## 🚀 Usage Guide

### Initial Setup
1. **Set Medicine Times**: Use time input widgets in app
2. **Verify Connection**: Check device status shows "Online"
3. **Test Drawers**: Use manual test feature to verify hardware
4. **Enable Notifications**: Allow push notifications in phone settings

### Daily Operation

#### Medicine Time Sequence
1. **Alert Phase**: Red LED + buzzer + app notification
2. **Taking Medicine**: Open drawer (reed switch triggered)
3. **Confirmation**: Green LED + confirmation notification + log entry
4. **Missed Alert**: If not taken within 30 minutes → warning notification

#### Mobile App Features
- **📅 Schedule Management**: Set/modify medicine times remotely
- **📊 Real-time Monitoring**: View current status and drawer states  
- **📝 Activity Logging**: Complete history with timestamps
- **🔔 Smart Notifications**: Never miss a dose
- **🧪 Remote Testing**: Test system functionality from anywhere

### Understanding LED Status
| Color | Meaning | Duration |
|-------|---------|----------|
| ⚫ Off | Normal state | - |
| 🔴 Red | Medicine time - take now | Until drawer opened |
| 🟢 Green | Medicine taken - confirmed | 10 minutes |

## 📱 Mobile App Interface

### Dashboard Layout
```
┌─────────────────────────────────────┐
│           Current Time              │
│           System Status             │  
├─────────────────────────────────────┤
│  🕐 Morning Before  🕐 Morning After │
│     Breakfast         Breakfast     │
├─────────────────────────────────────┤  
│   🕐 Noon Before    🕐 Noon After    │
│      Lunch            Lunch         │
├─────────────────────────────────────┤
│  🕐 Evening Before  🕐 Night After   │
│     Dinner           Dinner         │
├─────────────────────────────────────┤
│ 🔴 LED1  🔴 LED2  🔴 LED3           │
│ 🔴 LED4  🔴 LED5  🔴 LED6           │
├─────────────────────────────────────┤
│      [Reset Log]  [Test Drawer]     │
├─────────────────────────────────────┤
│        Medicine Intake Log          │
│    (Terminal showing full log)      │
└─────────────────────────────────────┘
```

## 📊 Sample Log Output

```
=== Daily Medicine Log ===
08:00 - ⏰ Morning Before Breakfast - Medicine time started
08:02 - ✅ Morning Before Breakfast - Medicine taken
08:30 - ⏰ Morning After Breakfast - Medicine time started
09:05 - ❌ Morning After Breakfast - MISSED (30+ min)
12:00 - ⏰ Noon Before Lunch - Medicine time started  
12:01 - ✅ Noon Before Lunch - Medicine taken

=== NEW DAY - 16/8/2025 ===
08:00 - ⏰ Morning Before Breakfast - Medicine time started
```

## 🔧 Troubleshooting

### Common Issues

#### Device Won't Connect to WiFi
```cpp
// Check in Serial Monitor:
- "Connecting to WiFi..." (should show your SSID)
- Verify SSID and password are correct
- Ensure 2.4GHz WiFi network (ESP32 doesn't support 5GHz)
```

#### Blynk Connection Failed  
```cpp
// Verify in code:
- Template ID matches Blynk Console
- Auth Token is correct (from Device tab, not Template)
- Device shows "Online" in Blynk Console
```

#### RTC Not Working
```cpp
// Check connections:
- SDA → GPIO 21
- SCL → GPIO 22  
- VCC → 3.3V (not 5V)
- Ensure RTC has backup battery
```

#### Reed Switches Not Responding
```cpp
// For GPIO 34-39:
- Must use external 10kΩ pull-up resistors
- Cannot use INPUT_PULLUP on these pins
- Check magnet polarity and distance
```

### Debug Commands
Monitor these in Serial Monitor (115200 baud):
```
✓ "WiFi connected!" - Network connection OK
✓ "Blynk Connected!" - IoT platform connected  
✓ "Smart Medicine Box with Blynk Initialized!" - Setup complete
✓ "Medicine time! Drawer X" - Schedule triggered
✓ "Medicine taken from drawer X" - Successful intake
```

## 🔮 Future Enhancements

### Planned Features
- [ ] **Multiple Patient Support**: Manage medicine for multiple family members
- [ ] **Advanced Analytics**: Weekly/monthly compliance reports  
- [ ] **Voice Alerts**: Audio medicine names and instructions
- [ ] **Pill Counter**: Track remaining medicine quantity
- [ ] **Doctor Integration**: Share reports directly with healthcare providers
- [ ] **Emergency Alerts**: Critical medication reminders
- [ ] **Backup Connectivity**: SMS alerts when WiFi is down

### Hardware Upgrades
- [ ] **LCD Display**: Local status display without app
- [ ] **Camera Module**: Visual confirmation of medicine taking
- [ ] **Weight Sensors**: Automatic pill counting
- [ ] **NFC Tags**: Quick patient identification
- [ ] **Battery Backup**: Continue operation during power outages



### Development Setup
```bash
git clone https://github.com/yourusername/smart-medicine-box.git
cd smart-medicine-box
# Open in Arduino IDE and install dependencies
```

## 🙏 Acknowledgments

- **Blynk Team** - For the excellent IoT platform
- **Adafruit** - For the reliable RTClib library  
- **Arduino Community** - For extensive ESP32 support
- **Healthcare Workers** - Inspiration for medication compliance solutions

## 📞 Support

### Documentation  
- **Blynk Documentation**: [docs.blynk.io](https://docs.blynk.io)
- **ESP32 Reference**: [docs.espressif.com](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- **Arduino ESP32**: [github.com/espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)

---

**⭐ Star this repo if it helped you manage your medications better!**

**Made with ❤️ for better healthcare management**
