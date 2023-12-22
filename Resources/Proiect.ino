#include "LedControl.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>

// Highscore variables
struct Highscore {
  char playerName[5];
  int score;
};
const int maxPlayers = 5;
const int notInTop = 6;
Highscore highscores[maxPlayers];

// Custom chars
byte arrowDown[8] = {
  B00000,
  B00000,
  B11111,
  B11111,
  B01110,
  B01110,
  B00100,
  B00000
};

byte arrowUp[8] = {
  B00000,
  B00000,
  B00100,
  B01110,
  B01110,
  B11111,
  B11111,
  B00000
};

byte doubleArrows[8] = {
  B00000,
  B00100,
  B01110,
  B11111,
  B00000,
  B11111,
  B01110,
  B00100
};

byte smiley[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b00000,
  0b00000,
  0b10001,
  0b01110,
  0b00000
};

byte heart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

byte selectArrow[8] = {
  B01000,
  B01100,
  B01110,
  B11111,
  B01110,
  B01100,
  B01000,
  B00000
};

// Matrix constants
const byte dinPin = 12;
const byte clockPin = A2;  // Initial schema was 11
const byte loadPin = 13;   // Initial schema was 10
const byte matrixSize = 8;
const byte bigMatrixSize = 16;
int offset = 8;
LedControl lc = LedControl(dinPin, clockPin, loadPin, 1);

const uint64_t matrixPatterns[] = {
  0x00dbdbdfdf1bdb00,  // Hi
  0x040c1c3c3c1c0c04,  // Start game (play button)
  0x3c18187ebdff7e00,  // Highscores (cup)
  0xbd66dbbdbddb66bd,  // Settings (gear)
  0x3c18181818001818,  // About (i)
  0x1800181830647c38,  // How to play (question mark)
  0x3048404e490d010e,  // GJ
  0x5a7e3c7edbdbff7e   // Difficulty (skull)
};
const int matrixPatterns_LEN = sizeof(matrixPatterns) / 8;

// Joystick Constants
const int pinSW = 2;
const int pinX = A0;
const int pinY = A1;
int xValue = 0;
int yValue = 0;
int prevXValue = 0;
int prevYValue = 0;
const int maxValue = 900;
const int minValue = 100;
unsigned long lastXMoveTime = 0;
const unsigned long autoRepeatDelay = 200;

// Buzzer and sound constants
const int pinBuzzer = 3;
const int fireFreq = 400;
const int collisionFreq = 100;
const int soundDuration = 50;
const int collisionSoundDuration = 200;
const int second = 1000;
const int pauseDelay = 1.30;
byte sound = 1;

// Musical note frequencies
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523

// Melody
int melody[] = {
  NOTE_C4, NOTE_D4, NOTE_E4, NOTE_C4,
  NOTE_C4, NOTE_D4, NOTE_E4, NOTE_C4,
  NOTE_E4, NOTE_F4, NOTE_G4,
  NOTE_A4, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4, NOTE_C4
};
const int maxNotes = 17;

int noteDurations[] = {
  8, 8, 4, 4,
  8, 8, 4, 4,
  8, 8, 4,
  4, 4, 4, 4, 4, 4
};

// Movement constants
const int UP = 0;
const int DOWN = 1;
const int RIGHT = 2;
const int LEFT = 3;

// Button debounce constants
volatile unsigned long interruptTime = 0;
volatile unsigned long lastInterruptTime = 0;
volatile byte lastButtonRead = LOW;
volatile byte buttonRead = LOW;
const unsigned long debounceDelay = 200;
bool buttonState = false;
unsigned long lastDebounceTime = 0;
bool lastButtonState = HIGH;
bool infoScreen2Flag = false;

// Bullet variables
int bulletPos = 0;
int lastBulletPos = 0;
const unsigned long bulletMoveDelay = 40;
unsigned long lastBulletBlinkMillis = 0;
const int bulletBlinkTime = 200;
bool bulletFired = false;

// Player variables
int currentPos = 0;
int score = 0;
int wallsDestroyed = 0;
const int neighbours = 4;
unsigned long timePlayedStart = 0;
unsigned long timePlayed = 0;
const unsigned long timeLimit = 60000;
int timeLeft = 0;
unsigned long lastUpdateTime = 0;
unsigned long timeElapsed = 0;
unsigned long lastBlinkMillis = 0;
bool pixelState = false;
const int playerBlinkTime = 500;
const float easyMultiplier = 1.0;
const float mediumMultiplier = 1.5;
const float hardMultiplier = 2.0;
float bonusMultiplier = 1.0;
const int easyBaseScore = 1;
const int mediumBaseScore = 2;
const int hardBaseScore = 3;
int baseScore = 1;
const int timeBonusPerSecond = 2;
enum PlayerState {
  moving,
  staying
};
PlayerState playerState = moving;
volatile int lastMovement = LEFT;

// wall constants
int wallPercentage = 10;
const int easyWallPercentage = 10;
const int mediumWallPercentage = 30;
const int hardWallPercentage = 50;
const int maxWallPercentage = 100;
bool walls[bigMatrixSize * bigMatrixSize];

// Display variables
const int maxContrast = 144;  // reasonable value to stay within visible bounds
const int maxBrightness = 255;
const int maxMatrixBrightness = 16;
const int maxLCDDisplayLength = 16;
volatile int displayContrast = 0;
int tempLcdBrightness;
int LCDBrightness = 254;
byte matrixBrightness = 5;
int tempMatrixBrightness;
int tempLcdContrast;
int stepSize;
int displayLevel;

// EEPROM addresses
const int addrLcdContrast = 0;
const int addrMatrixBrightness = sizeof(displayContrast);
const int addrHighscoreStart = addrMatrixBrightness + sizeof(matrixBrightness);
const int addrHighscoreEnd = addrHighscoreStart + sizeof(Highscore) * maxPlayers;
const int addrSound = addrHighscoreEnd + sizeof(sound);  // added sizeof(sound) just to be sure
const int addrLcdBrightness = addrSound + sizeof(sound);

// Name variables
char currentName[5] = "    ";
int letterIndex = 0;
int charIndex = 0;
bool isNameInputComplete = false;
const int maxNameCharacters = 36;
const int alphabet = 26;
const int lastNamePos = 4;
const int maxLetterIndex = 3;
int charIndexes[4] = { 0, 0, 0, 0 };

////// LCD
const byte rs = 11;  // Inital schema was 9
const byte en = 8;
const byte d4 = 7;
const byte d5 = 6;
const byte d6 = 5;
const byte d7 = 4;
const int contrastPin = 10;
const int bightnessPin = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Menu/submenu states and variables
enum GameState {
  MENU,
  GAME
};

GameState currentState = MENU;

enum MenuStates {
  MAIN_MENU,
  SETTINGS_SUBMENU,
  HIGHSCORES_SUBMENU,
  ABOUT_SUBMENU,
  HTP_SUBMENU,
  MATRIX_BRIGHTNESS_ADJUST,
  GAME_ONGOING,
  GAME_OVER,
  DISPLAY_NAME,
  SOUND_ADJUST,
  SCREEN_2_INFO,
  DIFFICULTY_SUBMENU,
  LCD_CONTRAST_ADJUST,
  LCD_BRIGHTNESS_ADJUST
};

