#include <EEPROM.h>

const int NONE = 0;

const int PURPLE[3] = {HIGH, LOW, HIGH};
const int YELLOW[3] = {HIGH, HIGH, LOW};

// BEGIN PIN MAPPINGS

// Countdown LEDs
const int READY = 1;
const int SET = 2;
const int GO = 3;
const int MOVE_TO_START = 4;

// Switches
const int BEGIN[2] = {5, 6};
const int END[2] = {7, 8};

// Buttons
const int START = 9;

// Win/lose LEDs
const int BULBS_RGB[3] = {10, 11, 12};

// Copper tape
const int TAPE[2] = {13, 14};

// END PIN MAPPINGS

void setCountdown(int value) {
  int pins[] = {READY, SET, GO, MOVE_TO_START};
  for (int i = 0; i < sizeof(pins)/sizeof(int); i++) {
    digitalWrite(pins[i], value == pins[i] ? HIGH : LOW);
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(READY, OUTPUT);
  pinMode(SET, OUTPUT);
  pinMode(GO, OUTPUT);
  pinMode(MOVE_TO_START, OUTPUT);
  digitalWrite(READY, LOW);
  digitalWrite(SET, LOW);
  digitalWrite(GO, LOW);
  digitalWrite(MOVE_TO_START, LOW);

  pinMode(START, INPUT);

  for (int i = 0; i < 2; i++) {
    pinMode(BEGIN[i], INPUT);
    pinMode(END[i], INPUT);
    pinMode(TAPE[i], INPUT);
  }

  for (int i = 0; i < 3; i++) {
    pinMode(BULBS_RGB[i], OUTPUT);
    digitalWrite(BULBS_RGB[i], LOW);
  }
}

bool okayToStart() {
  return (!digitalRead(TAPE[0]) &&
          !digitalRead(TAPE[1]) &&
          !digitalRead(END[0]) &&
          !digitalRead(END[1]) &&
          digitalRead(BEGIN[0]) &&
          digitalRead(BEGIN[1]));
}

void flashBackToStart() {
  for (int i = 0; i < 5; i++) {
    setCountdown(MOVE_TO_START);
    delay(500);
    setCountdown(NONE);
    delay(500);
  }
}

bool delayAndCheckForCheating(unsigned long value, unsigned long step_size = 50) {
  for (int i = 0; i < value; i += step_size) {
    delay(step_size);
    if (!okayToStart()) {
      return true; // cheating!
    }
  }
  return false;
}

bool doCountdown() {
  int pins[] = {READY, SET, GO};
  for (int i = 0; i < sizeof(pins)/sizeof(int); i++) {
    setCountdown(pins[i]);
    if (delayAndCheckForCheating(1000)) {
      flashBackToStart();
      return false;
    }
  }
  return true;
}

void waitForStart() {
  while (true) {
    while (!digitalRead(START)) { /* spin */ }
    if (!okayToStart()) {
      flashBackToStart();
    } else {
      bool successfulStart = doCountdown();
      if (successfulStart) {
        return;
      }
    }
  }
}

int playGame() {
  int winner = 0;
  while (!digitalRead(START)) {
    for (int i = 0; i < 2; i++) {
      if (digitalRead(TAPE[i])) winner = (1-i) + 1;
      if (digitalRead(END[i])) winner = i + 1;
    }
  }
  return winner;
}

void doWinSequence(int winner) {
  if (winner == 0) return;
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 3; j++) {
      digitalWrite(BULBS_RGB[j], (winner == 1 ? PURPLE[j] : YELLOW[j]));
    }
    delay(1000);
    for (int j = 0; j < 3; j++) {
      digitalWrite(BULBS_RGB[j], LOW);
    }
    delay(1000);
  }
}

void loop() {
  waitForStart();
  int winner = playGame();
  doWinSequence(winner);
  setCountdown(NONE);  
}

