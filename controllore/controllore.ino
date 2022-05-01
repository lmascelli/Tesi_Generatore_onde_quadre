#include <LiquidCrystal.h>

/****************************************
 *
 * MACRO SECTION
 *
 ****************************************/

#define KEYPAD_PIN 0
// #define MICROSECONDS_PRECISION

#ifdef MICROSECONDS_PRECISION
#define SCALE 1E6f
#define GET_TIME micros
#else
#define SCALE 1E3f
#define GET_TIME millis
#endif

/****************************************
 *
 * ANALOG VALUE ASSOCIATED TO KEYS
 *
 ****************************************/

#define RELEASE_VALUE 1000
#define RIGHT_VALUE 60
#define UP_VALUE 200
#define DOWN_VALUE 400
#define LEFT_VALUE 600
#define SELECT_VALUE 800

/****************************************
 *
 * POSSIBLE KEYS VALUES
 *
 ****************************************/
enum PIN_KEY_VAL
{
  UNKNOWN = -1,
  RELEASE,
  STILL_PRESSED,
  SELECT,
  UP,
  DOWN,
  RIGHT,
  LEFT,
};

/****************************************
 *
 * GLOABAL VARIABLES
 *
 ****************************************/

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
int key_an;
PIN_KEY_VAL key;
bool need_render = true, key_release = true;

float frequency, period, period_on; // Hertz
float duty_cycle;
float repetitions;
double start_time, current_time;

unsigned int decimals = 0, units = 0, tens = 0, hundreds = 0;

/****************************************
 *
 * UTILITY FUNCTIONS
 *
 ****************************************/

inline PIN_KEY_VAL decode_key(int value)
{
  if (value > RELEASE_VALUE)
    return RELEASE;
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
    return UNKNOWN;
}

void input()
{
  key_an = analogRead(KEYPAD_PIN);
  key = decode_key(key_an);
  if (RELEASE == key)
  {
    key_release = true;
    return;
  }
  if (key_release)
    key_release = false;
  else
    key = STILL_PRESSED;
}

void print(const char *text, int row = 0, int col = 0)
{
  if (need_render)
  {
    lcd.clear();
    lcd.print(text);
    need_render = false;
  }
}

inline void create_from_digits(float *num)
{
  *num = 100 * hundreds + 10 * tens + units + 0.1f * decimals;
  need_render = true;
}

