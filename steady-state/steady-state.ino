#include <EEPROM.h>

const int NONE = 0;

const int PURPLE[3] = {HIGH, LOW, HIGH};
const int YELLOW[3] = {HIGH, HIGH, LOW};

// BEGIN PIN MAPPINGS

// Countdown LEDs
const int READY = 6; // DONE
const int SET = 7; // DONE
const int GO = 8; // DONE
const int MOVE_TO_START = 5; // DONE

// Switches
const int BEGIN[2] = {10 /* DONE (GREEN) */, 11 /* DONE (YELLOW) */};
const int END[2] = {13 /* DONE (RED) */, 12 /* DONE (WHITE) */};

// Buttons
const int START = 9; // DONE (BLUE)

// Win/lose LEDs
const int BULBS_RGB[3] = {2, 3, 4}; // DONE

// Copper tape
const int TAPE[2] = {A0, A1}; // DONE

// Fake Ground
const int GROUND[2] = {A5, A6}; // DONE

// END PIN MAPPINGS

// Addresses
const int NO_WIN_ADDR = 0x0;
const int YELLOW_WIN_ADDR = 0x14;
const int PURPLE_WIN_ADDR = 0x28;

void setCountdown(int value) {
  digitalWrite(READY, value == READY ? HIGH : LOW);
  digitalWrite(SET, value == SET ? HIGH : LOW);
  digitalWrite(GO, value == GO ? HIGH : LOW);
  digitalWrite(MOVE_TO_START, value == MOVE_TO_START ? HIGH : LOW);
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

  pinMode(GROUND[0], OUTPUT);
  pinMode(GROUND[1], OUTPUT);
  digitalWrite(GROUND[0], LOW);
  digitalWrite(GROUND[1], LOW);

  pinMode(START, INPUT_PULLUP);

  for (int i = 0; i < 2; i++) {
    pinMode(BEGIN[i], INPUT_PULLUP);
    pinMode(END[i], INPUT_PULLUP);
    pinMode(TAPE[i], INPUT_PULLUP);
  }

  for (int i = 0; i < 3; i++) {
    pinMode(BULBS_RGB[i], OUTPUT);
    digitalWrite(BULBS_RGB[i], LOW);
  }

  unsigned long total_games = 0;
  for (int winner = 0; winner <= 2; ++winner) {
    int base_addr = NO_WIN_ADDR;
    if (winner == 1) {
      base_addr = PURPLE_WIN_ADDR;
      Serial.println("Purple win:");
    } else if (winner == 2) {
      base_addr = YELLOW_WIN_ADDR;
      Serial.println("Yellow win:");
    } else {
      Serial.println("No win:");
    }

    unsigned long count;
    
    Serial.print("< 5 seconds: ");
    EEPROM.get(base_addr + 0x0, count);
    Serial.println(count);
    Serial.print("< 30 seconds: ");
    EEPROM.get(base_addr + 0x4, count);
    Serial.println(count);
    total_games += count;
    Serial.print("< 60 seconds: ");
    EEPROM.get(base_addr + 0x8, count);
    Serial.println(count);
    total_games += count;
    Serial.print("< 120 seconds: ");
    EEPROM.get(base_addr + 0xc, count);
    Serial.println(count);
    total_games += count;
    Serial.print(">= 120 seconds: ");
    EEPROM.get(base_addr + 0x10, count);
    Serial.println(count);
    total_games += count;
  }

  Serial.print("Total games (not including < 5 seconds: ");
  Serial.println(total_games);
}

bool okayToStart() {
  return (digitalRead(TAPE[0]) &&
          digitalRead(TAPE[1]) &&
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

// Returns true if someone tries to cheat (i.e. move their rod) during the delay.
// Returns false otherwise.
bool delayAndCheckForCheating(unsigned long value) {
  unsigned long begin = millis();
  while (millis() - begin < value) {
    if (!okayToStart()) {
      return true; // cheating!
    }
  }
  return false;
}

// Returns true on success, false if someone moves before the countdown is over
bool doCountdown() {
  setCountdown(READY);
  if (delayAndCheckForCheating(1000)) return false;
  setCountdown(SET);
  if (delayAndCheckForCheating(1000)) return false;
  setCountdown(GO);
  return true;
}

void waitForStart() {
  while (true) {
    while (!digitalRead(START)) { 
      Serial.println("SPIN");
      /* spin */ }
    Serial.println("END SPIN!");
    if (!okayToStart()) {
      Serial.println("DO FLASH");
      flashBackToStart();
    } else {
      Serial.println("DO COUNTDOWN");
      bool successfulStart = doCountdown();
      if (successfulStart) {
        return;
      } else {
        flashBackToStart();
      }
    }
  }
}

int playGame() {
  int winner = 0;
  while (!digitalRead(START)) {
    for (int i = 0; i < 2; i++) {
      if (!digitalRead(TAPE[i])) {
        winner = (1-i) + 1;
        return winner;
      }
      if (digitalRead(END[i])) {
        winner = i + 1;
        return winner;
      }
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

void recordWin(unsigned long duration, int winner) {
  int base_addr = NO_WIN_ADDR;
  if (winner == 1) base_addr = PURPLE_WIN_ADDR;
  else if (winner == 2) base_addr = YELLOW_WIN_ADDR;
  
  int offset = 0;
  if (duration < 5L * 1000L) {
    offset = 0x0;
  } else if (duration < 30L * 1000L) {
    offset = 0x4;
  } else if (duration < 60L * 1000L) {
    offset = 0x8;
  } else if (duration < 120L * 1000L) {
    offset = 0xc;
  } else {
    offset = 0x10;
  }

  unsigned long count;
  EEPROM.get(base_addr + offset, count);
  EEPROM.put(base_addr + offset, count + 1);
}

void loop() {
  waitForStart();
  Serial.println("STARTED!");
  unsigned long start_time = millis();
  int winner = playGame();
  unsigned long end_time = millis();
  recordWin(start_time - end_time, winner);
  doWinSequence(winner);
  setCountdown(NONE);  
}

//void updateEEPROMTime() {
//  // Check if our clock has ticked forward ten minutes compared to the stored EEPROM value
//  unsigned long eeprom_time;
//  EEPROM.get(TIME_ADDR, eeprom_time);
//  unsigned long current_time = previous_time + millis();
//  if (current_time > eeprom_time + TEN_MINUTES_MS) {
//    // If so, update the EEPROM time
//    EEPROM.put(TIME_ADDR, current_time);
//    Serial.print("Updated EEPROM time to: ");
//    Serial.println(current_time);
//  }
//}
//
//// Get the number of plays stored in EEPROM for this hour
//unsigned int getEEPROMCount(unsigned int hour) {
//  unsigned int count; // 2 bytes
//  EEPROM.get(COUNTER_ADDR + 2*hour, count);
//  return count;
//}
//
//// Store count into the EEPROM at the given hour
//void putEEPROMCount(unsigned int hour, unsigned int count) {
//  EEPROM.put(COUNTER_ADDR + 2*hour, count);
//}

