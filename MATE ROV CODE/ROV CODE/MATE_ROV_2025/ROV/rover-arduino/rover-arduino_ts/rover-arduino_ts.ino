#include <DHT.h>

#define DHTPIN 47        // Use a free digital pin (e.g., D2)
#define DHTTYPE DHT22   // Define sensor type

DHT dht(DHTPIN, DHTTYPE);

#include <Servo.h>


const int thrusterMin   = 1100;
const int thrusterMax   = 1900;
const int clawMin       = 500;
const int clawMax       = 2500;
const int rotateMin     = 1000;
const int rotateMax     = 2000;

enum InputSignalRange {
  RANGE_NEG_ONE_TO_ONE,
  RANGE_ZERO_TO_ONE
};

#pragma pack(push, 1)
struct ROVPacket {
  uint8_t start;
  float tleft;
  float tright;
  float tup_left;
  float tup_right;
  float claw;
  float rotate;
  // float checksum;
  uint8_t end;
};
struct TelemetryPacket {
  uint8_t start = 0xAB;
  // float depth;
  // float pressure;
  float temperature;
  float humidity;
  uint8_t end = 0x54;
};
#pragma pack(pop)

const uint8_t START_BYTE = 0xAA;
const uint8_t END_BYTE = 0x55;

Servo servo_lf;
Servo servo_rt;
Servo servo_up1;
Servo servo_up2;
Servo servo_claw;
Servo servo_rotate;

const byte servoPin_lf     = 27;  // left horizontal ESC (CW)
const byte servoPin_rt     = 26;  // right horizontal ESC (CCW)
const byte servoPin_up1    = 28;  // left vertical ESC (CCW)
const byte servoPin_up2    = 22;  // right vertical ESC (CW)
const byte servoPin_claw   = 23;  // main claw
const byte servoPin_rotate = 24;  // rotating claw

const int tempPin = A1;           // (Optional) temperature sensor

// uint8_t computeChecksum(const ROVPacket& p) {
//   const uint8_t* data = reinterpret_cast<const uint8_t*>(&p);
//   uint8_t sum = 0;
//   for (size_t i = 1; i < sizeof(ROVPacket) - 2; ++i) {
//     sum ^= data[i];
//   }
//   return sum;
// }



int mapFloatSignal(float input, int minVal, int maxVal, InputSignalRange rangeType) {
  input = constrain(input, (rangeType == RANGE_NEG_ONE_TO_ONE ? -1.0f : 0.0f), 1.0f);

  if (rangeType == RANGE_NEG_ONE_TO_ONE) {
    float midpoint = (minVal + maxVal) / 2.0f;
    float halfRange = (maxVal - minVal) / 2.0f;
    // [-1, 1] → [minVal, maxVal]
    return static_cast<int>(input * halfRange + midpoint);
  } else {
    // [0, 1] → [minVal, maxVal]
    return static_cast<int>(input * (maxVal - minVal) + minVal);
  }
}

void applyControls(const ROVPacket& p) {
  // Horizontal thrusters
  int th_left_sig  = mapFloatSignal(p.tleft, thrusterMin, thrusterMax, RANGE_NEG_ONE_TO_ONE);
  int th_right_sig = mapFloatSignal(-p.tright, thrusterMin, thrusterMax, RANGE_NEG_ONE_TO_ONE);
  servo_lf.writeMicroseconds(th_left_sig);
  servo_rt.writeMicroseconds(th_right_sig);

  // Vertical thrusters
  int th_up_left_sig  = mapFloatSignal(p.tup_left, thrusterMin, thrusterMax, RANGE_NEG_ONE_TO_ONE);
  int th_up_right_sig = mapFloatSignal(p.tup_right, thrusterMin, thrusterMax, RANGE_NEG_ONE_TO_ONE);
  servo_up1.writeMicroseconds(th_up_left_sig);
  servo_up2.writeMicroseconds(th_up_right_sig);

  // Main claw
  int clawSignal = mapFloatSignal(p.claw, clawMin, clawMax, RANGE_ZERO_TO_ONE);
  // int clawSignal = constrain(p.claw * 10 + 700, 900, 1700);
  servo_claw.writeMicroseconds(clawSignal);

  // Rotating claw
  int rotateSignal = mapFloatSignal(p.rotate, clawMin, clawMax, RANGE_ZERO_TO_ONE);
  // int rotateSignal = constrain(p.rotate * 10 + 700, 900, 1700);
  servo_rotate.writeMicroseconds(rotateSignal);
}

