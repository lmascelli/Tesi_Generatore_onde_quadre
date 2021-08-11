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


#define I2C
#ifdef I2C
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);
#else
#include <LiquidCrystal.h>
LiquidCrystal lcd(0, 0, 0, 0, 0, 0);
#endif

float frequency, period, period_on;// Hertz
float duty_cycle;
float repetitions;
double start_time, current_time;

unsigned int decimals = 0, units = 0, tens  = 0, hundreds = 0;
bool buttons[4] = {false};

bool need_render = true;

enum STATE {
  PLAY,
  STOP,
  MENU_FREQ,
  MENU_FREQ_0,
  MENU_FREQ_1,
  MENU_FREQ_2,
  MENU_FREQ_3,
  MENU_DUTY_CYCLE,
  MENU_DUTY_CYCLE_0,
  MENU_DUTY_CYCLE_1,
  MENU_DUTY_CYCLE_2,
  MENU_DUTY_CYCLE_3,
  MENU_REPETITIONS,
  MENU_REPETITIONS_0,
  MENU_REPETITIONS_1,
  MENU_REPETITIONS_2,
  MENU_REPETITIONS_3,
  CHANGE_DIGIT
};

enum STATE current_state = STOP;


void set_digits(float num_to_convert) {
  if (num_to_convert >= 999.9) return;
  int num = (int)(num_to_convert * 10);
  hundreds = num % 1000;
  num = num / 10;
  tens = num % 10;
  num = num / 10;
  units = num % 10;
  num = num / 10;
  decimals = num % 10;
}

inline void create_from_digits(float *num) {
  *num = 100 * hundreds + 10 * tens + units + 0.1f * decimals;
}

inline bool compare_buttons(bool b0, bool b1, bool b2, bool b3) {
  return (buttons[0] == b0 && buttons[1] == b2 && buttons[2] == b2 && buttons[3] == b3);
}

inline void change_digit(unsigned int *num, bool decrease) {
  if (!decrease) {
    if (*num == 9) *num = 0;
    else *num++;
  }
  else {
    if (*num == 0) *num = 9;
    else *num--;
  }
  need_render = true;
}

inline void change_state(STATE state) {
  need_render = true;
  current_state = state;
}

