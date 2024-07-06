// Include necessary libraries
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <SPI.h>             // f.k. for Arduino-1.5.2
#define USE_SDFAT
#include <SdFat.h>           // Use the SdFat library
SdFatSoftSpi<12, 11, 13> SD; //Bit-Bang on the Shield pins
#include <FastLED.h>

// LED Configurations
#define LED_PIN    52
#define LED_COUNT  5
CRGB leds[LED_COUNT]; 

// Colour Indices
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// ALL Touch panels and wiring is DIFFERENT
// copy-paste results from TouchScreen_Calibr_native.ino

// Define pin configurations
#define XP 6
#define XM A2
#define YP A1
#define YM 7

// Touchscreen calibration values
#define TS_LEFT 185
#define TS_RT 865
#define TS_TOP 948
#define TS_BOT 209

// Touchscreen pressure thresholds
#define MINPRESSURE 200
#define MAXPRESSURE 1000

// Maximum number of attempts
#define MAX_TRIES 6

// Maximum word length
#define MAX_WORD_LENGTH 5

// Define Chip Select 
#define SD_CS     10

// Define pin configurations for physical buttons
const int plusBtnPin = 49;
const int minusBtnPin = 43;
const int nextBtnPin = 31;
const int prevBtnPin = 37;
const int okBtnPin = 25;

// Button state variables
bool plusBtnState = false;
bool minusBtnState = false;
bool nextBtnState = false;
bool prevBtnState = false;
bool okBtnState = false;

// TFT and touchscreen objects
MCUFRIEND_kbv tft;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// Button objects
Adafruit_GFX_Button plus_btn, minus_btn, next_btn, prev_btn, ok_btn;

// Arrays to store letters and attempts
char letterBoxes[5] = {'A', 'A', 'A', 'A', 'A'}; // Initialize letter variables for each box with 'A'
char enteredLetters[5] = {'@', '@', '@', '@', '@'}; // Initialize entered letters array
char enteredLettersHistory[MAX_TRIES][5]; // Define an array to store entered letters for each try
int currentBoxIndex = 0; // Define the index of the current box
int try_num = 0; // Define a variable to keep track of the number of tries
int pixel_x, pixel_y;     //Touch_getXY() updates global vars
bool okButtonActive = true; // Define a boolean variable to track whether the OK button should be active
char predefinedAnswer[MAX_WORD_LENGTH]; // Answer defined as the "WORD OF THE DAY" 
File dictionaryFile; // File object to hold the English dictionary

// Function prototypes
bool isValidWord(char enteredLetters[]);
void registerTry(char enteredLetters[]);
void displayEnteredLetters();
bool check(char* in, char* solution, uint8_t len, uint8_t* colorout);
bool isDuplicateTry(char enteredLetters[]);
bool Touch_getXY(void);
void printletter(char txt, int x, int y, int font_c, int bg_c);
void printnum(int txt, int x, int y, int font_c, int bg_c);
void printtry(char txt, int x, int y, int font_c, int bg_c);
void printEnteredLettersHistory();
void plusBtnPressed();
void minusBtnPressed();
void nextBtnPressed();
void prevBtnPressed();
void okBtnPressed();
void updateLEDs();

