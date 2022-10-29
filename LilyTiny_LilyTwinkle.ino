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
#define LIMITMIN1 230  // LED 1 brightness limit
#define LIMITMAX1 255
#define FADEMIN0 150  // LED0 gets a different fade rate than everyone else.
#define FADEMAX0 355
#define FADEFALSE0 10  // LED0 rolls enable more often
#define FADETRUE0 30

// Fast mode stuff -------------------------------------
#define FADEMINFAST 20  // Alternative "fast-mode" fade rate. Should be fast!
#define FADEMAXFAST 30
#define FADEFALSEFAST 15  // Fast mode-dice rolls. Should make more bright :D
#define FADETRUEFAST 30

#define FASTMODECYCLETRIGGERMIN 3   //  Used in a random number generator. Minimum fade cycles until next fast mode
#define FASTMODECYCLETRIGGERMAX 20  // Maximum fade cycles until next random fast mode

// After how many fade cycles will the first fast mode trigger?
int fastModeCycleCountTrigger = random(3, 5);  // Make sure fast mode triggers quickly the first time after opening the box.
int fastMode = false;                          // Are we in fast mode?
// End fast mode -----------------------------------------

// Variables for fade cycle counter. Allows an event to trigger after x fade cycles
int pin1FadeCycleCompletionCount = 0;

// Variables for celebration fade.
boolean waitingToCelebrate = false;  // Are we waiting to celebrate?
boolean celebrate = false;           // Are we celebrating right now?
int celebrationRoll = 0;             // Holds the magic number to match for celebrations.
boolean celebrationPeak = false;     // Keep track what phase of the celebration phase we're in.
byte celebrationPeakDirection = 0;
int celebrationPeakCount = 0;
boolean celebrationPeakDirectionMax = false;

const int numberOfLEDs = 5;  // Number of LED GPIO pins
// Constant replacement arrays

// Keep a copy of the "default" fade probabilities and speeds.
int defaultDynamicFadeMin[numberOfLEDs] = { FADEMIN, FADEMIN, FADEMIN, FADEMIN, FADEMIN };
int defaultDynamicFadeMax[numberOfLEDs] = { FADEMAX, FADEMAX, FADEMAX, FADEMAX, FADEMAX };  // But start out with the default rate so LED0 has a better chance to get coffee with the rest (the first time)
int dynamicFadeMin[numberOfLEDs];
int dynamicFadeMax[numberOfLEDs];

int defaultDynamicFadeTrue[numberOfLEDs] = { FADETRUE0, FADETRUE, FADETRUE, FADETRUE, FADETRUE };  // Values determining success probability of roll to enable LED next round.
int defaultDynamicFadeFalse[numberOfLEDs] = { FADEFALSE0, FADEFALSE, FADEFALSE, FADEFALSE, FADEFALSE };
int dynamicFadeTrue[numberOfLEDs];  // Initialize empty array to avoid duplication. Populated using memcpy in setup();
int dynamicFadeFalse[numberOfLEDs];

int defaultDynamicLimitMin[numberOfLEDs] = { LIMITMIN0, LIMITMIN1, LIMITMIN, LIMITMIN, LIMITMIN };
int defaultDynamicLimitMax[numberOfLEDs] = { LIMITMAX0, LIMITMAX1, LIMITMAX, LIMITMAX, LIMITMAX };  // But start out with the default rate so LED0 has a better chance to get coffee with the rest (the first time)
int dynamicLimitMin[numberOfLEDs];
int dynamicLimitMax[numberOfLEDs];

