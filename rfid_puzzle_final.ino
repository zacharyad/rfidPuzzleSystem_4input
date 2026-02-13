/*
 * RFID Puzzle System - Production Version
 * 
 * Hardware:
 * - Arduino Nano
 * - MFRC522 RFID Reader (SDA=8, RST=9)
 * - Button (Pin 4, INPUT_PULLUP)
 * - Buzzer (Pin 3)
 * - WS2812B LED Strip - 9 LEDs (Pin 2)
 * - Relay Module (Pin 5)
 * 
 * Features:
 * - RFID card sequence puzzle
 * - Card programming mode
 * - Combination setting mode
 * - Persistent EEPROM storage
 * - EQ-style linear LED animations
 * - Synchronized buzzer sounds
 * - Relay trigger on puzzle unlock
 * - Relay trigger on programming mode (bypass)
 * 
 * Operation:
 * - Button press: Enter programming mode (activates relay)
 * - Button hold 4s: Enter set combo mode
 * - Puzzle solve: Triggers relay for 3 seconds
 */

#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <FastLED.h>

// -------------------- PINS --------------------
#define SDA_PIN 8
#define RST_PIN 9
#define BUTTON_PIN 4
#define BUZZER_PIN 3
#define LED_STRIP_PIN 2
#define RELAY_PIN 5

// -------------------- LED STRIP CONFIGURATION --------------------
#define NUM_LEDS 9
#define LED_TYPE WS2812B
#define COLOR_ORDER RGB
#define BRIGHTNESS 75
CRGB leds[NUM_LEDS];

// -------------------- EEPROM ADDRESSES --------------------
#define EEPROM_MAGIC_ADDR 0      // Magic byte to verify valid data
#define EEPROM_LENGTH_ADDR 1     // Code length
#define EEPROM_CODE_ADDR 2       // Start of code values
#define EEPROM_MAGIC_VALUE 0x42  // Arbitrary magic number

// -------------------- READER --------------------
MFRC522 rfid(SDA_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// -------------------- MODES --------------------
enum Mode {PUZZLE, PROGRAM, SET_COMBO};
Mode currentMode = PUZZLE;

// Button timing
unsigned long buttonHoldStart = 0;
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 300;

// -------------------- PUZZLE DATA --------------------
#define MAX_CARDS 10
int correctCode[MAX_CARDS] = {1, 2, 3, 4}; // default puzzle code
int codeLength = 4;

int enteredValues[MAX_CARDS];
int currentIndex = 0;

// -------------------- PROGRAMMING DATA --------------------
int selectedValue = 0;

// -------------------- CARD TRACKING --------------------
bool cardCurrentlyPresent = false;
byte lastUID[10];
byte lastUIDSize = 0;

// -------------------- FUNCTION PROTOTYPES --------------------
void checkButtonPress();
bool readCard(byte &value);
bool writeCard(byte value);
bool compareUID(byte *uid1, byte size1, byte *uid2, byte size2);
void handleReader();
void programmingLoop();
void puzzleLoop();
void setComboLoop();
void saveComboToEEPROM();
void loadComboFromEEPROM();

// LED Strip Animations
void clearStrip();
void showNumber(int number);
void animateCardRead(int position, int total);
void animateSuccess();
void animateFailure();
void animateWriteSuccess();
void animateWriteFailure();
void animateModeChange(Mode newMode);
void animateProgress(int current, int total);
void rainbowCycle(int wait);
void showProgressBar(int current, int total);

// Buzzer Sounds
void beepCardRead();
void beepSuccess();
void beepFailure();
void beepModeChange();
void beepWriteSuccess();
void beepWriteFailure();
void beepSuccessNonBlocking();
void beepFailureNonBlocking();

// Relay Control
void triggerRelay();
void releaseRelay();

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);
  
  SPI.begin();
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);

  FastLED.addLeds<LED_TYPE, LED_STRIP_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  clearStrip();
  
  rainbowCycle(30);
  clearStrip();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  loadComboFromEEPROM();

  Serial.println("RFID Puzzle System - Ready");
  animateModeChange(PUZZLE);
}

// -------------------- MAIN LOOP --------------------
void loop() {
  static unsigned long lastHealthCheck = 0;
  if (millis() - lastHealthCheck > 5000) {
    byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
    if (version == 0x00 || version == 0xFF) {
      rfid.PCD_Init();
      rfid.PCD_SetAntennaGain(rfid.RxGain_max);
    }
    lastHealthCheck = millis();
  }
  
  checkButtonPress();

  switch (currentMode) {
    case PROGRAM:
      programmingLoop();
      break;
    case SET_COMBO:
      setComboLoop();
      break;
    case PUZZLE:
    default:
      puzzleLoop();
      break;
  }

  delay(10);
}