void setup(void)
{
    Serial.begin(9600); // Initialize serial communication
    uint16_t ID = tft.readID();
    Serial.print("TFT ID = 0x");
    Serial.println(ID, HEX);
    if (ID == 0x0D3D3) ID = 0x9481; // write-only shield
    tft.begin(ID);
    tft.setRotation(0);            //PORTRAIT
    tft.fillScreen(MAGENTA);
    
    // Initialize all letter boxes as empty rectangles
    for (int i = 0; i < 5; i++) {
        tft.fillRoundRect(30 + (i * 40), 15, 30, 40, 3, GREEN);
    }
    
    // Initialize all try boxes as empty rectangles
    for (int i = 0; i < 6; i++) {
        tft.fillRoundRect(40,50 + ((i+1) * 29), 170, 20, 3, WHITE); // Adjusted for vertical spacing
        printnum(i+1,40,55 + ((i+1) * 29),BLACK,RED);
    }
  
  // Set up physical buttons
    pinMode(plusBtnPin, INPUT_PULLUP); // Set the plus button pin as input with internal pull-up resistor
    pinMode(minusBtnPin, INPUT_PULLUP); // Set the minus button pin as input with internal pull-up resistor
    pinMode(nextBtnPin, INPUT_PULLUP); // Set the next button pin as input with internal pull-up resistor
    pinMode(prevBtnPin, INPUT_PULLUP); // Set the previous button pin as input with internal pull-up resistor
    pinMode(okBtnPin, INPUT_PULLUP); // Set the OK button pin as input with internal pull-up resistor
    
  // Initialize buttons
  plus_btn.initButton(&tft,  30, 280, 40, 40, BLUE, YELLOW, BLACK, "+", 3);
  minus_btn.initButton(&tft, 75, 280, 40, 40, BLUE, YELLOW, BLACK, "-", 3);
  next_btn.initButton(&tft,  165, 280, 40, 40, BLUE, YELLOW, BLACK, ">", 3);
  prev_btn.initButton(&tft, 120, 280, 40, 40, BLUE, YELLOW, BLACK, "<", 3);
  ok_btn.initButton(&tft, 210, 280, 40, 40, BLUE, YELLOW, BLACK, "OK", 2.75);
  
  // Draw buttons
  plus_btn.drawButton(false);
  minus_btn.drawButton(false);
  next_btn.drawButton(false);
  prev_btn.drawButton(false);
  ok_btn.drawButton(false);

  if (SD.begin(SD_CS)) {
      Serial.println("SD card is connected and initialized successfully.");
  } else {
      Serial.println("Failed to initialize SD card. Check the connections.");
  }

  // Generate a random seed using millis()
  randomSeed(analogRead(0)); // Use an unconnected analog pin for more randomness

  // Attempt to open the dictionary file
  dictionaryFile = SD.open("english.txt");
  if (!dictionaryFile) {
      Serial.println("Failed to open dictionary file.");
      while(true); // Stop here if unable to open the file
  }
  Serial.println("Dictionary file opened successfully.");
  
  // Generate a random index to select a random word from the dictionary file
  uint32_t fileSize = dictionaryFile.size();
  uint32_t randomIndex = random(fileSize);

  // Seek to the random position in the file
  dictionaryFile.seek(randomIndex);

  // Find the beginning of the current line
  while (dictionaryFile.available() && dictionaryFile.read() != '\n');

  // Read the next word from the file into the predefinedAnswer array
  dictionaryFile.readBytesUntil('\n', predefinedAnswer, MAX_WORD_LENGTH);
  predefinedAnswer[MAX_WORD_LENGTH] = '\0'; // Null-terminate the string

  // Print the randomly selected word
  Serial.print("Randomly selected word: ");
  Serial.println(predefinedAnswer);
  
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_COUNT);
  FastLED.clear(); // Initialize all LEDs to 'off'
  FastLED.show();
}

