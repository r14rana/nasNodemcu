// WiFi library
#include <ESP8266WiFi.h>

// SD cards library
#include <SPI.h>
#include <SD.h>
#include <FS.h>

// Web server library
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Discovery related library
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// OLED library
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* Hardware Configurations */
#define CS_PIN D8

//oled define
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

/* Software Global Variables */
AsyncWebServer server(80);
String adminUsername = "group12";
String adminPassword = "nas";
String mdnsName = "group12";
String authSession = "";

/* Time Keeping */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

/* Function definitions */
String loadWiFiInfoFromSD();

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1     // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Reference state for eye dimensions and positions
int ref_eye_height = 40;
int ref_eye_width = 40;
int ref_space_between_eye = 10;
int ref_corner_radius = 10;

// Current state of the eyes
int left_eye_height = ref_eye_height;
int left_eye_width = ref_eye_width;
int left_eye_x = 32;
int left_eye_y = 32;
int right_eye_x = 32 + ref_eye_width + ref_space_between_eye;
int right_eye_y = 32;
int right_eye_height = ref_eye_height;
int right_eye_width = ref_eye_width;

void draw_eyes(bool update = true) {
  display.clearDisplay();
  
  // Draw left eye
  int x = left_eye_x - left_eye_width / 2;
  int y = left_eye_y - left_eye_height / 2;
  display.fillRoundRect(x, y, left_eye_width, left_eye_height, ref_corner_radius, SSD1306_WHITE);
  
  // Draw right eye
  x = right_eye_x - right_eye_width / 2;
  y = right_eye_y - right_eye_height / 2;
  display.fillRoundRect(x, y, right_eye_width, right_eye_height, ref_corner_radius, SSD1306_WHITE);
  
  if (update) {
    display.display();
  }
}

void center_eyes(bool update = true) {
  // Center the eyes on the display
  left_eye_height = ref_eye_height;
  left_eye_width = ref_eye_width;
  right_eye_height = ref_eye_height;
  right_eye_width = ref_eye_width;
  
  left_eye_x = SCREEN_WIDTH / 2 - ref_eye_width / 2 - ref_space_between_eye / 2;
  left_eye_y = SCREEN_HEIGHT / 2;
  right_eye_x = SCREEN_WIDTH / 2 + ref_eye_width / 2 + ref_space_between_eye / 2;
  right_eye_y = SCREEN_HEIGHT / 2;
  
  draw_eyes(update);
}

void blink(int speed = 12) {
  draw_eyes();
  
  // Perform eye blink animation
  for (int i = 0; i < 3; i++) {
    left_eye_height -= speed;
    right_eye_height -= speed;
    draw_eyes();
    delay(50); // Adjust blink speed here
  }
  for (int i = 0; i < 3; i++) {
    left_eye_height += speed;
    right_eye_height += speed;
    draw_eyes();
    delay(50); // Adjust blink speed here
  }
}

void sleep() {
  // Close the eyes for sleep animation
  left_eye_height = 2;
  right_eye_height = 2;
  draw_eyes(true);
}

void wakeup() {
  // Open the eyes for wakeup animation
  sleep();
  for (int h = 0; h <= ref_eye_height; h += 2) {
    left_eye_height = h;
    right_eye_height = h;
    draw_eyes(true);
    delay(20); // Adjust wakeup animation speed here
  }
}

void happy_eye() {
  center_eyes(false); // Center eyes before drawing happiness animation
  
  // Draw inverted triangle over eye lower part for happiness effect
  int offset = ref_eye_height / 2;
  for (int i = 0; i < 10; i++) {
    display.fillTriangle(left_eye_x - left_eye_width / 2 - 1, left_eye_y + offset,
                         left_eye_x + left_eye_width / 2 + 1, left_eye_y + 5 + offset,
                         left_eye_x - left_eye_width / 2 - 1, left_eye_y + left_eye_height + offset,
                         SSD1306_BLACK);
    
    display.fillTriangle(right_eye_x + right_eye_width / 2 + 1, right_eye_y + offset,
                         right_eye_x - left_eye_width / 2 - 1, right_eye_y + 5 + offset,
                         right_eye_x + right_eye_width / 2 + 1, right_eye_y + right_eye_height + offset,
                         SSD1306_BLACK);
    
    offset -= 2;
    display.display();
    delay(50); // Adjust happiness animation speed here
  }
  
  delay(1000); // Pause for visibility
}

