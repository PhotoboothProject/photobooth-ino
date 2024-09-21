/**
 * Turn ESP32 into a Bluetooth LE keyboard
 * Get needed library: https://github.com/andi34/ESP32-BLE-Keyboard/archive/refs/heads/master.zip
 *
 * Button - Long Press Short Press
 * 
 * Created by ArduinoGetStarted.com
 *
 * This example code is in the public domain
 *
 * Tutorial page: https://arduinogetstarted.com/tutorials/arduino-button-long-press-short-press
 */

// The NimBLE mode enables a significant
// saving of RAM and FLASH memory.
#define USE_NIMBLE

#include <BleKeyboard.h>
#include <KeyCodes.h>

// Change the below values if desired
#define BUTTON_PIN 2
#define DEVICE_NAME "ESP32 Photo-Buzzer"
#define DEVICE_MANUFACTURER "Photobooth"

//
// Short button press = keycode 33 (PageUp)
// Long button press = keycode 34 (PageDown)
//
const int LONG_PRESS_TIME  = 1500; // 1500 milliseconds

// Variables will change:
int lastState = LOW;  // the previous state from the input pin
int currentState;     // the current reading from the input pin
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;


BleKeyboard bleKeyboard(DEVICE_NAME, DEVICE_MANUFACTURER, 100);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleKeyboard.setName(DEVICE_NAME);
  bleKeyboard.begin();

  // configure pin for button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  if(bleKeyboard.isConnected()) {
    // read the state of the switch/button:
    currentState = digitalRead(BUTTON_PIN);

    if(lastState == HIGH && currentState == LOW)        // button is pressed
      pressedTime = millis();
    else if(lastState == LOW && currentState == HIGH) { // button is released
      releasedTime = millis();

      long pressDuration = releasedTime - pressedTime;

      if( pressDuration < LONG_PRESS_TIME ) {
        Serial.println("A short press is detected");
        Serial.println("Sending KEY_PAGE_UP key.");
        bleKeyboard.write(KEY_PAGE_UP);
      }

      if( pressDuration >= LONG_PRESS_TIME ) {
        Serial.println("A long press is detected");
        Serial.println("Sending KEY_PAGE_DOWN key.");
        bleKeyboard.write(KEY_PAGE_DOWN);
      }
    }
    // save the the last state
    lastState = currentState;
  }
  delay(100);
}
