//-- OLED stuff
  // Test for minimum program size.
  #define RTN_CHECK 1

  #include <Wire.h>
  #include "SSD1306Ascii.h"
  #include "SSD1306AsciiWire.h"

  // 0X3C+SA0 - 0x3C or 0x3D
  #define I2C_ADDRESS 0x3C

  // Define proper RST_PIN if required.
  #define RST_PIN -1

  SSD1306AsciiWire oled;
//-- end OLED stuff

// ATtiny85 pin definitions
// #define LED0 0
// #define LED1 1
// #define LED2 2
// #define LED3 3
// #define LED4 4
// ATtiny84 pin definitions
// #define LED0 0
// #define LED1 1
// #define LED2 21
// #define LED3 3
// #define LED4 7
#define LED0 9   // not gate red & gate
#define LED1 10  // not gate blue
#define LED2 PA2
#define LED3 PA1
#define LED4 PA0

// Fast mode stuff -------------------------------------
  #define FADEMINFAST 20  // Alternative "fast-mode" fade rate. Should be fast!
  #define FADEMAXFAST 30
  #define FADEFALSEFAST 15  // Fast mode-dice rolls. Should make more bright :D
  #define FADETRUEFAST 30

  #define FASTMODECYCLETRIGGERMIN 3   //  Used in a random number generator. Minimum fade cycles until next fast mode
  #define FASTMODECYCLETRIGGERMAX 20  // Maximum fade cycles until next random fast mode

//-- Button debounce declarations
  const int buttonPin = 8;
  short int latchState = -1;
  int ledState = HIGH;
  int buttonState;
  int lastButtonState = LOW;
  unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 50;
//

//-- Fast mode declarations
  // After how many fade cycles will the first fast mode trigger?
  // int fastModeCycleCountTrigger = random(3, 5);  // Make sure fast mode triggers quickly the first time after opening the box.
  byte fastModeCycleCountTrigger = random(FASTMODECYCLETRIGGERMIN, FASTMODECYCLETRIGGERMAX);
  boolean fastMode = false;  // Are we in fast mode?

  // Variables for fade cycle counter. Allows an event to trigger after x fade cycles
  byte fastModeFadeCycleTimer = 0;

//-- Celebration variable declarations
  boolean waitingToCelebrate = false;         // Are we waiting to celebrate?
  boolean celebrating = false;                // Are we celebrating right now?
  boolean celebrationRampMaxReached = false;  // Flag to identify when PWM duty cycle = 100% on celebration brightness ramp-up
  byte celebrationRoll = 0;                   // Dice roll
  // int celebrationTrigger = random(50, 500); // Holds the magic number to match for celebrations.
  byte celebrationTrigger = 40;  // Holds the magic number to match for celebrations.

  byte celebrationPeakDirection = 0;
  byte celebrationTimerPhase2 = 0;
  byte celebrationLimit = 200;
  boolean celebrationPeakDirectionMax = false;

// Fade cycle parameter values
  const byte numberOfLEDs = 5;  // Number of LED GPIO pins

  // Keep a copy of the "default" fade probabilities and speeds.
  const byte dynamicFadeMinDEFAULT[numberOfLEDs] = { 70, 50, 40, 30, 50 };
  const byte dynamicFadeMaxDEFAULT[numberOfLEDs] = { 200, 255, 55, 40, 55 };  // But start out with the default rate so LED0 has a better chance to get coffee with the rest (the first time)
  byte dynamicFadeMin[numberOfLEDs];                                          // Initialize empty array to avoid duplication. Populated using memcpy in setup();
  byte dynamicFadeMax[numberOfLEDs];

  const byte dynamicFadeTrueDEFAULT[numberOfLEDs] = { 30, 30, 30, 30, 30 };  // Values determining success probability of roll to enable LED next round.
  const byte dynamicFadeFalseDEFAULT[numberOfLEDs] = { 8, 7, 25, 10, 25 };
  byte dynamicFadeTrue[numberOfLEDs];
  byte dynamicFadeFalse[numberOfLEDs];

  const byte dynamicLimitMinDEFAULT[numberOfLEDs] = { 230, 230, 125, 5, 125 };  // Available brightness range on each roll
  const byte dynamicLimitMaxDEFAULT[numberOfLEDs] = { 255, 255, 255, 100, 255 };
  byte dynamicLimitMin[numberOfLEDs];
  byte dynamicLimitMax[numberOfLEDs];

// LED PWM state monitoring
  byte fadeTimer[numberOfLEDs] = { random(dynamicFadeMinDEFAULT[4], dynamicFadeMaxDEFAULT[4]),
                                  random(dynamicFadeMinDEFAULT[4], dynamicFadeMaxDEFAULT[4]),
                                  random(dynamicFadeMinDEFAULT[2], dynamicFadeMaxDEFAULT[2]),
                                  random(dynamicFadeMinDEFAULT[3], dynamicFadeMaxDEFAULT[3]),
                                  random(dynamicFadeMinDEFAULT[4], dynamicFadeMaxDEFAULT[4]) };  // Value of variable for each LED n
  byte onTime[numberOfLEDs] = { 0, 0, 0, 0, 0 };
  byte onCounter[numberOfLEDs] = { 0, 0, 0, 0, 0 };
  byte fadeCounter[numberOfLEDs] = { 0, 0, 0, 0, 0 };
  char dir[numberOfLEDs] = { 1, 1, 1, 1, 1 };
  byte limit[numberOfLEDs] = { 255, 255, 255, 255, 255 };
  //boolean enable[numberOfLEDs] = { random(1), random(1), random(1), random(1), random(1) };
  boolean enable[numberOfLEDs] = { true, true, true, true, true };