void set_digits(float num_to_convert)
{
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

inline void change_digit(unsigned int *num, bool decrease)
{
  if (!decrease)
  {
    if (*num == 9)
      *num = 0;
    else
      (*num)++;
  }
  else
  {
    if (*num == 0)
      *num = 9;
    else
      (*num)--;
  }
}

/****************************************
 *
 * STATE MACHINE
 *
 ****************************************/

struct State
{
  void (*update)(void);
  bool input_enable;
  void *state_memory;
};

State *current_state;

State *state_reset;
State *state_ready;
State *state_set_menu;
State *state_run_menu;

void change_state(struct State *state)
{
  need_render = true;
  current_state = state;
}

void render_digits(float num_to_render)
{
  int num = (int)(num_to_render * 10);
  int decimals = num % 10;
  num = num / 10;
  int units = num % 10;
  num = num / 10;
  int tens = num % 10;
  num = num / 10;
  int hundreds = num % 10;

  lcd.setCursor(0, 2);
  lcd.print(hundreds);
  lcd.setCursor(1, 2);
  lcd.print(tens);
  lcd.setCursor(2, 2);
  lcd.print(units);
  lcd.setCursor(3, 2);
  lcd.print('.');
  lcd.setCursor(4, 2);
  lcd.print(decimals);
}

void setup()
{
  frequency = 1.0f;
  duty_cycle = 0.5f;
  repetitions = 10;
}

void loop()
{
  input();
  current_state->update();
}

// void manage_state()
// {
//   switch (current_state)
//   {
//   case STOP:
//   {
//     if (compare_buttons(false, false, false, true))
//     {
//       change_state(PLAY);
//     }
//     else if (compare_buttons(false, true, false, false))
//     {
//       change_state(MENU_FREQ);
//     }
//     break;
//   }
//   case PLAY:
//   {
//     period = 1.0f / frequency * SCALE;
//     period_on = period * duty_cycle;
//     float loop_repetitions = repetitions;
//     while (loop_repetitions > 0)
//     {
//       current_time = GET_TIME();
//       double elapsed_time = current_time - start_time;
//       if (elapsed_time < period_on)
//         digitalWrite(OUTPUT_PIN, HIGH);
//       else if (elapsed_time < period)
//         digitalWrite(OUTPUT_PIN, LOW);
//       else
//       {
//         start_time = GET_TIME();
//         loop_repetitions--;
//       }
//     }
//     change_state(STOP);
//     break;
//   }
//   case MENU_FREQ:
//   {
//     set_digits(frequency);
//     if (compare_buttons(true, false, false, false))
//       change_state(STOP);
//     else if (compare_buttons(false, true, false, false))
//       change_state(MENU_DUTY_CYCLE);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_FREQ_0);
//     break;
//   }
//   case MENU_FREQ_0:
//   {
//     if (compare_buttons(true, false, false, false))
//     {
//       change_digit(&hundreds, false);
//       create_from_digits(&frequency);
//     }
//     else if (compare_buttons(false, true, false, false))
//     {
//       change_digit(&hundreds, true);
//       create_from_digits(&frequency);
//     }
//     else if (compare_buttons(false, false, true, false))
//       change_state(MENU_FREQ);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_FREQ_1);
//     break;
//   }
//   case MENU_FREQ_1:
//   {
//     if (compare_buttons(true, false, false, false))
//     {
//       change_digit(&tens, false);
//       create_from_digits(&frequency);
//     }
//     else if (compare_buttons(false, true, false, false))
//     {
//       change_digit(&tens, true);
//       create_from_digits(&frequency);
//     }
//     else if (compare_buttons(false, false, true, false))
//       change_state(MENU_FREQ_0);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_FREQ_2);
//     break;
//   }
//   case MENU_FREQ_2:
//   {
//     if (compare_buttons(true, false, false, false))
//     {
//       change_digit(&units, false);
//       create_from_digits(&frequency);
//     }
//     else if (compare_buttons(false, true, false, false))
//     {
//       change_digit(&units, true);
//       create_from_digits(&frequency);
//     }
//     else if (compare_buttons(false, false, true, false))
//       change_state(MENU_FREQ_1);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_FREQ_3);
//     break;
//   }
//   case MENU_FREQ_3:
//   {
//     if (compare_buttons(true, false, false, false))
//     {
//       change_digit(&decimals, false);
//       create_from_digits(&frequency);
//     }
//     else if (compare_buttons(false, true, false, false))
//     {
//       change_digit(&decimals, true);
//       create_from_digits(&frequency);
//     }
//     else if (compare_buttons(false, false, true, false))
//       change_state(MENU_FREQ_2);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_FREQ);
//     break;
//   }
//   case MENU_DUTY_CYCLE:
//   {
//     set_digits(duty_cycle);
//     if (compare_buttons(true, false, false, false))
//       change_state(MENU_FREQ);
//     else if (compare_buttons(false, true, false, false))
//       change_state(MENU_REPETITIONS);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_DUTY_CYCLE_0);
//     break;
//   }
//   case MENU_DUTY_CYCLE_0:
//   {
//     if (compare_buttons(true, false, false, false))
//     {
//       change_digit(&hundreds, false);
//       create_from_digits(&duty_cycle);
//     }
//     else if (compare_buttons(false, true, false, false))
//     {
//       change_digit(&hundreds, true);
//       create_from_digits(&duty_cycle);
//     }
//     else if (compare_buttons(false, false, true, false))
//       change_state(MENU_DUTY_CYCLE);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_DUTY_CYCLE_1);
//     break;
//   }
//   case MENU_DUTY_CYCLE_1:
//   {
//     if (compare_buttons(true, false, false, false))
//     {
//       change_digit(&tens, false);
//       create_from_digits(&duty_cycle);
//     }
//     else if (compare_buttons(false, true, false, false))
//     {
//       change_digit(&tens, true);
//       create_from_digits(&duty_cycle);
//     }
//     else if (compare_buttons(false, false, true, false))
//       change_state(MENU_DUTY_CYCLE_0);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_DUTY_CYCLE_2);
//     break;
//   }
//   case MENU_DUTY_CYCLE_2:
//   {
//     if (compare_buttons(true, false, false, false))
//     {
//       change_digit(&units, false);
//       create_from_digits(&duty_cycle);
//     }
//     else if (compare_buttons(false, true, false, false))
//     {
//       change_digit(&units, true);
//       create_from_digits(&duty_cycle);
//     }
//     else if (compare_buttons(false, false, true, false))
//       change_state(MENU_DUTY_CYCLE_1);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_DUTY_CYCLE_3);
//     break;
//   }
//   case MENU_DUTY_CYCLE_3:
//   {
//     if (compare_buttons(true, false, false, false))
//     {
//       change_digit(&decimals, false);
//       create_from_digits(&duty_cycle);
//     }
//     else if (compare_buttons(false, true, false, false))
//     {
//       change_digit(&decimals, true);
//       create_from_digits(&duty_cycle);
//     }
//     else if (compare_buttons(false, false, true, false))
//       change_state(MENU_DUTY_CYCLE_2);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_DUTY_CYCLE);
//     break;
//   }
//   case MENU_REPETITIONS:
//   {
//     set_digits(repetitions);
//     if (compare_buttons(true, false, false, false))
//       change_state(MENU_DUTY_CYCLE);
//     else if (compare_buttons(false, true, false, false))
//       change_state(STOP);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_REPETITIONS_0);
//     break;
//   }
//   case MENU_REPETITIONS_0:
//   {
//     if (compare_buttons(true, false, false, false))
//     {
//       change_digit(&hundreds, false);
//       create_from_digits(&repetitions);
//     }
//     else if (compare_buttons(false, true, false, false))
//     {
//       change_digit(&hundreds, true);
//       create_from_digits(&repetitions);
//     }
//     else if (compare_buttons(false, false, true, false))
//       change_state(MENU_REPETITIONS);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_REPETITIONS_1);
//     break;
//   }
//   case MENU_REPETITIONS_1:
//   {
//     if (compare_buttons(true, false, false, false))
//     {
//       change_digit(&tens, false);
//       create_from_digits(&repetitions);
//     }
//     else if (compare_buttons(false, true, false, false))
//     {
//       change_digit(&tens, true);
//       create_from_digits(&repetitions);
//     }
//     else if (compare_buttons(false, false, true, false))
//       change_state(MENU_REPETITIONS_0);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_REPETITIONS_2);
//     break;
//   }
//   case MENU_REPETITIONS_2:
//   {
//     if (compare_buttons(true, false, false, false))
//     {
//       change_digit(&units, false);
//       create_from_digits(&repetitions);
//     }
//     else if (compare_buttons(false, true, false, false))
//     {
//       change_digit(&units, true);
//       create_from_digits(&repetitions);
//     }
//     else if (compare_buttons(false, false, true, false))
//       change_state(MENU_REPETITIONS_1);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_REPETITIONS_3);
//     break;
//   }
//   case MENU_REPETITIONS_3:
//   {
//     if (compare_buttons(true, false, false, false))
//     {
//       change_digit(&decimals, false);
//       create_from_digits(&repetitions);
//     }
//     else if (compare_buttons(false, true, false, false))
//     {
//       change_digit(&decimals, true);
//       create_from_digits(&repetitions);
//     }
//     else if (compare_buttons(false, false, true, false))
//       change_state(MENU_REPETITIONS_2);
//     else if (compare_buttons(false, false, false, true))
//       change_state(MENU_REPETITIONS);
//     break;
//   }
//   default:
//     break;
//   }
// }