void sendTelemetry() {
  // int analogPressure = analogRead(A0);  // still using analog pressure
  // float voltage = analogPressure * (5.0 / 1023.0);
  // float pressure_kPa = voltage * 100.0;  // example formula
  // float depth_m = (pressure_kPa - 101.3) / 9.8;

  float temperature = dht.readTemperature();
  if (isnan(temperature)) temperature = -99.0;

  float humidity = dht.readHumidity();
  if (isnan(humidity)) humidity = -99.0;

  TelemetryPacket t;
  // t.depth = depth_m;
  // t.pressure = pressure_kPa;
  t.temperature = temperature;
  t.humidity = humidity;

  Serial.write((uint8_t*)&t, sizeof(t));
  Serial.flush();
}


void setup() {

  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  digitalWrite(10, HIGH);
  Serial.begin(115200);
  dht.begin();


  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(44, OUTPUT);

  servo_up1.attach(servoPin_up1);
  servo_up2.attach(servoPin_up2);
  servo_lf.attach(servoPin_lf);
  servo_rt.attach(servoPin_rt);
  servo_claw.attach(servoPin_claw);
  servo_rotate.attach(servoPin_rotate);

  servo_lf.writeMicroseconds((thrusterMin + thrusterMax) / 2);
  servo_rt.writeMicroseconds((thrusterMin + thrusterMax) / 2);
  servo_up1.writeMicroseconds((thrusterMin + thrusterMax) / 2);
  servo_up2.writeMicroseconds((thrusterMin + thrusterMax) / 2);
  servo_claw.writeMicroseconds((clawMin + clawMax) / 2);
  servo_rotate.writeMicroseconds((rotateMin + rotateMax) / 2);
}

void loop() {
  static uint8_t buffer[sizeof(ROVPacket)];
  static size_t index = 0;
  static unsigned long lastTelemetryTime = 0;
  const unsigned long telemetryInterval = 500;

  if (millis() - lastTelemetryTime >= telemetryInterval) {
    sendTelemetry();
    lastTelemetryTime = millis();
  }

  while (Serial.available()) {
    uint8_t byte = Serial.read();

    if (index == 0 && byte != START_BYTE) {
      continue;
    }

    buffer[index++] = byte;

    if (index == sizeof(ROVPacket)) {
      ROVPacket pkt;
      memcpy(&pkt, buffer, sizeof(pkt));
      index = 0; 

      // might have to remove computeChecksum(pkt) from the if statement
      if (pkt.start == START_BYTE && pkt.end == END_BYTE) {// && pkt.checksum == computeChecksum(pkt)) {
        applyControls(pkt);
        // Serial.print("[OK] ");
        // Serial.print("TLEFT:"); Serial.print(pkt.tleft); Serial.print(" ");
        // Serial.print("TRIGHT:"); Serial.print(pkt.tright); Serial.print(" ");
        // Serial.print("V1:"); Serial.print(pkt.tup_left); Serial.print(" ");
        // Serial.print("V2:"); Serial.print(pkt.tup_right); Serial.print(" ");
        // Serial.print("CLAW:"); Serial.print(pkt.claw); Serial.print(" ");
        // Serial.print("ROTATE:"); Serial.print(pkt.rotate);
        // Serial.println();
      } else {
        Serial.println("[ERROR] Invalid packet");
      }
    }
  }

}