// General-operation-variable arrays
int fadeTimer[numberOfLEDs] = { 10, 10, 10, 10, 10 };  // Value of variable for each LED n
byte onTime[numberOfLEDs] = { 0, 0, 0, 0, 0 };
byte onCounter[numberOfLEDs] = { 0, 0, 0, 0, 0 };
int fadeCounter[numberOfLEDs] = { 0, 0, 0, 0, 0 };
char dir[numberOfLEDs] = { 1, 1, 1, 1, 1 };
byte limit[numberOfLEDs] = { 255, 255, 255, 255, 255 };
boolean enable[numberOfLEDs] = { true, true, true, true, true };

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
  memcpy(dynamicFadeTrue, defaultDynamicFadeTrue, sizeof defaultDynamicFadeTrue);
  memcpy(dynamicFadeFalse, defaultDynamicFadeFalse, sizeof defaultDynamicFadeFalse);
  memcpy(dynamicFadeMin, defaultDynamicFadeMin, sizeof defaultDynamicFadeMin);
  memcpy(dynamicFadeMax, defaultDynamicFadeMax, sizeof defaultDynamicFadeMax);
  memcpy(dynamicLimitMin, defaultDynamicLimitMin, sizeof defaultDynamicLimitMin);
  memcpy(dynamicLimitMax, defaultDynamicLimitMax, sizeof defaultDynamicLimitMax);
}