void loop(void)
{
    // Read the state of each button
    plusBtnState = digitalRead(plusBtnPin) == LOW; // Assuming LOW means the button is pressed
    minusBtnState = digitalRead(minusBtnPin) == LOW;
    nextBtnState = digitalRead(nextBtnPin) == LOW;
    prevBtnState = digitalRead(prevBtnPin) == LOW;
    okBtnState = digitalRead(okBtnPin) == LOW;
    
  // Check for button presses and trigger actions accordingly
  if (plusBtnState) {
    // Plus button pressed, perform corresponding action
    plusBtnPressed();
  }
  if (minusBtnState) {
    // Minus button pressed, perform corresponding action
    minusBtnPressed();
  }
  if (nextBtnState) {
    // Next button pressed, perform corresponding action
    nextBtnPressed();
  }
  if (prevBtnState) {
    // Previous button pressed, perform corresponding action
    prevBtnPressed();
  }
  if (okBtnState) {
    // OK button pressed, perform corresponding action
    okBtnPressed();
  }
    // Button presses handling
    bool down = Touch_getXY();
    plus_btn.press(down && plus_btn.contains(pixel_x, pixel_y));
    minus_btn.press(down && minus_btn.contains(pixel_x, pixel_y));
    next_btn.press(down && next_btn.contains(pixel_x, pixel_y));
    prev_btn.press(down && prev_btn.contains(pixel_x, pixel_y));
    ok_btn.press(down && ok_btn.contains(pixel_x, pixel_y));

    // Button releases handling
    if (plus_btn.justReleased())
        plus_btn.drawButton();
    if (minus_btn.justReleased())
        minus_btn.drawButton();
    
    if (next_btn.justReleased()) {
        next_btn.drawButton();
        currentBoxIndex=(currentBoxIndex+1)%5; // Move to the next box
        printletter(letterBoxes[currentBoxIndex], 35 + (currentBoxIndex * 40), 20, RED, GREEN);
        delay(150);
    }
    if (prev_btn.justReleased()) {
        prev_btn.drawButton();
        currentBoxIndex=(currentBoxIndex+4)%5; // Move to the previous box
        printletter(letterBoxes[currentBoxIndex], 35 + (currentBoxIndex * 40), 20, RED, GREEN);
        delay(150);
    }

    // Highlight the selected box
    for (int i = 0; i < 5; i++) {
        if (i == currentBoxIndex) {
            tft.fillRect(30 + i * 40, 58, 30, 3, BLUE); // Draw the Blue line under the selected box
        } 
        else {
            tft.fillRect(30 + i * 40, 58, 30, 3, WHITE); // Draw the White line under other boxes
        }
    }
    
    // Plus button pressed
    if (plus_btn.justPressed()) {
        plus_btn.drawButton(true);
        letterBoxes[currentBoxIndex] = (letterBoxes[currentBoxIndex] == 'Z') ? 'A' : letterBoxes[currentBoxIndex] + 1;
        printletter(letterBoxes[currentBoxIndex], 35 + (currentBoxIndex * 40), 20, RED, GREEN); // Print incremented letter
        delay(200);
    }

    // Minus button pressed    
    if (minus_btn.justPressed()) {
        minus_btn.drawButton(true);
        letterBoxes[currentBoxIndex] = (letterBoxes[currentBoxIndex] == 'A') ? 'Z' : letterBoxes[currentBoxIndex] - 1;
        printletter(letterBoxes[currentBoxIndex], 35 + (currentBoxIndex * 40), 20, RED, GREEN); // Print decremented letter
        delay(200);
    }

    // Check if the OK button can register a new attempt    
    if (ok_btn.justReleased() && okButtonActive){
      ok_btn.drawButton();
      if (!isDuplicateTry(letterBoxes)) {
          // Register the try if it's not a duplicate
          registerTry(letterBoxes);
          // Print entered letters history after registering each try
          printEnteredLettersHistory();
      }
      // Check if maximum number of tries is reached
      if (try_num >= MAX_TRIES) {
        // Disable the OK button to prevent further attempts
        okButtonActive = false;       
      }
      // Update LED colors based on the try result
      updateLEDs();
      delay(500); // Delay for readability
    }
  
}

// Function to check if the entered letters form a predefined word
bool isValidWord(char enteredLetters[]) {
    if (!dictionaryFile) {
        Serial.println("Failed to open dictionary file.");
        return false; // Unable to open file, treat as invalid word
    }
    // Convert the entered letters array to a string
    char enteredWord[MAX_WORD_LENGTH + 1]; // Include space for null terminator
    strncpy(enteredWord, enteredLetters, MAX_WORD_LENGTH);
    enteredWord[MAX_WORD_LENGTH] = '\0'; // Null-terminate the string

    // Reset the file pointer to the beginning of the file
    dictionaryFile.seek(0);

    // Temporary buffer to store read words
    char buffer[MAX_WORD_LENGTH + 1];

    // Iterate through each word in the dictionary file
    while (dictionaryFile.available()) {
        // Read the next word from the file
        size_t bytesRead = dictionaryFile.readBytesUntil('\n', buffer, MAX_WORD_LENGTH);
        buffer[bytesRead] = '\0'; // Null-terminate the string

        // Compare the entered word with the word from the dictionary
        if (strcasecmp(enteredWord, buffer) == 0) {
            return true; // Entered word is valid
        }
    }
    return false; // Entered word is not found in the dictionary
}