// -------------------- BUTTON PRESS CHECK --------------------
void checkButtonPress() {
  static bool wasPressed = false;
  static unsigned long pressStartTime = 0;
  
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (!wasPressed) {
      wasPressed = true;
      pressStartTime = millis();
    }
  } else {
    if (wasPressed) {
      unsigned long pressDuration = millis() - pressStartTime;
      
      if (pressDuration >= 4000) {
        if (currentMode != SET_COMBO) {
          currentMode = SET_COMBO;
          currentIndex = 0;
          animateModeChange(SET_COMBO);
          beepModeChange();
          clearStrip();
        }
      } else if (pressDuration >= 100 && pressDuration < 4000) {
        if (currentMode == PUZZLE) {
          currentMode = PROGRAM;
          selectedValue = 0;
          animateModeChange(PROGRAM);
          beepModeChange();
          showNumber(selectedValue);
          triggerRelay();
        }
      }
      
      wasPressed = false;
    }
  }
}

// -------------------- UID COMPARISON --------------------
bool compareUID(byte *uid1, byte size1, byte *uid2, byte size2) {
  if (size1 != size2) return false;
  for (byte i = 0; i < size1; i++) {
    if (uid1[i] != uid2[i]) return false;
  }
  return true;
}

// -------------------- READ CARD VALUE --------------------
bool readCard(byte &value) {
  value = 0xFF;
  byte block = 1;
  byte buffer[18];
  byte size = sizeof(buffer);

  for (int attempt = 0; attempt < 3; attempt++) {
    MFRC522::StatusCode status;
    
    unsigned long authStart = millis();
    status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &rfid.uid);
    
    if (millis() - authStart > 500) {
      rfid.PCD_StopCrypto1();
      return false;
    }
    
    if (status != MFRC522::STATUS_OK) {
      if (attempt < 2) {
        delay(10);
        continue;
      }
      rfid.PCD_StopCrypto1();
      return false;
    }

    unsigned long readStart = millis();
    status = rfid.MIFARE_Read(block, buffer, &size);
    
    if (millis() - readStart > 500) {
      rfid.PCD_StopCrypto1();
      return false;
    }
    
    if (status == MFRC522::STATUS_OK) {
      value = buffer[0];
      return true;
    } else if (status == MFRC522::STATUS_CRC_WRONG && attempt < 2) {
      delay(10);
      rfid.PCD_StopCrypto1();
      continue;
    } else {
      rfid.PCD_StopCrypto1();
      if (attempt < 2) {
        delay(10);
        continue;
      }
      return false;
    }
  }
  
  rfid.PCD_StopCrypto1();
  return false;
}

// -------------------- WRITE CARD VALUE --------------------
bool writeCard(byte value) {
  byte block = 1;
  byte dataBlock[16] = {0};
  dataBlock[0] = value;

  for (int attempt = 0; attempt < 3; attempt++) {
    MFRC522::StatusCode status;
    
    unsigned long authStart = millis();
    status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &rfid.uid);
    
    if (millis() - authStart > 500) {
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return false;
    }
    
    if (status != MFRC522::STATUS_OK) {
      if (attempt < 2) {
        delay(10);
        continue;
      }
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return false;
    }

    unsigned long writeStart = millis();
    status = rfid.MIFARE_Write(block, dataBlock, 16);
    
    if (millis() - writeStart > 500) {
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return false;
    }
    
    if (status == MFRC522::STATUS_OK) {
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return true;
    } else if (status == MFRC522::STATUS_CRC_WRONG && attempt < 2) {
      delay(10);
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      continue;
    } else {
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      if (attempt < 2) {
        delay(10);
        continue;
      }
      return false;
    }
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return false;
}