// Event state machine
  byte d20 = 0;           // Event die.
  boolean event = false;  // Are we in an event right now?
                        //

// Main loop timing
  // Wait in microseconds before allowing loop() to process again
  long delayTime = 50;
  long startTime = 0;

// OLED I2C Looper stuff
  // Loop timing
  // long i2cStartTime = 0;  // for i2c debugging
  // int i2cPWMCOunt = 0;

  // Math
  // int i2c5secondAverage = 0;
  // int i2c5secondAverageHolder = 0;
  // byte i2c5secondAverageCounter = 0;

  // For loop thing
  byte i;
//

// Setup
void setup() {
  randomSeed(analogRead(3));
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  startTime = micros();

  // Populate dynamic fade arrays
  memcpy(dynamicFadeTrue, dynamicFadeTrueDEFAULT, sizeof dynamicFadeTrueDEFAULT);
  memcpy(dynamicFadeFalse, dynamicFadeFalseDEFAULT, sizeof dynamicFadeFalseDEFAULT);
  memcpy(dynamicFadeMin, dynamicFadeMinDEFAULT, sizeof dynamicFadeMinDEFAULT);
  memcpy(dynamicFadeMax, dynamicFadeMaxDEFAULT, sizeof dynamicFadeMaxDEFAULT);
  memcpy(dynamicLimitMin, dynamicLimitMinDEFAULT, sizeof dynamicLimitMinDEFAULT);
  memcpy(dynamicLimitMax, dynamicLimitMaxDEFAULT, sizeof dynamicLimitMaxDEFAULT);
  // dynamicFadeMin[0] = dynamicFadeMin[4];  // Set LED0 and LED1's fade rate to the same as everyone else's
  // dynamicFadeMax[0] = dynamicFadeMax[4];  // ccccccat first.
  // dynamicFadeMin[1] = dynamicFadeMin[4];
  // dynamicFadeMax[1] = dynamicFadeMax[4];

  // OLED setup
  Wire.begin();
  Wire.setClock(400000L);

  // #if RST_PIN >= 0
  // oled.begin(&Adafruit128x32, I2C_ADDRESS, RST_PIN);
  // #else   // RST_PIN >= 0
  oled.begin(&Adafruit128x32, I2C_ADDRESS);
  // #endif  // RST_PIN >= 0

  // // Use Adafruit5x7, field at row 2, set1X, columns 16 through 100.
  oled.setFont(System5x7);
  oled.displayRemap(true);
  //

  // Button debounce setup
  pinMode(buttonPin, INPUT);
}

