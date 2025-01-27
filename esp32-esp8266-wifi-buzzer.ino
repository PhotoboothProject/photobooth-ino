/*
 *  WIFI BUZZER for Photobooth
 *  Author https://github.com/flacoonb
 */

#include <OneButton.h>

#if defined(ESP32)
// ESP32 includes
#include <WiFi.h>
#include <WebServer.h>

#else

// ESP8266 includes
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#endif

#if SOFTWARE_SERIAL_AVAILABLE
#include <SoftwareSerial.h>
#endif

// PIN definition
OneButton button(D1, true);

// WIFI Connection
const char * ssid = "Your SSID";
const char * password = "Your WIFI Password";

// Define WIFI-IP and Gateway
WiFiServer server(80);
// Static IP of ESP8266 or ESP32
IPAddress ip(192, 168, 1, 100);
// Gateway (typically your router)
IPAddress gateway(192, 168, 1, 1);
// Photobooth remotebuzzer server IP
const char *photoboothIP = "192.168.1.50";
// Photobooth remotebuzzer server port
const int photoboothPort = 14711;

WiFiClient client;
HTTPClient http;

void setup(void) {
  delay(500);
  Serial.begin(115200);
  Serial.print(F("Setting up static IP: "));
  Serial.println(ip);

  // Connect to Wi-Fi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, IPAddress(255, 255, 255, 0));
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Print connection details
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  #if defined(ESP32)
    // Disable Wi-Fi sleep on ESP32
    esp_wifi_set_ps(WIFI_PS_NONE);
  #else
    // Disable Wi-Fi sleep on ESP8266
    WiFi.setSleep(false);
  #endif

  // Enable auto-reconnect and persistent settings
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // Start the server
  server.begin();
  Serial.println("Server started");

  Serial.println(F("GET Request to Remotebuzzer-Server ready!"));
  Serial.println(F("---------------------------------------"));

  // Button click events
  button.attachLongPressStop(longclick);
  button.attachClick(singleclick);
}

void loop(void) {
  button.tick();
  delay(10);
}

void singleclick() {
  Serial.println(F("GET request for single picture"));

  String url = String("http://") + photoboothIP + ":" + photoboothPort + "/commands/start-picture";
  http.begin(client, url);

  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
  } else {
    Serial.println("Error on HTTP request");
  }

  http.end();
}

void longclick() {
  Serial.println(F("GET Request for Collage"));

  String url = String("http://") + photoboothIP + ":" + photoboothPort + "/commands/start-collage";
  http.begin(client, url);

  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
  } else {
    Serial.println("Error on HTTP request");
  }

  http.end();
}
