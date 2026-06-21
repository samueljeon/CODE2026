#include <DHT.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define DHTPIN 47
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
Adafruit_MPU6050 mpu;

//configs-change if needed
int elbow_default_pos = 1800 
//1500 us is default servo pos; however, due to the installation the default pos makes the elbow boot to a depressed angle, damaging servo. 
//1800 ms makes elbow actually level on boot
// Linear actuator speeds 
const byte MOTOR_START_SPEED = 128; // 50% power
const byte MOTOR_HOLD_SPEED  = 51;  // 20% power

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
  float rotate;
  float dc_motor;
  float dc_motor2;
  uint8_t end;
};

struct TelemetryPacket {
  uint8_t start = 0xAB;

  float accel_x_g;
  float accel_y_g;
  float accel_z_g;

  float pitch_deg;
  float roll_deg;

  float temp;

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
// DC Motor 1
const byte M1_IN1 = 4;
const byte M1_IN2 = 5;
const byte M1_EN  = 9;

// DC Motor 2
const byte M2_IN1 = 6;
const byte M2_IN2 = 7;
const byte M2_EN  = 10;


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
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    analogWrite(en, 0);
  }
}

//actuator step-down init
unsigned long actuatorStartTime = 0;
bool actuatorWasOn = false;
void driveLinearActuator(byte in1, byte in2, byte en, float value)
{
  bool motorOn = (value != 0);

  // detect OFF → ON transition
  if (motorOn && !actuatorWasOn)
  {
    actuatorStartTime = millis();
  }

  if (motorOn)
  {
    int pwm;

    if (millis() - actuatorStartTime < 1000)
      pwm = MOTOR_START_SPEED;   // 50%
    else
      pwm = MOTOR_HOLD_SPEED;    // 20%

    if (value > 0)
    {
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW);
    }
    else
    {
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);
    }

    analogWrite(en, pwm);
  }
  else
  {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    analogWrite(en, 0);
  }

  actuatorWasOn = motorOn;
}
//------------------------------------------------------------
void applyControls(const ROVPacket& p) {
  servo_lf.writeMicroseconds(mapFloatSignal(p.tleft,   thrusterMin, thrusterMax, RANGE_NEG_ONE_TO_ONE));
  servo_rt.writeMicroseconds(mapFloatSignal(-p.tright, thrusterMin, thrusterMax, RANGE_NEG_ONE_TO_ONE));

  servo_up1.writeMicroseconds(mapFloatSignal(p.tup_left,  thrusterMin, thrusterMax, RANGE_NEG_ONE_TO_ONE));
  servo_up2.writeMicroseconds(mapFloatSignal(p.tup_right, thrusterMin, thrusterMax, RANGE_NEG_ONE_TO_ONE));

  servo_rotate.writeMicroseconds(mapFloatSignal(p.rotate, rotateMin, rotateMax, RANGE_ZERO_TO_ONE));

  //motor 1 (linear actuator)
  driveLinearActuator(M1_IN1, M1_IN2, M1_EN, p.dc_motor);
  //motor 2
  driveMotor(M2_IN1, M2_IN2, M2_EN, p.dc_motor2);
}

void sendTelemetry() {
  float dhtTemperature = dht.readTemperature();
  if (isnan(dhtTemperature)) dhtTemperature = -99.0f;

  float humidity = dht.readHumidity();
  if (isnan(humidity)) humidity = -99.0f;

  sensors_event_t accelEvent;
  sensors_event_t gyroEvent;
  sensors_event_t mpuTempEvent;

  mpu.getEvent(&accelEvent, &gyroEvent, &mpuTempEvent);

  const float G = 9.80665f;

  float accelX = accelEvent.acceleration.x / G;
  float accelY = accelEvent.acceleration.y / G;
  float accelZ = accelEvent.acceleration.z / G;

  float roll = atan2(accelY, accelZ) * 180.0f / PI;

  float pitch = atan2(
    -accelX,
    sqrt(accelY * accelY + accelZ * accelZ)
  ) * 180.0f / PI;

  float mpuTempC = mpuTempEvent.temperature;

  TelemetryPacket t;

  t.accel_x_g = accelX;
  t.accel_y_g = accelY;
  t.accel_z_g = (accelZ-0.115f);  //Default shows Z-accel of 1.115g, so I calibrated it

  t.pitch_deg = pitch;
  t.roll_deg = roll;

  // This sends MPU6050 internal temperature.
  // If you want DHT22 temperature instead, change this to: t.temp = dhtTemperature;
  t.temp = mpuTempC;

  Serial.write((uint8_t*)&t, sizeof(t));
  Serial.flush();
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  Wire.begin();

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1) {
      delay(10);
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(44, OUTPUT);

  servo_lf.attach(servoPin_lf);
  servo_rt.attach(servoPin_rt);
  servo_up1.attach(servoPin_up1);
  servo_up2.attach(servoPin_up2);
  servo_rotate.attach(servoPin_rotate);

  servo_lf.writeMicroseconds((thrusterMin + thrusterMax) / 2);
  servo_rt.writeMicroseconds((thrusterMin + thrusterMax) / 2);
  servo_up1.writeMicroseconds((thrusterMin + thrusterMax) / 2);
  servo_up2.writeMicroseconds((thrusterMin + thrusterMax) / 2);
  servo_rotate.writeMicroseconds(elbow_default_pos); 

  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
  pinMode(M2_IN1, OUTPUT);
  pinMode(M2_IN2, OUTPUT);

  pinMode(M1_EN, OUTPUT);
  pinMode(M2_EN, OUTPUT);

  analogWrite(M1_EN, 0);
  analogWrite(M2_EN, 0);

  digitalWrite(M1_IN1, LOW);
  digitalWrite(M1_IN2, LOW);
  digitalWrite(M2_IN1, LOW);
  digitalWrite(M2_IN2, LOW);
}

void loop() {
  static uint8_t buffer[sizeof(ROVPacket)];
  static size_t index = 0;
  static unsigned long lastTelemetryTime = 0;
  const unsigned long telemetryInterval = 100;

  if (millis() - lastTelemetryTime >= telemetryInterval) {
    sendTelemetry();
    lastTelemetryTime = millis();
  }

  while (Serial.available()) {
    uint8_t incomingByte = Serial.read();

    if (index == 0 && incomingByte != START_BYTE) {
      continue;
    }

    buffer[index++] = incomingByte;

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