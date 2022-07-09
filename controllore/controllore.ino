#include <LiquidCrystal.h>

/****************************************

  MACRO SECTION

 ****************************************/

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
KEY_VAL key = KEY_UNKNOWN;
KEY_STATE state = UNVALID;
int key_an;


// render
bool need_render = true;
unsigned int decimals = 0, units = 0, tens = 0, hundreds = 0;


// stimulation parameters
float frequency, period, period_on;  // Hertz
float duty_cycle;
float repetitions;


// timing
double start_time, current_time;


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

void button_state() {
  key_an = analogRead(KEYPAD_PIN);
  KEY_VAL keyread = decode_key(key_an);

// prevedere eventualmente un timer minimo di pressione ed un timer per 
// still pressing
  if (keyread == key) {
    if (keyread == NONE)
      state=NO_KEY_PRESSED;
    else
      state=STILL_PRESSING;
  }
  else {
    if (keyread == NONE){
      state = RELEASED;
    } else {
      key = keyread;
      state = PRESSED;
    } 
  }
}

void print(const char *text, int row = 0, int col = 0) {
  // TODO
  // parse for newlines character
  if (need_render) {
    lcd.clear();
    lcd.setCursor(row, col);
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
State *state_start_programmed;
State *state_started_programmed;
State *state_stopped_programmed;

State *state_continuum;
State *state_start_continuum;
State *state_started_continuum;
State *state_stopped_continuum;

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
      case SELECT:
        break;
      case UP:
        state_change(state_programmed);
        break;
      case DOWN:
        state_change(state_continuum);
        break;
    }
}

void state_ready_render() {
  print("READY");
}

// STATE PROGRAMMED

void state_programmed_input(){
  if (state==PRESSED)
    switch(key){
      case DOWN:
        state_change(state_ready);
        break;
      case UP:
        state_change(state_continuum);
        break;
    }
}

void state_programmed_render(){
  print("PROGRAMMED\nSTIMULATION");
}

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
}

void loop() {
  button_state();
  if (current_state->input)
    current_state->input();
  if (current_state->render)
    current_state->render();
  if (current_state->update)
    current_state->update();
}
