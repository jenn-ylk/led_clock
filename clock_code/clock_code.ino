// arduino code base for the clock
//
// - keep 12 hour time
// - be able to set the time with buttons
// - modes for rings are:
//    - idle (no display, low power and keeps time)
//    - clock (LED rings as hands)
//    - lamp (LED ringlights)
//    - sound processing

#include <Adafruit_NeoPixel.h>

enum mode {
  CLOCK,
  LAMP,
  SPECTRUM,
  // SOUND,
  SLEEP,
  NUM_MODES
};

enum setting {
  SET,
  HOURS,
  MINUTES,
  NUM_SETTINGS
};

// - shift register pins
#define DATA_PIN  8
#define CLOCK_PIN 9
#define LATCH_PIN 10
// - digit pins
#define DIG1_PIN  2
#define DIG2_PIN  3
#define DIG3_PIN  4
#define DIG4_PIN  5
// - button pins
#define MODE_PIN  0
#define SET_PIN   A1
#define UP_PIN    A2
#define DOWN_PIN  A3

// - neopixels
#define PIX_PIN   6 
#define NUM_PIX   84
#define HRS_PIX   24
#define MINS_PIX  60
#define HRS_ZERO  0
#define MINS_ZERO 24

#define MIC_IN    A0 

// constants for timers
#define HOUR_SEC    (60 * 60)
#define MINUTE_SEC  (60)

// TODO: look into interrupts for the buttons

void disp_time(unsigned long time);
void disp_pixels(int hour, int minute, int disp_hour, int disp_minute);
void disp_hour(int hour);
void disp_minute(int minute);
void disp_digit(int digit, int pin);
unsigned long set_time();

void process_buttons();

unsigned long last_twelve;
// clock setting variables
int clock_setting = SET;
int prev_set_hour = 0;
int prev_set_minute = 0;
int set_hour = 0;
int set_minute = 0;
bool previous_set = false;

int cur_hour = set_hour;
int cur_minute = set_minute;
int clock_mode = CLOCK;
bool previous_mode = false;

bool previous_up = false;
bool previous_down = false;

Adafruit_NeoPixel pixels(NUM_PIX, PIX_PIN, NEO_GRB + NEO_KHZ800);
uint32_t clock_col = pixels.Color(255, 250, 185);
uint32_t lamp_col = pixels.Color(30, 0, 50);
uint32_t off_col = pixels.Color(0, 0, 0);

void setup() {
  Serial.begin(9600);
  // set the last twelve o'clock
  last_twelve = millis();
  // set up 7 segment display pins (digits + bit shifter)
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(DIG1_PIN, OUTPUT);
  pinMode(DIG2_PIN, OUTPUT);
  pinMode(DIG3_PIN, OUTPUT);
  pinMode(DIG4_PIN, OUTPUT);
  // button setup (pullup so a "low" means the button is pressed)
  pinMode(MODE_PIN, INPUT_PULLUP);
  pinMode(SET_PIN, INPUT_PULLUP);
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);

  // set up the neopixels
  pixels.begin();
  for (int i = 0; i < HRS_PIX; i++) {
    if (i != (cur_hour + (cur_minute >= (MINS_PIX / 2)))) {
      pixels.setPixelColor(HRS_ZERO + i, lamp_col);
      pixels.show();
    }
  }
  for (int i = 0; i < MINS_PIX; i++) {
    if (i != cur_minute) {
      pixels.setPixelColor(MINS_ZERO + i, lamp_col);
      pixels.show();
    }
  }
  
}

