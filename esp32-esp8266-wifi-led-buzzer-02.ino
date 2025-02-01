/*
 * ESP8266 / ESP32 LED STRIPE and Wi-Fi Buzzer for Photobooth
 * Raphael Schib (https://github.com/flighter18)
 * and https://github.com/flacoonb
 *
 * Based on https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/strandtest/strandtest.ino
 * GET request handling from RENZO MISCHIANTI https://www.mischianti.org/2020/07/15/how-to-create-a-rest-server-on-esp8266-or-esp32-cors-request-option-and-get-part-4/
 */

#include "Arduino.h"
#include <OneButton.h>
#include <Adafruit_NeoPixel.h>

#if defined(ESP32)
// ESP32 includes
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <esp_wifi.h>

#else
// ESP8266 includes
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#endif

//
// U S E R S E T U P  S T A R T S  H E R E
//

// NeoPixels connected pin
#define LED_PIN D4

// NeoPixels count
#define LED_COUNT 60

// ADD SSID AND PASSWORD
const char *ssid = "Your SSID";
const char *password = "Your WIFI Password";

// Define WIFI-IP and Gateway
WebServer server(80);
// Static IP of ESP8266 or ESP32
IPAddress ip(192, 168, 1, 100);
// Gateway (typically your router)
IPAddress gateway(192, 168, 1, 1);
// Photobooth remotebuzzer server IP
const char *photoboothIP = "192.168.1.50";
// Photobooth remotebuzzer server port
const int photoboothPort = 14711;

// Button pin
OneButton button(D1, true);

// NeoPixels changing on Photo Countdown (value = LED_COUNT / Countdown in seconds)
int cntdwnPhoto = 6;

// NeoPixels changing on Collage Countdown (value = LED_COUNT / Countdown in seconds)
int cntdwnCollage = 12;

// Set BRIGHTNESS to about 1/5 (max = 255)
int brightness = 150;

// Declare our NeoPixel strip object
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// LED Colors
uint32_t color = strip.Color(0, 0, 0);
uint32_t fillColor = strip.Color(255, 255, 255);

// Timings
int waitPhotoled = 1000;
int waitRainbow = 4;
int wheelRangeRainbow = 65536;
int pixelAdditionRainbow = 256;
int waitTheaterChaseRainbow = 30;
int cyclesTheaterChaseRainbow = 20;
int holdTimeCollage = 2000;
int holdTimePhoto = 2000;
int holdTimeDoneCollage = 2000;
int holdTimeDonePhoto = 2000;

// Wi-Fi client and HTTP client
WiFiClient client;
HTTPClient http;

//
// U S E R S E T U P  E N D S  H E R E
//

void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}

void photoled(int wait, int ledChange, int holdTime) {
  int ledLimit = ledChange * 2;
  int p = LED_COUNT;

  strip.fill(fillColor, 2, LED_COUNT);
  strip.show();
  delay(1000);

  for (int i = 0; i < strip.numPixels();) {
    if (ledLimit > p) {
      strip.fill(color, i + p, p);
      i += p;
    } else {
      strip.fill(color, i, ledChange);
    }

    strip.show();
    if (i >= LED_COUNT) {
      delay(holdTime);
      break;
    }
    i += ledChange;
    p -= ledChange;
    delay(wait);
  }
  stripClear();
}

void rainbow(int wait, int wheelRange, int pixelAddition, int holdTime) {
  strip.clear();

  for (long firstPixelHue = 0; firstPixelHue < 1 * wheelRange; firstPixelHue += pixelAddition) {
    for (int i = 0; i < strip.numPixels(); i++) {
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    delay(wait);
  }
  delay(holdTime);
  stripClear();
}

void theaterChaseRainbow(int wait, int cycles, int holdTime) {
  int firstPixelHue = 0;
  for (int a = 0; a < cycles; a++) {
    for (int b = 0; b < 3; b++) {
      strip.clear();
      for (int c = b; c < strip.numPixels(); c += 3) {
        int hue = firstPixelHue + c * 65536L / strip.numPixels();
        strip.setPixelColor(c, strip.gamma32(strip.ColorHSV(hue)));
      }
      strip.show();
      delay(wait);
      firstPixelHue += 65536 / 90;
    }
  }
  delay(holdTime);
  stripClear();
}

void stripClear() {
  for (int ii = 0; ii < strip.numPixels(); ++ii) {
    strip.setPixelColor(ii, 0, 0, 0);
  }
  strip.show();
}

void greenLight() {
  strip.fill(strip.Color(0, 255, 0));
  strip.setBrightness(2000);
  strip.show();
  delay(2000);
  stripClear();
}

void photoCntdwn() {
  photoled(waitPhotoled, cntdwnPhoto, holdTimePhoto);
}

void collageCntdwn() {
  photoled(waitPhotoled, cntdwnCollage, holdTimeCollage);
}

void photoDone() {
  rainbow(waitRainbow, wheelRangeRainbow, pixelAdditionRainbow, holdTimeDonePhoto);
}

void collageDone() {
  theaterChaseRainbow(waitTheaterChaseRainbow, cyclesTheaterChaseRainbow, holdTimeDoneCollage);
}

void setCrossOrigin() {
  server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
  server.sendHeader(F("Access-Control-Max-Age"), F("600"));
  server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
  server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));
}

void restServerRouting() {
  setCrossOrigin();

  server.on(F("/CNTDWNCOLLAGE"), HTTP_GET, collageCntdwn);
  server.on(F("/CNTDWNPHOTO"), HTTP_GET, photoCntdwn);
  server.on(F("/chroma"), HTTP_GET, photoDone);
  server.on(F("/collage"), HTTP_GET, collageDone);
  server.on(F("/photo"), HTTP_GET, photoDone);

  server.onNotFound([]() {
    setCrossOrigin();
    String message = "File Not Found\n\n";
    message += "URI: " + server.uri() + "\nMethod: " + (server.method() == HTTP_GET ? "GET" : "POST") + "\nArguments: " + server.args() + "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
  });
}

// Button click events
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

void setup() {
  delay(500);
  Serial.begin(115200);

  // Initialize NeoPixel strip and turn off all pixels
  strip.begin();
  stripClear();

  // Connect to Wi-Fi
  Serial.print(F("Setting up static IP: "));
  Serial.println(ip);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, IPAddress(255, 255, 255, 0));
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    strip.fill(strip.Color(255, 0, 0));
    strip.setBrightness(2000);
    strip.show();
    delay(1000);
    Serial.print(".");
    stripClear();
  }

  greenLight();

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

  strip.setBrightness(brightness);

  // Setup REST server routing
  restServerRouting();

  // Button click event configuration
  button.attachLongPressStop(longclick);
  button.attachClick(singleclick);

  Serial.println(F("GET Request to Remotebuzzer-Server ready!"));
  Serial.println(F("---------------------------------------"));
}

void loop() {
  // Handle button events
  button.tick();
  
  // Handle REST API requests
  server.handleClient();
  
  // A short delay to avoid high CPU usage
  delay(10);
}