void saccade(int direction_x, int direction_y) {
  // Quick movement of the eye, no size change, stay at position after movement
  // Direction == -1 : move left/up, Direction == 1 : move right/down
  int direction_x_movement_amplitude = 8;
  int direction_y_movement_amplitude = 6;

  for (int i = 0; i < 1; i++) {
    left_eye_x += direction_x_movement_amplitude * direction_x;
    right_eye_x += direction_x_movement_amplitude * direction_x;
    left_eye_y += direction_y_movement_amplitude * direction_y;
    right_eye_y += direction_y_movement_amplitude * direction_y;
    
    draw_eyes();
    delay(50); // Adjust saccade animation speed here
  }
}

void move_right_big_eye() {
  move_big_eye(1);
}

void move_left_big_eye() {
  move_big_eye(-1);
}

void move_big_eye(int direction) {
  // Direction == -1 : move left, Direction == 1 : move right
  int direction_oversize = 1;
  int direction_movement_amplitude = 2;

  for (int i = 0; i < 3; i++) {
    left_eye_x += direction_movement_amplitude * direction;
    right_eye_x += direction_movement_amplitude * direction;
    
    if (direction > 0) {
      right_eye_height += direction_oversize;
      right_eye_width += direction_oversize;
    } else {
      left_eye_height += direction_oversize;
      left_eye_width += direction_oversize;
    }
    
    draw_eyes();
    delay(50); // Adjust eye movement speed here
  }
  
  delay(1000); // Pause after movement
  for (int i = 0; i < 3; i++) {
    left_eye_x -= direction_movement_amplitude * direction;
    right_eye_x -= direction_movement_amplitude * direction;
    
    if (direction > 0) {
      right_eye_height -= direction_oversize;
      right_eye_width -= direction_oversize;
    } else {
      left_eye_height -= direction_oversize;
      left_eye_width -= direction_oversize;
    }
    
    draw_eyes();
    delay(50); // Adjust eye movement speed here
  }
  
  center_eyes();
}

void setup() {
  // Setup Debug Serial Port
  Serial.begin(9600);

  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();

  // Try Initialize SD card (blocking)
  bool sdInitialized = SD.begin(CS_PIN, 32000000);
  if (sdInitialized) {
    Serial.println("SD card initialized");
    Serial.println("\n\nStorage Info:");
    Serial.println("----------------------");
    getSDCardTotalSpace();
    getSDCardUsedSpace();
    Serial.println("----------------------");
    Serial.println();

    display.setCursor(0, 30); // Start at top-left corner
    display.setTextSize(1); // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.println(F("SD card initialized"));

    display.setCursor(0, 40); // Move to the next line
    display.print(F("Website:"));
    display.println(mdnsName+F(".local"));
    display.display();

    // Initialize SSD1306 display
     // Pause after startup
  
    // Perform a sequence of animations
    
  } else {
    Serial.println("SD card not detected");
    display.setCursor(0, 30); // Start at top-left corner
    display.setTextSize(1); // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.println(F("SD card not detected"));
    display.display();
  }

  // Connect to WiFi based on settings (cfg/wifi.txt)
  initWiFiConn();

  // Load admin credentials from SD card (cfg/admin.txt)

  // Start mDNS service
  if (!MDNS.begin(mdnsName)) {
    Serial.println("mDNS Error. Skipping.");
  } else {
    Serial.println("mDNS started. Connect to your webstick using http://" + mdnsName + ".local");
    MDNS.addService("http", "tcp", 80);
  }

  // Start NTP time client
  timeClient.begin();
  Serial.print("Requesting time from NTP (unix timestamp): ");
  timeClient.update();
  Serial.println(getTime());

  // Initialize database
  DBInit();

  // Resume login session if any
  initLoginSessionKey();

  // Start listening to HTTP Requests
  initWebServer();
}

void loop() {
  MDNS.update();
  timeClient.update();

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.display();
  wakeup();          // Open eyes
  center_eyes();     // Center eyes
  blink();           // Blink eyes
  happy_eye();       // Display happiness
  sleep();           // Close eyes for sleep
  move_right_big_eye();  // Move eyes to the right
  move_left_big_eye();   // Move eyes to the left
  saccade(1, 1);     // Perform a saccade movement
}
