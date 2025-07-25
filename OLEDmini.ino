#include <ESP8266WiFi.h>
#include <time.h> // The built-in time library

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi Credentials
const char* ssid     = "Home";
const char* password = "23456789";

// NTP and Timezone Settings
const char* ntpServer = "pool.ntp.org";
// Find your TZ String here: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
// Example for Central European Time (Germany, France, etc.)
const char* tz_string = "EET-2EEST,M3.5.0/3,M10.5.0/4"; 

// OLED Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Loop forever if display fails
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting to WiFi...");
  display.display();

  // Connect to Wi-Fi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  // Configure time using NTP server and timezone string
  configTime(0, 0, ntpServer); // First call with 0,0 for legacy reasons
  setenv("TZ", tz_string, 1);  // Now set the TZ environment variable
  tzset();                     // Process the TZ variable

  Serial.println("\nTime configured.");
  display.println("Time configured!");
  display.display();
  delay(1000);
}

void loop() {
  struct tm timeinfo;

  // getLocalTime() automatically applies the timezone and DST rules
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }


  // --- Format the time and date for the OLED display ---
  char time_hour_minute[6]; // HH:MM + null terminator
  char time_seconds[4];     // :SS + null terminator
  char date_string[24];     // Day, Mon DD

  strftime(time_hour_minute, sizeof(time_hour_minute), "%H:%M", &timeinfo);
  strftime(time_seconds, sizeof(time_seconds), ":%S", &timeinfo);
  strftime(date_string, sizeof(date_string), "%A, %b %d", &timeinfo);

  // --- Update the OLED Display ---
  display.clearDisplay();
  
  // Display HH:MM in a large font
  display.setTextSize(2);
  display.setCursor(12, 0);
  display.print(time_hour_minute);

  // Display :SS in a smaller font right next to it
  display.setTextSize(1);
  display.setCursor(95, 6);
  display.print(time_seconds);

  // Display the date below
  display.setTextSize(1);
  display.setCursor(18, 22);
  display.print(date_string);

  display.display();

  delay(1000);
}