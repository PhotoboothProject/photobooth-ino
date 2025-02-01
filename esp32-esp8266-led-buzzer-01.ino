/*
 * ESP8266 / ESP32 LED STRIPE 
 * for Photobooth > https://github.com/andi34/photobooth
 * Raphael Schib (https://github.com/flighter18)
 *
 * Based on https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/strandtest/strandtest.ino
 * GET request handling from RENZO MISCHIANTI https://www.mischianti.org/2020/07/15/how-to-create-a-rest-server-on-esp8266-or-esp32-cors-request-option-and-get-part-4/
 */

#include <Arduino.h>

#if defined(ESP32)
// ESP32 includes
#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>

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
int cntdwnPhoto = 20;

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

const char INDEX_HTML[] =
  "<!DOCTYPE HTML>"
"<html>"
"<br><br>"
"Click <a href=\"/CNTDWNPHOTO\">here</a> to start the animation visible for photo countdown<br>"
"Click <a href=\"/CNTDWNCOLLAGE\">here</a> to start the animation visible for collage countdown<br>"
"Click <a href=\"/collage\">here</a> to start the animation visible once a collage was processed<br>"
"Click <a href=\"/photo\">here</a> to start the animation visible once a photo was processed<br>"
"Click <a href=\"/chroma\">here</a> to start the animation visible once a chroma photo was processed<br>"
"</html>";

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
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
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
        int hue = firstPixelHue + c * 65536L / strip.numPixels();
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

void testPage() {
  server.send(200, "text/html", INDEX_HTML);
}

void setCrossOrigin() {
  server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
  server.sendHeader(F("Access-Control-Max-Age"), F("600"));
  server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
  server.sendHeader(F("Access-Control-Allow-Headers"), F("*"));
};

void sendCrossOriginHeader() {
  Serial.println(F("sendCORSHeader"));
  setCrossOrigin();
  server.send(204);
}

void sendInfo() {
  setCrossOrigin();
  String message = "{\n";
  message += "  \"ip\": \"" + WiFi.localIP().toString() + "\",\n";
  message += "  \"gateway\": \"" + WiFi.gatewayIP().toString() + "\",\n";
  message += "  \"subnetMask\": \"" + WiFi.subnetMask().toString() + "\",\n";
  message += "  \"signalStrengh\": \"" + String(WiFi.RSSI()) + "\",\n";
  message += "  \"flashChipSize\": \"" + String(ESP.getFlashChipSize()) + "\",\n";
#if !defined(ESP32)
  message += "  \"flashChipRealSize\": \"" + String(ESP.getFlashChipRealSize()) + "\",\n";
  message += "  \"chipId\": \"" + String(ESP.getChipId()) + "\",\n";
  message += "  \"flashChipId\": \"" + String(ESP.getFlashChipId()) + "\",\n";
#endif
  message += "  \"freeHeap\": \"" + String(ESP.getFreeHeap()) + "\",\n";

  message += "\"\n";
  message += "}";
  server.send(200, F("application/json"), message);
}

void sendSuccess(String success) {
  setCrossOrigin();
  String message = "{\n";
  message += "  \"success\" : \"";
  message += success;
  message += "\"\n";
  message += "}";
  server.send(200, F("application/json"), message);
}

// Define routing
void restServerRouting() {
  server.on(F("/"), testPage);

  // Respond to HTTP_OPTIONS request
  // https://forum.arduino.cc/t/esp8266webserver-how-to-respond-jquery-get-request-http_options/457720
  server.on(F("/CNTDWNCOLLAGE"), HTTP_OPTIONS, []() {
    sendCrossOriginHeader();
  });

  server.on(F("/CNTDWNPHOTO"), HTTP_OPTIONS, []() {
    sendCrossOriginHeader();
  });

  server.on(F("/chroma"), HTTP_OPTIONS, []() {
    sendCrossOriginHeader();
  });

  server.on(F("/collage"), HTTP_OPTIONS, []() {
    sendCrossOriginHeader();
  });

  server.on(F("/photo"), HTTP_OPTIONS, []() {
    sendCrossOriginHeader();
  });

  // Respond to GET requests
  server.on(F("/CNTDWNCOLLAGE"), HTTP_GET, []() {
    sendSuccess("CollageCountdown");
    photoled(waitPhotoled, cntdwnCollage, holdTimeCollage);
  });

  server.on(F("/CNTDWNPHOTO"), HTTP_GET, []() {
    sendSuccess("PhotoCountdown");
    photoled(waitPhotoled, cntdwnPhoto, holdTimePhoto);
  });

  server.on(F("/chroma"), HTTP_GET, []() {
    sendSuccess("Chroma-done");
    rainbow(waitRainbow, wheelRangeRainbow, pixelAdditionRainbow, holdTimeDonePhoto);
  });

  server.on(F("/collage"), HTTP_GET, []() {
    sendSuccess("CollageDone");
    theaterChaseRainbow(waitTheaterChaseRainbow, cyclesTheaterChaseRainbow, holdTimeDoneCollage);
  });

  server.on(F("/photo"), HTTP_GET, []() {
    sendSuccess("PhotoDone");
    rainbow(waitRainbow, wheelRangeRainbow, pixelAdditionRainbow, holdTimeDonePhoto);
  });
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

// setup() function -- runs once at start
void setup() {
  // Start serial communication
  Serial.begin(115200);
  delay(10);

  // Print compilation info
  Serial.print(F("Date: "));
  Serial.println(__DATE__);
  Serial.print(F("Time: "));
  Serial.println(__TIME__);
  Serial.print(F("File: "));
  Serial.println(__FILE__);
  Serial.println();
  WiFi.mode(WIFI_STA);
  Serial.print(F("Setting static IP to: "));
  Serial.println(ip);
  WiFi.config(ip, gateway, subnet);
  Serial.println();
#if defined(ESP32)
  esp_wifi_set_ps(WIFI_PS_NONE);
#else
  WiFi.setSleep(false);
#endif
  // Connect to Wi-Fi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Print connected Wi-Fi details
  Serial.println("\nConnected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Enable auto-reconnect and persistent Wi-Fi
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // Setup REST server routing
  restServerRouting();
  server.onNotFound(handleNotFound);

  // Start HTTP server
  server.begin();
  Serial.println("HTTP Server started");

  // LED Strip setup
  Serial.print("LED Count: ");
  Serial.println(LED_COUNT);
  Serial.print("Turn off count for photo: ");
  Serial.println(cntdwnPhoto);
  Serial.print("Turn off count for collage: ");
  Serial.println(cntdwnCollage);

  // Initialize NeoPixel strip
  strip.begin();
  strip.show(); // Turn off all pixels
  strip.setBrightness(brightness);

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1); // Trinket-specific code
#endif
}

// loop() function -- runs repeatedly as long as board is on
void loop() {
  server.handleClient();
}
