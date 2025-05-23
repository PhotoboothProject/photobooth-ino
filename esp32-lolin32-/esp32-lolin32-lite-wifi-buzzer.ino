/**************************************************************************************************
 *
 * ESP-32 Lolin32 Lite Remote for Photobooth -> https://photoboothproject.github.io/
 * Telegram -> https://t.me/PhotoboothGroup
 *
 *
 * Unter Benutzereinstellung WLAN-SSID, WLAN-PW, IP-Fotobox, IP-Router und IP-ESP eintragen/anpassen.
 *
 * Anschlüsse ESP:  5 - Collage-Taste
 *                 17 - Foto-Taste
 *                 18 - Druck-Taste
 *                 19 - SDA-Display
 *                 23 - SDC-Display
 *                 34 - Spannungsteiler (3x10kOhm Wiederstände zw. Masse und + am Akku, bei 2/3 abgreifen)
 *
 *
 * Lademodus starten: beim Wlan-Verbindungsaufbau die "Druck-Taste" betätigen
 * Lademodus benden:  "Foto-Taste" betätigen -> reset , oder auschalten
 *
**************************************************************************************************/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <esp_sleep.h>       

// ===== User Settings =====
const char* ssid = "!!!FotoBox!!!";      // WLAN name
const char* password = "diefotobox";     // WLAN password
char HOST_NAME[] = "192.168.15.10";      // Remote Buzzer Server IP
int HTTP_PORT = 14711;                   // PORT for GET Request

IPAddress staticIP(192, 168, 15, 17);  // IP des Buzzer-ESP
IPAddress gateway(192, 168, 15, 1);    // IP Router
IPAddress subnet(255, 255, 255, 0);    // Subnet-Maske
IPAddress dns(192, 168, 15, 1);        // IP Router
// ===== User Settings End =====

// ===== Display Settings =====
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== Define I2C Pins =====
#define SDA_PIN 19         // I2C SDA pin
#define SCL_PIN 23         // I2C SCL pin

// ===== Define GPIO Pins =====
#define PHOTO_BUTTON   17  // Pin for photo button
#define COLLAGE_BUTTON 5   // Pin for collage button
#define PRINT_BUTTON   18  // Pin for print button
#define LED_PIN        22  // LED for connection feedback
#define VOLTAGE_SENSOR 34  // Voltage sensor pin (for battery measurement)

// ===== Other Global Variables =====
int val = 0;
float vbat = 0;
int batProz = 0;
int korrF = 250;  // Korrekturfaktor in mV (orginal 110, ggf. anpassen damit bei vollem Akku ca 100% angezeigt wird)
long batteryMeasureTimer = 0;
const long batteryMeasureInterval = 20000;  // Interval between full battery measurements
int lademodus = 0;

String HTTP_METHOD = "GET";          // or "POST"
String PATH_NAME = "";               // String PATH_NAME   = "/commands/start-picture";

String WLANStatus[7];  // Array für WLAN-Status Codes