// -------------------- HANDLE CARD READER --------------------
void handleReader() {
  static unsigned long lastCardCheckTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastCardCheckTime < 20) {
    return;
  }
  lastCardCheckTime = currentTime;
  
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if (!cardCurrentlyPresent) {
      cardCurrentlyPresent = true;

      if (!compareUID(rfid.uid.uidByte, rfid.uid.size, lastUID, lastUIDSize)) {
        if (rfid.uid.size <= 10) {
          memcpy(lastUID, rfid.uid.uidByte, rfid.uid.size);
          lastUIDSize = rfid.uid.size;
        }

        if (currentMode == PROGRAM) {
          if (writeCard(selectedValue)) {
            tone(BUZZER_PIN, 800, 100);
            animateWriteSuccess();
            showNumber(selectedValue);
          } else {
            tone(BUZZER_PIN, 200, 100);
            animateWriteFailure();
            showNumber(selectedValue);
          }
          rfid.PICC_HaltA();
          rfid.PCD_StopCrypto1();
        } 
        else if (currentMode == PUZZLE) {
          byte val;
          if (readCard(val)) {
            enteredValues[currentIndex++] = val;
            tone(BUZZER_PIN, 1500, 100);
            animateCardRead(currentIndex, codeLength);
            showProgressBar(currentIndex, codeLength);
            rfid.PICC_HaltA();
            rfid.PCD_StopCrypto1();
          } else {
            tone(BUZZER_PIN, 200, 100);
            animateWriteFailure();
            showProgressBar(currentIndex, codeLength);
            rfid.PICC_HaltA();
            rfid.PCD_StopCrypto1();
          }
        } 
        else if (currentMode == SET_COMBO) {
          byte val;
          if (readCard(val)) {
            correctCode[currentIndex] = val;
            currentIndex++;
            tone(BUZZER_PIN, 1500, 100);
            animateProgress(currentIndex, codeLength);
            rfid.PICC_HaltA();
            rfid.PCD_StopCrypto1();
          } else {
            tone(BUZZER_PIN, 200, 100);
            animateWriteFailure();
            animateProgress(currentIndex, codeLength);
            rfid.PICC_HaltA();
            rfid.PCD_StopCrypto1();
          }
        }
      }
    }
  } else {
    if (cardCurrentlyPresent) {
      cardCurrentlyPresent = false;
      lastUIDSize = 0;
    }
  }
}

// -------------------- PROGRAMMING LOOP --------------------
void programmingLoop() {
  static bool buttonWasPressed = false;
  
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (!buttonWasPressed) {
      buttonWasPressed = true;
      selectedValue++;
      if (selectedValue > 9) selectedValue = 0;
      showNumber(selectedValue);
      tone(BUZZER_PIN, 1000, 50);
      delay(200);
    }
  } else {
    buttonWasPressed = false;
  }
  
  handleReader();
}

// -------------------- PUZZLE LOOP --------------------
void puzzleLoop() {
  handleReader();

  if (currentIndex >= codeLength) {
    bool correct = true;
    
    for (int i = 0; i < codeLength; i++) {
      if (enteredValues[i] != correctCode[i]) correct = false;
    }
    
    if (correct) {
      animateSuccess();
      triggerRelay();
    } else {
      animateFailure();
    }

    currentIndex = 0;
    clearStrip();
  }
}

// -------------------- SET COMBO LOOP --------------------
void setComboLoop() {
  handleReader();

  if (currentIndex >= codeLength) {
    saveComboToEEPROM();
    
    int melody[] = {523, 587, 659, 784, 880, 988, 1047};
    int noteIndex = 0;
    
    for (int j = 0; j < 2; j++) {
      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CHSV((i * 256 / NUM_LEDS) + (j * 50), 255, 255);
        FastLED.show();
        
        if (noteIndex < 7) {
          tone(BUZZER_PIN, melody[noteIndex], 150);
          noteIndex++;
        }
        
        delay(50);
      }
    }
    
    tone(BUZZER_PIN, 1047, 400);
    delay(400);
    noTone(BUZZER_PIN);
    
    clearStrip();
    
    currentIndex = 0;
    currentMode = PUZZLE;
    animateModeChange(PUZZLE);
  }
}

// -------------------- EEPROM FUNCTIONS --------------------
void saveComboToEEPROM() {
  EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
  EEPROM.write(EEPROM_LENGTH_ADDR, codeLength);
  for (int i = 0; i < codeLength; i++) {
    EEPROM.write(EEPROM_CODE_ADDR + i, correctCode[i]);
  }
}

void loadComboFromEEPROM() {
  if (EEPROM.read(EEPROM_MAGIC_ADDR) == EEPROM_MAGIC_VALUE) {
    byte savedLength = EEPROM.read(EEPROM_LENGTH_ADDR);
    if (savedLength > 0 && savedLength <= MAX_CARDS) {
      codeLength = savedLength;
      for (int i = 0; i < codeLength; i++) {
        correctCode[i] = EEPROM.read(EEPROM_CODE_ADDR + i);
      }
    }
  }
}