void setup() {
  frequency = 1.0f;
  period = 1.0f / frequency * SCALE;
  duty_cycle = 0.5f;
  period_on = period * duty_cycle;
  repetitions = 10;
  lcd.begin(20, 4);
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
        if (compare_buttons(false, false, false, true)) {
          change_state(PLAY);
        } else if (compare_buttons(true, false, false, false)) {
          change_state(MENU_FREQ);
        }
        break;
      }
    case PLAY:
      {
        float loop_repetitions = repetitions;
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
        change_state(STOP);
        break;
      }
    case MENU_FREQ:
      {
        set_digits(frequency);
        if (compare_buttons(true, false, false, false))
          change_state(STOP);
        else if (compare_buttons(false, true, false, false))
          change_state(MENU_DUTY_CYCLE);
        else if (compare_buttons(false, false, false, true))
          change_state(MENU_FREQ_0);
        break;
      }
    case MENU_FREQ_0:
      {
        if (compare_buttons(true, false, false, false))
          change_digit(&hundreds, false);
        else if (compare_buttons(false, true, false, false))
          change_digit(&hundreds, true);
        else if (compare_buttons(false, false, true, false))
          change_state(MENU_FREQ);
        else if (compare_buttons(false, false, false, true))
          change_state(MENU_FREQ_1);
        break;
      }
    case MENU_FREQ_1:
      {
        if (compare_buttons(true, false, false, false))
          change_digit(&tens, false);
        else if (compare_buttons(false, true, false, false))
          change_digit(&tens, true);
        else if (compare_buttons(false, false, true, false))
          change_state(MENU_FREQ_0);
        else if (compare_buttons(false, false, false, true))
          change_state(MENU_FREQ_2);
        break;
      }
    case MENU_FREQ_2:
      {
        if (compare_buttons(true, false, false, false))
          change_digit(&units, false);
        else if (compare_buttons(false, true, false, false))
          change_digit(&units, true);
        else if (compare_buttons(false, false, true, false))
          change_state(MENU_FREQ_1);
        else if (compare_buttons(false, false, false, true))
          change_state(MENU_FREQ_2);
        break;
      }
    case MENU_FREQ_3:
      {
        if (compare_buttons(true, false, false, false))
          change_digit(&decimals, false);
        else if (compare_buttons(false, true, false, false))
          change_digit(&decimals, true);
        else if (compare_buttons(false, false, true, false))
          change_state(MENU_FREQ_2);
        else if (compare_buttons(false, false, false, true))
        {
          change_state(MENU_FREQ);
          create_from_digits(&frequency);
        }
        break;
      }
    case MENU_DUTY_CYCLE:
      {
        set_digits(duty_cycle);
        if (compare_buttons(true, false, false, false))
          change_state(MENU_FREQ);
        else if (compare_buttons(false, true, false, false))
          change_state(MENU_REPETITIONS);
        else if (compare_buttons(false, false, false, true))
          change_state(MENU_DUTY_CYCLE_0);
        break;
      }
    case MENU_DUTY_CYCLE_0:
      {
        if (compare_buttons(true, false, false, false))
          change_digit(&hundreds, false);
        else if (compare_buttons(false, true, false, false))
          change_digit(&hundreds, true);
        else if (compare_buttons(false, false, true, false))
          change_state(MENU_DUTY_CYCLE);
        else if (compare_buttons(false, false, false, true))
          change_state(MENU_DUTY_CYCLE_1);
        break;
      }
    case MENU_DUTY_CYCLE_1:
      {
        if (compare_buttons(true, false, false, false))
          change_digit(&tens, false);
        else if (compare_buttons(false, true, false, false))
          change_digit(&tens, true);
        else if (compare_buttons(false, false, true, false))
          change_state(MENU_DUTY_CYCLE_0);
        else if (compare_buttons(false, false, false, true))
          change_state(MENU_DUTY_CYCLE_1);
        break;
      }
    case MENU_DUTY_CYCLE_2:
      {
        if (compare_buttons(true, false, false, false))
          change_digit(&units, false);
        else if (compare_buttons(false, true, false, false))
          change_digit(&units, true);
        else if (compare_buttons(false, false, true, false))
          change_state(MENU_DUTY_CYCLE_1);
        else if (compare_buttons(false, false, false, true))
          change_state(MENU_DUTY_CYCLE_3);
        break;
      }
    case MENU_DUTY_CYCLE_3:
      {
        if (compare_buttons(true, false, false, false))
          change_digit(&decimals, false);
        else if (compare_buttons(false, true, false, false))
          change_digit(&decimals, true);
        else if (compare_buttons(false, false, true, false))
          change_state(MENU_DUTY_CYCLE_2);
        else if (compare_buttons(false, false, false, true))
        {
          change_state(MENU_DUTY_CYCLE);
          create_from_digits(&duty_cycle);
        }
        break;
      }
    case MENU_REPETITIONS:
      {
        set_digits(repetitions);
        if (compare_buttons(true, false, false, false))
          change_state(MENU_DUTY_CYCLE);
        else if (compare_buttons(false, true, false, false))
          change_state(STOP);
        else if (compare_buttons(false, false, false, true))
          change_state(MENU_REPETITIONS_0);
        break;
      }
    case MENU_REPETITIONS_0:
      {
        if (compare_buttons(true, false, false, false))
          change_digit(&hundreds, false);
        else if (compare_buttons(false, true, false, false))
          change_digit(&hundreds, true);
        else if (compare_buttons(false, false, true, false))
          change_state(MENU_REPETITIONS);
        else if (compare_buttons(false, false, false, true))
          change_state(MENU_REPETITIONS_1);
        break;
      }
    case MENU_REPETITIONS_1:
      {
        if (compare_buttons(true, false, false, false))
          change_digit(&tens, false);
        else if (compare_buttons(false, true, false, false))
          change_digit(&tens, true);
        else if (compare_buttons(false, false, true, false))
          change_state(MENU_REPETITIONS_0);
        else if (compare_buttons(false, false, false, true))
          change_state(MENU_REPETITIONS_2);
        break;
      }
    case MENU_REPETITIONS_2:
      {
        if (compare_buttons(true, false, false, false))
          change_digit(&units, false);
        else if (compare_buttons(false, true, false, false))
          change_digit(&units, true);
        else if (compare_buttons(false, false, true, false))
          change_state(MENU_REPETITIONS_1);
        else if (compare_buttons(false, false, false, true))
          change_state(MENU_REPETITIONS_3);
        break;
      }
    case MENU_REPETITIONS_3:
      {
        if (compare_buttons(true, false, false, false))
          change_digit(&decimals, false);
        else if (compare_buttons(false, true, false, false))
          change_digit(&decimals, true);
        else if (compare_buttons(false, false, true, false))
          change_state(MENU_REPETITIONS_2);
        else if (compare_buttons(false, false, false, true))
        {
          change_state(MENU_REPETITIONS);
          create_from_digits(&repetitions);
        }
        break;
      }
    default:
      break;
  }

  // render to display
  {
    if (need_render){
      lcd.clear();
    switch (current_state) {
      case STOP:
        {
          lcd.print("STOP");
          break;
        }
      case PLAY:
        {
          lcd.print("ACTIVE");
          break;
        }
      case MENU_FREQ:
      {
          lcd.print("Frequency");
          lcd.setCursor(0, 2);
          lcd.print(frequency);
          break;
      }
      case MENU_FREQ_0:
      {
          lcd.print("Frequency");
          lcd.setCursor(0, 2);
          lcd.print(frequency);
          lcd.setCursor(0, 3);
          lcd.print("_");
          break;
      }
      case MENU_FREQ_1:
      {
          lcd.print("Frequency");
          lcd.setCursor(0, 2);
          lcd.print(frequency);
          lcd.setCursor(1, 3);
          lcd.print("_");
          break;
      }
      case MENU_FREQ_2:
      {
          lcd.print("Frequency");
          lcd.setCursor(0, 2);
          lcd.print(frequency);
          lcd.setCursor(2, 3);
          lcd.print("_");
          break;
      }
      case MENU_FREQ_3:
        {
          lcd.print("Frequency");
          lcd.setCursor(0, 2);
          lcd.print(frequency);
          lcd.setCursor(3, 3);
          lcd.print("_");
          break;
      }
      case CHANGE_DIGIT:
        {
          lcd.print("Change Digit");
          break;
        }
      default:
        break;
    }
  }
  }
  need_render = false;
}
