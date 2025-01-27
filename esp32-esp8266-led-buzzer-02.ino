/*
 * ESP8266 / ESP32 LED STRIPE 
 * for Photobooth > https://github.com/andi34/photobooth
 * Raphael Schib (https://github.com/flighter18)
 *
 * Based on https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/strandtest/strandtest.ino
 * GET request handling from RENZO MISCHIANTI https://www.mischianti.org/2020/07/15/how-to-create-a-rest-server-on-esp8266-or-esp32-cors-request-option-and-get-part-4/
 */

#include "Arduino.h"

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

#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

//
// U S E R S E T U P  S T A R T S  H E R E
//
// NeoPixels connected pin
#define LED_PIN D4

// NeoPixels count
#define LED_COUNT 60

// ADD SSID AND PASSWORD
// *****************************
const char * ssid = "<Your-SSID>";
const char * password = "<Your-Password>";

// ADD NETWORK SETTINGS
WebServer server(80);
// Static IP of ESP8266 or ESP32
IPAddress ip(192, 168, 1, 100);
// Gateway (typically your router)
IPAddress gateway(192, 168, 1, 1);
 // set subnet mask to match your network
IPAddress subnet(255, 255, 255, 0);

// NeoPixels changing on Photo Countdown (value = LED_COUNT / Countdown in seconds), Photbooth defaults to 10 seconds
int cntdwnPhoto = 6;

// NeoPixels changing on Collage Countdown (value = LED_COUNT / Countdown in seconds), Photbooth defaults to 5 seconds
int cntdwnCollage = 12;

// Set BRIGHTNESS to about 1/5 (max = 255)
int brightness = 150;

// Declare our NeoPixel strip object:
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// LED Colors
uint32_t color = strip.Color(0, 0, 0);
uint32_t fillColor = strip.Color(255, 255, 255);

// Timings
int waitPhotoled = 1000; // Change pixel on countdown every defined ms, default 1000ms
int waitRainbow = 4; // Change pixel on photo/chroma done, default 4ms
int wheelRangeRainbow = 65536; // Color wheel range (default 65536) along the length of the strip
int pixelAdditionRainbow = 256; // Adding pixelAdditionRainbow (deafault) 256 to firstPixelHue each time
int waitTheaterChaseRainbow = 30; // Change pixel on collage done, default 30ms
int cyclesTheaterChaseRainbow = 20; // Cycles to run theaterChaseRainbow, default 20
int holdTimeCollage = 2000; // Time before strip get cleared after collage countdown
int holdTimePhoto = 2000; // Time before strip get cleared after photo countdown
int holdTimeDoneCollage = 2000; // Time before strip get cleared after collage was taken
int holdTimeDonePhoto = 2000; // Time before strip get cleared after photo was taken

//
// U S E R S E T U P  E N D S  H E R E
//

void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color); // Set pixel's color (in RAM)
    strip.show(); // Update strip to match
    delay(wait); // Pause for a moment
  }
}

void photoled(int wait, int ledChange, int holdTime) {
  int ledLimit = ledChange * 2;
  int p = LED_COUNT;

  strip.fill(fillColor, 2, LED_COUNT);
  strip.show();
  delay(1000);

  // For each pixel in strip...
  for (int i = 0; i < strip.numPixels();) {
    if (ledLimit > p) {
      strip.fill(color, i + p, p);
      i = i + p;
    } else {
      strip.fill(color, i, ledChange);
    }

    // Update strip to match
    strip.show();
    if (i >= LED_COUNT) {
      delay(holdTime);
      break;
    }
    i = i + ledChange;
    p = p - ledChange;
    delay(wait);
  }
  stripClear();

}

void rainbow(int wait, int wheelRange, int pixelAddition, int holdTime) {
  strip.clear();

  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for (long firstPixelHue = 0; firstPixelHue < 1 * wheelRange; firstPixelHue += pixelAddition) {
    for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536 / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait); // Pause for a moment
  }
  delay(holdTime);
  stripClear();
}

void theaterChaseRainbow(int wait, int cycles, int holdTime) {
  int firstPixelHue = 0; // First pixel starts at red (hue 0)
  for (int a = 0; a < cycles; a++) { // Repeat 20 times...
    for (int b = 0; b < 3; b++) { //  'b' counts from 0 to 2...
      strip.clear(); //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for (int c = b; c < strip.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int hue = firstPixelHue + c * 65536 L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      delay(wait); // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
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
};

// Define routing
void restServerRouting() {
  setCrossOrigin();
  int value = LOW;

  server.on(F("/CNTDWNCOLLAGE"), HTTP_GET, collageCntdwn);
  server.on(F("/CNTDWNPHOTO"), HTTP_GET, photoCntdwn);
  server.on(F("/chroma"), HTTP_GET, photoDone);
  server.on(F("/collage"), HTTP_GET, collageDone);
  server.on(F("/photo"), HTTP_GET, photoDone);

  String message = "ok\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(200, "text/plain", message);
}

// Manage not found URL
void handleNotFound() {
  setCrossOrigin();
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

// setup() function -- runs once at startup
void setup() {
  // Start serial communication
  Serial.begin(115200);
  delay(10);

  // Print static IP configuration
  Serial.print(F("Setting static IP to: "));
  Serial.println(ip);

  // Connect to Wi-Fi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Print connection details for ESP32
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

#if defined(ESP32)
  esp_wifi_set_ps(WIFI_PS_NONE);
#else
  WiFi.setSleep(false);
#endif
 
  // Enable auto-reconnect and persistent settings
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // Configure server routing and start the server
  restServerRouting();
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP Server started");

  // LED Strip setup
  Serial.print("LED Count: ");
  Serial.println(LED_COUNT);
  Serial.print("Turn off count for photo: ");
  Serial.println(cntdwnPhoto);
  Serial.print("Turn off count for collage: ");
  Serial.println(cntdwnCollage);
  Serial.println();

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  // Trinket-specific code
  clock_prescale_set(clock_div_1);
#endif

  // Initialize NeoPixel strip and turn off all pixels
  strip.begin();
  strip.show();
  strip.setBrightness(brightness);
}

// loop() function -- runs repeatedly as long as board is on
void loop() {
  server.handleClient();
}
