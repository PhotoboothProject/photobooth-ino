/**
 * Turn ESP32 into a Bluetooth LE keyboard
 * Get needed library: https://github.com/andi34/ESP32-BLE-Keyboard/archive/refs/heads/master.zip
 */

#include <BleKeyboard.h>
#include <KeyCodes.h>

#define BUTTON_PIN_F 2
#define BUTTON_PIN_C 4
#define SIGNAL_PIN 5
#define DEVICE_NAME "FJTouchBuzzer"
#define DEVICE_MANUFACTURER "Photobooth"

// Vorheriger Zustand des Foto-Touch-Buttons
int lastState_F = LOW;
// Aktueller Zustand des Foto-Touch-Buttons
int currentState_F;
// Vorheriger Zustand des Collage-Touch-Buttons
int lastState_C = LOW;
// Aktueller Zustand des Collage-Touch-Buttons
int currentState_C;
// Pin für die Status-LED
int LED = 16;

BleKeyboard bleKeyboard(DEVICE_NAME, DEVICE_MANUFACTURER, 100);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleKeyboard.setName(DEVICE_NAME);
  bleKeyboard.begin();

  // Konfiguriere Foto-Touch-Button-Pin als Eingang mit Pull-up-Widerstand
  pinMode(BUTTON_PIN_F, INPUT_PULLUP);
  // Konfiguriere Collage-Touch-Button-Pin als Eingang mit Pull-up-Widerstand
  pinMode(BUTTON_PIN_C, INPUT_PULLUP);
  // Konfiguriere Bewegungssensor-Pin als Eingang
  pinMode(SIGNAL_PIN, INPUT);
  // Konfiguriere LED-Pin als Ausgang
  pinMode(LED, OUTPUT);
}

void loop() {
  if(bleKeyboard.isConnected()) {
    // Lese den Zustand der Tasten (Foto und Collage):
    currentState_F = digitalRead(BUTTON_PIN_F);
    currentState_C = digitalRead(BUTTON_PIN_C);

    if(digitalRead(SIGNAL_PIN) == HIGH) {
      // Bewegung erkannt
      Serial.println("Movement detected.");
      // Schalte die Status-LED ein
      digitalWrite(LED, HIGH);
    } else {
      // Keine Bewegung erkannt
      Serial.println("Did not detect movement.");
      // Schalte die Status-LED aus
      digitalWrite(LED, LOW);
    }

    // Foto-Touch-Button wurde gedrückt
    if(lastState_F == HIGH && currentState_F == LOW) {
      Serial.println("Touch-Button Foto press is detected");
      Serial.println("Sending KEY_PAGE_UP key.");
      // Sende den Tastenanschlag "KEY_PAGE_UP" über Bluetooth
      bleKeyboard.write(KEY_PAGE_UP);
      delay(500);
     }

    // Collage-Touch-Button wurde gedrückt
    if(lastState_C == HIGH && currentState_C == LOW) { 
      Serial.println("Touch-Button Collage press is detected");
      Serial.println("Sending KEY_PAGE_DOWN key.");
      // Sende den Tastenanschlag "KEY_PAGE_DOWN" über Bluetooth
      bleKeyboard.write(KEY_PAGE_DOWN);
      delay(500);
    }
    lastState_F = currentState_F;
    lastState_C = currentState_C;
  } else {
    // Wenn nicht verbunden, schalte die Onboard-LED alle 500 Millisekunden ein und aus
    digitalWrite(LED, LOW);
    delay(500);
    digitalWrite(LED, HIGH);
    delay(500);
  }
  delay(500);
}
