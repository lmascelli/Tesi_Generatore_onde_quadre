#include <LiquidCrystal.h>
#include <assert.h>

/****************************************

  MACRO SECTION

 ****************************************/

#define LCD_COLS 20
#define KEYPAD_PIN 0

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
unsigned int decimals = 0, units = 0, tens = 0, hundreds = 0;


// stimulation parameters
float frequency, period, period_on;  // Hertz
float duty_cycle;
float repetitions;


// timing
unsigned long start_time, current_time;

/****************************************

  UTILITY FUNCTIONS

 ****************************************/

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

inline void create_from_digits(float *num) {
  *num = 100 * hundreds + 10 * tens + units + 0.1f * decimals;
  need_render = true;
}

void set_digits(float num_to_convert) {
  if (num_to_convert > 999.9)
    return;
  int num = (int)(num_to_convert * 10);
  decimals = num % 10;
  num = num / 10;
  units = num % 10;
  num = num / 10;
  tens = num % 10;
  num = num / 10;
  hundreds = num % 10;
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

// questa struttura era prevista per gestire casi molto generali ma per il 
// progetto in corso potrebbe essere più semplice con tutto in update
struct State {
  void (*input)(void);
  void (*update)(void);
  void (*render)(void);
  void *state_memory;
};

State *current_state;


/******************** state utility functions ****************/

void state_create(struct State **state,
    void (*update_func)(void) = NULL,
    void (*input_func)(void) = NULL,
    void (*render_func)(void) = NULL,
    unsigned int memory = 0) {
  (*state) = (State *)malloc(sizeof(State));
  (*state)->update = update_func;
  (*state)->input = input_func;
  (*state)->render = render_func;
  (*state)->state_memory = malloc(memory);
}

void state_delete(struct State *state) {
  free(state->state_memory);
  free(state);
}

// Probably unnecessary
void state_change(struct State *state, bool render = true) {
  need_render = render;
  current_state = state;
}

/******************** state pointers ********************/

State *state_reset;
State *state_ready;

State *state_programmed;
State *state_show_parameters;
State *state_set_parameters;
State *state_programmed_start;
State *state_programmed_started;
State *state_programmed_stopped;

State *state_continuum;
State *state_continuum_start;
State *state_continuum_started;
State *state_continuum_stopped;

// STATE RESET

void state_reset_update() {
  state_change(state_ready);
};

void state_reset_render() {
  print("RESET");
}

// STATE READY

void state_ready_input() {
  if (state == PRESSED) 
    switch(key){
      case UP:
        state_change(state_programmed);
        break;
      case DOWN:
        state_change(state_continuum);
        break;
    }
}

void state_ready_render() {
  print("READY", 1);
}

// STATE PROGRAMMED

void state_programmed_input(){
  if (state==PRESSED)
    switch(key){
      case SELECT:
        state_change(state_show_parameters);
      case DOWN:
        state_change(state_ready);
        break;
      case UP:
        state_change(state_continuum);
        break;
    }
}

void state_programmed_render(){
  print("PROGRAMMED STIM");
}

// STATE PROGRAMMED START

void state_programmed_start_input()
{
  if (state == PRESSED)
    switch(key){
      case SELECT:
        state_change(state_programmed_started);
        break;
      case LEFT:
        state_change(state_ready);
        break;
      case UP:
        state_change(state_set_parameters);
        break;
      case DOWN:
        state_change(state_show_parameters);
        break;
    }
}

void state_programmed_start_render()
{
  print("START STIMULATION?\n"
      "SELECT TO START");
}

// STATE PROGRAMMED STARTED



// STATE PROGRAMMED STOPPED



// STATE SHOW PARAMETERS



// STATE SET PARAMETERS



// STATE CONTINUUM

void state_continuum_input(){
  if (state==PRESSED)
    switch(key){
      case UP:
        state_change(state_ready);
        break;
      case DOWN:
        state_change(state_programmed);
        break;
    }
}

void state_continuum_render(){
  print("CONTINUUM\n STIMULATION");
}

// STATE CONTINUUM START



// STATE CONTINUUM STARTED



// STATE CONTINUUM STOPPED


/**************** states creation ***************/

void inline create_states()
{
  state_create(&state_reset, state_reset_update, nullptr, state_reset_render, 0);
  state_create(&state_ready, nullptr, state_ready_input, state_ready_render, 0);
  state_create(&state_programmed, nullptr, state_programmed_input,
      state_programmed_render, 0);
  state_create(&state_continuum, nullptr, state_continuum_input,
      state_continuum_render, 0);
  current_state = state_reset;
}

/****************************************

  MAIN SETUP AND LOOP

 ****************************************/

void setup() {
  create_states();
  frequency = 1.0f;
  duty_cycle = 0.5f;
  repetitions = 10;
  Serial.begin(9600);
}

void loop() {
  readKeyVal();
  if (current_state->input)
    current_state->input();
  if (current_state->render)
    current_state->render();
  if (current_state->update)
    current_state->update();
}
