# ESP32 Smart Notification Display 📲

An ESP32 project that turns a small OLED screen into a smart display hub.
It receives two types of data over Wi-Fi:
1. **Phone notifications** sent by MacroDroid
2. **Recognized face names** sent by a Python face recognition script

Both are displayed centered on a **128×32 SSD1306 OLED** for 10 seconds,
then the screen reverts to the READY state.

---

## Features
- Hosts a lightweight HTTP web server on port **12345**
- Receives GET requests from **MacroDroid** for phone notifications
- Receives POST requests from a **Python face recognition script**
- URL decodes incoming notification text (handles `%xx` and `+` encoding)
- Auto-reverts to READY screen after 10 seconds
- Reconnects to Wi-Fi automatically if connection drops

---

## Hardware

| Component | Details |
|-----------|---------|
| Microcontroller | ESP32 Dev Module |
| Display | SSD1306 128×32 OLED |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| I2C Address | 0x3C |

---

## API Endpoints

### GET `/NOTIFY/{TITLE}:{MESSAGE}`
Triggered by MacroDroid when a phone notification arrives.
http://<ESP32_IP>:12345/NOTIFY/WhatsApp:You+have+a+new+message

### POST `/box`
Triggered by a Python face recognition script with a JSON body.

```json
{ "name": "Mohamed" }
```
Displays `WELCOME` on line 1 and the recognized name on line 2.

---

## Setup

### 1. Hardware
Connect the SSD1306 OLED to the ESP32 via I2C (SDA=GPIO21, SCL=GPIO22).

### 2. Firmware
1. Install libraries in Arduino IDE:
   - `Adafruit SSD1306`
   - `Adafruit GFX`
2. Update Wi-Fi credentials in the sketch:
```cpp
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```
3. Flash to ESP32 and open Serial Monitor (115200 baud)
4. Note the IP address printed on the OLED

### 3. MacroDroid Setup
Create an HTTP action in MacroDroid:
URL: http://<ESP32_IP>:12345/NOTIFY/[TITLE]:[MESSAGE]
Method: GET

### 4. Python Face Recognition
Point your recognition script's POST request to:
http://<ESP32_IP>:12345/box
Body: {"name": "Detected Name"}

---

## Project Structure
ESP32-Notification-Display/
├── firmware/
│   └── esp32_notify.ino    # Main ESP32 sketch
└── README.md

## Future Improvements
- Support multiple notification types with different display styles
- Add icons or animations for specific apps
- Replace polling reconnect with Wi-Fi event callbacks
- Store last N notifications and scroll through them with a button
