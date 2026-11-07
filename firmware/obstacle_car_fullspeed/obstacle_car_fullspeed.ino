// Obstacle car, FULL SPEED version (ENA/ENB caps stay ON, no PWM)
// Arduino Uno + HC-SR04 + 16x2 I2C LCD + L298N
// Library needed: LiquidCrystal_I2C (Frank de Brabander)

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---- Pins ----
const int TRIG = 9;
const int ECHO = 10;
const int BUZZER = 12;

const int IN1 = 6;    // left side
const int IN2 = 7;
const int IN3 = 8;    // right side
const int IN4 = 11;

// ---- Tuning ----
const int STOP_DIST_CM = 20;    // avoid below this
const int WARN_DIST_CM = 40;    // warn (beep) below this
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

// -1 = backward, 0 = stop, 1 = forward for each side
void setMotors(int left, int right) {
  digitalWrite(IN1, left > 0);
  digitalWrite(IN2, left < 0);
  digitalWrite(IN3, right > 0);
  digitalWrite(IN4, right < 0);
}

void stopCar()   { setMotors(0, 0); }
void forward()   { setMotors(1, 1); }
void backward()  { setMotors(-1, -1); }
void turnLeft()  { setMotors(0, 1); }   // left side stopped, right side runs
void turnRight() { setMotors(1, 0); }   // right side stopped, left side runs

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
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
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
    forward();
    show(d, "Warning: close");
    tone(BUZZER, 1000, 50);
  } else {
    forward();
    show(d, "Path clear");
    noTone(BUZZER);
  }
  delay(40);
}
