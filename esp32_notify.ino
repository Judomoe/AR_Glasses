// Required Libraries
// You will need to install these libraries in the Arduino IDE or PlatformIO:
// 1. ESP32 WebServer: Included by default for ESP32 boards
// 2. Adafruit SSD1306: For the OLED display (use the Adafruit library manager)
// 3. Adafruit GFX: Dependency for SSD1306 (usually installed with it)

#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Configuration ---

// 1. Wi-Fi Credentials
const char* ssid = "Bruh";         // REPLACE with your Wi-Fi name
const char* password = "7a4ee42005"; // REPLACE with your Wi-Fi password

// 2. Web Server Port (Updated to 12345)
const int HTTP_PORT = 12345;

// 3. OLED Display Configuration
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 32    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // I2C address for 128x32 OLED

// 4. I2C Pins (Check your specific ESP32 board for these)
#define I2C_SDA 21 // Commonly GPIO 21 on ESP32
#define I2C_SCL 22 // Commonly GPIO 22 on ESP32

// Initialize the OLED display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Initialize the Web Server on the specified port (12345)
WebServer server(HTTP_PORT);

// --- State Variables ---
// These variables hold the current content and its display duration
String displayLine1 = "";
String displayLine2 = "";
unsigned long displayTimeout = 0;
const unsigned long DISPLAY_DURATION = 10000; // 10 seconds

// --- Function Prototypes ---
void setupWiFi();
void handleRoot();
void handleNotification();
void handleRecognition(); // New handler for face recognition
void drawCenteredText(const char* line1, const char* line2);
void drawActiveMessage(); // Function to draw based on current state
void displayError(const String& errorMsg);

// New URL decoding functions from working code
String urlDecode(String str);
unsigned char h2int(char c);

// --- Setup ---

void setup() {
  Serial.begin(115200);

  // Initialize OLED
  Wire.begin(I2C_SDA, I2C_SCL); // Explicitly define I2C pins
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed
  }

  // Initial display screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("ESP32 Notify Ready");
  display.display();

  setupWiFi();
  
  // Server route setup
  server.onNotFound(handleRoot); // Default handler for any other request
  server.on("/NOTIFY", HTTP_GET, handleNotification); // MacroDroid notification route
  server.on("/box", HTTP_POST, handleRecognition); // Face Recognition POST route (based on your Python)

  server.begin();
  Serial.printf("HTTP server started on port %d\n", HTTP_PORT);
}

// --- Main Loop ---

void loop() {
  server.handleClient();
  
  // Check if timeout has expired and draw the default screen if needed
  if (displayTimeout != 0 && millis() >= displayTimeout) {
    displayTimeout = 0; // Clear the timeout flag
    // Revert to the READY screen after the message expires
    setupWiFi(); 
  }

  // Keep the Wi-Fi connection alive
  if (WiFi.status() != WL_CONNECTED) {
    setupWiFi();
  }
}

// --- Implementation ---

/**
 * Connects the ESP32 to the specified Wi-Fi network.
 */
void setupWiFi() {
  // Display connecting message
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting to WiFi...");
  display.println(ssid);
  display.display();

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // Display connection success and IP - using size 2 for better visibility
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("READY!");
    
    display.setTextSize(1);
    display.setCursor(0, 18);
    display.print(ip);
    display.print(":");
    display.println(HTTP_PORT);
    display.display();

  } else {
    Serial.println("\nConnection failed! Check credentials.");
    displayError("WiFi Failed!");
  }
}

/**
 * Handles the incoming notification request from MacroDroid.
 * Expected format: http://<ip>:<port>/NOTIFY/{TITLE}:{MESSAGE}
 */
void handleNotification() {
  // CRITICAL DEBUGGING LINE: Print the raw URI the server received
  Serial.print("SUCCESS: Notification Route Hit. URI: ");
  Serial.println(server.uri());
  
  // Get the full request path, e.g., "/NOTIFY/TITLE:MESSAGE"
  String path = server.uri();
  
  // Find the start of the actual data (after "/NOTIFY/")
  int dataStart = path.indexOf("/NOTIFY/") + 8;
  if (dataStart >= path.length()) {
    displayError("Invalid URL format.");
    server.send(400, "text/plain", "Bad Request: No notification data found.");
    return;
  }
  
  String encodedData = path.substring(dataStart);
  String notificationData = urlDecode(encodedData); 
  
  // Find the separator (colon)
  int separatorIndex = notificationData.indexOf(':');

  String titleStr;
  String messageStr;

  if (separatorIndex != -1) {
    titleStr = notificationData.substring(0, separatorIndex);
    messageStr = notificationData.substring(separatorIndex + 1);
  } else {
    titleStr = "Notification";
    messageStr = notificationData;
  }
  
  // --- Update Display State ---
  displayLine1 = titleStr;
  displayLine2 = messageStr;
  displayTimeout = millis() + DISPLAY_DURATION;
  
  Serial.print("Title: "); Serial.println(displayLine1);
  Serial.print("Message: "); Serial.println(displayLine2);

  drawActiveMessage();

  // Send a success response back to MacroDroid
  server.send(200, "text/plain", "Notification received and displayed.");
}