void registerTry(char enteredLetters[]) {
    // Store the entered letters for the current try if it's a valid word and the maximum number of tries is not reached
    if (try_num < MAX_TRIES && isValidWord(enteredLetters)) {
        // Store the entered letters for the current try
        for (int i = 0; i < MAX_WORD_LENGTH; i++) {
            enteredLettersHistory[try_num][i] = enteredLetters[i];
        }
        // Increment the try number
        try_num++;
        // Display entered letters for the current try
        displayEnteredLetters();

        // Check if all letters are guessed correctly (all green)
        bool allGreen = true;
        for (int i = 0; i < MAX_WORD_LENGTH; i++) {
            if (enteredLetters[i] != predefinedAnswer[i]) {
                allGreen = false;
                break;
            }
        }

        // If all letters are guessed correctly, print the number of tries and disable further attempts
        if (allGreen) {
            Serial.println("CONGRATULATIONS YOU WON !!");
            Serial.print("Number of tries to guess the word: ");
            Serial.println(try_num);
            okButtonActive = false;
            Serial.print("WORD OF THE DAY: ");
            for (int i = 0; i < MAX_WORD_LENGTH; i++) {
                Serial.print(predefinedAnswer[i]);
            }
            Serial.println();
        }
        // Print the current try on serial monitor
        Serial.print("Try ");
        Serial.print(try_num);
        Serial.print(": ");
        for (int i = 0; i < MAX_WORD_LENGTH; i++) {
            Serial.print(enteredLetters[i]);
        }
        Serial.println();
    }
    else{
        if (!isValidWord(enteredLetters)) {
            Serial.println("BETTER LUCK NEXT TIME :(");
            Serial.print("WORD OF THE DAY: ");
            for (int i = 0; i < MAX_WORD_LENGTH; i++) {
                Serial.print(predefinedAnswer[i]);
            }
            Serial.println();
        }
    else {
        Serial.println();
        Serial.println("INVALID WORD");
    }
  }
}

void displayEnteredLetters() {
    // Iterate through each try
    for (int i = 0; i < try_num; i++) {
        char userWord[MAX_WORD_LENGTH];
        for (int j = 0; j < MAX_WORD_LENGTH; j++) {
            userWord[j] = enteredLettersHistory[i][j];
        }
        uint8_t colors[MAX_WORD_LENGTH];
        bool solved = check(userWord, predefinedAnswer, MAX_WORD_LENGTH, colors);

        // Output the letters with appropriate colors
        for (int j = 0; j < MAX_WORD_LENGTH; j++) {
            switch(colors[j]) {
                case 0:
                    printtry(enteredLettersHistory[i][j], 100 + (j * 13), 80 + (i * 29), RED, WHITE);
                    break;
                case 1:
                    printtry(enteredLettersHistory[i][j], 100 + (j * 13), 80 + (i * 29), BLUE, WHITE);
                    break;
                case 2:
                    printtry(enteredLettersHistory[i][j], 100 + (j * 13), 80 + (i * 29), GREEN, WHITE);
                    break;
                default:
                    break;
            }
        }
        
        // Check if the word is solved
        if (solved) {
            // Handle win condition
            break;
        }
    }
}

bool check(char* in, char* solution, uint8_t len, uint8_t* colorout){
    bool solved = true;
    // Initialize color output array with 0 (RED) for all letters
    memset(colorout, 0, len * sizeof(uint8_t));

    // Check for letters in correct position
    for(uint8_t i = 0; i < len; i++){
        if(in[i] == solution[i]){
            colorout[i] = 2; // Set color to GREEN
        } else {
            solved = false; // Not solved if any letter is incorrect
        }
    }

    // Check for letters in wrong position
    for(uint8_t i = 0; i < len; i++){
        if(colorout[i] == 0){ // If the letter is not correctly placed
            for(uint8_t j = 0; j < len; j++){
                if(i != j && in[i] == solution[j]){
                    int8_t ctr = 0;
                    for(uint8_t k = 0; k < len; k++){
                        if(solution[k] == solution[j]) ctr++;
                        if(in[k] == solution[j] && colorout[k] != 0) ctr--;
                    }
                    if(ctr > 0) colorout[i] = 1; // Set color to YELLOW
                }
            }
        }
    }
    return solved;
}


bool isDuplicateTry(char enteredLetters[]) {
    // Compare the current entered letters with all previous sets
    for (int i = 0; i < try_num; i++) {
        bool isDuplicate = true;
        for (int j = 0; j < 5; j++) {
            if (enteredLettersHistory[i][j] != enteredLetters[j]) {
                isDuplicate = false;
                break;
            }
        }
        if (isDuplicate) {
            // Current set matches a previous set, so it's a duplicate try
            return true;
        }
    }
    // No matches found, so it's not a duplicate try or a predefined word
    return false;
}

bool Touch_getXY(void)
{
  TSPoint p = ts.getPoint();
  pinMode(YP, OUTPUT);      //restore shared pins
  pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH);   //because TFT control pins
  digitalWrite(XM, HIGH);
  bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
  if (pressed) {
      pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width()); //.kbv makes sense to me
      pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
  }
  return pressed;
}

