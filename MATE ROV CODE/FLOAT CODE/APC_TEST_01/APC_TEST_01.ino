#include <AltSoftSerial.h>

AltSoftSerial radio;  // On Arduino Nano/Uno: RX = D8, TX = D9

const int pumpPin = 5;
const int valvePin = 6;
const int ledPin = 13;

unsigned long lastPrint = 0;
int counter = 0;

void setup() {
  // Keep pump/valve OFF for safety during this test
  pinMode(pumpPin, OUTPUT);
  pinMode(valvePin, OUTPUT);
  digitalWrite(pumpPin, LOW);
  digitalWrite(valvePin, LOW);

  pinMode(ledPin, OUTPUT);

  // USB debug, optional
  Serial.begin(9600);

  // APC220 serial
  radio.begin(9600);

  delay(1000);

  Serial.println("USB DEBUG: Arduino Nano started.");
  Serial.println("USB DEBUG: Sending APC220 test messages on D8/D9.");

  radio.println("APC220 TEST: Arduino Nano started.");
  radio.println("APC220 TEST: If you see this on the Mac-side APC220 terminal, the radio link works.");
}

void loop() {
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();

    digitalWrite(ledPin, !digitalRead(ledPin));

    radio.print("APC220 TEST MESSAGE #");
    radio.print(counter);
    radio.print("  millis=");
    radio.println(millis());

    Serial.print("USB DEBUG: Sent APC220 test message #");
    Serial.println(counter);

    counter++;
  }

  // Optional: if the Mac sends characters through APC220, echo them back
  if (radio.available()) {
    char c = radio.read();

    radio.print("APC220 ECHO: I received -> ");
    radio.println(c);

    Serial.print("USB DEBUG: APC220 received -> ");
    Serial.println(c);
  }
}