// ==================== LED STRIP ANIMATIONS ====================

// Clear all LEDs
void clearStrip() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

// Display a number (0-9) using lit LEDs
void showNumber(int number) {
  clearStrip();
  
  // Clamp number to 0-9
  if (number < 0) number = 0;
  if (number > 9) number = 9;
  
  // Light up 'number' LEDs in a gradient
  for (int i = 0; i < number; i++) {
    // Create a blue to cyan gradient
    leds[i] = CHSV(160 - (i * 10), 255, 255);
  }
  FastLED.show();
}

// Animate card read - EQ style rush from first to last LED, then show progress bar
void animateCardRead(int position, int total) {
  // Rush animation - light travels from first to last LED
  for (int i = 0; i < NUM_LEDS; i++) {
    clearStrip();
    leds[i] = CRGB::Cyan;
    if (i > 0) leds[i-1] = CRGB(0, 50, 50); // Dim trail
    FastLED.show();
    delay(30);
  }
  
  // Flash all white
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
  delay(80);
  
  // Don't clear - showProgressBar will be called next to display the progress
}

// Show progress bar that stays lit - 2 LEDs per card entry
void showProgressBar(int current, int total) {
  clearStrip();
  
  // Calculate how many LED pairs to light (2 LEDs per card)
  int ledsPerCard = 2;
  int ledsToLight = current * ledsPerCard;
  
  // Make sure we don't exceed the strip
  if (ledsToLight > NUM_LEDS) ledsToLight = NUM_LEDS;
  
  // Light the LEDs in green to show progress
  for (int i = 0; i < ledsToLight; i++) {
    leds[i] = CRGB::Green;
  }
  
  FastLED.show();
}

// Success animation - rainbow explosion with integrated melody
void animateSuccess() {
  // Victory melody notes and timing integrated with animation
  int melody[] = {523, 587, 659, 784, 880, 988, 1047};
  int noteIndex = 0;
  unsigned long lastNoteTime = 0;
  int noteDuration = 150;
  
  // Rainbow chase 3 times with music
  for (int cycle = 0; cycle < 3; cycle++) {
    for (int hue = 0; hue < 256; hue += 5) {
      fill_rainbow(leds, NUM_LEDS, hue, 7);
      FastLED.show();
      
      // Play notes during animation
      if (millis() - lastNoteTime > noteDuration && noteIndex < 7) {
        tone(BUZZER_PIN, melody[noteIndex], noteDuration);
        noteIndex++;
        lastNoteTime = millis();
      }
      
      delay(10);
    }
  }
  
  // Sparkle effect with final ascending notes
  for (int i = 0; i < 30; i++) {
    int pos = random(NUM_LEDS);
    leds[pos] = CHSV(random(256), 255, 255);
    FastLED.show();
    delay(50);
    leds[pos] = CRGB::Black;
  }
  
  // Final flash with triumph chord
  fill_solid(leds, NUM_LEDS, CRGB::Gold);
  tone(BUZZER_PIN, 1047, 600);
  FastLED.show();
  delay(500);
  clearStrip();
  noTone(BUZZER_PIN);
}

// Failure animation - red strobe with descending tones
void animateFailure() {
  // Descending failure melody
  int melody[] = {392, 349, 311, 262};
  
  // Red strobe effect with descending tones
  for (int i = 0; i < 4; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    tone(BUZZER_PIN, melody[i], 200);
    FastLED.show();
    delay(200);
    clearStrip();
    delay(100);
  }
  
  // One final strobe
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  delay(100);
  clearStrip();
  
  // Fade out red
  for (int brightness = 255; brightness >= 0; brightness -= 5) {
    fill_solid(leds, NUM_LEDS, CRGB(brightness, 0, 0));
    FastLED.show();
    delay(10);
  }
  clearStrip();
  noTone(BUZZER_PIN);
}

// Write success - green rush with ascending tones
void animateWriteSuccess() {
  // Green rush from start to end with ascending tones
  int tones[] = {800, 1000, 1200};
  int toneIndex = 0;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    clearStrip();
    leds[i] = CRGB::Green;
    if (i > 0) leds[i-1] = CRGB(0, 128, 0);
    FastLED.show();
    
    // Play ascending tones at intervals
    if (i % 3 == 0 && toneIndex < 3) {
      tone(BUZZER_PIN, tones[toneIndex], 100);
      toneIndex++;
    }
    
    delay(40);
  }
  
  // All green flash
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  delay(300);
  clearStrip();
  noTone(BUZZER_PIN);
}

