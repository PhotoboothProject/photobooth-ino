/*
 *  WIFI BUZZER for Photobooth
 *  Author https://github.com/flacoonb
 */

#include <OneButton.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#if SOFTWARE_SERIAL_AVAILABLE
#include <SoftwareSerial.h>

#endif

// PIN definition
OneButton button(D1, true);

// WIFI Connection
const char * ssid = "SSID";
const char * password = "WIFI Password";

HTTPClient http;
WiFiClient client;

// Define WIFI-IP and Gateway
WiFiServer server(80);
IPAddress ip(xx, xx, xx, xx);
IPAddress gateway(xx, xx, xx, xx);

void setup(void) {
  delay(500);
  Serial.begin(115200);
  Serial.print(F("Setting up static IP: "));
  Serial.println(ip);

  // Connect WIFI
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Starte den server
  server.begin();
  Serial.println("Server started");

  // Show IP Address inside Serial Monitor
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  Serial.println(F("GET Request to Buzzer-Remote-Server ready!"));
  Serial.println(F("---------------------------------------"));

  /* Button Click Event */
  button.attachLongPressStop(longclick);
  button.attachClick(singleclick);
}

void loop(void) {
  button.tick();
  delay(10);
}

void singleclick() {
  Serial.println(F("GET request for single picture"));

  HTTPClient http;
  // http://[Photobooth IP]:[Port]/commands/start-picture
  http.begin(client, "http://xx.xx.xx.xx:14711/commands/start-picture");

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

  HTTPClient http;

  // http://[Photobooth IP]:[Port]/commands/start-collage
  http.begin(client, "http://xx.xx.xx.xx:14711/commands/start-collage");

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