enum Screen2InfoItems {
  SCREEN_2_INFO_1,
  SCREEN_2_INFO_2,
  SCREEN_2_INFO_3,
  SCREEN_2_INFO_4,
  SCREEN_2_INFO_5,
  SCREEN_2_INFO_6,
  SCREEN_2_INFO_BACK,
  NUM_INFO_ITEMS
};

enum MenuItems {
  START_GAME,
  DIFFICULTY,
  HIGHSCORES,
  SETTINGS,
  ABOUT,
  HOW_TO_PLAY,
  NUM_MENU_ITEMS
};

enum SettingsItems {
  LCD_CONTRAST,
  LCD_BRIGHTNESS,
  MATRIX_BRIGHTNESS,
  SOUND_SWITCH,
  SETTINGS_BACK,
  NUM_SETTINGS_ITEMS
};

enum AboutItems {
  GAME_NAME,
  AUTHOR,
  GITHUB,
  ABOUT_BACK,
  NUM_ABOUT_ITEMS
};

enum HowToPlayItems {
  HTP_1,
  HTP_2,
  HTP_3,
  HTP_4,
  HTP_BACK,
  NUM_HTP_ITEMS
};

enum HighscoresItems {
  H1,
  H2,
  H3,
  H4,
  H5,
  H_RESET,
  H_BACK,
  NUM_H_ITEMS
};

enum SoundOnItems {
  SWITCH_ON_OFF,
  SOUND_BACK,
  NUM_SOUND_ITEMS
};

enum DifficultyItems {
  EASY,
  MEDIUM,
  HARD,
  DIFFICULTY_BACK,
  NUM_DIF_ITEMS
};

int currentMenuItem = 0;
MenuStates currentMenuState = MAIN_MENU;
bool menuNeedsUpdate = true;
unsigned long gameOverTime = 0;
const int gameOverTimeDelay = 3000;
DifficultyItems gameDifficulty = EASY;

// Matrix rooms/quarters variables
enum Rooms {
  TOP_LEFT,
  TOP_RIGHT,
  BOTTOM_LEFT,
  BOTTOM_RIGHT
};
const int rightRowStartIndex = 56;
const int leftRowLastIndex = 7;
const int leftBigRowStartIndex = 128;
const int leftBigRowLastIndex = 143;

Rooms currentRoom = TOP_LEFT;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Settings function

