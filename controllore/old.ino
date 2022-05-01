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

#define DELAY 300

#define I2C
#ifdef I2C
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);
#else
#include <LiquidCrystal.h>
LiquidCrystal lcd(0, 0, 0, 0, 0, 0);
#endif

float frequency, period, period_on; // Hertz
float duty_cycle;
float repetitions;
double start_time, current_time;

unsigned int decimals = 0, units = 0, tens = 0, hundreds = 0;
bool buttons[4] = {false};
float buttons_timer_start[4] = {0.0f};
float buttons_timer_pressed[4] = {0.0f};

bool need_render = true;

void input();
void manage_state();
void render();

enum STATE
{
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

inline void create_from_digits(float *num)
{
    *num = 100 * hundreds + 10 * tens + units + 0.1f * decimals;
    need_render = true;
}

#define PRESSED_THRESHOLD 0.1 * SCALE
#define CHECK_BTN(X) (buttons_timer_pressed[X] - buttons_timer_start[X] > PRESSED_THRESHOLD)

inline bool compare_buttons(bool b0, bool b1, bool b2, bool b3)
{
    return (buttons[0] == b0 && buttons[1] == b1 && buttons[2] == b2 && buttons[3] == b3);
    // return (CHECK_BTN(0) == b0 && CHECK_BTN(1) == b1 && CHECK_BTN(2) == b2 && CHECK_BTN(3) == b3);
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
    delay(DELAY);
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

inline void change_state(STATE state)
{
    need_render = true;
    current_state = state;
    delay(DELAY);
}

void setup()
{
    frequency = 1.0f;
    duty_cycle = 0.5f;
    repetitions = 10;
    lcd.begin(20, 4);
    start_time = GET_TIME();

    pinMode(OUTPUT_PIN, OUTPUT);
    pinMode(PIN_RIGHT, INPUT);
    digitalWrite(OUTPUT_PIN, LOW);
}

void loop()
{
    // manage input
    // TODO check if some waiting time is needed for multiple buttons handling
    input();

    // manage state
    manage_state();

    // render to display
    render();
}

void input()
{
    if (digitalRead(PIN_UP) == HIGH && !buttons[0])
    {
        buttons[0] = true;
        buttons_timer_start[0] = GET_TIME();
    }
    else if (digitalRead(PIN_UP) == HIGH && buttons[0])
    {
        buttons_timer_pressed[0] = GET_TIME();
    }
    else
    {
        buttons[0] = false;
        buttons_timer_start[0] = buttons_timer_pressed[0] = 0.0f;
    }
    if (digitalRead(PIN_DOWN) == HIGH && !buttons[1])
    {
        buttons[1] = true;
        buttons_timer_start[1] = GET_TIME();
    }
    else if (digitalRead(PIN_DOWN) == HIGH && buttons[1])
    {
        buttons_timer_pressed[1] = GET_TIME();
    }
    else
    {
        buttons[1] = false;
        buttons_timer_start[1] = buttons_timer_pressed[1] = 0.0f;
    }
    if (digitalRead(PIN_LEFT) == HIGH && !buttons[2])
    {
        buttons[2] = true;
        buttons_timer_start[2] = GET_TIME();
    }
    else if (digitalRead(PIN_LEFT) == HIGH && buttons[2])
    {
        buttons_timer_pressed[2] = GET_TIME();
    }
    else
    {
        buttons[2] = false;
        buttons_timer_start[2] = buttons_timer_pressed[2] = 0.0f;
    }
    if (digitalRead(PIN_RIGHT) == HIGH && !buttons[3])
    {
        buttons[3] = true;
        buttons_timer_start[3] = GET_TIME();
    }
    else if (digitalRead(PIN_RIGHT) == HIGH && buttons[3])
    {
        buttons_timer_pressed[3] = GET_TIME();
    }
    else
    {
        buttons[3] = false;
        buttons_timer_start[3] = buttons_timer_pressed[3] = 0.0f;
    }
}
void manage_state()
{
    switch (current_state)
    {
    case STOP:
    {
        if (compare_buttons(false, false, false, true))
        {
            change_state(PLAY);
        }
        else if (compare_buttons(false, true, false, false))
        {
            change_state(MENU_FREQ);
        }
        break;
    }
    case PLAY:
    {
        period = 1.0f / frequency * SCALE;
        period_on = period * duty_cycle;
        float loop_repetitions = repetitions;
        while (loop_repetitions > 0)
        {
            current_time = GET_TIME();
            double elapsed_time = current_time - start_time;
            if (elapsed_time < period_on)
                digitalWrite(OUTPUT_PIN, HIGH);
            else if (elapsed_time < period)
                digitalWrite(OUTPUT_PIN, LOW);
            else
            {
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
        {
            change_digit(&hundreds, false);
            create_from_digits(&frequency);
        }
        else if (compare_buttons(false, true, false, false))
        {
            change_digit(&hundreds, true);
            create_from_digits(&frequency);
        }
        else if (compare_buttons(false, false, true, false))
            change_state(MENU_FREQ);
        else if (compare_buttons(false, false, false, true))
            change_state(MENU_FREQ_1);
        break;
    }
    case MENU_FREQ_1:
    {
        if (compare_buttons(true, false, false, false))
        {
            change_digit(&tens, false);
            create_from_digits(&frequency);
        }
        else if (compare_buttons(false, true, false, false))
        {
            change_digit(&tens, true);
            create_from_digits(&frequency);
        }
        else if (compare_buttons(false, false, true, false))
            change_state(MENU_FREQ_0);
        else if (compare_buttons(false, false, false, true))
            change_state(MENU_FREQ_2);
        break;
    }
    case MENU_FREQ_2:
    {
        if (compare_buttons(true, false, false, false))
        {
            change_digit(&units, false);
            create_from_digits(&frequency);
        }
        else if (compare_buttons(false, true, false, false))
        {
            change_digit(&units, true);
            create_from_digits(&frequency);
        }
        else if (compare_buttons(false, false, true, false))
            change_state(MENU_FREQ_1);
        else if (compare_buttons(false, false, false, true))
            change_state(MENU_FREQ_3);
        break;
    }
    case MENU_FREQ_3:
    {
        if (compare_buttons(true, false, false, false))
        {
            change_digit(&decimals, false);
            create_from_digits(&frequency);
        }
        else if (compare_buttons(false, true, false, false))
        {
            change_digit(&decimals, true);
            create_from_digits(&frequency);
        }
        else if (compare_buttons(false, false, true, false))
            change_state(MENU_FREQ_2);
        else if (compare_buttons(false, false, false, true))
            change_state(MENU_FREQ);
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
        {
            change_digit(&hundreds, false);
            create_from_digits(&duty_cycle);
        }
        else if (compare_buttons(false, true, false, false))
        {
            change_digit(&hundreds, true);
            create_from_digits(&duty_cycle);
        }
        else if (compare_buttons(false, false, true, false))
            change_state(MENU_DUTY_CYCLE);
        else if (compare_buttons(false, false, false, true))
            change_state(MENU_DUTY_CYCLE_1);
        break;
    }
    case MENU_DUTY_CYCLE_1:
    {
        if (compare_buttons(true, false, false, false))
        {
            change_digit(&tens, false);
            create_from_digits(&duty_cycle);
        }
        else if (compare_buttons(false, true, false, false))
        {
            change_digit(&tens, true);
            create_from_digits(&duty_cycle);
        }
        else if (compare_buttons(false, false, true, false))
            change_state(MENU_DUTY_CYCLE_0);
        else if (compare_buttons(false, false, false, true))
            change_state(MENU_DUTY_CYCLE_2);
        break;
    }
    case MENU_DUTY_CYCLE_2:
    {
        if (compare_buttons(true, false, false, false))
        {
            change_digit(&units, false);
            create_from_digits(&duty_cycle);
        }
        else if (compare_buttons(false, true, false, false))
        {
            change_digit(&units, true);
            create_from_digits(&duty_cycle);
        }
        else if (compare_buttons(false, false, true, false))
            change_state(MENU_DUTY_CYCLE_1);
        else if (compare_buttons(false, false, false, true))
            change_state(MENU_DUTY_CYCLE_3);
        break;
    }
    case MENU_DUTY_CYCLE_3:
    {
        if (compare_buttons(true, false, false, false))
        {
            change_digit(&decimals, false);
            create_from_digits(&duty_cycle);
        }
        else if (compare_buttons(false, true, false, false))
        {
            change_digit(&decimals, true);
            create_from_digits(&duty_cycle);
        }
        else if (compare_buttons(false, false, true, false))
            change_state(MENU_DUTY_CYCLE_2);
        else if (compare_buttons(false, false, false, true))
            change_state(MENU_DUTY_CYCLE);
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
        {
            change_digit(&hundreds, false);
            create_from_digits(&repetitions);
        }
        else if (compare_buttons(false, true, false, false))
        {
            change_digit(&hundreds, true);
            create_from_digits(&repetitions);
        }
        else if (compare_buttons(false, false, true, false))
            change_state(MENU_REPETITIONS);
        else if (compare_buttons(false, false, false, true))
            change_state(MENU_REPETITIONS_1);
        break;
    }
    case MENU_REPETITIONS_1:
    {
        if (compare_buttons(true, false, false, false))
        {
            change_digit(&tens, false);
            create_from_digits(&repetitions);
        }
        else if (compare_buttons(false, true, false, false))
        {
            change_digit(&tens, true);
            create_from_digits(&repetitions);
        }
        else if (compare_buttons(false, false, true, false))
            change_state(MENU_REPETITIONS_0);
        else if (compare_buttons(false, false, false, true))
            change_state(MENU_REPETITIONS_2);
        break;
    }
    case MENU_REPETITIONS_2:
    {
        if (compare_buttons(true, false, false, false))
        {
            change_digit(&units, false);
            create_from_digits(&repetitions);
        }
        else if (compare_buttons(false, true, false, false))
        {
            change_digit(&units, true);
            create_from_digits(&repetitions);
        }
        else if (compare_buttons(false, false, true, false))
            change_state(MENU_REPETITIONS_1);
        else if (compare_buttons(false, false, false, true))
            change_state(MENU_REPETITIONS_3);
        break;
    }
    case MENU_REPETITIONS_3:
    {
        if (compare_buttons(true, false, false, false))
        {
            change_digit(&decimals, false);
            create_from_digits(&repetitions);
        }
        else if (compare_buttons(false, true, false, false))
        {
            change_digit(&decimals, true);
            create_from_digits(&repetitions);
        }
        else if (compare_buttons(false, false, true, false))
            change_state(MENU_REPETITIONS_2);
        else if (compare_buttons(false, false, false, true))
            change_state(MENU_REPETITIONS);
        break;
    }
    default:
        break;
    }
}

void render()
{
    if (need_render)
    {
        lcd.clear();
        switch (current_state)
        {
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
            render_digits(frequency);
            break;
        }
        case MENU_FREQ_0:
        {
            lcd.print("Frequency");
            render_digits(frequency);
            lcd.setCursor(0, 3);
            lcd.print("_");
            break;
        }
        case MENU_FREQ_1:
        {
            lcd.print("Frequency");
            render_digits(frequency);
            lcd.setCursor(1, 3);
            lcd.print("_");
            break;
        }
        case MENU_FREQ_2:
        {
            lcd.print("Frequency");
            render_digits(frequency);
            lcd.setCursor(2, 3);
            lcd.print("_");
            break;
        }
        case MENU_FREQ_3:
        {
            lcd.print("Frequency");
            render_digits(frequency);
            lcd.setCursor(4, 3);
            lcd.print("_");
            break;
        }
        case MENU_DUTY_CYCLE:
        {
            lcd.print("Duty Cycle");
            render_digits(duty_cycle);
            break;
        }
        case MENU_DUTY_CYCLE_0:
        {
            lcd.print("Duty Cycle");
            render_digits(duty_cycle);
            lcd.setCursor(0, 3);
            lcd.print("_");
            break;
        }
        case MENU_DUTY_CYCLE_1:
        {
            lcd.print("Duty Cycle");
            render_digits(duty_cycle);
            lcd.setCursor(1, 3);
            lcd.print("_");
            break;
        }
        case MENU_DUTY_CYCLE_2:
        {
            lcd.print("Duty Cycle");
            render_digits(duty_cycle);
            lcd.setCursor(2, 3);
            lcd.print("_");
            break;
        }
        case MENU_DUTY_CYCLE_3:
        {
            lcd.print("Duty Cycle");
            render_digits(duty_cycle);
            lcd.setCursor(4, 3);
            lcd.print("_");
            break;
        }
        case MENU_REPETITIONS:
        {
            lcd.print("Repetitions");
            render_digits(repetitions);
            break;
        }
        case MENU_REPETITIONS_0:
        {
            lcd.print("Repetitions");
            render_digits(repetitions);
            lcd.setCursor(0, 3);
            lcd.print("_");
            break;
        }
        case MENU_REPETITIONS_1:
        {
            lcd.print("Repetitions");
            render_digits(repetitions);
            lcd.setCursor(1, 3);
            lcd.print("_");
            break;
        }
        case MENU_REPETITIONS_2:
        {
            lcd.print("Repetitions");
            render_digits(repetitions);
            lcd.setCursor(2, 3);
            lcd.print("_");
            break;
        }
        case MENU_REPETITIONS_3:
        {
            lcd.print("Repetitions");
            render_digits(repetitions);
            lcd.setCursor(4, 3);
            lcd.print("_");
            break;
        }

        default:
            break;
        }
        need_render = false;
    }
}
