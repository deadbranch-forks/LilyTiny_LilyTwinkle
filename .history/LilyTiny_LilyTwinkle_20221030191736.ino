// AVRDUDE fuse settings string:
// avrdude -p t85 -c avrispmkii -B 4 -P usb -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
// By the time most users see this board, these settings will already
//  have been applied. However, when programming from a blank ATTiny, it
//  is necessary to program the fuse bits to disable the divide-by-8
//  on the clock frequency.

// ATtiny85 pin definitions
// #define LED0 0
// #define LED1 1
// #define LED2 2
// #define LED3 3
// #define LED4 4
// ATtiny84 pin definitions
// #define LED0 0
// #define LED1 1
// #define LED2 2
// #define LED3 3
// #define LED4 7
#define LED0 9   // not gate red
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

// After how many fade cycles will the first fast mode trigger?
//int fastModeCycleCountTrigger = random(3, 5);  // Make sure fast mode triggers quickly the first time after opening the box.
int fastModeCycleCountTrigger = random(FASTMODECYCLETRIGGERMIN, FASTMODECYCLETRIGGERMAX);
int fastMode = false;  // Are we in fast mode?
// End fast mode -----------------------------------------

// Variables for fade cycle counter. Allows an event to trigger after x fade cycles
int fastModeFadeCycleTimer = 0;

// Variable
boolean waitingToCelebrate = false;         // Are we waiting to celebrate?
boolean celebrating = false;                // Are we celebrating right now?
boolean celebrationRampMaxReached = false;  // Flag to identify when PWM duty cycle = 100% on celebration brightness ramp-up
int celebrationRoll = 0;                    // Dice roll
// int celebrationTrigger = random(50, 500); // Holds the magic number to match for celebrations.
int celebrationTrigger = 40;  // Holds the magic number to match for celebrations.


byte celebrationPeakDirection = 0;
int celebrationTimerPhase2 = 0;
int celebrationLimit = 200;
boolean celebrationPeakDirectionMax = false;

const int numberOfLEDs = 5;  // Number of LED GPIO pins
// Constant replacement arrays

// Keep a copy of the "default" fade probabilities and speeds.
const int dynamicFadeMinDEFAULT[numberOfLEDs] = { 70, 50, 40, 30, 50 };
const int dynamicFadeMaxDEFAULT[numberOfLEDs] = { 200, 255, 55, 40, 55 };  // But start out with the default rate so LED0 has a better chance to get coffee with the rest (the first time)
int dynamicFadeMin[numberOfLEDs];                                          // Initialize empty array to avoid duplication. Populated using memcpy in setup();
int dynamicFadeMax[numberOfLEDs];

const int dynamicFadeTrueDEFAULT[numberOfLEDs] = { 30, 30, 30, 30, 30 };  // Values determining success probability of roll to enable LED next round.
const int dynamicFadeFalseDEFAULT[numberOfLEDs] = { 8, 7, 25, 10, 25 };
int dynamicFadeTrue[numberOfLEDs];
int dynamicFadeFalse[numberOfLEDs];

const int dynamicLimitMinDEFAULT[numberOfLEDs] = { 230, 230, 125, 5, 125 };  // Available brightness range on each roll
const int dynamicLimitMaxDEFAULT[numberOfLEDs] = { 255, 255, 255, 100, 255 };
int dynamicLimitMin[numberOfLEDs];
int dynamicLimitMax[numberOfLEDs];

// General-operation-variable arrays
int fadeTimer[numberOfLEDs] = { random(dynamicFadeMinDEFAULT[4], dynamicFadeMaxDEFAULT[4]),
                                random(dynamicFadeMinDEFAULT[4], dynamicFadeMaxDEFAULT[4]),
                                random(dynamicFadeMinDEFAULT[2], dynamicFadeMaxDEFAULT[2]),
                                random(dynamicFadeMinDEFAULT[3], dynamicFadeMaxDEFAULT[3]),
                                random(dynamicFadeMinDEFAULT[4], dynamicFadeMaxDEFAULT[4]) };  // Value of variable for each LED n
byte onTime[numberOfLEDs] = { 0, 0, 0, 0, 0 };
byte onCounter[numberOfLEDs] = { 0, 0, 0, 0, 0 };
int fadeCounter[numberOfLEDs] = { 0, 0, 0, 0, 0 };
char dir[numberOfLEDs] = { 1, 1, 1, 1, 1 };
byte limit[numberOfLEDs] = { 255, 255, 255, 255, 255 };
//boolean enable[numberOfLEDs] = { random(1), random(1), random(1), random(1), random(1) };
boolean enable[numberOfLEDs] = { true, true, true, true, true };

byte d20 = 0;           // Event die.
boolean event = false;  // Are we in an event right now?

// Wait in microseconds before allowing loop() to process again
long delayTime = 50;
long startTime = 0;

// For loop thing
byte i;

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
  // dynamicFadeMax[0] = dynamicFadeMax[4];  // at first.
  // dynamicFadeMin[1] = dynamicFadeMin[4];
  // dynamicFadeMax[1] = dynamicFadeMax[4];
}

void loop() {
  long currTime = micros();

  if ((currTime - startTime) < delayTime) {
    return;
  }
  startTime = currTime;  // Start the timer over.

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
  if ((onTime[pin] == 0) && (dir[pin] = 1)) {                             // Timer has run out and fade direction had reached 0 (and reversed)
    limit[pin] = random(dynamicLimitMin[pin], dynamicLimitMax[pin]);      // Brightness limit
    fadeTimer[pin] = random(dynamicFadeMin[pin], dynamicFadeMax[pin]);    // Fade cycle speed
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