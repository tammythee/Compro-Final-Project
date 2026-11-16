
#define ENA 3
#define IN1 4
#define IN2 5

#define ENB 9
#define IN3 6
#define IN4 8

#define IR_PIN 2

struct Sensor {
  int trig;
  int echo;
  float distance;
};

Sensor front = {A3, A4, 100};
Sensor right = {10, 11, 100};
Sensor left  = {A2, A5, 100};

int assignmentArray[5] = {10, 20, 30, 40, 50};

int motorSpeed = 75;

const int frontLimit = 16;
const int rightLimit = 22;
const int leftLimit  = 22;

const int turnTime = 232;
const int uTurnTime = 464;

const int afterTurnTime = 150;
const int settleTime = 150;

const int maxDistance = 100;
const unsigned long echoTimeout = 7000;

const int steerDeadband = 3;
const int steerGain = 2;
const int steerMax = 30;

bool stoppedByIR = false;

void stopRobot() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void driveForward(int leftSpeed, int rightSpeed) {
  if (stoppedByIR) {
    stopRobot();
    return;
  }

  leftSpeed = constrain(leftSpeed, 0, 255);
  rightSpeed = constrain(rightSpeed, 0, 255);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, leftSpeed);
  analogWrite(ENB, rightSpeed);
}

void forward() {
  driveForward(motorSpeed, motorSpeed);
}

void backward() {
  if (stoppedByIR) return;

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
}

bool irActive() {
  for (int i = 0; i < 3; i++) {
    if (digitalRead(IR_PIN) != HIGH) {
      return false;
    }

    delayMicroseconds(500);
  }

  return true;
}

bool checkIR() {
  if (stoppedByIR) {
    stopRobot();
    return false;
  }

  if (irActive()) {
    Serial.println("IR DETECTED - EMERGENCY STOP");

    stoppedByIR = true;
    stopRobot();

    return false;
  }

  return true;
}

bool waitForIR(unsigned long duration) {
  unsigned long startTime = millis();

  while (millis() - startTime < duration) {
    if (!checkIR()) {
      return false;
    }

    delay(1);
  }

  return true;
}

float getDistance(Sensor *s) {
  digitalWrite(s->trig, LOW);
  delayMicroseconds(3);

  digitalWrite(s->trig, HIGH);
  delayMicroseconds(10);

  digitalWrite(s->trig, LOW);

  unsigned long duration =
    pulseIn(s->echo, HIGH, echoTimeout);

  if (duration == 0) {
    s->distance = maxDistance;
  } else {
    s->distance = duration * 0.0343 / 2.0;

    s->distance = constrain(
      s->distance, 2, maxDistance
    );
  }

  return s->distance;
}

bool readSensors() {
  if (!checkIR()) return false;

  front.distance = getDistance(&front);

  if (!waitForIR(35)) return false;

  right.distance = getDistance(&right);

  if (!waitForIR(35)) return false;

  left.distance = getDistance(&left);

  if (!checkIR()) return false;

  return true;
}

void showDistance() {
  Serial.print("Front: ");
  Serial.print(front.distance);

  Serial.print(" cm | Right: ");
  Serial.print(right.distance);

  Serial.print(" cm | Left: ");
  Serial.print(left.distance);

  Serial.println(" cm");
}

void forwardCentered() {
  int l = motorSpeed;
  int r = motorSpeed;

  if (right.distance < rightLimit &&
      left.distance < leftLimit) {

    float error = right.distance - left.distance;

    if (error > steerDeadband) {
      l += constrain(
        (int)(error * steerGain),
        0,
        steerMax
      );
    }

    else if (error < -steerDeadband) {
      r += constrain(
        (int)(-error * steerGain),
        0,
        steerMax
      );
    }
  }

  driveForward(l, r);
}

void turnRight() {
  if (!checkIR()) return;

  Serial.println("TURN RIGHT");

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);

  if (!waitForIR(turnTime)) {
    stopRobot();
    return;
  }

  stopRobot();

  waitForIR(150);
}

void turnLeft() {
  if (!checkIR()) return;

  Serial.println("TURN LEFT");

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);

  if (!waitForIR(turnTime)) {
    stopRobot();
    return;
  }

  stopRobot();

  waitForIR(150);
}

void turnAround() {
  if (!checkIR()) return;

  Serial.println("U-TURN 180");

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);

  if (!waitForIR(uTurnTime)) {
    stopRobot();
    return;
  }

  stopRobot();

  waitForIR(200);
}

void stepAfterTurn() {
  if (!checkIR()) return;

  getDistance(&front);

  if (!checkIR()) return;

  if (front.distance > frontLimit + 4) {
    Serial.println("MOVE FORWARD AFTER TURN");

    forward();

    if (!waitForIR(afterTurnTime)) {
      stopRobot();
      return;
    }
  }

  stopRobot();

  waitForIR(100);
}

void solveMaze() {
  if (!readSensors()) return;

  showDistance();

  if (front.distance > frontLimit) {
    Serial.println("FRONT CLEAR - FORWARD");

    forwardCentered();

    return;
  }

  Serial.println("FRONT BLOCKED");

  stopRobot();

  if (!waitForIR(settleTime)) return;

  if (!readSensors()) return;

  showDistance();

  if (front.distance > frontLimit) {
    Serial.println("FRONT CLEAR - RESUME");

    forwardCentered();

    return;
  }

  if (right.distance > rightLimit) {
    Serial.println("RIGHT CLEAR");

    turnRight();

    if (stoppedByIR) return;

    stepAfterTurn();
  }

  else if (left.distance > leftLimit) {
    Serial.println("LEFT CLEAR");

    turnLeft();

    if (stoppedByIR) return;

    stepAfterTurn();
  }

  else {
    Serial.println("DEAD END - U TURN");

    turnAround();

    if (stoppedByIR) return;

    stepAfterTurn();
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(front.trig, OUTPUT);
  pinMode(front.echo, INPUT);

  pinMode(right.trig, OUTPUT);
  pinMode(right.echo, INPUT);

  pinMode(left.trig, OUTPUT);
  pinMode(left.echo, INPUT);

  pinMode(IR_PIN, INPUT);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopRobot();

  Serial.println("======================");
  Serial.println("MAZE ROBOT READY");
  Serial.println("Right-Hand Priority");
  Serial.println("IR: Active HIGH");
  Serial.println("Ultrasonic: 100 cm");
  Serial.println("======================");

  delay(1000);
}

void loop() {
  if (stoppedByIR) {
    stopRobot();
    return;
  }

  if (!checkIR()) {
    stopRobot();
    return;
  }

  solveMaze();
}
