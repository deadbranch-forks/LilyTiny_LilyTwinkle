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

#define FADEMIN 50
#define FADEMAX 55
#define FADEFALSE 25
#define FADETRUE 30
#define LIMITMIN 125
#define LIMITMAX 255

// Restricting LED groups 0 and 1 to a minimum 90% PWM duty cycle ensurs that
// the NOT gate LEDs actually turn off during the fade loop.
#define LIMITMIN0 230  // LED 0 brightness limit
#define LIMITMAX0 255
#define LIMITMIN1 230 
#define LIMITMAX1 255
#define FADEMIN0 150  // LED0 gets a different fade rate than everyone else.
#define FADEMAX0 355
#define FADEMIN1 150  
#define FADEMAX1 355
#define FADEFALSE0 7  // LED0 rolls enable more often
#define FADETRUE0 30
#define FADEFALSE1 8  
#define FADETRUE1 30

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
int fastMode = false;                          // Are we in fast mode?
// End fast mode -----------------------------------------

// Variables for fade cycle counter. Allows an event to trigger after x fade cycles
int pin1FadeCycleCompletionCount = 0;

// Variable
boolean waitingToCelebrate = false;  // Are we waiting to celebrate?
boolean celebrate = false;           // Are we celebrating right now?
boolean celebrationRampMaxReached = false;     // Flag to identify when PWM duty cycle = 100% on celebration brightness ramp-up
int celebrationRoll = 0;             // Dice roll
// int celebrationTrigger = random(50, 500); // Holds the magic number to match for celebrations.
int celebrationTrigger = 10; // Holds the magic number to match for celebrations.


byte celebrationPeakDirection = 0;
int celebrationTimerPhase2 = 0;
int celebrationLimit = 200;
boolean celebrationPeakDirectionMax = false;

const int numberOfLEDs = 5;  // Number of LED GPIO pins
// Constant replacement arrays

// Keep a copy of the "default" fade probabilities and speeds.
const int dynamicFadeMinDEFAULT[numberOfLEDs] = { 70, 50, 50, 50, 50 };
const int dynamicFadeMaxDEFAULT[numberOfLEDs] = { 200, 255, 55, 55, 55 };  // But start out with the default rate so LED0 has a better chance to get coffee with the rest (the first time)
int dynamicFadeMin[numberOfLEDs]; // Initialize empty array to avoid duplication. Populated using memcpy in setup();
int dynamicFadeMax[numberOfLEDs];

const int dynamicFadeTrueDEFAULT[numberOfLEDs] = { 30, 30, 30, 30, 30 };  // Values determining success probability of roll to enable LED next round.
const int dynamicFadeFalseDEFAULT[numberOfLEDs] = { 8, 7, 25, 25, 25 };
int dynamicFadeTrue[numberOfLEDs];  
int dynamicFadeFalse[numberOfLEDs];

const int dynamicLimitMinDEFAULT[numberOfLEDs] = { 230, 230, 125, 125, 125 }; // Available brightness range on each roll
const int dynamicLimitMaxDEFAULT[numberOfLEDs] = { 255, 255, 255, 255, 255 };
int dynamicLimitMin[numberOfLEDs];
int dynamicLimitMax[numberOfLEDs];

// General-operation-variable arrays
int fadeTimer[numberOfLEDs] = { 10, 10, 10, 10, 10 };  // Value of variable for each LED n
byte onTime[numberOfLEDs] = { 0, 0, 0, 0, 0 };
byte onCounter[numberOfLEDs] = { 0, 0, 0, 0, 0 };
int fadeCounter[numberOfLEDs] = { 0, 0, 0, 0, 0 };
char dir[numberOfLEDs] = { 1, 1, 1, 1, 1 };
byte limit[numberOfLEDs] = { 255, 255, 255, 255, 255 };
boolean enable[numberOfLEDs] = { random(1), random(1), random(1), random(1), random(1) };

// Wait in microseconds before allowing loop() to process again
long delayTime = 50;
long startTime = 0;

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
  
}