void loadSettings() {
  displayContrast = EEPROM.read(addrLcdContrast);
  matrixBrightness = EEPROM.read(addrMatrixBrightness);
  sound = EEPROM.read(addrSound);
  LCDBrightness = EEPROM.read(addrLcdBrightness);
  updateHighscores();
  tempMatrixBrightness = matrixBrightness;
  tempLcdContrast = displayContrast;
  tempLcdBrightness = LCDBrightness;
  analogWrite(contrastPin, displayContrast);
  analogWrite(bightnessPin, LCDBrightness);
  lc.setIntensity(0, matrixBrightness);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  pinMode(pinBuzzer, OUTPUT);
  pinMode(pinSW, INPUT_PULLUP);
  pinMode(clockPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(pinSW), handleInterrupt, CHANGE);
  lc.shutdown(0, false);
  // load settings
  loadSettings();
  lcd.begin(16, 2);
  lcd.print("Welcome!");
  displayMatrixPattern(matrixPatterns[0]);
  playMelody();
  lcd.clear();
  // Create and upload custom characters
  lcd.createChar(0, arrowDown);
  lcd.createChar(1, arrowUp);
  lcd.createChar(2, doubleArrows);
  lcd.createChar(3, smiley);
  lcd.createChar(4, heart);
  lcd.createChar(5, selectArrow);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Loop

void loop() {
  handleMenuLogic();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Map generation functions

void generateRandomMap() {
  memset(walls, 0, sizeof(walls));  // we initialize everything with 0 (not needed usually, but just to be sure)

  if (gameDifficulty == EASY) {  // we check for the game difficulty and set the starting game constants as needed
    wallPercentage = easyWallPercentage;
    bonusMultiplier = easyMultiplier;
    baseScore = easyBaseScore;
  } else if (gameDifficulty == MEDIUM) {
    wallPercentage = mediumWallPercentage;
    bonusMultiplier = mediumMultiplier;
    baseScore = mediumBaseScore;
  } else if (gameDifficulty == HARD) {
    wallPercentage = hardWallPercentage;
    bonusMultiplier = hardMultiplier;
    baseScore = hardBaseScore;
  }
  walls[currentPos] = false;

  for (int i = 0; i < bigMatrixSize * bigMatrixSize; ++i) {
    if (i == currentPos || isNeighbor(i)) {
      continue;  // skip generating walls next to the player
    }
    if (isLeftEdgeOfRightQuarters(i) || isTopRowOfBottomQuarters(i)) {
      continue;  // skip generating walls on specified edges
    }
    int randomValue = random(maxWallPercentage);  // we fill the walls array randomly utilising the fact that random() is uniformly distributed
    walls[i] = randomValue < wallPercentage;      // so we compare the value to the wallPercentage and we will get roughly the number of walls expected for that percentage
  }
  updateMatrix();
}

bool isNeighbor(int position) {  // function that computes neighbours of player's random starting position
  int neighbors[neighbours] = {
    currentPos + bigMatrixSize,
    currentPos - bigMatrixSize,
    currentPos - 1,
    currentPos + 1
  };
  for (int i = 0; i < neighbours; i++) {
    if (position == neighbors[i]) {
      return true;
    }
  }
  return false;
}

bool isLeftEdgeOfRightQuarters(int position) {  // we check if position is on the left edge of the RIGHT quarters
  for (int i = matrixSize; i < bigMatrixSize * bigMatrixSize; i += bigMatrixSize) {
    if (position == i) {
      return true;
    }
  }
  return false;
}

bool isTopRowOfBottomQuarters(int position) {  // we check if position is in the top row of the BOTTOM quarters
  return (position >= leftBigRowStartIndex && position <= leftBigRowLastIndex);
}
// functions above are to permit the player to have space to traverse through rooms

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Highscore functions

void resetHighscores() {
  Highscore emptyHighscore;
  strcpy(emptyHighscore.playerName, "----");
  emptyHighscore.score = 0;

  for (int i = 0; i < 5; i++) {
    saveHighscore(emptyHighscore, i);
  }
}

void writeHighscore(int address, const Highscore &highscore) {  // function that writes the Highscore in EEPROM
  const byte *p = (const byte *)(const void *)&highscore;       // we cast the highscore struct to a byte pointer (from the general pointer const void)
  int i;                                                        // *used const pointer to indicate that it points to a constant value
  for (i = 0; i < sizeof(Highscore); i++) {                     // we iterate over each byte of the highscore and update the EEPROM
    EEPROM.update(address++, *p++);                             // after updating the EEPROM we increment the address by 1 byte
  }
}

void readHighscore(int address, Highscore &highscore) {  // function that reads the Highscore from EEPROM
  byte *p = (byte *)(void *)&highscore;                  // cast to byte pointer
  int i;
  for (i = 0; i < sizeof(Highscore); i++) {  // similar to the write function, we read the highscore byte by byte
    *p++ = EEPROM.read(address++);
  }
}

void saveHighscore(const Highscore &highscore, int slot) {  // function that saves the highscore at the address calculated based on what position the highscore is
  int address = addrHighscoreStart + slot * sizeof(Highscore);
  writeHighscore(address, highscore);
}

Highscore readHighscore(int slot) {  // function that reads the highscore from the ranking position(slot) specified
  Highscore highscore;
  int address = addrHighscoreStart + slot * sizeof(Highscore);
  readHighscore(address, highscore);
  return highscore;
}

// we read existing highscores and find the right position for the new score
void insertHighscore(const Highscore &newHighscore) {
  Highscore currentHighscores[5];
  bool nameFound = false;
  int insertPosition = -1;

  for (int i = 0; i < 5; i++) {
    currentHighscores[i] = readHighscore(i);
  }

  // check if the name already exists and update the score if it's higher
  for (int i = 0; i < 5; i++) {
    if (strcmp(newHighscore.playerName, currentHighscores[i].playerName) == 0) {
      if (newHighscore.score > currentHighscores[i].score) {
        currentHighscores[i].score = newHighscore.score;
      }
      nameFound = true;
      break;
    }
  }

  // find the position where the new highscore should be inserted if the name wasn't found
  if (!nameFound) {
    for (int i = 0; i < 5; i++) {
      if (newHighscore.score > currentHighscores[i].score) {
        insertPosition = i;
        break;
      }
    }
  }

  // insert the new highscore if a position was found
  if (insertPosition != -1) {
    for (int i = 4; i > insertPosition; i--) {
      currentHighscores[i] = currentHighscores[i - 1];
    }
    currentHighscores[insertPosition] = newHighscore;
  }

  // save updated highscores
  for (int i = 0; i < 5; i++) {
    saveHighscore(currentHighscores[i], i);
  }
}

bool checkIfBeatHighscore() {
  for (int i = 0; i < 5; i++) {
    if (score > highscores[i].score) {
      return true;
    }
  }
  return false;
}

void updateHighscores() {  // we initialize the highscores in a global variable
  for (int i = 0; i < 5; i++) {
    highscores[i] = readHighscore(i);
  }
}

int getRanking() {  //function that gets the ranking of the current player by comparing the score with the leaderboard
  for (int i = 0; i < 5; i++) {
    if (score > highscores[i].score) {
      return i;
    }
  }
  return notInTop;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Name functions

void handleNameInput() {  // function that checks for joystick input and updates the current letter as needed
  xValue = analogRead(pinX);
  yValue = analogRead(pinY);

  unsigned long currentMillis = millis();
  if (xValue > maxValue) {
    if (prevXValue <= maxValue || currentMillis - lastXMoveTime > autoRepeatDelay) {
      lastXMoveTime = currentMillis;
      charIndexes[letterIndex] = (charIndexes[letterIndex] + 1) % maxNameCharacters;  // loop from '9' back to 'A'
      menuNeedsUpdate = true;
    }
  } else if (xValue < minValue) {
    if (prevXValue >= minValue || currentMillis - lastXMoveTime > autoRepeatDelay) {
      lastXMoveTime = currentMillis;
      charIndexes[letterIndex] = (charIndexes[letterIndex] + (maxNameCharacters - 1)) % maxNameCharacters;  // loop from 'A' back to '9'
      menuNeedsUpdate = true;
    }
  }

  if (charIndexes[letterIndex] < alphabet) {
    currentName[letterIndex] = 'A' + charIndexes[letterIndex];
  } else {
    currentName[letterIndex] = '0' + (charIndexes[letterIndex] - alphabet);
  }

  if ((yValue < minValue && prevYValue >= minValue) || (yValue > maxValue && prevYValue <= maxValue)) {
    letterIndex = constrain(letterIndex + (yValue < minValue ? 1 : -1), 0, maxLetterIndex);
    menuNeedsUpdate = true;
  }

  bool currentButtonState = digitalRead(pinSW);
  if (currentButtonState == LOW && lastButtonState == HIGH) {  // if we press the button, we save the highscore
    if (millis() - lastDebounceTime > debounceDelay) {
      Highscore playerHighscore;
      strncpy(playerHighscore.playerName, currentName, sizeof(playerHighscore.playerName));
      playerHighscore.playerName[lastNamePos] = '\0';
      playerHighscore.score = score;
      insertHighscore(playerHighscore);
      updateHighscores();
      lastDebounceTime = millis();
      menuNeedsUpdate = true;
    }
  }

  lastButtonState = currentButtonState;
  prevXValue = xValue;
  prevYValue = yValue;
}

bool nameNotInHighscores() {
  for (int i = 0; i < 5; i++) {
    if (strcmp(currentName, highscores[i].playerName) == 0) {
      return false;
    }
  }
  return true;
}

void displayCurrentName() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Name:");
  lcd.setCursor(0, 1);
  lcd.print(currentName);
  lcd.setCursor(letterIndex, 1);
  lcd.cursor();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Adjust functions
void adjustMatrixBrightness() {  // function that checks for joystick input and updates the matrix brightness
  yValue = analogRead(pinY);

  if (yValue > maxValue && prevYValue <= maxValue && tempMatrixBrightness > 0) {
    tempMatrixBrightness--;
    lc.setIntensity(0, tempMatrixBrightness);  // we update the brightness in real time
    menuNeedsUpdate = true;
  } else if (yValue < minValue && prevYValue >= minValue && tempMatrixBrightness < maxMatrixBrightness) {
    tempMatrixBrightness++;
    lc.setIntensity(0, tempMatrixBrightness);
    menuNeedsUpdate = true;
  }
  prevYValue = yValue;

  bool currentButtonState = digitalRead(pinSW);
  if (currentButtonState == LOW && lastButtonState == HIGH) {  // on button press we update the setting in EEPROM
    if (millis() - lastDebounceTime > debounceDelay) {
      matrixBrightness = tempMatrixBrightness;
      EEPROM.update(addrMatrixBrightness, matrixBrightness);
      currentMenuItem = 0;
      menuNeedsUpdate = true;
      lastDebounceTime = millis();
    }
  }
  lastButtonState = currentButtonState;
}

void adjustLcdContrast() {  // function that checks for joystick input and updates the LCD contrast
  yValue = analogRead(pinY);
  stepSize = maxContrast / maxLCDDisplayLength;  // in this case we took the maxContrast range and divided it into steps based on the LCD length (16)

  if (yValue > maxValue && prevYValue <= maxValue && tempLcdContrast - stepSize > 0) {
    tempLcdContrast -= stepSize;
    analogWrite(contrastPin, tempLcdContrast);  // we update the contrast in real time
    menuNeedsUpdate = true;
  }

  else if (yValue < minValue && prevYValue >= minValue && tempLcdContrast <= maxContrast) {
    tempLcdContrast += stepSize;
    analogWrite(contrastPin, tempLcdContrast);
    menuNeedsUpdate = true;
  }
  prevYValue = yValue;

  bool currentButtonState = digitalRead(pinSW);
  if (currentButtonState == LOW && lastButtonState == HIGH) {  // on button press we update the contrast in EEPROM
    if (millis() - lastDebounceTime > debounceDelay) {
      displayContrast = tempLcdContrast;
      EEPROM.update(addrLcdContrast, displayContrast);
      currentMenuItem = 0;
      menuNeedsUpdate = true;
      lastDebounceTime = millis();
    }
  }
  lastButtonState = currentButtonState;
}

void adjustLcdBrightness() {  // similar to the adjustLcdContrast function
  yValue = analogRead(pinY);
  stepSize = maxBrightness / maxLCDDisplayLength;

  if (yValue > maxValue && prevYValue <= maxValue && tempLcdBrightness - stepSize > 0) {
    tempLcdBrightness -= stepSize;
    analogWrite(bightnessPin, tempLcdBrightness);
    menuNeedsUpdate = true;
  } else if (yValue < minValue && prevYValue >= minValue && tempLcdBrightness <= maxBrightness - stepSize) {
    tempLcdBrightness += stepSize;
    analogWrite(bightnessPin, tempLcdBrightness);
    menuNeedsUpdate = true;
  }
  prevYValue = yValue;

  bool currentButtonState = digitalRead(pinSW);
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    if (millis() - lastDebounceTime > debounceDelay) {
      LCDBrightness = tempLcdBrightness;
      EEPROM.update(addrLcdBrightness, LCDBrightness);
      currentMenuItem = 0;
      menuNeedsUpdate = true;
      lastDebounceTime = millis();
    }
  }
  lastButtonState = currentButtonState;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LCD and Menu functions

void handleMenuNavigation() {  // function that handles the menu navigation with the joystick (up and down movement)
  int previousMenuItem = currentMenuItem;
  xValue = analogRead(pinX);

  // the logic is that we have enums for each menu and submenu, and we have different max number of items for each menu, and we limit the scrolling to the last item

  if (xValue > maxValue && prevXValue <= maxValue) {
    if (currentMenuState == MAIN_MENU) {
      currentMenuItem++;
      if (currentMenuItem >= NUM_MENU_ITEMS) {
        currentMenuItem = NUM_MENU_ITEMS - 1;  // limit the menu scrolling to the last item
      }
    } else if (currentMenuState == ABOUT_SUBMENU) {
      currentMenuItem++;
      if (currentMenuItem >= NUM_ABOUT_ITEMS) {
        currentMenuItem = NUM_ABOUT_ITEMS - 1;
      }
    } else if (currentMenuState == SETTINGS_SUBMENU) {
      currentMenuItem++;
      if (currentMenuItem >= NUM_SETTINGS_ITEMS) {
        currentMenuItem = NUM_SETTINGS_ITEMS - 1;
      }
    } else if (currentMenuState == HTP_SUBMENU) {
      currentMenuItem++;
      if (currentMenuItem >= NUM_HTP_ITEMS) {
        currentMenuItem = NUM_HTP_ITEMS - 1;
      }
    } else if (currentMenuState == HIGHSCORES_SUBMENU) {
      currentMenuItem++;
      if (currentMenuItem >= NUM_H_ITEMS) {
        currentMenuItem = NUM_H_ITEMS - 1;
      }
    } else if (currentMenuState == SOUND_ADJUST) {
      currentMenuItem++;
      if (currentMenuItem >= NUM_SOUND_ITEMS) {
        currentMenuItem = NUM_SOUND_ITEMS - 1;
      }
    } else if (currentMenuState == SCREEN_2_INFO) {
      currentMenuItem++;
      if (currentMenuItem >= NUM_INFO_ITEMS) {
        currentMenuItem = NUM_INFO_ITEMS - 1;
      }
    } else if (currentMenuState == DIFFICULTY_SUBMENU) {
      currentMenuItem++;
      if (currentMenuItem >= NUM_DIF_ITEMS) {
        currentMenuItem = NUM_DIF_ITEMS - 1;
      }
    }
  }

  if (xValue < minValue && prevXValue >= minValue) {
    currentMenuItem--;
    if (currentMenuItem <= 0) {
      currentMenuItem = 0;  // limit scrolling to the first menu item
    }
  }

  prevXValue = xValue;

  if (currentMenuItem != previousMenuItem) {
    menuNeedsUpdate = true;
    playSound();
  }
}


void displayMenu() {  // function that displays each menu/submenu based on the current menu state and item
  if (menuNeedsUpdate) {
    lcd.clear();
    lcd.setCursor(0, 0);
    if (currentMenuState == MAIN_MENU) {
      lcd.setCursor(0, 0);
      lcd.print("Main Menu ");
      if (currentMenuItem == 0)
        lcd.write((byte)0);
      else if (currentMenuItem == NUM_MENU_ITEMS - 1)
        lcd.write((byte)1);
      else
        lcd.write((byte)2);

      lcd.setCursor(0, 1);
      switch (currentMenuItem) {
        case START_GAME:
          lcd.write((byte)5);
          lcd.print("Start Game");
          displayMatrixPattern(matrixPatterns[1]);
          break;
        case DIFFICULTY:
          lcd.write((byte)5);
          lcd.print("Difficulty");
          displayMatrixPattern(matrixPatterns[7]);
          break;
        case HIGHSCORES:
          lcd.write((byte)5);
          lcd.print("Highscores");
          displayMatrixPattern(matrixPatterns[2]);
          break;
        case SETTINGS:
          lcd.write((byte)5);
          lcd.print("Settings");
          displayMatrixPattern(matrixPatterns[3]);
          break;
        case ABOUT:
          lcd.write((byte)5);
          lcd.print("About");
          displayMatrixPattern(matrixPatterns[4]);
          break;
        case HOW_TO_PLAY:
          lcd.write((byte)5);
          lcd.print("How to play");
          displayMatrixPattern(matrixPatterns[5]);
          break;
      }
    } else if (currentMenuState == DIFFICULTY_SUBMENU) {
      lcd.setCursor(0, 0);
      lcd.print("Difficulty ");
      if (currentMenuItem == 0)
        lcd.write((byte)0);
      else if (currentMenuItem == NUM_DIF_ITEMS - 1)
        lcd.write((byte)1);
      else
        lcd.write((byte)2);

      lcd.setCursor(0, 1);
      switch (currentMenuItem) {
        case EASY:
          lcd.write((byte)5);
          lcd.print("Easy");
          if (gameDifficulty == EASY) {
            lcd.setCursor(7, 1);
            lcd.print("(current)");
          }
          break;
        case MEDIUM:
          lcd.write((byte)5);
          lcd.print("Medium");
          if (gameDifficulty == MEDIUM) {
            lcd.setCursor(7, 1);
            lcd.print("(current)");
          }
          break;
        case HARD:
          lcd.write((byte)5);
          lcd.print("Hard");
          if (gameDifficulty == HARD) {
            lcd.setCursor(7, 1);
            lcd.print("(current)");
          }
          break;
        case DIFFICULTY_BACK:
          lcd.write((byte)5);
          lcd.print("Back");
          break;
      }
    } else if (currentMenuState == HIGHSCORES_SUBMENU) {
      updateHighscores();
      lcd.setCursor(0, 0);
      lcd.print("Highscores ");
      if (currentMenuItem == 0)
        lcd.write((byte)0);
      else if (currentMenuItem == NUM_H_ITEMS - 1)
        lcd.write((byte)1);
      else
        lcd.write((byte)2);

      lcd.setCursor(0, 1);
      switch (currentMenuItem) {
        case H1:
          lcd.print("1.");
          lcd.print(highscores[0].playerName);
          lcd.print(" ");
          lcd.print(highscores[0].score);
          break;
        case H2:
          lcd.print("2.");
          lcd.print(highscores[1].playerName);
          lcd.print(" ");
          lcd.print(highscores[1].score);
          break;
        case H3:
          lcd.print("3.");
          lcd.print(highscores[2].playerName);
          lcd.print(" ");
          lcd.print(highscores[2].score);
          break;
        case H4:
          lcd.print("4.");
          lcd.print(highscores[3].playerName);
          lcd.print(" ");
          lcd.print(highscores[3].score);
          break;
        case H5:
          lcd.print("5.");
          lcd.print(highscores[4].playerName);
          lcd.print(" ");
          lcd.print(highscores[4].score);
          break;
        case H_RESET:
          lcd.write((byte)5);
          lcd.print("Reset scores");
          break;
        case H_BACK:
          lcd.write((byte)5);
          lcd.print("Back");
          break;
      }
    } else if (currentMenuState == SETTINGS_SUBMENU) {
      lcd.setCursor(0, 0);
      lcd.print("Settings ");
      if (currentMenuItem == 0)
        lcd.write((byte)0);
      else if (currentMenuItem == NUM_SETTINGS_ITEMS - 1)
        lcd.write((byte)1);
      else
        lcd.write((byte)2);

      lcd.setCursor(0, 1);
      lcd.write((byte)5);
      switch (currentMenuItem) {
        case LCD_CONTRAST:
          lcd.print("LCD Contrast");
          break;
        case LCD_BRIGHTNESS:
          lcd.print("LCD Brightness");
          break;
        case MATRIX_BRIGHTNESS:
          lcd.print("Mat Brightness");
          break;
        case SOUND_SWITCH:
          lcd.print("Sound");
          break;
        case SETTINGS_BACK:
          lcd.print("Back");
          break;
      }
    } else if (currentMenuState == ABOUT_SUBMENU) {
      lcd.setCursor(0, 0);
      lcd.print("About ");
      if (currentMenuItem == 0)
        lcd.write((byte)0);
      else if (currentMenuItem == NUM_ABOUT_ITEMS - 1)
        lcd.write((byte)1);
      else
        lcd.write((byte)2);

      lcd.setCursor(0, 1);
      switch (currentMenuItem) {
        case GAME_NAME:
          lcd.print(">Shooter mania");
          break;
        case AUTHOR:
          lcd.print(">by Timi");
          break;
        case GITHUB:
          lcd.print(">Git:TimiAndrei");
          break;
        case ABOUT_BACK:
          lcd.write((byte)5);
          lcd.print("Back");
          break;
      }
    } else if (currentMenuState == HTP_SUBMENU) {
      lcd.setCursor(0, 0);
      lcd.print("How to play ");
      if (currentMenuItem == 0)
        lcd.write((byte)0);
      else if (currentMenuItem == NUM_HTP_ITEMS - 1)
        lcd.write((byte)1);
      else
        lcd.write((byte)2);

      lcd.setCursor(0, 1);
      switch (currentMenuItem) {
        case HTP_1:
          lcd.print(">Move with js");
          break;
        case HTP_2:
          lcd.print(">Shoot with btn");
          break;
        case HTP_3:
          lcd.print(">Destroy walls");
          break;
        case HTP_4:
          lcd.print(">Have fun");
          break;
        case HTP_BACK:
          lcd.write((byte)5);
          lcd.print("Back");
          break;
      }
    } else if (currentMenuState == MATRIX_BRIGHTNESS_ADJUST) {
      lcd.setCursor(0, 0);
      lcd.print("Mat Brightness:");
      lcd.setCursor(0, 1);
      for (int i = 0; i < tempMatrixBrightness; i++) {
        lcd.print("#");
      }
    } else if (currentMenuState == LCD_CONTRAST_ADJUST) {
      displayLevel = map(tempLcdContrast, 0, maxContrast, 0, maxLCDDisplayLength);
      lcd.setCursor(0, 0);
      lcd.print("LCD Contrast:");
      lcd.setCursor(0, 1);
      for (int i = 0; i < displayLevel; i++) {
        lcd.print("#");
      }
    } else if (currentMenuState == LCD_BRIGHTNESS_ADJUST) {
      displayLevel = map(tempLcdBrightness, 0, maxBrightness, 0, maxLCDDisplayLength);
      lcd.setCursor(0, 0);
      lcd.print("LCD Brightness:");
      lcd.setCursor(0, 1);
      for (int i = 0; i < displayLevel; i++) {
        lcd.print("#");
      }
    } else if (currentMenuState == SOUND_ADJUST) {
      lcd.setCursor(0, 0);
      lcd.print("Sound ");
      if (currentMenuItem == 0)
        lcd.write((byte)0);
      else if (currentMenuItem == NUM_SOUND_ITEMS - 1)
        lcd.write((byte)1);
      else
        lcd.write((byte)2);

      lcd.setCursor(0, 1);
      switch (currentMenuItem) {
        case SWITCH_ON_OFF:
          if (sound == 1) {
            lcd.write((byte)5);
            lcd.print("Turn Off");
          } else {
            lcd.write((byte)5);
            lcd.print("Turn On");
          }
          break;
        case SOUND_BACK:
          lcd.write((byte)5);
          lcd.print("Back");
          break;
      }
    } else if (currentMenuState == SCREEN_2_INFO) {
      lcd.setCursor(0, 0);
      lcd.print("Game Info ");
      if (currentMenuItem == 0)
        lcd.write((byte)0);
      else if (currentMenuItem == NUM_INFO_ITEMS - 1)
        lcd.write((byte)1);
      else
        lcd.write((byte)2);

      lcd.setCursor(0, 1);
      switch (currentMenuItem) {
        case SCREEN_2_INFO_1:
          lcd.print(">Name:");
          lcd.print(currentName);
          break;
        case SCREEN_2_INFO_2:
          lcd.print(">Time:");
          lcd.print(timePlayed);
          lcd.print("s");
          break;
        case SCREEN_2_INFO_3:
          lcd.print(">Score:");
          lcd.print(score);
          break;
        case SCREEN_2_INFO_4:
          lcd.print(">Destroyed:");
          lcd.print(wallsDestroyed);
          break;
        case SCREEN_2_INFO_5:
          lcd.print(">Walls left:");
          lcd.print(wallsLeft());
          break;
        case SCREEN_2_INFO_6:
          lcd.print(">Ranking:");
          if (getRanking() == notInTop) {
            lcd.print("N/A");
          } else {
            lcd.print(getRanking());
          }
          break;
        case SCREEN_2_INFO_BACK:
          lcd.write((byte)5);
          lcd.print("Back");
          break;
      }
    } else if (currentMenuState == DISPLAY_NAME) {
      displayCurrentName();
    } else if (currentMenuState == GAME_ONGOING) {
      displayTimeLeft();
      displayWallsLeft();
    } else if (currentMenuState == GAME_OVER) {
      playGameOverSound();
      displayGameOver();
    }
    menuNeedsUpdate = false;
  }
}

void selectMenuItem() {  //function that handles selection when when button is pressed in Menu state
  switch (currentMenuState) {
    case MAIN_MENU:
      switch (currentMenuItem) {
        case START_GAME:
          currentState = GAME;
          currentMenuState = GAME_ONGOING;
          startGame();
          break;
        case DIFFICULTY:
          currentMenuItem = 0;
          currentMenuState = DIFFICULTY_SUBMENU;
          break;
        case HIGHSCORES:
          currentMenuItem = 0;
          currentMenuState = HIGHSCORES_SUBMENU;
          break;
        case SETTINGS:
          currentMenuItem = 0;
          currentMenuState = SETTINGS_SUBMENU;
          break;
        case ABOUT:
          currentMenuItem = 0;
          currentMenuState = ABOUT_SUBMENU;
          break;
        case HOW_TO_PLAY:
          currentMenuItem = 0;
          currentMenuState = HTP_SUBMENU;
          break;
      }
      break;

    case DIFFICULTY_SUBMENU:
      if (currentMenuItem == EASY) {
        gameDifficulty = EASY;
        currentMenuItem = 0;
        currentMenuState = MAIN_MENU;
      }
      if (currentMenuItem == MEDIUM) {
        gameDifficulty = MEDIUM;
        currentMenuItem = 0;
        currentMenuState = MAIN_MENU;
      }
      if (currentMenuItem == HARD) {
        gameDifficulty = HARD;
        currentMenuItem = 0;
        currentMenuState = MAIN_MENU;
      }
      if (currentMenuItem == DIFFICULTY_BACK) {
        currentMenuItem = DIFFICULTY;
        currentMenuState = MAIN_MENU;
      }
      break;
    case ABOUT_SUBMENU:
      if (currentMenuItem == ABOUT_BACK) {
        currentMenuItem = ABOUT;
        currentMenuState = MAIN_MENU;
      }
      break;

    case DISPLAY_NAME:
      lcd.noCursor();
      currentMenuState = MAIN_MENU;
      if (infoScreen2Flag) {
        currentMenuState = SCREEN_2_INFO;
        currentMenuItem = SCREEN_2_INFO_1;
        infoScreen2Flag = false;
      }
      break;

    case SCREEN_2_INFO:
      if (currentMenuItem == SCREEN_2_INFO_BACK) {
        currentMenuItem = START_GAME;
        currentMenuState = MAIN_MENU;
      }
      break;

    case HIGHSCORES_SUBMENU:
      if (currentMenuItem == H_BACK) {
        currentMenuItem = HIGHSCORES;
        currentMenuState = MAIN_MENU;
      }
      if (currentMenuItem == H_RESET) {
        resetHighscores();
        currentMenuItem = H1;
        currentMenuState = HIGHSCORES_SUBMENU;
      }
      break;

    case SETTINGS_SUBMENU:
      if (currentMenuItem == MATRIX_BRIGHTNESS) {
        currentMenuState = MATRIX_BRIGHTNESS_ADJUST;
      }
      if (currentMenuItem == LCD_CONTRAST) {
        currentMenuState = LCD_CONTRAST_ADJUST;
      }
      if (currentMenuItem == LCD_BRIGHTNESS) {
        currentMenuState = LCD_BRIGHTNESS_ADJUST;
      }
      if (currentMenuItem == SOUND_SWITCH) {
        currentMenuItem = SWITCH_ON_OFF;
        currentMenuState = SOUND_ADJUST;
      }
      if (currentMenuItem == SETTINGS_BACK) {
        currentMenuItem = SETTINGS;
        currentMenuState = MAIN_MENU;
      }
      break;

    case HTP_SUBMENU:
      if (currentMenuItem == HTP_BACK) {
        currentMenuItem = HOW_TO_PLAY;
        currentMenuState = MAIN_MENU;
      }
      break;

    case MATRIX_BRIGHTNESS_ADJUST:
      adjustMatrixBrightness();
      currentMenuItem = MATRIX_BRIGHTNESS;
      currentMenuState = SETTINGS_SUBMENU;
      break;

    case LCD_CONTRAST_ADJUST:
      adjustLcdContrast();
      currentMenuItem = LCD_CONTRAST;
      currentMenuState = SETTINGS_SUBMENU;
      break;

    case LCD_BRIGHTNESS_ADJUST:
      adjustLcdBrightness();
      currentMenuItem = LCD_BRIGHTNESS;
      currentMenuState = SETTINGS_SUBMENU;
      break;

    case SOUND_ADJUST:
      if (currentMenuItem == SWITCH_ON_OFF) {
        switchSound();
      }
      if (currentMenuItem == SOUND_BACK) {
        currentMenuItem = SOUND_SWITCH;
        currentMenuState = SETTINGS_SUBMENU;
      }
      break;
    case GAME_OVER:
      currentMenuState = MAIN_MENU;
      currentState = MENU;
      break;
  }
  menuNeedsUpdate = true;
}

void handleMenuLogic() { // function that handles the MENU and GAME states
  if (currentState == MENU) {
    if (currentMenuState == MATRIX_BRIGHTNESS_ADJUST) { 
      adjustMatrixBrightness(); // functions like this needed to be called in the loop for constant updates
    }
    if (currentMenuState == LCD_CONTRAST_ADJUST) {
      adjustLcdContrast();
    }
    if (currentMenuState == LCD_BRIGHTNESS_ADJUST) {
      adjustLcdBrightness();
    }
    if (currentMenuState == DISPLAY_NAME) {
      handleNameInput();
    }
    if (currentMenuState == GAME_OVER) {
      handleGameOver();
    }
    handleMenuNavigation();
    if (menuNeedsUpdate) {
      displayMenu();
      menuNeedsUpdate = false;
    }
  } else if (currentState == GAME) {
    if (currentMenuState == GAME_ONGOING) {
      unsigned long currentTime = millis();
      if (currentTime - lastUpdateTime >= second) {
        displayTimeLeft();
        displayWallsLeft();
        lastUpdateTime = currentTime;
      }
    }
    timePlayed = (millis() - timePlayedStart) / second;
    if (menuNeedsUpdate) {
      displayMenu();
      menuNeedsUpdate = false;
    }
    handleGameLogic();
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Display functions
void displayWallsLeft() {
  lcd.setCursor(0, 1);
  lcd.print("Walls left: ");
  lcd.print(wallsLeft());
}

void displayScore() {
  lcd.setCursor(0, 0);
  lcd.print("Score: ");
  lcd.print(score);
}

void displayTimeLeft() {
  unsigned long currentTime = millis();
  timeElapsed = currentTime - timePlayedStart;
  timeLeft = (timeLimit - timeElapsed) / second;

  lcd.setCursor(0, 0);
  lcd.print("Time left: ");
  lcd.print(timeLeft);
  lcd.print("s");
  menuNeedsUpdate = true;
}

void displayGameOver() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Congratulations!");
  lcd.write((byte)3);
  lcd.setCursor(0, 1);
  lcd.print("Score: ");
  lcd.print(score);
}

void displayMatrixPattern(uint64_t pattern) { // function to display the matrix patter stored as uint64_t
  for (int row = 0; row < matrixSize; row++) {
    uint8_t rowValue = (pattern >> (matrixSize * row)) & 0xFF;

    for (int col = 0; col < matrixSize; col++) {
      bool pixel = rowValue & (1 << col);
      lc.setLed(0, row, col, pixel);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Game handling functions
void startGame() { // function that 
  currentPos = random(matrixSize * matrixSize); // generates random starting position for player
  randomSeed(analogRead(A3));                   // change the seed by reading random value on Analog input pin
  generateRandomMap();                          // generate the map
  lc.setLed(0, currentPos % matrixSize, currentPos / matrixSize, pixelState); //(not needed, but used to instantly light up the player on game start)
  score = 0;
  wallsDestroyed = 0;
  timePlayedStart = millis();
}

void handleGameLogic() { // function that checks handles the game state, player and bullet movement
  if (areAllWallsRemoved()) {
    displayMatrixPattern(matrixPatterns[6]);
    currentState = MENU;
    currentMenuState = GAME_OVER;
    menuNeedsUpdate = true;
    return;
  }
  if (millis() - timePlayedStart >= timeLimit) {
    displayMatrixPattern(matrixPatterns[6]);
    currentState = MENU;
    currentMenuState = GAME_OVER;
    menuNeedsUpdate = true;
    return;
  }
  handleMovement();
  handleBlinking();
  handleBulletMovement();
}

void handleGameOver() { //function that handles the game over state
  static unsigned long gameOverTime = 0;

  if (gameOverTime == 0) {
    gameOverTime = millis();
  }

  if (millis() - gameOverTime > gameOverTimeDelay) {
    gameOverTime = 0;
    currentMenuState = DISPLAY_NAME;
    infoScreen2Flag = true;
    menuNeedsUpdate = true;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Wall functions
void updateMatrix() { // function that displays the walls on the 8x8 physical matrix
  int startX, startY;
  switch (currentRoom) {
    case TOP_LEFT:
      startX = 0;
      startY = 0;
      break;
    case TOP_RIGHT:
      startX = 8;
      startY = 0;
      break;
    case BOTTOM_LEFT:
      startX = 0;
      startY = 8;
      break;
    case BOTTOM_RIGHT:
      startX = 8;
      startY = 8;
      break;
  }

  for (int i = 0; i < 8; i++) { 
    for (int j = 0; j < 8; j++) {
      lc.setLed(0, i, j, walls[(startY + i) * bigMatrixSize + (startX + j)]);  // turns on the led if there's a wall on the position calculated based on the current room
    }
  }
}  

int wallsLeft() {
  int wallsLeft = 0;
  for (int i = 0; i < bigMatrixSize * bigMatrixSize; ++i) {
    if (walls[i]) {
      wallsLeft++;
    }
  }
  return wallsLeft;
}

bool areAllWallsRemoved() {
  for (int i = 0; i < bigMatrixSize * bigMatrixSize; ++i) {
    if (walls[i]) {
      return false;
    }
  }
  return true;
}

bool isWall(int position) {
  int adjustedPosition = getAdjustedPosition(position);
  if (adjustedPosition != -1) {
    return walls[adjustedPosition];
  } else {
    return false;
  }
}

void removeWall(int position) {
  int adjustedPosition = getAdjustedPosition(position);
  walls[adjustedPosition] = false;
  updateMatrix();  // Update the matrix to reflect the change
}

int getAdjustedPosition(int position) { // helper function to compute the current physical matrix position from the 16x16 walls array based on the current room
  int x = position % matrixSize;
  int y = position / matrixSize;
  int startX, startY;

  switch (currentRoom) {
    case TOP_LEFT:
      startX = 0;
      startY = 0;
      break;
    case TOP_RIGHT:
      startX = 0;
      startY = 8;
      break;
    case BOTTOM_LEFT:
      startX = 8;
      startY = 0;
      break;
    case BOTTOM_RIGHT:
      startX = 8;
      startY = 8;
      break;
  }

  int adjustedX = startX + x;
  int adjustedY = startY + y;
  int adjusted_position = adjustedX * bigMatrixSize + adjustedY;

  return adjusted_position;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Player movement

void handleMovement() {  // handles joystick inputs and computes next position
  xValue = analogRead(pinX);
  yValue = analogRead(pinY);

  if (xValue > maxValue && prevXValue <= maxValue) {
    currentPos = movePlayer(currentPos, RIGHT);
  }
  if (xValue < minValue && prevXValue >= minValue) {
    currentPos = movePlayer(currentPos, LEFT);
  }

  if (yValue > maxValue && prevYValue <= maxValue) {
    currentPos = movePlayer(currentPos, DOWN);
  }

  if (yValue < minValue && prevYValue >= minValue) {
    currentPos = movePlayer(currentPos, UP);
  }

  prevXValue = xValue;
  prevYValue = yValue;
}


int movePlayer(int currentPos, int direction) {  // function that checks for joystick movement and computes the player movement checking for what room it is on as well
  int newPos = currentPos;
  lc.setLed(0, currentPos % bigMatrixSize, currentPos / bigMatrixSize, LOW);
  playerState = moving;
  if (direction == UP) {  // we check for the direction we're going and check if we're able to move to another room based on the current room
    if (currentRoom == TOP_LEFT && currentPos >= rightRowStartIndex) {
      currentRoom = TOP_RIGHT;
      newPos = currentPos - rightRowStartIndex;
    } else if (currentRoom == BOTTOM_LEFT && currentPos >= rightRowStartIndex) {
      currentRoom = BOTTOM_RIGHT;
      newPos = currentPos - rightRowStartIndex;
    }
    if (currentPos < matrixSize * (matrixSize - 1) && !isWall(currentPos + matrixSize)) {  // there are also the physical matrix and wall constraints checked
      newPos = currentPos + matrixSize;
      lastMovement = UP;
    }
  }  // the rest of the ifs are similar, but for the other directions and specific rooms
  if (direction == DOWN) {
    if (currentRoom == TOP_RIGHT && currentPos <= leftRowLastIndex) {
      currentRoom = TOP_LEFT;
      newPos = currentPos + rightRowStartIndex;
    } else if (currentRoom == BOTTOM_RIGHT && currentPos <= leftRowLastIndex) {
      currentRoom = BOTTOM_LEFT;
      newPos = currentPos + rightRowStartIndex;
    }

    if (currentPos >= matrixSize && !isWall(currentPos - matrixSize)) {
      newPos = currentPos - matrixSize;
      lastMovement = DOWN;
    }
  }
  if (direction == LEFT) {
    if (currentRoom == BOTTOM_LEFT && currentPos % matrixSize == 0) {
      currentRoom = TOP_LEFT;
      newPos = currentPos + leftRowLastIndex;
    } else if (currentRoom == BOTTOM_RIGHT && currentPos % matrixSize == 0) {
      currentRoom = TOP_RIGHT;
      newPos = currentPos + leftRowLastIndex;
    }
    if (currentPos % matrixSize != 0 && !isWall(currentPos - 1)) {
      newPos = currentPos - 1;
      lastMovement = LEFT;
    }
  }
  if (direction == RIGHT) {

    if (currentRoom == TOP_LEFT && currentPos % matrixSize == leftRowLastIndex) {
      currentRoom = BOTTOM_LEFT;
      newPos = currentPos - leftRowLastIndex;
    } else if (currentRoom == TOP_RIGHT && currentPos % matrixSize == leftRowLastIndex) {
      currentRoom = BOTTOM_RIGHT;
      newPos = currentPos - leftRowLastIndex;
    }
    if ((currentPos + 1) % matrixSize != 0 && !isWall(currentPos + 1)) {
      newPos = currentPos + 1;
      lastMovement = RIGHT;
    }
  }
  updateMatrix();
  return newPos;
}

void handleBlinking() {  // player blinking function
  unsigned long currentMillis = millis();

  if (areAllWallsRemoved()) {
    return;
  }

  if (currentMillis - lastBlinkMillis >= playerBlinkTime) {
    pixelState = !pixelState;
    lc.setLed(0, currentPos % matrixSize, currentPos / matrixSize, pixelState);
    lastBlinkMillis = currentMillis;
  }
  if (playerState == moving) {
    lc.setLed(0, currentPos % matrixSize, currentPos / matrixSize, HIGH);
    playerState = staying;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sound functions

void playMelody() {
  if (sound) {
    for (int thisNote = 0; thisNote < maxNotes; thisNote++) {
      int noteDuration = second / noteDurations[thisNote];
      tone(pinBuzzer, melody[thisNote], noteDuration);

      int pauseBetweenNotes = noteDuration * pauseDelay;
      delay(pauseBetweenNotes);  // meant to be blocking

      noTone(pinBuzzer);
    }
  } else
    delay(3000);  // wait a few moments for the welcome message to stay displayed before clearing the lcd if sound is off
  // instead of playing the song
}  // function called only in setup, and it's meant to be blocking

void playGameOverSound() {
  if (sound) {
    int noteDuration = second / noteDurations[0];
    tone(pinBuzzer, NOTE_E4, noteDuration);
    int pauseBetweenNotes = noteDuration * pauseDelay;
    delay(pauseBetweenNotes);
    tone(pinBuzzer, NOTE_D4, noteDuration);
    delay(pauseBetweenNotes);
    tone(pinBuzzer, NOTE_C4, noteDuration);
    delay(pauseBetweenNotes);
    noTone(pinBuzzer);
  }
}  // all these delays are meant to be blocking and the function is called only in the game over state

void switchSound() {
  if (sound == 1)
    sound = 0;
  else
    sound = 1;
  EEPROM.update(addrSound, sound);
  menuNeedsUpdate = true;
}

void playSound() {
  if (sound) {
    tone(pinBuzzer, fireFreq, soundDuration);
  }
}

void playCollisionSound() {
  if (sound) {
    tone(pinBuzzer, collisionFreq, collisionSoundDuration);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bullet functions

void fireBullet() {  // function to initiate the bullet action, starting from the player's position (currentPos)
  bulletFired = true;
  bulletPos = currentPos;
  lastBulletPos = currentPos;
  playSound();
}

void handleBulletMovement() {  // function that makes the bullet move across the 8x8 maatrix
  static unsigned long lastBulletMoveMillis = 0;

  if (bulletFired) {
    unsigned long currentMillis = millis();
    switchBulletLed(bulletPos, true);                               // we instantly light up the led for the current bullet position
    if (currentMillis - lastBulletMoveMillis >= bulletMoveDelay) {  // we add a clock to wait some delay before the bullet goes to the next position
      switchBulletLed(lastBulletPos, false);                        // we turn off the led for the last position
      lastBulletPos = bulletPos;                                    // in each if we check if below, we check if the next position is valid based on the direction the bullet is going
      if (lastMovement == UP && bulletPos < matrixSize * (matrixSize - 1) && !isWall(bulletPos + matrixSize)) {
        bulletPos = bulletPos + matrixSize;
      } else if (lastMovement == DOWN && bulletPos >= matrixSize && !isWall(bulletPos - matrixSize)) {
        bulletPos = bulletPos - matrixSize;
      } else if (lastMovement == LEFT && bulletPos % matrixSize != 0 && !isWall(bulletPos - 1)) {
        bulletPos = bulletPos - 1;
      } else if (lastMovement == RIGHT && (bulletPos + 1) % matrixSize != 0 && !isWall(bulletPos + 1)) {
        bulletPos = bulletPos + 1;
      } else {
        if (isWall(computeBulletNextPos())) {  // if there's no position valid, we check if we hit a wall and apply collision logic and we update the score
          playCollisionSound();
          removeWall(computeBulletNextPos());
          wallsDestroyed++;
          score += baseScore;
          int timeBonus = timeLeft * timeBonusPerSecond * bonusMultiplier;
          score += timeBonus;
          menuNeedsUpdate = true;
        }
        bulletFired = false;
      }
      lastBulletMoveMillis = currentMillis;
    }
    switchBulletLed(bulletPos, false);  // we turn off the led on current position
  }
}

void switchBulletLed(int position, bool state) {  // helper function to mturn the led on/off
  if (position != -1 && position != currentPos) {
    lc.setLed(0, position % matrixSize, position / matrixSize, state);
  }
}

int computeBulletNextPos() {  // helper function to calculate the next position the bullet should be going
  if (lastMovement == UP && bulletPos < matrixSize * (matrixSize - 1)) {
    return bulletPos + matrixSize;
  } else if (lastMovement == DOWN && bulletPos >= matrixSize) {
    return bulletPos - matrixSize;
  } else if (lastMovement == LEFT && bulletPos % matrixSize != 0) {
    return bulletPos - 1;
  } else if (lastMovement == RIGHT && (bulletPos + 1) % matrixSize != 0) {
    return bulletPos + 1;
  } else
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Interrupt function

void handleInterrupt() {
  buttonRead = digitalRead(pinSW);
  if (buttonRead != lastButtonRead) {
    interruptTime = micros();
  }

  if (interruptTime - lastInterruptTime > debounceDelay * second) {
    if (buttonState != buttonRead) {
      buttonState = buttonRead;
    }
    if (buttonState == LOW) {  // if we press the game and we're in Game state, then we fire a bullet
      if (currentState == GAME) {
        fireBullet();
      } else if (currentState == MENU) {  // else if we're in Menu state we make a select action in menu
        playSound();
        selectMenuItem();
      }
    }
  }
  lastInterruptTime = interruptTime;
  lastButtonRead = buttonRead;
}