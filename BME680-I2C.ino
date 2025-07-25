#include <WiFi.h>
#include <HTTPClient.h>
#include "Zanshin_BME680.h"

// ===============================================================
//                          USER CONFIGURATION
// ===============================================================

// -- WiFi Credentials --
const char* ssid = "Home";
const char* password = "23456789";

// -- Google Form Details --
String GOOGLE_FORM_ID = "1FAIpQLSe9YtyCJNZHHzoIm2fO3zTcjRiBhZDinc00IJTBaFMqzGSJLw";
String GOOGLE_FORM_FIELD_TEMPERATURE = "entry.800561544";
String GOOGLE_FORM_FIELD_HUMIDITY    = "entry.1271334897";
String GOOGLE_FORM_FIELD_PRESSURE    = "entry.1511590799";
String GOOGLE_FORM_FIELD_ALTITUDE    = "entry.2050183380";
String GOOGLE_FORM_FIELD_GAS         = "entry.994873585";

// -- Deep Sleep Configuration --
// How often to wake up and send data (in seconds)
uint64_t uS_TO_S_FACTOR = 1000000;  // Conversion factor for micro seconds to seconds
uint64_t TIME_TO_SLEEP  = 60*2;       // Time ESP32 will be in deep sleep (in seconds)

// ===============================================================
//                         HELPER FUNCTIONS
// ===============================================================

BME680_Class BME680;

float altitude(const int32_t press, const float seaLevel = 1009.5) {
  return 44330.0 * (1.0 - pow(((float)press / 100.0) / seaLevel, 0.1903));
}

void sendDataToGoogleForm(float temperature, float humidity, float pressure, float alt, float gas) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Error: WiFi is not connected.");
    return;
  }

  HTTPClient http;
  String url = "https://docs.google.com/forms/d/e/" + GOOGLE_FORM_ID + "/formResponse";
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Create the data payload
  String postData = GOOGLE_FORM_FIELD_TEMPERATURE + "=" + String(temperature, 2) + "&" +
                    GOOGLE_FORM_FIELD_HUMIDITY    + "=" + String(humidity, 2) + "&" +
                    GOOGLE_FORM_FIELD_PRESSURE    + "=" + String(pressure, 2) + "&" +
                    GOOGLE_FORM_FIELD_ALTITUDE    + "=" + String(alt, 2) + "&" +
                    GOOGLE_FORM_FIELD_GAS         + "=" + String(gas, 2);

  Serial.println("Sending data to Google Form...");
  Serial.print("POST Data: ");
  Serial.println(postData);

  // Send the POST request
  int httpCode = http.POST(postData);

  if (httpCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpCode);
    if (httpCode == 200) {
      Serial.println("Successfully submitted form data.");
    } else {
      String payload = http.getString();
      Serial.println("Server Response:");
      Serial.println(payload);
    }
  } else {
    Serial.printf("HTTP POST request failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

// ===============================================================
//                              SETUP
//           (This function now runs on every wake-up)
// ===============================================================

void setup() {
  Serial.begin(115200);
  delay(1000); // Allow time to open Serial Monitor

  // --- Configure Deep Sleep Timer ---
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Woke up from sleep. Starting process...");

  // --- Connect to WiFi ---
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int wifi_retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifi_retries++;
    if (wifi_retries > 20) { // Timeout after 10 seconds
        Serial.println("\nFailed to connect to WiFi. Going back to sleep.");
        esp_deep_sleep_start(); // Sleep immediately if WiFi fails
    }
  }
  Serial.println("\nWiFi connected!");

  // --- Initialize BME680 Sensor ---
  Serial.println("- Initializing BME680 sensor");
  if (!BME680.begin(I2C_STANDARD_MODE)) {
    Serial.println("- Unable to find BME680. Going back to sleep.");
    esp_deep_sleep_start(); // Sleep if sensor not found
  }
  BME680.setOversampling(TemperatureSensor, Oversample16);
  BME680.setOversampling(HumiditySensor, Oversample16);
  BME680.setOversampling(PressureSensor, Oversample16);
  BME680.setIIRFilter(IIR4);
  BME680.setGas(320, 150);

  // --- Take Sensor Readings ---
  // Allow time for sensor to stabilize after power-on
  Serial.println("Waiting for sensor to get a reading...");
  delay(2000); // Small delay to ensure sensor is ready
  
  static int32_t temp, humidity, pressure, gas;
  BME680.getSensorData(temp, humidity, pressure, gas);

  // Check for valid readings (sometimes BME680 returns 0 on first read)
  if (pressure == 0) {
    Serial.println("Failed to read from sensor. Retrying once.");
    delay(1000);
    BME680.getSensorData(temp, humidity, pressure, gas); // Try again
  }
  
  if (pressure != 0) {
      // --- Process and Send Data ---
      float alt = altitude(pressure);
      float temp_c = (float)temp / 100.0;
      float humidity_p = (float)humidity / 1000.0;
      float pressure_hpa = (float)pressure / 100.0;
      float gas_kohm = (float)gas / 100.0; // Assuming gas is in ohms, not kohms from library

      Serial.print("  Temperature = "); Serial.print(temp_c, 2); Serial.println(" *C");
      Serial.print("  Humidity    = "); Serial.print(humidity_p, 2); Serial.println(" %");
      Serial.print("  Pressure    = "); Serial.print(pressure_hpa, 2); Serial.println(" hPa");
      Serial.print("  Altitude    = "); Serial.print(alt, 2); Serial.println(" m");
      Serial.print("  Gas         = "); Serial.print(gas_kohm, 2); Serial.println(" KOhms");

      sendDataToGoogleForm(temp_c, humidity_p, pressure_hpa, alt, gas_kohm);
  } else {
      Serial.println("Failed to read from sensor after two attempts. Going to sleep.");
  }


  // --- Go to Sleep ---
  Serial.print("All tasks complete. Going to sleep for ");
  Serial.print(TIME_TO_SLEEP);
  Serial.println(" seconds.");
  
  WiFi.disconnect(true); // Disconnect WiFi before sleeping
  esp_deep_sleep_start();
}


// ===============================================================
//                              LOOP
//           (This function will never be reached)
// =================================M==============================

void loop() {
  // This part is not used when in a deep sleep cycle.
  // The ESP32 will restart and run setup() again on wake-up.
}