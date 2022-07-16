#include <LiquidCrystal.h>
#include <assert.h>

/****************************************

  MACRO SECTION

 ****************************************/

#define LCD_COLS 20
#define KEYPAD_PIN 0
#define OUT_PIN LED_BUILTIN

#define change_state(X) {need_render = true; current_state = X;}

/****************************************

  ANALOG VALUE ASSOCIATED TO KEYS

 ****************************************/

#define RELEASE_VALUE 1024
#define RIGHT_VALUE 129
#define UP_VALUE 305
#define DOWN_VALUE 478
#define LEFT_VALUE 719
#define SELECT_VALUE 1023

/****************************************

  POSSIBLE KEYS VALUES

 ****************************************/
enum KEY_VAL {
  KEY_UNKNOWN= -1,
  SELECT,
  UP,
  DOWN,
  RIGHT,
  LEFT,
  NONE,
};

enum KEY_STATE {
  RELEASED,
  PRESSED,
  STILL_PRESSING,
  NO_KEY_PRESSED,
  UNVALID,
};

/****************************************

  GLOABAL VARIABLES

 ****************************************/

// lcd
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);


// key
#define KEY_THRESHOLD 100
KEY_VAL key = KEY_UNKNOWN;
KEY_STATE state = UNVALID;
int key_an;
KEY_VAL pinValue = KEY_UNKNOWN;
unsigned long key_timer = 0, previous_time = 0;


// render
bool need_render = true;

// es. 1234
// digits[0] = 1;
// digits[1] = 2;
// digits[2] = 3;
// digits[3] = 4;
unsigned char digits[4] = {0}; 

unsigned long blink_timer, blink_period_on = 900, blink_period_off = 100;


// programmed stimulation parameters

bool stimulation_on = false;

float frequency, period, period_on;  // Hertz
float duty_cycle;
unsigned int total_repetitions, current_repetition;

unsigned char show_param_index = 0, set_param_index = 0, set_param_digit = 0;


// timing
unsigned long stim_period_start;

/****************************************

  UTILITY FUNCTIONS

 ****************************************/

/**************** LCD ******************/

inline KEY_VAL decode_key(int value) {
  if (value > RELEASE_VALUE)
    return NONE;
  if (value < RIGHT_VALUE)
    return RIGHT;
  if (value < UP_VALUE)
    return UP;
  if (value < DOWN_VALUE)
    return DOWN;
  if (value < LEFT_VALUE)
    return LEFT;
  if (value < SELECT_VALUE)
    return SELECT;
  else
    return KEY_UNKNOWN;
}

void readKeyVal(){
  unsigned long now_time = millis();
  key_an = analogRead(KEYPAD_PIN);
  KEY_VAL readval = decode_key(key_an);
  if (readval == pinValue){
    key_timer += (now_time - previous_time);
    Serial.println(key_timer);
    if (key_timer > KEY_THRESHOLD)
      button_state(pinValue);
  } else {
    pinValue = readval;
    key_timer = 0;
  }
  previous_time = now_time;
}

void button_state(KEY_VAL keyval) {
  if (keyval == key) {
    if (keyval == NONE)
      state=NO_KEY_PRESSED;
    else
      state=STILL_PRESSING;
  }
  else {
    if (keyval == NONE){
      state = RELEASED;
    } else {
      key = keyval;
      state = PRESSED;
    } 
  }
}

void print(const char *text, int col = 0) {
  assert(0 <= col && col < 20);
  if (need_render) {
    lcd.clear();
    lcd.setCursor(col, 0);
    lcd.print(text);
    need_render = false;
  }
}

/**************** DIGITS ******************/

void create_from_digits(float *num, unsigned int scale = 10) {
  *num = scale * (        digits[0] +
                  1e-1f * digits[1] + 
                  1e-2f * digits[2] +
                  1e-3f * digits[3]);

  need_render = true;
}

inline void set_digits(float num_to_convert, unsigned int decimals = 1) {
  if (num_to_convert > 10 * scale)
    return;
  unsigned int scale = 1;
  for (unsigned int i=decimals; i>1; i++)
    scale *= 10;
  unsigned int num = (unsigned int)(num_to_convert * scale);
  digits[3] = num % 10;
  num = num / 10;
  digits[2] = num % 10;
  num = num / 10;
  digits[1] = num % 10;
  num = num / 10;
  digits[0] = num % 10;
}

inline void change_digit(unsigned int *num, bool decrease) {
  if (!decrease) {
    if (*num == 9)
      *num = 0;
    else
      (*num)++;
  } else {
    if (*num == 0)
      *num = 9;
    else
      (*num)--;
  }
}

/****************************************

  STATE MACHINE

 ****************************************/

typedef void (*State)(void);

State current_state;

// STATE RESET

void state_reset() {
  print("RESET");
  delay(1000);
  change_state(state_ready);
};


// STATE READY

void state_ready() {
  print("READY");

  if (state == PRESSED) 
    switch(key){
      case UP:
        change_state(state_programmed);
        break;
      case DOWN:
        change_state(state_continuum);
        break;
    }
}

// STATE PROGRAMMED

void state_programmed(){
  print("PROGRAMMED STIM");

  if (state==PRESSED)
    switch(key){
      case SELECT:
      case RIGHT:
        change_state(state_show_parameters);
        break;
      case DOWN:
        change_state(state_ready);
        break;
      case UP:
        change_state(state_continuum);
        break;
    }
}

// STATE PROGRAMMED START