// Logo Photobooth-Bitmap
const unsigned char Photobooth [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0x7f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xc0, 0x00, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x07, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x7f, 0xe0, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0xc0, 0x18, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x82, 0x0c, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x3f, 0xe2, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x06, 0x60, 0x33, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x0c, 0xc0, 0x11, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x09, 0x80, 0x00, 0x80, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x09, 0x00, 0x04, 0x80, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x19, 0x00, 0x04, 0xc0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x13, 0x00, 0x06, 0xc0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x12, 0x00, 0x06, 0xc0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x13, 0x00, 0x06, 0xc0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x1b, 0x00, 0x04, 0xc0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x19, 0x00, 0x04, 0x80, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x09, 0x80, 0x0c, 0x80, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x0c, 0xc0, 0x19, 0x80, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04, 0x60, 0x33, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x02, 0x3f, 0xe2, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x07, 0x0c, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0xc0, 0x18, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x7d, 0xf0, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x0f, 0x80, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x07, 0x82, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x0f, 0xe2, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x32, 0x00, 0x00, 0x20, 0x00, 0x20, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x12, 0x00, 0x00, 0x20, 0x00, 0x20, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x12, 0x00, 0x00, 0x20, 0x00, 0x20, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x12, 0x00, 0x00, 0x20, 0x00, 0x20, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x12, 0xf0, 0x3c, 0x3c, 0x7c, 0x2f, 0x07, 0x81, 0xe1, 0xe5, 0xe0, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x33, 0x08, 0x42, 0x20, 0xc6, 0x31, 0x88, 0x42, 0x31, 0x06, 0x30, 0x00, 0x00, 
	0x00, 0x00, 0x0f, 0xe3, 0x04, 0xc1, 0x20, 0x82, 0x20, 0x98, 0x24, 0x11, 0x04, 0x10, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x02, 0x04, 0x81, 0x20, 0x82, 0x20, 0x90, 0x24, 0x19, 0x04, 0x18, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x02, 0x04, 0x81, 0x20, 0x82, 0x20, 0x90, 0x24, 0x09, 0x04, 0x18, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x02, 0x04, 0x81, 0x20, 0x82, 0x20, 0x90, 0x24, 0x09, 0x04, 0x18, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x02, 0x04, 0x81, 0x20, 0x82, 0x20, 0x90, 0x24, 0x19, 0x04, 0x18, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x02, 0x04, 0xc3, 0x20, 0x82, 0x20, 0x88, 0x66, 0x11, 0x04, 0x18, 0x00, 0x00, 
	0x00, 0x00, 0x08, 0x02, 0x04, 0x66, 0x10, 0x44, 0x31, 0x0c, 0xc2, 0x31, 0x84, 0x18, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x0c, 0x38, 0x0e, 0x03, 0x81, 0xc0, 0x60, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// ========= Reset Function =========
void (*resetFunc)(void) = 0;

// ========= Battery Status Function =========
void battStatus() {
  // Take 100 readings with a 10ms delay between them
  for (int i = 0; i <= 100; i++) {
    val = val + analogRead(VOLTAGE_SENSOR);
    delay(10);
  }
  val = val / 100;
  vbat = map(val, 0, 4095, 0, 3300);        // DAC Counts umrechnen in mV
  vbat = vbat / 2 * 3;                      // SPannungsteiler 2/3
  vbat = vbat + korrF;                      // Korrekturfaktor hunzufügen
  batProz = map(vbat, 3300, 4180, 0, 100);  // 0...100% Grenzspannungen festlegen (in mV)  3,1v bis 4,18
  vbat = vbat / 1000.00;  // Convert to volts

  // Clear and update the OLED display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Fotobox-Fernbedienung"));
  display.setCursor(0, 8);
  display.print("Akku: ");
  display.print(vbat, 2);
  display.print(" V ");
  display.print("ca.");
  display.print(batProz);
  display.print("%");
  display.println("");

  if (lademodus == 1) {
    display.println("Lademodus mit Foto");
    display.println("beenden! => RESET");
  }

  display.drawRect(0, 50, 127, 10, SSD1306_WHITE);
  display.fillRect(0, 50, map(batProz, 0, 100, 0, 127), 10, SSD1306_WHITE);
  display.display();
}

// ========= Charging Mode =========
void runChargingMode() {
  while (digitalRead(PHOTO_BUTTON) == HIGH) {  // Schleife, bis PHOTO gedrückt wird
    lademodus = 1;
    battStatus();
    delay(100);
  }
  display.println("Lademodus beendet!");
  display.display();
  delay(5000);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.display();
  resetFunc();
}

// ========= LED Flash Function =========
void ledflash(int times, int speed) {
  for (int i = 0; i <= times - 1; i++) {
    digitalWrite(LED_PIN, 1);
    delay(speed);
    digitalWrite(LED_PIN, 0);
    delay(speed);
  }
}

// ========= WiFi Connection =========
void connect_wifi() {
  WLANStatus[0] = "WLAN idle";
  WLANStatus[1] = "No WLAN available";
  WLANStatus[2] = "WLAN scan completed";
  WLANStatus[3] = "WLAN connected";
  WLANStatus[4] = "WLAN connect failed";
  WLANStatus[5] = "WLAN connection lost";
  WLANStatus[6] = "WLAN disconnected";
  delay(10);

  Serial.println();
  Serial.print("WLAN: ");
  Serial.println(ssid);
  display.print("WLAN: ");
  display.print(ssid);
  display.display();

  if (WiFi.config(staticIP, gateway, subnet, dns, dns) == false) {
    Serial.println("Configuration failed.");
    display.print("Config failed.");
    display.display();
  }

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED && digitalRead(PRINT_BUTTON) == HIGH) {
    ledflash(10, 50);
    Serial.print(".");
    display.print(".");
    display.display();
  }
  if (digitalRead(PRINT_BUTTON) == LOW) {
    Serial.println("PRINT-Taster wurde gedrückt, starte Lademodus ohne WIFI");
    runChargingMode();
  }

  display.println("");
  display.println("WiFi connected");
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  digitalWrite(LED_PIN, 0);
}

// ========= Send GET Request =========
void SendGETrequest(String PATH_NAME) {
  WiFiClient client;
  if (client.connect(HOST_NAME, HTTP_PORT)) {
    display.setCursor(9, 16);
    display.print("gestartet");
    display.display();
    ledflash(2, 500);
  } else {
    display.setTextSize(1);
    display.println("Verbindungsfehler");
    display.display();
    ledflash(10, 50);
  }
  // send HTTP request header
  client.println(HTTP_METHOD + " " + PATH_NAME + " HTTP/1.1");
  client.println("Host: " + String(HOST_NAME));
  client.println("Connection: close");
  client.println();

  display.setCursor(4, 32);
  display.println("Verbindung");
  display.setCursor(22, 48);
  display.println("beendet");
  display.display();
  delay(5000);

  battStatus();
}

// ========= Setup =========
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PHOTO_BUTTON, INPUT_PULLUP);
  pinMode(COLLAGE_BUTTON, INPUT_PULLUP);
  pinMode(PRINT_BUTTON, INPUT_PULLUP);
  delay(100);

  // start I2C Display
  Wire.begin(SDA_PIN, SCL_PIN);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  // Photobooth Logo
  display.clearDisplay();
  // drawBitmap(x position, y position, bitmap data, bitmap width, bitmap height, color)
  display.drawBitmap(0, 0, Photobooth, 128, 64, WHITE);
  display.display();
  delay(5000);

  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.display();
  battStatus();
}

// ========= Main Loop =========
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    display.println(WLANStatus[WiFi.status()]);
    display.display();
    connect_wifi();
  }

  // Check buttons for actions
  if (digitalRead(PHOTO_BUTTON) == LOW) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(34, 0);
    display.println("FOTO");
    display.display();
    PATH_NAME = "/commands/start-picture";
    SendGETrequest(PATH_NAME);
  }

  if (digitalRead(COLLAGE_BUTTON) == LOW) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(22, 0);
    display.println("COLLAGE");
    display.display();
    PATH_NAME = "/commands/start-collage";
    SendGETrequest(PATH_NAME);
  }

  if (digitalRead(PRINT_BUTTON) == LOW) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(34, 0);
    display.println("DRUCKEN");
    display.display();
    PATH_NAME = "/commands/start-print";
    SendGETrequest(PATH_NAME);
  }

  if (millis() > batteryMeasureTimer + batteryMeasureInterval) {
    battStatus();
    batteryMeasureTimer = millis();
  }
}
