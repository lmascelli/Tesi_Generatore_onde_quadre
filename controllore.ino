#define OUTPUT_PIN 13
#define PIN_UP 12
#define PIN_DOWN 11
#define PIN_LEFT 10
#define PIN_RIGHT 9
// #define MICROSECONDS_PRECISION

#ifdef MICROSECONDS_PRECISION
#define SCALE 1E6f
#define GET_TIME micros
#else
#define SCALE 1E3f
#define GET_TIME millis
#endif

#define MENU_FLAG char
#define FREQUENCY 0
#define DUTY_CYCLE 1
#define REPETITIONS 2

float frequency, period, period_on;// Hertz
float duty_cycle;
unsigned int repetitions;
double start_time, current_time;

unsigned int decimals = 0, units = 0, tens  = 0, hundreds = 0;
bool buttons[4] = {false};

enum STATE {
  PLAY,
  STOP,
  MENU_FREQ_0,
  MENU_FREQ_1,
  MENU_FREQ_2,
  MENU_FREQ_3,
  MENU_DUTY_CYCLE_0,
  MENU_DUTY_CYCLE_1,
  MENU_DUTY_CYCLE_2,
  MENU_DUTY_CYCLE_3,
  MENU_REPETITIONS_0,
  MENU_REPETITIONS_1,
  MENU_REPETITIONS_2,
  MENU_REPETITIONS_3,
  CHANGE_DIGIT
};

enum STATE current_state = STOP;

struct change_digit_info_struct {
  STATE prev;
  STATE next;
  unsigned int *digit;
} change_digit_info;


void set_digits(float num_to_convert){
  if (num_to_convert >= 999.9) return;
  int num = (int)(num_to_convert*10);
  hundreds = num % 1000;
  num = num / 10;
  tens = num % 10;
  num = num / 10;
  units = num % 10;
  num = num / 10;
  decimals = num % 10;
}

inline float create_from_digits(){
  return 100*hundreds + 10*tens + units + 0.1f*decimals;
}

void set_num(MENU_FLAG num){
  switch (num) {
    case FREQUENCY:
      frequency = create_from_digits();
      break;
    case DUTY_CYCLE:
      duty_cycle = create_from_digits();
      break;
    case REPETITIONS:
      repetitions = create_from_digits();
    default:
      break;
  }
}

inline bool compare_buttons(bool b0, bool b1, bool b2, bool b3){
  return (buttons[0] == b0 && buttons[1] == b2 && buttons[2] == b2 && buttons[3] == b3);
}

inline void change_digit(STATE calling, STATE next, int *num){
  
}

void setup() {
  frequency = 1.0f;
  period = 1.0f / frequency * SCALE;
  duty_cycle = 0.5f;
  period_on = period * duty_cycle;
  repetitions = 10;
  start_time = GET_TIME();

  
  pinMode(OUTPUT_PIN, OUTPUT);
  pinMode(PIN_RIGHT, INPUT);
  digitalWrite(OUTPUT_PIN, LOW);
}

void loop() {
  // manage input
  // TODO check if some waiting time is needed for multiple buttons handling
  {
    if (digitalRead(PIN_UP) == HIGH)
      buttons[0] = true;
    else buttons[0] = false;
    if (digitalRead(PIN_DOWN) == HIGH)
      buttons[1] = true;
    else buttons[1] = false;
    if (digitalRead(PIN_LEFT) == HIGH)
      buttons[2] = true;
    else buttons[2] = false;
    if (digitalRead(PIN_RIGHT) == HIGH)
      buttons[3] = true;
    else buttons[3] = false;
  }

  // manage state
  switch (current_state) {
    case STOP:
    {
      if (compare_buttons(false, false, false, true)){
        current_state = PLAY;
      } else if (compare_buttons(true, false, false, false)){
        current_state = MENU_FREQ_0;
      }
      break;
    }
    case PLAY:
    {
      unsigned int loop_repetitions = repetitions;
      while (loop_repetitions > 0) {
        current_time = GET_TIME();
        double elapsed_time = current_time - start_time;
        if (elapsed_time < period_on)
          digitalWrite(OUTPUT_PIN, HIGH);
        else if (elapsed_time < period)
          digitalWrite(OUTPUT_PIN, LOW);
        else {
          start_time = GET_TIME();
          loop_repetitions--;
        }
      }
      current_state = STOP;
      break;
    }
    case CHANGE_DIGIT:
    {
      if (compare_buttons(true, false, false, false)){
        if (*(change_digit_info.digit) == 9) *(change_digit_info.digit) = 0;
        else (*(change_digit_info.digit))++;
      }
      else if (compare_buttons(false, true, false, false)){
        if (*(change_digit_info.digit) == 0) *(change_digit_info.digit) = 9;
        else (*(change_digit_info.digit))--;
      }
      else if (compare_buttons(false, false, true, false))
        current_state = change_digit_info.prev;
      else if (compare_buttons(false, false, false, true))
        current_state = change_digit_info.next;
      break;
    }
    case MENU_FREQ_0:
    {
      if (compare_buttons(true, false, false, false))
        current_state = STOP;
      else if (compare_buttons(false, true, false, false))
        current_state = MENU_DUTY_CYCLE_0;
      else if (compare_buttons(false, false, false, true))
        {
          change_digit_info.prev = STOP;
          change_digit_info.next = MENU_FREQ_0;
          change_digit_info.digit = &hundreds; 
          current_state = CHANGE_DIGIT;
        }
      break;
    }
  }

  // render to display
  {
    
  }
}