void state_programmed_start()
{
  print("SELECT TO START");
  if (state == PRESSED)
    switch(key){
      case SELECT:
        current_repetition = 1;
        change_state(state_programmed_started);
        stimulation_on = true;
        stim_period_start = millis();
        digitalWrite(OUT_PIN, HIGH);
        break;
      case LEFT:
        change_state(state_programmed);
        break;
      case UP:
        change_state(state_set_parameters);
        break;
      case DOWN:
        change_state(state_show_parameters);
        break;
    }
}

// STATE PROGRAMMED STARTED

void state_programmed_started()
{
  String text = "REP :" + String(current_repetition)
    + "/" + String(total_repetitions);
  print(text.c_str());

  // ----- stimulation logic

  if (current_repetition <= total_repetitions) {
    unsigned long elapsed = millis() - stim_period_start;
    if (elapsed > period) {
      stim_period_start = millis();
      digitalWrite(OUT_PIN, HIGH);
      stimulation_on = true;
      current_repetition++;
      need_render = true;
    } else if (elapsed > period_on and stimulation_on) {
      digitalWrite(OUT_PIN, LOW);
      stimulation_on = false;
    }
  } else {
    digitalWrite(OUT_PIN, LOW);
    stimulation_on = false;
    change_state(state_programmed_start);
  }

  // ----- end stimulation logic

  if (state == PRESSED)
    switch(key){
      case SELECT:
        change_state(state_programmed_stopped);
        break;
    }
}


// STATE PROGRAMMED STOPPED

void state_programmed_stopped()
{
  print("SELECT TO RESUME");

  if (state == PRESSED)
    switch(key){
      case SELECT:
        change_state(state_programmed_started);
        break;
    }
}


// STATE SHOW PARAMETERS

void state_show_parameters(){
  print("SHOW PARAMETERS");
  if (state == PRESSED)
    switch(key){
      case SELECT:
      case RIGHT:
        show_param_index = 0;
        change_state(state_showing_parameters);
        break;
      case UP:
        change_state(state_programmed_start);
        break;
      case DOWN:
        change_state(state_set_parameters);
        break;
      case LEFT:
        change_state(state_programmed);
        break;
    }
}

// STATE SHOWING PARAMETERS

void state_showing_parameters(){
  String text;
  switch(show_param_index)
  {
    case 0:
      text = "TOT PERIOD: " + String(period);
      break;
    case 1:
      text = "ON PERIOD: " + String(period_on);
      break;
    case 2:
      text = "NUM RIPET: " + String(total_repetitions);
      break;
  }
  print(text.c_str());

  if (state == PRESSED)
    switch(key){
      case LEFT:
        change_state(state_show_parameters);
        break;
      case UP:
        show_param_index--;
        show_param_index = show_param_index < 0 ? 2 : show_param_index;
        need_render = true;
        break;
      case DOWN:
        show_param_index++;
        show_param_index = show_param_index > 2 ? 0 : show_param_index;
        need_render = true;
        break;
    }
}

// STATE SET PARAMETERS

void state_set_parameters(){
  print ("SET PARAMETERS");

  if (state == PRESSED)
    switch(key){
      case SELECT:
      case RIGHT:
        set_param_index = 0;
        set_param_digit = 0;
        blink_timer = millis();
        change_state(state_setting_parameters);
        break;
      case UP:
        change_state(state_show_parameters);
        break;
      case DOWN:
        change_state(state_programmed_start);
        break;
      case LEFT:
        change_state(state_programmed);
        break;
    }
}

// STATE SETTING PARAMETERS

void state_setting_parameters(){
  String text;
  switch(set_param_index){
    case 0:
      text = "TOT PERIOD: ";
      set_digits(period);
      break;
    case 1:
      text = "ON PERIOD: ";
      set_digits(period_on);
      break;
    case 2:
      text = "N° RIPET: ";
      set_digits(total_repetitions);
      break;
  }
  for (unsigned int i=0; i<3; i++){
    if (i == set_param_digit){

    } else {

    }
  }
}

// STATE CONTINUUM

void state_continuum(){
  print("CONTINUUM STIM");

  if (state==PRESSED)
    switch(key){
      case SELECT:
      case RIGHT:
        change_state(state_continuum_start);
        break;
      case UP:
        change_state(state_ready);
        break;
      case DOWN:
        change_state(state_programmed);
        break;
    }
}

// STATE CONTINUUM START

void state_continuum_start(){
  print("SELECT TO START");

  if (state == PRESSED)
    switch(key){
      case SELECT:
        change_state(state_continuum_started);
        stimulation_on = true;
        stim_period_start = millis();
        digitalWrite(OUT_PIN, HIGH);
        break;
      case LEFT:
        change_state(state_continuum);
        break;
    }
}

// STATE CONTINUUM STARTED

void state_continuum_started(){
  print ("SELECT TO STOP");

  // ----- stimulation logic

  unsigned long elapsed = millis() - stim_period_start;
  if (elapsed > period) {
    stim_period_start = millis();
    digitalWrite(OUT_PIN, HIGH);
    stimulation_on = true;
  } else if (elapsed > period_on and stimulation_on) {
    digitalWrite(OUT_PIN, LOW);
    stimulation_on = false;
  }

  // ----- end stimulation logic

  if (state == PRESSED)
    switch(key){
      case SELECT:
        digitalWrite(OUT_PIN, LOW);
        stimulation_on = false;
        change_state(state_continuum_start);
        break;
    }
}


/****************************************

  MAIN SETUP AND LOOP

 ****************************************/

void setup() {
  frequency = 1.0f;
  period = 1000./frequency;
  duty_cycle = 0.5f;
  period_on = period * duty_cycle;
  total_repetitions = 10;
  current_state = state_reset;
  pinMode(OUT_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  readKeyVal();
  current_state();
}