void printletter(char txt, int x, int y, int font_c, int bg_c) {
  // Draw rounded rectangle as background
  tft.fillRoundRect(x - 5, y - 5, 30, 40, 3, bg_c);  // Adjust dimensions and radius as needed

  // Set text color and print the letter
  tft.setTextColor(font_c);
  tft.setTextSize(4);
  tft.setCursor(x, y);
  tft.print(txt);
}

void printnum(int txt, int x, int y, int font_c, int bg_c) {
  // Draw rounded rectangle as background
  tft.fillRoundRect(x - 5, y - 5, 20, 20, 3, bg_c);  // Adjust dimensions and radius as needed

  // Set text color and print the letter
  tft.setTextColor(font_c);
  tft.setTextSize(2);
  tft.setCursor(x, y);
  tft.print(txt);
}

void printtry(char txt, int x, int y, int font_c, int bg_c) {
  // Draw rounded rectangle as background
  tft.fillRoundRect(x - 5, y - 5, 0, 0, 3, bg_c);  // Adjust dimensions and radius as needed

  // Set text color and print the letter
  tft.setTextColor(font_c);
  tft.setTextSize(2.75);
  tft.setCursor(x, y);
  tft.print(txt);
}

void printEnteredLettersHistory() {
    Serial.println("Entered Letters History:");
    for (int i = 0; i < try_num; i++) {
        Serial.print("Try ");
        Serial.print(i + 1);
        Serial.print(": ");
        for (int j = 0; j < 5; j++) {
            Serial.print(enteredLettersHistory[i][j]);
            Serial.print(" ");
        }
        Serial.println();
    }
    Serial.println();
}

// Function to handle plus button press
void plusBtnPressed() {
    letterBoxes[currentBoxIndex] = (letterBoxes[currentBoxIndex] == 'Z') ? 'A' : letterBoxes[currentBoxIndex] + 1;
    printletter(letterBoxes[currentBoxIndex], 35 + (currentBoxIndex * 40), 20, RED, GREEN); // Print incremented letter
    delay(200);
}

// Function to handle minus button press
void minusBtnPressed() {
    letterBoxes[currentBoxIndex] = (letterBoxes[currentBoxIndex] == 'A') ? 'Z' : letterBoxes[currentBoxIndex] - 1;
    printletter(letterBoxes[currentBoxIndex], 35 + (currentBoxIndex * 40), 20, RED, GREEN); // Print decremented letter
    delay(200);
}

// Function to handle next button press
void nextBtnPressed() {
    currentBoxIndex = (currentBoxIndex + 1) % 5; // Move to the next box
    printletter(letterBoxes[currentBoxIndex], 35 + (currentBoxIndex * 40), 20, RED, GREEN);
    delay(150);
}

// Function to handle previous button press
void prevBtnPressed() {
    currentBoxIndex = (currentBoxIndex + 4) % 5; // Move to the previous box
    printletter(letterBoxes[currentBoxIndex], 35 + (currentBoxIndex * 40), 20, RED, GREEN);
    delay(150);
}

// Function to handle OK button press
void okBtnPressed() {
    if (!isDuplicateTry(letterBoxes)) {
        // Register the try if it's not a duplicate
        registerTry(letterBoxes);
        // Print entered letters history after registering each try
        printEnteredLettersHistory();
    }
    // Check if maximum number of tries is reached
    if (try_num >= MAX_TRIES) {
        // Disable the OK button to prevent further attempts
        okButtonActive = false;
    }
    delay(500); // Delay for readability
}

void updateLEDs() {
    for (int i = 0; i < try_num; i++) {
        char userWord[MAX_WORD_LENGTH];
        for (int j = 0; j < MAX_WORD_LENGTH; j++) {
            userWord[j] = enteredLettersHistory[i][j];
        }
        uint8_t colors[MAX_WORD_LENGTH];
        bool solved = check(userWord, predefinedAnswer, MAX_WORD_LENGTH, colors);

        // Set LED colors based on the try result
        for (int j = 0; j < LED_COUNT; j++) {
            if (j < MAX_WORD_LENGTH) {
                if (colors[j] == 0) {
                    leds[j] = CRGB::Red;
                } else if (colors[j] == 1) {
                    leds[j] = CRGB::Blue;
                } else {
                    leds[j] = CRGB::Green;
                }
            } else {
                leds[j] = CRGB::Black; // Turn off extra LEDs
            }
        }
        
        // Show LED colors
        FastLED.show();
        delay(500); // Adjust delay as needed
    }
  }