void loop() {
  long currTime = micros();

  if ((currTime - startTime) < delayTime)
    return;

  startTime = currTime;  // Start the timer over.

  // Check to see if we're waiting to celebrate, and if everyone has arrived.
  if ((waitingToCelebrate) && (!enable[0]) && (!enable[1]) && (!enable[2]) && (!enable[3]) && (!enable[4])) {
    waitingToCelebrate = false;  // We're not waiting anymore!!
    celebrate = true;            // Start celebrating

    // Every LED has enable[n] == false. Modify fade-setting values for
    // synchronised a long slow fade-up.
    byte i;
    for (i = 0; i < numberOfLEDs; i++) {  // Loop through each LED
      fadeTimer[i] = 500;                 // Set very slow fade rate
      onTime[i] = 0;                      // reset pwm counter.
      fadeCounter[i] = 0;                 // Reset fade-position counter
      dir[i] = 1;                         // Direction now = forward
      limit[i] = 255;
      enable[i] = true;
    }
  }

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
    if ((celebrate) && (!celebrationPeak) && (onTime[0] == limit[0])) {
      celebrationPeak = true;  // Set "peak-phase" flag.

      // Do something different.
      for (i = 0; i < numberOfLEDs; i++) {  // Loop through each LED
        dynamicFadeTrue[i] = 1000;          // Set a high enable roll probability.
        dynamicFadeFalse[i] = 1;
        dynamicFadeMin[i] = 50;
        dynamicFadeMax[i] = 50;
        dynamicLimitMin[i] = 230;  // The minimum brightness the LED can get is equal
        dynamicLimitMax[i] = 230;  // to the max. This a pulsing effect
      }

      // Choose a direction for the pulse
      celebrationPeakDirection = random(0, 1);

    }
    //else if ((celebrationPeak) && ((onTime[0]/limit[0]) > 0.75)) {
    //}
    else if ((onTime[0] == limit[0]) || (onTime[0] == 0))
      dir[0] *= -1;

    // End of a fade cycle when the LED was on.
    if ((onTime[0] == 0) && (dir[0] = 1)) {
      limit[0] = random(dynamicLimitMin[0], dynamicLimitMax[0]);    // pin-specific brightness values
      fadeTimer[0] = random(dynamicFadeMin[0], dynamicFadeMax[0]);  // pin specific dynamic-fade speed variables

      if (celebrationPeakCount == 200) {
        celebrationPeak = false;
        celebrate = false;
        celebrationPeakCount = 0;
        celebrationPeakDirection = 0;
        celebrationPeakDirectionMax = false;
        // Restore dynamicFadeTrue values from defaultDynamicFadeTrue array.
        memcpy(dynamicFadeTrue, defaultDynamicFadeTrue, sizeof defaultDynamicFadeTrue);
        memcpy(dynamicFadeFalse, defaultDynamicFadeFalse, sizeof defaultDynamicFadeFalse);
        memcpy(dynamicFadeMin, defaultDynamicFadeMin, sizeof defaultDynamicFadeMin);
        memcpy(dynamicFadeMax, defaultDynamicFadeMax, sizeof defaultDynamicFadeMax);
        memcpy(dynamicLimitMin, defaultDynamicLimitMin, sizeof defaultDynamicLimitMin);
        memcpy(dynamicLimitMax, defaultDynamicLimitMax, sizeof defaultDynamicLimitMax);
      }

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
      if (celebrationRoll == 25) waitingToCelebrate = true;  // Inform everyone it's time to gather for the celebration!
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

void celebrationRoutine() {
  if (!celebrate) {
    return;
  }

  if (!celebrationPeak) {
    celebrate = false;      // The celebration fade is over. That was fun.
    celebrationRoll = 500;  // Next celebration in 500 cycles
    return;
  }

  celebrationPeakCount++;

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
  } else if (celebrationPeakDirectionMax) {
    for (i = 0; i < numberOfLEDs; i++) {  // Loop through each LED
      dynamicLimitMin[i] = dynamicLimitMin[i] + random(-4, 1);
      dynamicLimitMax[i] = dynamicLimitMax[i] + random(-3, 1);
    }

    srandom(2499492929);
    for (i = 0; i < numberOfLEDs; i++) {  // Loop through each LED
      dynamicFadeMin[i] = dynamicFadeMin[i] + random((celebrationPeakCount / 2), (celebrationPeakCount * 2));
      dynamicFadeMax[i] = dynamicFadeMax[i] + random((celebrationPeakCount / 2), (celebrationPeakCount * 2));
    }
  }
}

void giveEveryoneCoffee() {
  // ON THE nth FADE
  // Give everyone coffee!!!!!!!!!!!!!!!!!!!!!!!

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
  // Restore dynamicFadeTrue values from defaultDynamicFadeTrue array.
  memcpy(dynamicFadeTrue, defaultDynamicFadeTrue, sizeof defaultDynamicFadeTrue);
  memcpy(dynamicFadeFalse, defaultDynamicFadeFalse, sizeof defaultDynamicFadeFalse);
  memcpy(dynamicFadeMin, defaultDynamicFadeMin, sizeof defaultDynamicFadeMin);
  memcpy(dynamicFadeMax, defaultDynamicFadeMax, sizeof defaultDynamicFadeMax);
  pin1FadeCycleCompletionCount = 0;
  fastModeCycleCountTrigger = random(FASTMODECYCLETRIGGERMIN, FASTMODECYCLETRIGGERMAX);  // How many LED1 fade cycles until fast mode again?
  return;
}

void endOfFadeCycleThings(int pin) {
  if (!enable[1]) {
    return;
  }
  if (pin1FadeCycleCompletionCount == fastModeCycleCountTrigger) giveEveryoneCoffee();
  if (pin1FadeCycleCompletionCount == 11) endFastMode();  // Uh oh coffee has worn off eveyrone is sleepy.
  // Only increment the fast mode counter if celebration-mode flags are false
  // and not already in fast mode.
  if ((!celebrate) && (!waitingToCelebrate) && (!fastMode)) pin1FadeCycleCompletionCount++;
  return;
}

void compareFadeTimerWithCounter(int pin) {
  if (fadeCounter[pin] != fadeTimer[pin]) {
    return;
  }
  fadeCounter[pin] = 0;
  onTime[pin] += dir[pin];

  if ((onTime[pin] == limit[pin]) || (onTime[pin] == 0)) dir[pin] *= -1;  // change fade direction
  if ((onTime[pin] == 0) && (dir[pin] = 1)) {                             // Timer has run out and fade direction had reached 0 (and reversed)
    limit[pin] = random(dynamicLimitMin[pin], dynamicLimitMax[pin]);
    fadeTimer[pin] = random(dynamicFadeMin[pin], dynamicFadeMax[pin]);
    decideLEDState(pin);
  }
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

  // As long as we're not waiting to celebrate,
  // and it's not fast mode,
  // then roll another die.
  enable[pin] = random(0, dynamicFadeTrue[pin] + 1) >= dynamicFadeFalse[pin];
  return;
}