// Write failure - red strobe with harsh buzzer
void animateWriteFailure() {
  for (int i = 0; i < 6; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    tone(BUZZER_PIN, 200, 80);
    FastLED.show();
    delay(80);
    clearStrip();
    delay(80);
  }
  noTone(BUZZER_PIN);
}

// Mode change animation
void animateModeChange(Mode newMode) {
  CRGB modeColor;
  
  switch(newMode) {
    case PUZZLE:
      modeColor = CRGB::Blue;
      break;
    case PROGRAM:
      modeColor = CRGB::Purple;
      break;
    case SET_COMBO:
      modeColor = CRGB::Orange;
      break;
  }
  
  // Linear wipe effect
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = modeColor;
    FastLED.show();
    delay(40);
  }
  
  delay(200);
  
  // Fade out
  for (int brightness = 255; brightness >= 0; brightness -= 10) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = modeColor;
      leds[i].fadeToBlackBy(255 - brightness);
    }
    FastLED.show();
    delay(10);
  }
  
  clearStrip();
}

// Progress indicator for set combo mode
void animateProgress(int current, int total) {
  clearStrip();
  
  int ledsToFill = map(current, 0, total, 0, NUM_LEDS);
  
  // Fill progress bar with orange
  for (int i = 0; i < ledsToFill; i++) {
    leds[i] = CRGB::Orange;
  }
  
  // Add a bright leading LED
  if (ledsToFill < NUM_LEDS) {
    leds[ledsToFill] = CRGB::Yellow;
  }
  
  FastLED.show();
  delay(400);
}

// Rainbow cycle - startup animation
void rainbowCycle(int wait) {
  for (int j = 0; j < 256; j++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(((i * 256 / NUM_LEDS) + j) & 255, 255, 255);
    }
    FastLED.show();
    delay(wait);
  }
}

// ==================== BUZZER SOUNDS ====================

// Card read beep - simple confirmation tone
void beepCardRead() {
  tone(BUZZER_PIN, 1500, 100);
}

// Success melody - triumphant tune (BLOCKING - for reference)
void beepSuccess() {
  // Victory fanfare
  int melody[] = {523, 587, 659, 784, 880, 988, 1047};
  int durations[] = {150, 150, 150, 150, 150, 150, 400};
  
  for (int i = 0; i < 7; i++) {
    tone(BUZZER_PIN, melody[i], durations[i]);
    delay(durations[i] + 50);
  }
  
  // Final chord simulation
  tone(BUZZER_PIN, 1047, 600);
  delay(600);
  noTone(BUZZER_PIN);
}

// Failure tune - descending sad notes (BLOCKING - for reference)
void beepFailure() {
  // Descending failure sound
  int melody[] = {392, 349, 311, 262};
  int durations[] = {200, 200, 200, 400};
  
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], durations[i]);
    delay(durations[i] + 50);
  }
  noTone(BUZZER_PIN);
}

// NON-BLOCKING success - starts melody, continues in background
void beepSuccessNonBlocking() {
  // Start the first note of victory fanfare
  // The melody will play as tones expire naturally
  tone(BUZZER_PIN, 523, 150);  // C
}

// NON-BLOCKING failure - starts descending tone
void beepFailureNonBlocking() {
  // Start first note of failure sound
  tone(BUZZER_PIN, 392, 200);  // G
}

// Mode change beep - two-tone
void beepModeChange() {
  tone(BUZZER_PIN, 1000, 100);
  delay(150);
  tone(BUZZER_PIN, 1200, 100);
  delay(150);
  noTone(BUZZER_PIN);
}

// Write success - ascending positive notes
void beepWriteSuccess() {
  tone(BUZZER_PIN, 800, 100);
  delay(120);
  tone(BUZZER_PIN, 1000, 100);
  delay(120);
  tone(BUZZER_PIN, 1200, 150);
  delay(180);
  noTone(BUZZER_PIN);
}

// Write failure - harsh buzzer
void beepWriteFailure() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 200, 100);
    delay(150);
  }
  noTone(BUZZER_PIN);
}

// ==================== RELAY CONTROL ====================

void triggerRelay() {
  digitalWrite(RELAY_PIN, HIGH);
  delay(3000);
  digitalWrite(RELAY_PIN, LOW);
}

void releaseRelay() {
  digitalWrite(RELAY_PIN, LOW);
}