/**
 * Handles incoming JSON POST request from Face Recognition script
 * Expected format: POST /box with {"name": "Ragab"}
 */
void handleRecognition() {
  Serial.println("SUCCESS: Recognition Route Hit (POST /box).");
  
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed. Use POST.");
    return;
  }

  // --- Start Parsing ---
  String name = "Unknown"; // Default name if parsing fails
  String body = "";

  if (server.hasArg("plain")) {
    body = server.arg("plain");
    Serial.print("Received POST Body: ");
    Serial.println(body);
    
    // Find the start of the name value (after "name":")
    // Use the index of "name" to make the search more robust
    int keyIndex = body.indexOf("\"name\"");
    if (keyIndex != -1) {
        // Search for the opening quote *after* the "name" key
        int nameStart = body.indexOf("\"", keyIndex + 6); // +6 to skip "name":"
        if (nameStart != -1) {
            nameStart++; // Move past the opening quote
            // Search for the closing quote
            int nameEnd = body.indexOf("\"", nameStart);
            
            if (nameEnd != -1 && nameStart < nameEnd) {
                // Name found
                name = body.substring(nameStart, nameEnd);
            } else if (nameEnd != -1 && nameStart == nameEnd) {
                // Case: {"name":""} -> empty string
                name = "Unidentified";
            }
        }
    }
    
    // Added warning for port mismatch, based on common error paths
    if (server.client().remotePort() == 80) {
      Serial.println("WARNING: Recognition sent on port 80. Ensure Python is targeting port 12345.");
    }
  } else {
    Serial.println("WARNING: POST body missing (no 'plain' argument).");
  }
  
  // --- Update Display State ---
  displayLine1 = "WELCOME";
  displayLine2 = name; 
  displayTimeout = millis() + DISPLAY_DURATION;
  
  Serial.print("Recognition Name Displayed: "); Serial.println(displayLine2);
  drawActiveMessage();
  server.send(200, "text/plain", "Name received and displayed.");
}


/**
 * Handles all other requests (not found).
 */
void handleRoot() {
  String uri = server.uri();
  
  // FIX: If the URI starts with /NOTIFY/, manually dispatch to the notification handler.
  if (uri.startsWith("/NOTIFY/")) {
    Serial.println("INFO: URI starts with /NOTIFY/. Manually redirecting to handleNotification.");
    handleNotification();
    return;
  }
  
  Serial.print("WARNING: Unexpected Request URI received: ");
  Serial.println(uri);

  String response = "ESP32 Notification Server\n";
  response += "Use /NOTIFY/TITLE:MESSAGE or POST /box to send data.\n";
  response += "Current Address: " + WiFi.localIP().toString() + ":" + String(HTTP_PORT);
  
  server.send(200, "text/plain", response);
}

/**
 * Draws the currently active message from the state variables.
 */
void drawActiveMessage() {
  // This function is only called when a new message/name is received, 
  // or when the timeout check in loop() requires drawing.
  
  // The line content is already stored in displayLine1 and displayLine2.
  drawCenteredText(displayLine1.c_str(), displayLine2.c_str());
}

/**
 * Draws two lines of text, centered on the 128x32 OLED using text size 2.
 * @param line1 The string for the top line (Title).
 * @param line2 The string for the bottom line (Message).
 */
void drawCenteredText(const char* line1, const char* line2) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  // --- Center and Draw Line 1 (Title) ---
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
  // Calculate X position to center: (Screen Width - Text Width) / 2
  int16_t xPos1 = (SCREEN_WIDTH - w) / 2;
  // Calculate Y position for the top line: e.g., 2 pixels from top
  int16_t yPos1 = 2;

  display.setCursor(xPos1, yPos1);
  display.println(line1);

  // --- Center and Draw Line 2 (Message) ---
  display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
  int16_t xPos2 = (SCREEN_WIDTH - w) / 2;
  // Calculate Y position for the bottom line: below the first line (size 2 text is 16 pixels high)
  int16_t yPos2 = yPos1 + 16; 

  display.setCursor(xPos2, yPos2);
  display.println(line2);
  
  display.display();
}

/**
 * Displays an error message on the OLED.
 */
void displayError(const String& errorMsg) {
  Serial.println("Error: " + errorMsg);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("ERROR:");
  display.println(errorMsg);
  display.display();
  delay(3000); // Keep error on screen for a moment
}

// --- HELPER: Hex to Int Conversion ---
unsigned char h2int(char c) {
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

// --- HELPER: Robust URL Decoder ---
// Handles '+' for spaces and '%xx' hex encoding
String urlDecode(String str) {
  String encodedString = str;
  String decodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < encodedString.length(); i++) {
    c = encodedString.charAt(i);
    if (c == '+') {
      decodedString += ' ';
    } else if (c == '%') {
      i++;
      code0 = encodedString.charAt(i);
      i++;
      code1 = encodedString.charAt(i);
      // Combine the two hex characters into a single character
      c = (h2int(code0) << 4) | h2int(code1); 
      decodedString += c;
    } else {
      decodedString += c;
    }
  }
  return decodedString;
}