void loop() {
  long currTime = micros();

  if ((currTime - startTime) < delayTime) {
    return;
  }

  startTime = currTime;  // Start the timer over.

  // Check to see if we're waiting to celebrate, and if everyone has arrived.
  if ((waitingToCelebrate) && (!enable[0]) && (!enable[1]) && (!enable[2]) && (!enable[3]) && (!enable[4])) {
    waitingToCelebrate = false;           // We're not waiting anymore!!
    celebrate = true;                     // Start celebrating
    fadeEffect("slowSyncronisedFadeUp");  // Modify fade-setting values for a synchronised a long slow fade-up.
  }

// TODO: Sequential effect needs a proper trigger.
//  if (celebrationRoll == 2) fadeEffect("sequential");
  if (celebrationRoll == celebrationTrigger) waitingToCelebrate = true;  // Inform everyone it's time to gather for the celebration!

  // LED0 section
  if (!enable[0]) digitalWrite(LED0, LOW);
  else if (onCounter[0] > onTime[0]) digitalWrite(LED0, LOW);
  else digitalWrite(LED0, HIGH);

  onCounter[0]++;
  fadeCounter[0]++;

  if (fadeCounter[0] == fadeTimer[0]) {
    fadeCounter[0] = 0;
    onTime[0] += dir[0];

    // Fade direction has changed, thus we're at peak brightness.
    // Ensure we're celebrating, then do something different.
    if ((celebrate) && (!celebrationRampMaxReached) && (onTime[0] == limit[0])) {
      celebrationRampMaxReached = true;  // Set flag to track that slow-ramp up has reached brightness of 100%

      // Opportunity to trigger pulsing effect.
      celebrationPeakDirection = random(0, 1); // Choose a direction for the pulse
      fadeEffect("pulsing");
      
      
      //celebrationPeakDirection = 0;
    }

    else if ((onTime[0] == limit[0]) || (onTime[0] == 0))
      dir[0] *= -1;

    // End of a fade cycle when the LED was on.
    if ((onTime[0] == 0) && (dir[0] = 1)) {
      limit[0] = random(dynamicLimitMin[0], dynamicLimitMax[0]);    // pin-specific brightness values
      fadeTimer[0] = random(dynamicFadeMin[0], dynamicFadeMax[0]);  // pin specific dynamic-fade speed variables

      celebrationRoutine();

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

      endOfFadeCycleThings(1);
      celebrationRoll++;                                     // Celebration dice roll
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
    limit[pin] = random(dynamicLimitMin[pin], dynamicLimitMax[pin]);    // Brightness limit
    fadeTimer[pin] = random(dynamicFadeMin[pin], dynamicFadeMax[pin]);  // Fade cycle speed
    decideLEDState(pin);
  }
}

void endOfFadeCycleThings(int pin) {
  if (!enable[1]) {  // Only work on a fade-cycle phase where LED1 was enabled.
    return;
  }
  if (pin1FadeCycleCompletionCount == fastModeCycleCountTrigger) giveEveryoneCoffee();
  if (pin1FadeCycleCompletionCount == 11) endFastMode();                                     // Uh oh coffee has worn off eveyrone is sleepy.
  if ((!celebrate) && (!waitingToCelebrate) && (!fastMode)) pin1FadeCycleCompletionCount++;  // Only increment the fast mode
                                                                                             // counter if celebration-mode flags are false and not already in fast mode.
  return;
}



void decideLEDState(int pin) {
  // Decide if an LED should be enabled next round.
  if (waitingToCelebrate) {
    enable[pin] = false;  // Stay disabled
    return;
  }
  if (fastMode) {
    enable[pin] = true;  // Enable next round
    return;
  }
  if (dynamicFadeTrue[i] == 1001) { // We are in sequential mode. 
    effectSequentialCheck(pin);
    return;
  }
  enable[pin] = random(0, dynamicFadeTrue[pin] + 1) >= dynamicFadeFalse[pin];  // Roll for LED on|off state
  return;
}

void effectSequentialCheck(int pin) {
  int previousPin;
  previousPin = pin - 1;
  if (previousPin < 0) previousPin = numberOfLEDs; // For LED0, previous pin will equal 5 or whatever.
  if (onTime[previousPin] != 15) {  // How far into it's fade cycle is the LED before me?
    return;
  }
  enable[pin] = true;
}

// // // Effect stuff
void fadeEffect(char effect[]) {
  if (effect == "slowSyncronisedFadeUp") {
    for (i = 0; i < numberOfLEDs; i++) {  // Loop through each LED
      fadeTimer[i] = 500;                 // Set very slow fade rate
      onTime[i] = 0;                      // reset pwm counter.
      fadeCounter[i] = 0;                 // Reset fade-position counter
      dir[i] = 1;                         // Direction now = forward
      limit[i] = 255;
      enable[i] = true;
    }
  }

  if (effect == "pulsing") {
    for (i = 0; i < numberOfLEDs; i++) {  // Loop through each LED
      dynamicFadeTrue[i] = 1000;          // Set a high enable roll probability.
      dynamicFadeFalse[i] = 1;
      dynamicFadeMin[i] = 50;
      dynamicFadeMax[i] = 50;
      dynamicLimitMin[i] = 230;  // The minimum brightness the LED can get is equal
      dynamicLimitMax[i] = 230;  // to the max. This a pulsing effect
    }
    celebrationPeakDirection = random(0, 1);  // Choose a direction for the pulse frequency change
                                              // 0 = slow down, 1 = speed up
  }
  if (effect == "sequential") {
    if (dynamicFadeTrue[0] == 1001) {
      return; // don't if we're already doing.
    }

    for (i = 0; i < numberOfLEDs; i++) {  // Loop through each LED
      dynamicFadeTrue[i] = 1001; // frequency probability
      dynamicFadeFalse[i] = 1;
      dynamicFadeMin[i] = 20;   // duration
      dynamicFadeMax[i] = 20;   // 
      dynamicLimitMin[i] = 240;  // brightness
      dynamicLimitMax[i] = 240;  // 
    }

    prematurelyEndFadeCycle();  // Make every LED finish their current fade REALLY FAST.
  }
}

void prematurelyEndFadeCycle() {   // Re-set fade speed. Useful to interrupt LEDs in a long fade for a new effect when set low.
  for (i = 0; i < numberOfLEDs; i++) {
    //limit[i] = 20;
    if (dir[i] > 0) dir[i] *= -1;
    fadeTimer[i] = 20;
  }
}

void celebrationRoutine() {
  if (!celebrate) {
    return;
  }

  if (celebrationTimerPhase2 == celebrationLimit) {
    celebrate = false;
    celebrationRampMaxReached = false;
    celebrationTimerPhase2 = 0;
    celebrationPeakDirection = 0;
    celebrationPeakDirectionMax = false;
    // Restore dynamicFadeTrue values from dynamicFadeTrueDEFAULT array.
    memcpy(dynamicFadeTrue, dynamicFadeTrueDEFAULT, sizeof dynamicFadeTrueDEFAULT);
    memcpy(dynamicFadeFalse, dynamicFadeFalseDEFAULT, sizeof dynamicFadeFalseDEFAULT);
    memcpy(dynamicFadeMin, dynamicFadeMinDEFAULT, sizeof dynamicFadeMinDEFAULT);
    memcpy(dynamicFadeMax, dynamicFadeMaxDEFAULT, sizeof dynamicFadeMaxDEFAULT);
    memcpy(dynamicLimitMin, dynamicLimitMinDEFAULT, sizeof dynamicLimitMinDEFAULT);
    memcpy(dynamicLimitMax, dynamicLimitMaxDEFAULT, sizeof dynamicLimitMaxDEFAULT);
    celebrationRoll = 500;  // Next celebration in 500 cycles
    return;
  }

  celebrationTimerPhase2++;  // Only stay in the celebration peak phase for a limited amount of time.

  if ((celebrationPeakDirection == 1)) {
    for (i = 0; i < numberOfLEDs; i++) {
      dynamicFadeMin[i]++;
      dynamicFadeMax[i]++;
    }
    return;
  }

  // We're in the pulse phase. Do something cool.
  if ((dynamicFadeMin[0] > 10) && (!celebrationPeakDirectionMax)) {
    // Decrement each cycle
    for (i = 0; i < numberOfLEDs; i++) {  // Loop through each LED
      dynamicFadeMin[i]--;
      dynamicFadeMax[i]--;
    }
    celebrationPeakDirectionMax == true;
    return;
  }

  for (i = 0; i < numberOfLEDs; i++) {  // Loop through each LED
    srandom(2499492929);
    dynamicLimitMin[i] = dynamicLimitMin[i] + random(-4, 1);
    dynamicLimitMax[i] = dynamicLimitMax[i] + random(-3, 1);
    dynamicFadeMin[i] = dynamicFadeMin[i] + random((celebrationTimerPhase2 / 2), (celebrationTimerPhase2 * 2));
    dynamicFadeMax[i] = dynamicFadeMax[i] + random((celebrationTimerPhase2 / 2), (celebrationTimerPhase2 * 2));
  }
}

void giveEveryoneCoffee() {
  // ON THE nth FADE
  // Give everyone coffee!!!!!!!!!!!!!!!!!!!!!!!
  fastMode = true;

  for (i = 0; i < numberOfLEDs; i++) {  // Loop through each LED
    dynamicFadeTrue[i] = FADETRUEFAST;  // Set a high enable roll probability.
    dynamicFadeFalse[i] = FADEFALSEFAST;
    dynamicFadeMin[i] = FADEMINFAST;
    dynamicFadeMax[i] = FADEMAXFAST;
  }
  fadeTimer[0] = random(FADEMINFAST, FADEMAXFAST);  // Hijack LED0's fade timer just in case it's
                                                    // in the middle of a really long fade. Replace it with something short. That way it
                                                    // will coffee too!!!!
  for (i = 0; i < numberOfLEDs; i++) {              // Loop through each LED
    enable[i] = true;                               // Turn each on.
  }
  return;
}

void endFastMode() {
  // Restore dynamicFadeTrue values from dynamicFadeTrueDEFAULT array.
  memcpy(dynamicFadeTrue, dynamicFadeTrueDEFAULT, sizeof dynamicFadeTrueDEFAULT);
  memcpy(dynamicFadeFalse, dynamicFadeFalseDEFAULT, sizeof dynamicFadeFalseDEFAULT);
  memcpy(dynamicFadeMin, dynamicFadeMinDEFAULT, sizeof dynamicFadeMinDEFAULT);
  memcpy(dynamicFadeMax, dynamicFadeMaxDEFAULT, sizeof dynamicFadeMaxDEFAULT);
  pin1FadeCycleCompletionCount = 0;
  fastModeCycleCountTrigger = random(FASTMODECYCLETRIGGERMIN, FASTMODECYCLETRIGGERMAX);  // How many LED1 fade cycles until fast mode again?
  return;
}

