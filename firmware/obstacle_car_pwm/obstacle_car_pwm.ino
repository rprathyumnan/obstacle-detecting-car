// Obstacle car with SPEED CONTROL (ENA/ENB caps REMOVED, wired to D5 / D3)
// Arduino Uno + HC-SR04 + 16x2 I2C LCD + L298N
// Library needed: LiquidCrystal_I2C (Frank de Brabander)

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---- Pins ----
const int TRIG = 9;
const int ECHO = 10;
const int BUZZER = 12;

const int ENA = 5;    // left side speed (PWM)
const int IN1 = 6;
const int IN2 = 7;
const int IN3 = 8;    // right side
const int IN4 = 11;
const int ENB = 3;    // right side speed (PWM)

// ---- Tuning (0-255) ----
const int CRUISE_SPEED = 200;   // path clear
const int SLOW_SPEED   = 150;   // 20-40 cm from obstacle
const int BACK_SPEED   = 180;
const int TURN_SPEED   = 220;

const int STOP_DIST_CM = 20;
const int WARN_DIST_CM = 40;
const int BACK_TIME_MS = 400;
const int TURN_TIME_MS = 800;   // tune on the floor

LiquidCrystal_I2C lcd(0x27, 16, 2);  // try 0x3F if the LCD stays blank

long readDistanceOnce() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long duration = pulseIn(ECHO, HIGH, 30000UL);
  if (duration == 0) return 400;
  return duration * 0.034 / 2;
}

// Median of 3 readings to reject noise
long readDistance() {
  long a = readDistanceOnce();
  delay(10);
  long b = readDistanceOnce();
  delay(10);
  long c = readDistanceOnce();
  if (a > b) { long t = a; a = b; b = t; }
  if (b > c) { long t = b; b = c; c = t; }
  if (a > b) { b = a; }
  return b;
}

// left/right: -255..255 (sign = direction, size = speed)
void setMotors(int left, int right) {
  digitalWrite(IN1, left > 0);
  digitalWrite(IN2, left < 0);
  digitalWrite(IN3, right > 0);
  digitalWrite(IN4, right < 0);
  analogWrite(ENA, abs(left));
  analogWrite(ENB, abs(right));
}

void stopCar()        { setMotors(0, 0); }
void forward(int s)   { setMotors(s, s); }
void backward()       { setMotors(-BACK_SPEED, -BACK_SPEED); }
void turnLeft()       { setMotors(0, TURN_SPEED); }   // left side stopped
void turnRight()      { setMotors(TURN_SPEED, 0); }   // right side stopped

void show(long d, const char* status) {
  lcd.setCursor(0, 0);
  lcd.print("Dist: ");
  if (d >= 400) lcd.print(">400");
  else lcd.print(d);
  lcd.print(" cm    ");
  lcd.setCursor(0, 1);
  lcd.print(status);
  lcd.print("          ");
}

void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(BUZZER, OUTPUT);
  int motorPins[] = {ENA, IN1, IN2, IN3, IN4, ENB};
  for (int i = 0; i < 6; i++) pinMode(motorPins[i], OUTPUT);
  stopCar();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Obstacle Car");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();
}

void loop() {
  long d = readDistance();

  if (d <= STOP_DIST_CM) {
    stopCar();
    show(d, "OBSTACLE! STOP");
    tone(BUZZER, 2000);
    delay(300);
    noTone(BUZZER);

    show(d, "Reversing");
    backward();
    delay(BACK_TIME_MS);

    show(d, "Turning");
    turnRight();
    delay(TURN_TIME_MS);
    stopCar();
    delay(100);
  } else if (d <= WARN_DIST_CM) {
    forward(SLOW_SPEED);
    show(d, "Slowing down");
    tone(BUZZER, 1000, 50);
  } else {
    forward(CRUISE_SPEED);
    show(d, "Path clear");
    noTone(BUZZER);
  }
  delay(40);
}
