#include <SPI.h>
#include <MFRC522.h>

// Pin definitions for RFID
#define SS_PIN 10  
#define RST_PIN 9  
#define RFID_LED_PIN 50
#define BUZZER_PIN 35

// Pins for LEDs controlled by RFID
#define LED1 3
#define LED2 12
#define LED3 13

// Pins for the wire component
int outputs[] = {32, 34, 36};
int inputs[] = {38, 40, 42};

// Simon Says Pins
const int simonLedPins[] = {8, 22, 24, 26};
const int simonButtonPins[] = {4, 5, 6, 7};
const int simonLedSuccessPin = 33;
int sequence[10];  
int sequenceLength = 1;  
int currentLevel = 0;
bool simonSuccess = true;

// Mario Theme Pin
#define melodyPin BUZZER_PIN

// Wires success LED pin
#define WIRES_SUCCESS_LED_PIN 52

// Timer Variables
unsigned long timerStart = 0;
unsigned long buzzerInterval = 60000;  // 1 minute
unsigned long gameTime = 180000;       // 3 minutes
unsigned long previousBuzzerTime = 0;
bool gameOver = false;

// RFID Reader
MFRC522 rfid(SS_PIN, RST_PIN);
boolean alright = 1;            

// Simon Says timing and gameplay variables
unsigned long startTime;
const unsigned long waitTime = 10000;  // 10 seconds for player input
bool inputCorrect = true;
const int maxLevels = 3;

// Mario Theme Melody
int melody[] = {
  NOTE_E7, NOTE_E7, 0, NOTE_E7, 0, NOTE_C7, NOTE_E7, 0,
  NOTE_G7, 0, 0, 0, NOTE_G6, 0, 0, 0,
  NOTE_C7, 0, 0, NOTE_G6, 0, 0, NOTE_E6, 0,
  0, NOTE_A6, 0, NOTE_B6, 0, NOTE_AS6, NOTE_A6, 0,
  NOTE_G6, NOTE_E7, NOTE_G7, NOTE_A7, 0, NOTE_F7, NOTE_G7,
  0, NOTE_E7, 0, NOTE_C7, NOTE_D7, NOTE_B6, 0, 0
};
int tempo[] = {
  12, 12, 12, 12, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12
};

// Setup function
void setup() {
  // Setup for Serial, SPI, and RFID
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  pinMode(RFID_LED_PIN, OUTPUT);

  // Setup LEDs for RFID task
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  // Setup for wire task
  for (int i = 0; i < (sizeof(inputs) / sizeof(inputs[0])); i++) {
    pinMode(inputs[i], INPUT_PULLUP);
  }
  for (int i = 0; i < (sizeof(outputs) / sizeof(outputs[0])); i++) {
    pinMode(outputs[i], OUTPUT);
    digitalWrite(outputs[i], HIGH);
  }

  // Setup Simon Says LED and buttons
  for (int i = 0; i < 4; i++) {
    pinMode(simonLedPins[i], OUTPUT);
    pinMode(simonButtonPins[i], INPUT_PULLUP);
  }
  pinMode(simonLedSuccessPin, OUTPUT);

  // Setup wires success LED
  pinMode(WIRES_SUCCESS_LED_PIN, OUTPUT);

  // Setup buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);

  // Start the timer
  timerStart = millis();
  previousBuzzerTime = millis();

  // Announce game start
  tone(BUZZER_PIN, 1000, 100);

  // Generate initial Simon Says sequence
  generateSequence(sequenceLength);
  playSequence(sequenceLength);
}

// Main loop function
void loop() {
  unsigned long currentTime = millis();

  // Handle RFID task
  handleRFID();

  // Handle wire task
  handleWires();

  // Handle Simon Says task
  handleSimonSays();

  // Handle timer and buzzer
  handleTimer(currentTime);
}

// Function to handle RFID task
void handleRFID() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.print("UID tag: ");
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    digitalWrite(RFID_LED_PIN, HIGH);
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    delay(1000);
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    delay(1000);
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    rfid.PICC_HaltA();
  }
}

// Function to handle wire task
void handleWires() {
  int buttonState;
  for (int j = 0; j < (sizeof(inputs) / sizeof(inputs[0])); j++) {
    for (int i = 0; i < (sizeof(outputs) / sizeof(outputs[0])); i++) {
      int currentCheck = outputs[i];
      digitalWrite(currentCheck, LOW);
      delay(100);
      buttonState = digitalRead(j);
      if (buttonState == LOW) {
        if (buttonState == LOW && i != j) {
          alright = 0;
        }
      }
      digitalWrite(currentCheck, HIGH);
    }
  }
  if (alright) {
    digitalWrite(WIRES_SUCCESS_LED_PIN, HIGH);
  } else {
    digitalWrite(WIRES_SUCCESS_LED_PIN, LOW);
  }
}

// Function to handle Simon Says task
void handleSimonSays() {
  inputCorrect = true;
  for (int i = 0; i < sequenceLength; i++) {
    if (!waitForPlayerInput(sequence[i])) {
      inputCorrect = false;
      break;
    }
  }
  if (inputCorrect) {
    currentLevel++;
    if (currentLevel >= maxLevels) {
      digitalWrite(simonLedSuccessPin, HIGH);
      playMarioTheme();  // Play Mario Theme on success
    } else {
      sequenceLength++;
      generateSequence(sequenceLength);
      playSequence(sequenceLength);
    }
  } else {
    playSequence(sequenceLength);  // Replay the sequence if incorrect
  }
}

// Function to handle the game timer and buzzer
void handleTimer(unsigned long currentTime) {
  if (currentTime - previousBuzzerTime >= buzzerInterval) {
    tone(BUZZER_PIN, 1000, 100);
    previousBuzzerTime = currentTime;
  }
  if (currentTime - timerStart >= gameTime && !gameOver) {
    gameOver = true;
    for (int i = 0; i < 10; i++) {
      tone(BUZZER_PIN, 2000, 1000);
      delay(1000);
    }
  }
}

// Functions for Simon Says gameplay
void generateSequence(int length) {
  for (int i = 0; i < length; i++) {
    sequence[i] = random(0, 4);
  }
}
void playSequence(int length) {
  for (int i = 0; i < length; i++) {
    digitalWrite(simonLedPins[sequence[i]], HIGH);
    delay(500);
    digitalWrite(simonLedPins[sequence[i]], LOW);
    delay(300);
  }
}
bool waitForPlayerInput(int expectedButton) {
  unsigned long startTime = millis();
  bool buttonPressed = false;
  while (millis() - startTime < waitTime) {
    if (digitalRead(simonButtonPins[expectedButton]) == LOW) {
      delay(100);
      while (digitalRead(simonButtonPins[expectedButton]) == LOW);
      buttonPressed = true;
      break;
    }
  }
  return buttonPressed;
}

// Function to play Mario Theme
void playMarioTheme() {
  int size = sizeof(melody) / sizeof(int);
  for (int thisNote = 0; thisNote < size; thisNote++) {
    int noteDuration = 1000 / tempo[thisNote];
    buzz(melodyPin, melody[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    buzz(melodyPin, 0, noteDuration);
  }
}

// Function to play a specific note
void buzz(int targetPin, long frequency, long length) {
  long delayValue = 1000000 / frequency / 2;
  long numCycles = frequency * length / 1000;
  for (long i = 0; i < numCycles; i++) {
    digitalWrite(targetPin, HIGH);
    delayMicroseconds(delayValue);
    digitalWrite(targetPin, LOW);
    delayMicroseconds(delayValue);
  }
}
