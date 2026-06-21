#include <DHT.h>
#include <Servo.h>

#define DHTPIN 47
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const int thrusterMin = 1100;
const int thrusterMax = 1900;
const int rotateMin   = 500;
const int rotateMax   = 2500;

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
  float rotate;       // Elbow servo:  L2/R2 triggers
  float dc_motor;     // DC motor 1:   Dpad Left/Right
  float dc_motor2;    // DC motor 2:   Dpad Up/Down
  uint8_t end;
};
struct TelemetryPacket {
  uint8_t start = 0xAB;
  float temperature;
  float humidity;
  uint8_t end = 0x54;
};
#pragma pack(pop)

const uint8_t START_BYTE = 0xAA;
const uint8_t END_BYTE   = 0x55;

// --- Thrusters & elbow servo ---
Servo servo_lf;
Servo servo_rt;
Servo servo_up1;
Servo servo_up2;
Servo servo_rotate;

const byte servoPin_lf     = 27;
const byte servoPin_rt     = 26;
const byte servoPin_up1    = 28;
const byte servoPin_up2    = 22;
const byte servoPin_rotate = 23;

// --- H-bridge pins ---
// DC Motor 1 (Dpad Left/Right)
// --- H-bridge pins ---
// DC Motor 1 (Dpad Left/Right)
const byte M1_IN1 = 4;
const byte M1_IN2 = 5;
const byte M1_EN  = 9;   // PWM enable pin

// DC Motor 2 (Dpad Up/Down)
const byte M2_IN1 = 6;
const byte M2_IN2 = 7;
const byte M2_EN  = 10;   // PWM enable pin

const byte MOTOR_SPEED = 128; // 50% of 255


int mapFloatSignal(float input, int minVal, int maxVal, InputSignalRange rangeType) {
  input = constrain(input, (rangeType == RANGE_NEG_ONE_TO_ONE ? -1.0f : 0.0f), 1.0f);

  if (rangeType == RANGE_NEG_ONE_TO_ONE) {
    float midpoint  = (minVal + maxVal) / 2.0f;
    float halfRange = (maxVal - minVal) / 2.0f;
    return static_cast<int>(input * halfRange + midpoint);
  } else {
    return static_cast<int>(input * (maxVal - minVal) + minVal);
  }
}

void driveMotor(byte in1, byte in2, byte en, float value) {
  if (value > 0.01f) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    analogWrite(en, MOTOR_SPEED);
  } else if (value < -0.1f) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    analogWrite(en, MOTOR_SPEED);
  } else {
    // Coast
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    analogWrite(en, 0);
  }
}

void applyControls(const ROVPacket& p) {
  // Horizontal thrusters
  servo_lf.writeMicroseconds(mapFloatSignal(p.tleft,   thrusterMin, thrusterMax, RANGE_NEG_ONE_TO_ONE));
  servo_rt.writeMicroseconds(mapFloatSignal(-p.tright, thrusterMin, thrusterMax, RANGE_NEG_ONE_TO_ONE));

  // Vertical thrusters
  servo_up1.writeMicroseconds(mapFloatSignal(p.tup_left,  thrusterMin, thrusterMax, RANGE_NEG_ONE_TO_ONE));
  servo_up2.writeMicroseconds(mapFloatSignal(p.tup_right, thrusterMin, thrusterMax, RANGE_NEG_ONE_TO_ONE));

  // Elbow servo
  servo_rotate.writeMicroseconds(mapFloatSignal(p.rotate, rotateMin, rotateMax, RANGE_ZERO_TO_ONE));

  // DC motors via H-bridge
  driveMotor(M1_IN1, M1_IN2, M1_EN, p.dc_motor);
  driveMotor(M2_IN1, M2_IN2, M2_EN, p.dc_motor2);
}

void sendTelemetry() {
  float temperature = dht.readTemperature();
  if (isnan(temperature)) temperature = -99.0;

  float humidity = dht.readHumidity();
  if (isnan(humidity)) humidity = -99.0;

  TelemetryPacket t;
  t.temperature = temperature;
  t.humidity    = humidity;

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

  // Thrusters & elbow servo
  servo_lf.attach(servoPin_lf);
  servo_rt.attach(servoPin_rt);
  servo_up1.attach(servoPin_up1);
  servo_up2.attach(servoPin_up2);
  servo_rotate.attach(servoPin_rotate);

  servo_lf.writeMicroseconds((thrusterMin + thrusterMax) / 2);
  servo_rt.writeMicroseconds((thrusterMin + thrusterMax) / 2);
  servo_up1.writeMicroseconds((thrusterMin + thrusterMax) / 2);
  servo_up2.writeMicroseconds((thrusterMin + thrusterMax) / 2);
  servo_rotate.writeMicroseconds((rotateMin + rotateMax) / 2);

  // H-bridge pins
  pinMode(M1_IN1, OUTPUT); pinMode(M1_IN2, OUTPUT);
  pinMode(M2_IN1, OUTPUT); pinMode(M2_IN2, OUTPUT);

  // PWM enable pins at 50% speed
  pinMode(M1_EN, OUTPUT); analogWrite(M1_EN, MOTOR_SPEED);
  pinMode(M2_EN, OUTPUT); analogWrite(M2_EN, MOTOR_SPEED);

  // Start motors coasting
  digitalWrite(M1_IN1, LOW); digitalWrite(M1_IN2, LOW);
  digitalWrite(M2_IN1, LOW); digitalWrite(M2_IN2, LOW);
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

      if (pkt.start == START_BYTE && pkt.end == END_BYTE) {
        applyControls(pkt);
      } else {
        Serial.println("[ERROR] Invalid packet");
      }
    }
  }
}