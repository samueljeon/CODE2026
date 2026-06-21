#include <Wire.h>
#include "Arduino.h"
#include <RTClib.h>
#include "uEEPROMLib.h"
#include <SparkFun_PHT_MS8607_Arduino_Library.h>
#include <AltSoftSerial.h>
#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
#include <avr/power.h>
#endif

#define LED_PIN 11

const int pressureInput = A3; // analog pinzz
float pressureZero = 90.5;
float pressureMax = 920.7;
const int pressuretransducermaxPSI = 30;
const int baudRate = 9600;
const int sensorreadDelay = 250; // not being used?
const int pump = 5;
const int valve = 6;
int f_size = 4;  // float has four bytes
int i_size = 1;
int year_size = 2;

bool start = false;
bool descend = false;
bool ascend = false;
bool written = false; // not being used?
bool bottom = false;
bool transmit = false;
bool surface = false;

AltSoftSerial softSerial;  // for sending logs to console/serial
MS8607 barometricSensor; // for getting pressure, humidity, and temperature
RTC_DS3231 rtc;  // for getting time
uEEPROMLib eeprom(0x57);

float init_pressure; // not being used?
int datapoints_counter = 0;
unsigned int addr_EEPROM;
uint8_t month, day, hour, minute, second;
uint16_t year;
float temperature, humidity, pressure, outer_pressure = 0, analog_pressure = 0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(10, LED_PIN, NEO_GRB + NEO_KHZ800);

void theaterChase(uint32_t c, uint8_t wait);  // function for how the light should display c = color, wait= time between intervals
void initializeComponents();                  // booting up the float
void logDataToEEPROM();                       // write data to the eeprom
void readDataFromEEPROM();                    // read the data to transmit
void handleDescent();
void handleAscent();

void setup() {
  initializeComponents();
}

void loop() {
  if (start) {
    if (softSerial.available()) {
      char val = softSerial.read();
      if (val == 'D' && !descend && !ascend) {
        softSerial.println("Float's descent...");
        handleDescent();
      } else if (val == 'T' && surface) {
        handleAscent();
        datapoints_counter = 0;
      }
    }
  } else {
    softSerial.println("Start failed..");
  }
}

void initializeComponents() {
  Wire.begin();
  rtc.begin();
  Serial.begin(baudRate);
  softSerial.begin(baudRate);
  strip.begin();
  strip.show();

  #if defined(__AVR_ATtiny85__)
    if (F_CPU == 16000000)
      clock_prescale_set(clock_div_1);
  #endif

  // set the clock the same of the laptop
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // error handling for Barometric meter
  if (!barometricSensor.begin()) {
    softSerial.println("MS8607 sensor did not respond. Please check wiring.");
    while (1)
      ;
  }

  int err = barometricSensor.set_humidity_resolution(MS8607_humidity_resolution_12b);
  if (err != MS8607_status_ok) {
    softSerial.print("Problem setting the MS8607 sensor humidity resolution. Error code = ");
    softSerial.println(err);
    while (1)
      ;
  }

  err = barometricSensor.disable_heater();
  if (err != MS8607_status_ok) {
    softSerial.print("Problem disabling the MS8607 humidity sensor heater. Error code = ");
    softSerial.println(err);
    while (1)
      ;
  }

  // set the pinModes so that the air pump can do its thing
  pinMode(pump, OUTPUT);
  pinMode(valve, OUTPUT);

  // visible feedback from the float that the float is working as it should
  theaterChase(strip.Color(0, 255, 0), 50);  // green
  digitalWrite(pump, HIGH);
  digitalWrite(valve, HIGH);
  softSerial.println("Please wait 10s while the bladder is being filled.");

  delay(9000);
  digitalWrite(pump, LOW);
  start = true;
  softSerial.println("Program has started. The float will stay above water before the rover moves it into position.");
}

void logDataToEEPROM() {
  DateTime now = rtc.now();
  month = now.month();
  day = now.day();
  year = now.year();
  hour = now.hour();
  minute = now.minute();
  second = now.second();
  temperature = barometricSensor.getTemperature();
  pressure = barometricSensor.getPressure() / 10;
  humidity = barometricSensor.getHumidity();
  analog_pressure = analogRead(pressureInput);
  outer_pressure = (analog_pressure - pressureZero) * (pressuretransducermaxPSI / (pressureMax - pressureZero));
  outer_pressure = (outer_pressure * 6.89476) + pressure;  // 6.89476 kPa = 1 PSI

  eeprom.eeprom_write(addr_EEPROM, month);
  addr_EEPROM += i_size;
  eeprom.eeprom_write(addr_EEPROM, day);
  addr_EEPROM += i_size;
  eeprom.eeprom_write(addr_EEPROM, year);
  addr_EEPROM += year_size;
  eeprom.eeprom_write(addr_EEPROM, hour);
  addr_EEPROM += i_size;
  eeprom.eeprom_write(addr_EEPROM, minute);
  addr_EEPROM += i_size;
  eeprom.eeprom_write(addr_EEPROM, second);
  addr_EEPROM += i_size;

  eeprom.eeprom_write(addr_EEPROM, temperature);
  addr_EEPROM += f_size;
  eeprom.eeprom_write(addr_EEPROM, pressure);
  addr_EEPROM += f_size;
  eeprom.eeprom_write(addr_EEPROM, humidity);
  addr_EEPROM += f_size;
  // eeprom.eeprom_write(addr_EEPROM, compensated_RH); addr_EEPROM += f_size;
  // eeprom.eeprom_write(addr_EEPROM, dew_point); addr_EEPROM += f_size;
  eeprom.eeprom_write(addr_EEPROM, outer_pressure);
  addr_EEPROM += f_size;
}