void loop() {
  // TODO: if yo uuse an RTC, then this is not an issue!
  // TODO: deal with time going past 12 hours, and also with overflow of millis
  // in case the clock was idling for a while (this isn't totally foolproof if millis() overflows twice but will be alright)
  unsigned long time = (millis() - last_twelve) / 1000;
  while (time / HOUR_SEC >= 12) {
    unsigned long add_hour = HOUR_SEC;
    unsigned long add_millis = add_hour * 1000 * 12;
    last_twelve += add_millis;
  }
  // TODO: microphone
  /*
  analogRead(MIC_IN);
  */
  
  // TODO: buttons, set up the clock time if needed, ensure this only happens with _separate_ presses
  bool mode = (digitalRead(MODE_PIN) == LOW);
  if (mode && !previous_mode) {
    // TODO: make the modes meaningful, go to sleep if it changes to sleep, otherwise, different lighting mode
    clock_mode = (clock_mode + 1) % NUM_MODES;
    
  }
  previous_mode = mode;

  bool set = (digitalRead(SET_PIN) == LOW);
  bool up = (digitalRead(UP_PIN) == LOW);
  bool down = (digitalRead(DOWN_PIN) == LOW);
  if (set && !previous_set) {
    // set last_twelve, or the starting numbers
    clock_setting = (clock_setting + 1) % NUM_SETTINGS;
    if (clock_setting == SET) {
      last_twelve = millis() - (set_hour * HOUR_SEC + set_minute * MINUTE_SEC) * 1000;
    } else if (clock_setting == HOURS) {
      set_hour = prev_set_hour = time / HOUR_SEC;
      if (set_hour == 0) set_hour = 12;
      set_minute = prev_set_minute = (time % HOUR_SEC) / MINUTE_SEC;
    }
  } else if (up && !previous_up) {
    if (clock_setting == HOURS) {
      set_hour++;
    } else if (clock_setting == MINUTES) {
      set_minute++;
      // deal with hour roll over
      set_hour += set_minute / 60;
      set_minute = set_minute % 60;
    }
    set_hour = set_hour % 12;
  } else if (down && !previous_down) {
    if (clock_setting == HOURS) {
      set_hour--;
    } else if (clock_setting == MINUTES) {
      set_minute--;
      while (set_minute < 0) {
        set_hour--;
        set_minute += 60;
      }
    }
    if (set_hour == 0) set_hour = 12;
  }
  previous_set = set;
  previous_up = up;
  previous_down = down;

  // TODO: neopixels
  // TODO: this is ugly, put it in a function
  disp_time(time);

}

void disp_time(unsigned long time) {
  if (clock_setting == HOURS) {
    Serial.println("Setting hour");
    Serial.print(set_hour);
    Serial.print(":");
    Serial.println(set_minute);
    disp_hour(set_hour);
    disp_minute(set_minute);
    disp_pixels(set_hour, set_minute, prev_set_hour, prev_set_minute);
  } else if (clock_setting == MINUTES) {
    Serial.println("Setting minute");
    Serial.print(set_hour);
    Serial.print(":");
    Serial.println(set_minute);
    disp_hour(set_hour);
    disp_minute(set_minute);
    disp_pixels(set_hour, set_minute, prev_set_hour, prev_set_minute);
  } else {
    int hour = time / HOUR_SEC;
    int minute = (time % HOUR_SEC) / MINUTE_SEC;
    if (hour == 0) hour = 12;
    Serial.print(hour);
    Serial.print(":");
    Serial.println(minute);
    disp_hour(hour);
    disp_minute(minute);
    disp_pixels(hour, minute, cur_hour, cur_minute);
  }
}

void disp_pixels(int hour, int minute, int disp_hour, int disp_minute) {
  if (minute != disp_minute) {
    pixels.setPixelColor(minute + MINS_ZERO, lamp_col);
    pixels.setPixelColor(disp_minute + MINS_ZERO, off_col);
    // change the hour counter over if onto the next half hour
    if ((disp_minute >= MINS_PIX / 2) ^ (minute >= MINS_PIX / 2)) {
      pixels.setPixelColor(minute + HRS_ZERO, lamp_col);
      pixels.setPixelColor(disp_minute + HRS_ZERO, off_col);
    }
    pixels.show();
    cur_hour = hour;
    cur_minute = minute;
  }
}

void disp_hour(int hour) {
  disp_digit(hour / 10, DIG1_PIN);
  disp_digit(hour % 10, DIG2_PIN);
}

void disp_minute(int minute) {
  disp_digit(minute / 10, DIG3_PIN);
  disp_digit(minute % 10, DIG4_PIN);
}

void disp_digit(int digit, int pin) {
  unsigned char digits[] = {
    0x02,
    0x9E,
    0x24,
    0x0C,
    0x98,
    0x48,
    0x40,
    0x1E,
    0x00,
    0x08    
  };
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, digits[digit]);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(pin, HIGH);
  delay(2);
  digitalWrite(pin, LOW);
}
