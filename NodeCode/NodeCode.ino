#define RESET 7
#define CLOCK 8
#define LED 9
#define BUTTON 11
#define NODE_TYPE 4

#define OUTPUT_IDENT 3
#define OUTPUT_1 2
#define OUTPUT_2 5
#define OUTPUT_3 6
#define OUTPUT_4 10
#define OUTPUT_CV 10

#define INPUT_1 A4
#define INPUT_2 A5
#define INPUT_3 12
#define INPUT_4 13

#define POT_1 A0
#define POT_2 A1
#define POT_3 A2
#define POT_4 A3
#define POT_CV A3

#define LONG_BUTTON_PUSH 1500ul
#define OUTPUT_MODE_NORMAL 0
#define OUTPUT_MODE_CV 1

int outputMode = OUTPUT_MODE_NORMAL;
unsigned long buttonTimer = 0;
unsigned long flashSchedule[] = {0, 0, 0, 0, 0, 0};
bool triggered = false;
bool clocked = false;
 
void setup() {
  Serial.begin(9600);

  pinMode(CLOCK, INPUT);
  pinMode(RESET, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(NODE_TYPE, INPUT);

  pinMode(OUTPUT_IDENT, OUTPUT);
  pinMode(OUTPUT_1, OUTPUT);
  pinMode(OUTPUT_2, OUTPUT);
  pinMode(OUTPUT_3, OUTPUT);
  pinMode(OUTPUT_4, OUTPUT);

  pinMode(INPUT_1, INPUT);
  pinMode(INPUT_2, INPUT);
  pinMode(INPUT_3, INPUT);
  pinMode(INPUT_4, INPUT);

  pinMode(POT_1, INPUT);
  pinMode(POT_2, INPUT);
  pinMode(POT_3, INPUT);
  pinMode(POT_4, INPUT);

  setTrigger(false);
  clearOutputLinks();
}

void loop() {
  checkTriggers();
  checkButton();
  checkReset();
  checkClock();

  updateLED();
}

void checkTriggers() {
  int input1 = digitalRead(INPUT_1);
  int input2 = digitalRead(INPUT_2);
  int input3 = digitalRead(INPUT_3);
  int input4 = digitalRead(INPUT_4);

  if (input1 == HIGH || input2 == HIGH || input3 == HIGH || input4 == HIGH) {
    setTrigger(true);
  }
}

void checkButton() {
  int button = digitalRead(BUTTON);

  if (button == LOW) {
    buttonTimer = 0;
    return;
  }
  
  if (buttonTimer == 0) {
    setTrigger(true);
    buttonTimer = millis();
    return;
  }

  if (millis() - buttonTimer > LONG_BUTTON_PUSH) {
    incrementOutputMode();
    buttonTimer = millis();
  }
}

void checkReset() {
  int reset = digitalRead(RESET);
  int type = digitalRead(NODE_TYPE);

  if (reset == HIGH) {
    if (type == HIGH) {
      // Primary Node gets triggered by resets
      setTrigger(true);
    } else {
      // Secondary Nodes all get their triggerness cleared by resets
      setTrigger(false);
      clearOutputCV();
      clearOutputLinks();
    }
  }
}

void checkClock() {
  int clockIn = digitalRead(CLOCK);

  if (clockIn == HIGH) {
    if (!clocked) {
      // Analog outputs in CV mode last until the next clock signal
      clearOutputCV();
      
      if (triggered) {
        processTrigger();
  
        delay(10);
  
        setTrigger(false);
  
        // Need to catch any triggers coming from self-patching before clearing output links
        checkTriggers(); 
  
        // Link outputs are short trigger pulses, so they get cleared here
        clearOutputLinks();
      }
    }
    
    clocked = true;
    return;
  }

  clocked = false;
}

// Kind of complicated, but needed to be able to flash the LED without blocking
void updateLED() {
  for (int i = 0; i < 5; i++) {
    if (flashSchedule[i] == 0) {
      break;
    }
    
    if (millis() > flashSchedule[i] && millis() < flashSchedule[i + 1]) {
      digitalWrite(LED, i % 2 == 0 ? LOW : HIGH);
      return;
    }
  }

  digitalWrite(LED, triggered ? HIGH : LOW);
}

void processTrigger() {
  int ports[] = {OUTPUT_1, OUTPUT_2, OUTPUT_3, OUTPUT_4};
  int pots[] = {POT_1, POT_2, POT_3, POT_4};
  float vals[] = {0, 0, 0, 0};
  int indexEnd = 4;
  int total = 0;

  // CV output
  if (outputMode == OUTPUT_MODE_CV) {
    int cv = analogRead(POT_CV);
    cv = map(cv, 0, 1023, 0, 255);  
    analogWrite(OUTPUT_CV, cv);
    indexEnd = 3;
  }

  // Send out identity trigger
  digitalWrite(OUTPUT_IDENT, HIGH);

  // Collect and normalize link probabilities
  for (int i = 0; i < indexEnd; i++) {
    vals[i] = analogRead(pots[i]);
    total += vals[i];
  }

  if (total == 0) {
    return;
  }

  for (int i = 0; i < indexEnd; i++) {
    vals[i] /= total;
  }

  // Randomly choose one of the links and send out a trigger
  float rand = (float)random(0, 1023) / 1023;
  float total2 = 0;
  for (int i = 0; i < indexEnd; i++) {
    total2 += vals[i];
    if (rand < total2) {
      digitalWrite(ports[i], HIGH);
      return;
    }
  }
}

void setTrigger(bool val) {
  triggered = val;
}

void clearOutputLinks() {
  digitalWrite(OUTPUT_IDENT, LOW);
  
  if (outputMode != OUTPUT_MODE_CV) {
    digitalWrite(OUTPUT_1, LOW);
  }
  digitalWrite(OUTPUT_2, LOW);
  digitalWrite(OUTPUT_3, LOW);
  digitalWrite(OUTPUT_4, LOW);
}

void clearOutputCV() {
  if (outputMode == OUTPUT_MODE_CV) {
    analogWrite(OUTPUT_CV, 0);
  }
}

void incrementOutputMode() {
  outputMode = (outputMode + 1) % 2;

  switch (outputMode) {
    case 0:
      scheduleFlashes(2, 750);
      break;
    case 1:
      scheduleFlashes(3, 200);
      break;
  }
}

void scheduleFlashes(unsigned long reps, unsigned long spacing) {
  for (unsigned long i = 0; i < 3; i++) {
    unsigned long start = 0;
    unsigned long finish = 0;
    
    if (i < reps) {
      start = millis() + (i * 2ul) * spacing;
      finish = millis() + (i * 2ul + 1ul) * spacing;
    }

    flashSchedule[i * 2] = start;
    flashSchedule[i * 2 + 1] = finish;
  }
}