void loop() {
  long currTime = micros();

  // Button Debounce stuff
    // int reading = digitalRead(buttonPin);
    // if (reading != lastButtonState) {
    //   lastDebounceTime = millis();
    // }

    // if ((millis() - lastDebounceTime) > debounceDelay) {
    //   if (reading != buttonState) {
    //     buttonState = reading;
    //     oled.print("state: ");
    //     oled.println(buttonState);
    //   }
    //   if (buttonState = HIGH) {
    //     ledState = !ledState;
    //   }
    // }
    // // oled.print("state: ");
    // // oled.print(ledState);
    // lastButtonState = reading;
  //

  // I2C ILED stuff
    // if ((currTime - i2cStartTime) > 1000000) {  // 1s
    // i2c5secondAverageCounter++;
    // i2c5secondAverageHolder = i2c5secondAverageHolder + (currTime - i2cStartTime);

    //oled.clear();
    // oled.print(i2cPWMCOunt);
    // oled.print(" fps (");
    // oled.print(i2c5secondAverage);
    // oled.println(")");
    // i2cStartTime = currTime;
    // i2cPWMCOunt = 0;
    // if (i2c5secondAverageCounter == 5) {
    //   i2c5secondAverage = (i2c5secondAverageHolder / 5);
    //   i2c5secondAverageHolder = 0;
    //   i2c5secondAverageCounter = 0;
    // }
  // }

  if ((currTime - startTime) < delayTime) {
    return;
  }
  startTime = currTime;  // Start the timer over.

  int reading = digitalRead(buttonPin);
  oledDisplay(reading);




  //i2cPWMCOunt++;

  // Roll a die.
  //if (!event) d20 = random(20);

  // LED0 section
  if (!enable[0]) digitalWrite(LED0, LOW);
  else if (onCounter[0] > onTime[0]) digitalWrite(LED0, LOW);
  else digitalWrite(LED0, HIGH);

  onCounter[0]++;
  fadeCounter[0]++;

  if (fadeCounter[0] == fadeTimer[0]) {
    fadeCounter[0] = 0;
    onTime[0] += dir[0];

    if ((onTime[0] == limit[0]) || (onTime[0] == 0)) {
      dir[0] *= -1;
    }

    // End of a fade cycle when the LED was on.
    if ((onTime[0] == 0) && (dir[0] = 1)) {
      limit[0] = random(dynamicLimitMin[0], dynamicLimitMax[0]);    // pin-specific brightness values
      fadeTimer[0] = random(dynamicFadeMin[0], dynamicFadeMax[0]);  // pin specific dynamic-fade speed variables
      decideLEDState(0);
    }
  }

  //  LED1 section-----------------------------------------------------------------
  if (!enable[1]) digitalWrite(LED1, LOW);
  else if (onCounter[1] > onTime[1]) digitalWrite(LED1, LOW);
  else digitalWrite(LED1, HIGH);

  onCounter[1]++;
  fadeCounter[1]++;
  if (fadeCounter[1] == fadeTimer[1]) {
    fadeCounter[1] = 0;
    onTime[1] += dir[1];

    if ((onTime[1] == limit[1]) || (onTime[1] == 0)) dir[1] *= -1;

    if ((onTime[1] == 0) && (dir[1] = 1)) {
      limit[1] = random(dynamicLimitMin[1], dynamicLimitMax[1]);  // pin-specific brightness values
      fadeTimer[1] = random(dynamicFadeMin[1], dynamicFadeMax[1]);
      decideLEDState(1);
    }
  }

  //  LED2 section-----------------------------------------------------------------
  if (!enable[2]) digitalWrite(LED2, LOW);
  else if (onCounter[2] > onTime[2]) digitalWrite(LED2, LOW);
  else digitalWrite(LED2, HIGH);

  onCounter[2]++;
  fadeCounter[2]++;
  compareFadeTimerWithCounter(2);

  //  LED3 section-----------------------------------------------------------------
  if (!enable[3]) digitalWrite(LED3, LOW);
  else if (onCounter[3] > onTime[3]) digitalWrite(LED3, LOW);
  else digitalWrite(LED3, HIGH);

  onCounter[3]++;
  fadeCounter[3]++;
  compareFadeTimerWithCounter(3);

  //  LED4 section-----------------------------------------------------------------
  if (!enable[4]) digitalWrite(LED4, LOW);
  else if (onCounter[4] > onTime[4]) digitalWrite(LED4, LOW);
  else digitalWrite(LED4, HIGH);

  onCounter[4]++;
  fadeCounter[4]++;
  compareFadeTimerWithCounter(4);
}

void compareFadeTimerWithCounter(int pin) {
  if (fadeCounter[pin] < fadeTimer[pin]) {
    return;
  }

  fadeCounter[pin] = 0;  // Reset counter
  onTime[pin] += dir[pin];

  if ((onTime[pin] == limit[pin]) || (onTime[pin] == 0)) dir[pin] *= -1;  // change fade direction

  if ((onTime[pin] == 0) && (dir[pin] = 1)) {                           // Timer has run out and fade direction had reached 0 (and reversed)
    limit[pin] = random(dynamicLimitMin[pin], dynamicLimitMax[pin]);    // Brightness limit
    fadeTimer[pin] = random(dynamicFadeMin[pin], dynamicFadeMax[pin]);  // Fade cycle speed
    decideLEDState(pin);
  }
}


void decideLEDState(int pin) {
  // Decide if an LED should be enabled next round.
  enable[pin] = random(0, dynamicFadeTrue[pin] + 1) >= dynamicFadeFalse[pin];  // Roll for LED on|off state
  return;
}


void restoreDefaultFadeValues() {
  memcpy(dynamicFadeTrue, dynamicFadeTrueDEFAULT, sizeof dynamicFadeTrueDEFAULT);
  memcpy(dynamicFadeFalse, dynamicFadeFalseDEFAULT, sizeof dynamicFadeFalseDEFAULT);
  memcpy(dynamicFadeMin, dynamicFadeMinDEFAULT, sizeof dynamicFadeMinDEFAULT);
  memcpy(dynamicFadeMax, dynamicFadeMaxDEFAULT, sizeof dynamicFadeMaxDEFAULT);
  memcpy(dynamicLimitMin, dynamicLimitMinDEFAULT, sizeof dynamicLimitMinDEFAULT);
  memcpy(dynamicLimitMax, dynamicLimitMaxDEFAULT, sizeof dynamicLimitMaxDEFAULT);
}

void oledDisplay(int reading) {
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {

    if (reading != buttonState) {
      buttonState = reading;

      latchStateToggle(buttonState, latchState);


      oled.clear();
      oled.print("st: ");
      oled.print(buttonState);
      // oled.print(lastDebounceTime);

      oled.print(" lst: ");
      oled.print(lastButtonState);
      oled.print("ltch: ");
      oled.println(latchState);
    }
  }
    lastButtonState = reading;
}

void latchStateToggle(byte button, short int latch) {
      if (button != 1) {
        return;
      }
        latchState = latch*-1;   // Invert latchState
}