void readDataFromEEPROM() {
  uint8_t read_month, read_day, read_hour, read_minute, read_second;
  uint16_t read_year;
  float read_temperature, read_pressure, read_humidity, read_outer_pressure;

  for (int i = 0; i < datapoints_counter; i++) {
    eeprom.eeprom_read(addr_EEPROM, &read_month);
    addr_EEPROM += i_size;
    eeprom.eeprom_read(addr_EEPROM, &read_day);
    addr_EEPROM += i_size;
    eeprom.eeprom_read(addr_EEPROM, &read_year);
    addr_EEPROM += year_size;
    eeprom.eeprom_read(addr_EEPROM, &read_hour);
    addr_EEPROM += i_size;
    eeprom.eeprom_read(addr_EEPROM, &read_minute);
    addr_EEPROM += i_size;
    eeprom.eeprom_read(addr_EEPROM, &read_second);
    addr_EEPROM += i_size;

    eeprom.eeprom_read(addr_EEPROM, &read_temperature);
    addr_EEPROM += f_size;
    eeprom.eeprom_read(addr_EEPROM, &read_pressure);
    addr_EEPROM += f_size;
    eeprom.eeprom_read(addr_EEPROM, &read_humidity);
    addr_EEPROM += f_size;
    // eeprom.eeprom_read(addr_EEPROM, &read_compensated_RH);  addr_EEPROM += f_size;
    // eeprom.eeprom_read(addr_EEPROM, &read_dew_point);  addr_EEPROM += f_size;
    eeprom.eeprom_read(addr_EEPROM, &read_outer_pressure);
    addr_EEPROM += f_size;

    softSerial.print("Date: ");
    softSerial.print(read_month);
    softSerial.print('/');
    softSerial.print(read_day);
    softSerial.print('/');
    softSerial.print(read_year);

    softSerial.print("  Time: ");
    softSerial.print(read_hour);
    softSerial.print(':');
    softSerial.print(read_minute);
    softSerial.print(':');
    softSerial.print(read_second);
    softSerial.print(" ");

    softSerial.print("Temperature= ");
    softSerial.print(read_temperature);
    softSerial.print("C, ");

    softSerial.print("Inner Pressure= ");
    softSerial.print(read_pressure);
    softSerial.print(" kPa, ");

    softSerial.print("Humidity= ");
    softSerial.print(read_humidity);
    softSerial.print("%RH, ");

    softSerial.print("Outer Pressure= ");
    softSerial.print(read_outer_pressure);
    softSerial.println(" kpa");
  }
}

void handleDescent() {
  descend = true;
  theaterChase(strip.Color(255, 0, 0), 50);
  digitalWrite(pump, LOW);
  digitalWrite(valve, LOW);

  addr_EEPROM = 0;
  float previous_outer_pressure;
  unsigned long ascendStartTime;
  unsigned long maxAscendTime = 40000;
  unsigned long descentStartTime = millis();
  unsigned long maxDescentTime = 40000;  // 40 seconds (adjust based on expected max descent time)

  softSerial.println("Writing data while the float is descending");

  while (descend) {
    datapoints_counter++;
    delay(5000);  // 1 second delay for data collection
    previous_outer_pressure = outer_pressure;
    logDataToEEPROM();

    if (millis() - descentStartTime > maxDescentTime) {
      softSerial.println("Reached maximum descent time. Assuming the float has reached the bottom.");
      bottom = true;
      break;
    }

    if ((millis() - descentStartTime > 20000) && abs(outer_pressure - previous_outer_pressure) <= 1.0) {
      softSerial.println("Detected minimal pressure change. Assuming the float has reached the bottom.");
      bottom = true;
      break;
    }
  }
  descend = false;
  if (bottom) {
    ascendStartTime = millis();
    softSerial.println("Reached the bottom...");
    digitalWrite(pump, HIGH);
    digitalWrite(valve, HIGH);
    theaterChase(strip.Color(0, 0, 255), 50);

    ascend = true;
    while (ascend) {
      datapoints_counter++;
      delay(5000);  // 1 second delay for data collection for now
      previous_outer_pressure = outer_pressure;
      logDataToEEPROM();
      if (millis() - ascendStartTime > maxAscendTime) {
        softSerial.println("Reached maximum ascent time. Assuming the float has reached the surface.");
        surface = true;
        break;
      }

      if ((millis() - ascendStartTime > 20000) && abs(outer_pressure - previous_outer_pressure) <= 1.0) {
        softSerial.println("Detected minimal pressure change. Assuming the float has reached the bottom.");
        surface = true;
        break;
      }
    }
    ascend = false;
    bottom = false;
  }
  if (surface) {
    digitalWrite(pump, LOW);
  }
  softSerial.println("Float is at the surface. Ready to Transmit.");
  theaterChase(strip.Color(0, 255, 0), 50);
}

void handleAscent() {
  softSerial.println("Data Transmitting:");
  addr_EEPROM = 0;
  readDataFromEEPROM();
  transmit = true;

  if (transmit) { // seems useless
    softSerial.println("Clearing the entire EEPROM. This will take a few moments");
    for (addr_EEPROM = 0; addr_EEPROM < 1024; addr_EEPROM++) {
      eeprom.eeprom_write(addr_EEPROM, 0);
      if (addr_EEPROM % 100 == 0)
        softSerial.print(".");
    }
    softSerial.println("Memory Erase Complete");
    softSerial.println("Finished sending data. Ready to Descend Again. Press D to Descend.");
  }
  transmit = false;
}

void theaterChase(uint32_t c, uint8_t wait) {
  for (int j = 0; j < 10; j++) {  // do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, c);  // turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i = 0; i < strip.numPixels(); i = i + 2) {
        strip.setPixelColor(i + q, 0);  // turn every second pixel off
      }
    }
  }
}
