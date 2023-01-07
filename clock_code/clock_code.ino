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
#include <RTClib.h>
#include <avr/sleep.h>

enum mode {
  CLOCK,
  LAMP,
  SPECTRUM,
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
#define DIG1_PIN  3
#define DIG2_PIN  4
#define DIG3_PIN  5
#define DIG4_PIN  6
// - button pins
#define MODE_PIN  2
#define SET_PIN   A1
#define UP_PIN    A2
#define DOWN_PIN  A3

// - neopixels
#define PIX_PIN   7 
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

void disp_time();
void disp_pixels(int hour, int minute, int disp_hour, int disp_minute);
void disp_hour(int hour);
void disp_minute(int minute);
void disp_digit(int digit, int pin);
unsigned long set_time();

void switch_wake();

RTC_DS1307 rtc;
DateTime now;
// clock setting variables
int clock_setting = SET;
int prev_set_hour = 0;
int prev_set_minute = 0;
int set_hour = 12;
int set_minute = 0;
bool previous_set = false;

int cur_hour = set_hour;
int cur_minute = set_minute;
int clock_mode = CLOCK;
bool previous_mode = false;

bool previous_up = false;
bool previous_down = false;
bool switched = false;

Adafruit_NeoPixel pixels(NUM_PIX, PIX_PIN, NEO_GRB + NEO_KHZ800);

uint32_t clock_col = pixels.Color(10, 0, 25);
uint32_t lamp_col = pixels.Color(200, 150, 60);
uint32_t off_col = pixels.Color(0, 0, 0);

void setup() {
  Serial.begin(9600);
  
  if (!rtc.begin()) {
    Serial.println("Didn't find RTC");
    Serial.flush();
    while (1) delay(1000);
  }
  if (!rtc.isrunning()) {
    // start at compile/push time
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
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
  attachInterrupt(digitalPinToInterrupt(MODE_PIN), switch_wake, FALLING);
  pinMode(SET_PIN, INPUT_PULLUP);
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);

  // set up the neopixels
  pixels.begin();
  for (int i = 0; i < HRS_PIX / 2; i++) {
    if (i != (cur_hour % 12)) {
      if (clock_mode == CLOCK) {
        pixels.setPixelColor(HRS_ZERO + i * 2, clock_col);
        pixels.setPixelColor(HRS_ZERO + i * 2 + 1, clock_col);
      } else if (clock_mode == LAMP) {
        pixels.setPixelColor(HRS_ZERO + i * 2, lamp_col);
        pixels.setPixelColor(HRS_ZERO + i * 2 + 1, lamp_col);
      } else if (clock_mode == SPECTRUM) {
        Serial.print("aaa");
      }
      pixels.show();
    }
  }
  for (int i = 0; i < MINS_PIX; i++) {
    if (i != cur_minute) {
      if (clock_mode == CLOCK) {
        pixels.setPixelColor(MINS_ZERO + i, clock_col);
      } else if (clock_mode == LAMP) {
        pixels.setPixelColor(MINS_ZERO + i, lamp_col);
      } else if (clock_mode == SPECTRUM) {
        Serial.print("aaa");
      }
      pixels.show();
    }
  }
  
}

void loop() {
  now = rtc.now();
  // neopixels and 7 seg
  disp_time();

  bool set = (digitalRead(SET_PIN) == LOW);
  bool up = (digitalRead(UP_PIN) == LOW);
  bool down = (digitalRead(DOWN_PIN) == LOW);
  if (set && !previous_set) {
    // set last_twelve, or the starting numbers
    clock_setting = (clock_setting + 1) % NUM_SETTINGS;
    if (clock_setting == SET) {
      // Serial.println("setting time");
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), set_hour, set_minute, 0));
      cur_hour = set_hour;
      cur_minute = set_minute;
    } else if (clock_setting == HOURS) {
      set_hour = prev_set_hour = now.hour();
      if (set_hour == 0) set_hour = 12;
      set_minute = prev_set_minute = now.minute();
    }
  } else if (up && !previous_up) {
    if (clock_setting == HOURS) {
      prev_set_hour = set_hour;
      set_hour++;
    } else if (clock_setting == MINUTES) {
      prev_set_minute = set_minute;
      set_minute++;
      // deal with hour roll over
      if (set_minute >= 60) prev_set_hour = set_hour;
      set_hour += set_minute / 60;
      set_minute = set_minute % 60;
    }
    set_hour = set_hour % 12;
  } else if (down && !previous_down) {
    if (clock_setting == HOURS) {
      prev_set_hour = set_hour;
      set_hour--;
    } else if (clock_setting == MINUTES) {
      prev_set_minute = set_minute;
      set_minute--;
      while (set_minute < 0) {
        prev_set_hour = set_hour;
        set_hour--;
        set_minute += 60;
      }
    }
  }
  if (set_hour == 0) set_hour = 12;
  previous_set = set;
  previous_up = up;
  previous_down = down;

}

void disp_time() {
  if (clock_setting == HOURS) {
    /*
    Serial.println("Setting hour");
    Serial.print(set_hour);
    Serial.print(":");
    Serial.println(set_minute);
    */
    if ((millis() / 200) % 2) disp_hour(set_hour);
    disp_minute(set_minute);
    disp_pixels(set_hour, set_minute, prev_set_hour, prev_set_minute);
  } else if (clock_setting == MINUTES) {
    /*
    Serial.println("Setting minute");
    Serial.print(set_hour);
    Serial.print(":");
    Serial.println(set_minute);
    */
    disp_hour(set_hour);
    if ((millis() / 200) % 2) disp_minute(set_minute);
    disp_pixels(set_hour, set_minute, prev_set_hour, prev_set_minute);
  } else {
    // TODO: should there be a pm marker?
    int hour = (int) now.hour() % 12;
    int minute = (int) now.minute();
    if (hour == 0) hour = 12;
    /*
    Serial.print(hour);
    Serial.print(":");
    Serial.println(minute);
    */
    disp_hour(hour);
    disp_minute(minute);
    disp_pixels(hour, minute, cur_hour, cur_minute);
  }
}

void disp_pixels(int hour, int minute, int disp_hour, int disp_minute) {
  if (switched) {}
  else {
    if (minute != disp_minute) {
      // Serial.println(minute, DEC);
      // Serial.println(disp_minute, DEC);
      pixels.setPixelColor(minute + MINS_ZERO, off_col);
      if (clock_mode == CLOCK) {
        pixels.setPixelColor(disp_minute + MINS_ZERO, clock_col);
      } else if (clock_mode == LAMP) {
        pixels.setPixelColor(disp_minute + MINS_ZERO, lamp_col);
      } else if (clock_mode == SPECTRUM) {
        Serial.print("aaa");
      }
      // change the hour counter over if onto the next half hour
      // if ((disp_minute >= MINS_PIX / 2) ^ (minute >= MINS_PIX / 2)) {
    }
    if (hour != disp_hour) {
      pixels.setPixelColor((hour % 12) * 2 + HRS_ZERO, off_col);
      pixels.setPixelColor((hour % 12) * 2 + 1 + HRS_ZERO, off_col);
      if (clock_mode == CLOCK) {
        pixels.setPixelColor((disp_hour % 12) * 2 + HRS_ZERO, clock_col);
        pixels.setPixelColor((disp_hour % 12) * 2 + 1 + HRS_ZERO, clock_col);
      } else if (clock_mode == LAMP) {
        pixels.setPixelColor((disp_hour % 12) * 2 + HRS_ZERO, lamp_col);
        pixels.setPixelColor((disp_hour % 12) * 2 + 1 + HRS_ZERO, lamp_col);
      } else if (clock_mode == SPECTRUM) {
        Serial.print("aaa");
      }
    }
  }
  
  if (hour != disp_hour || minute != disp_minute) {
    pixels.show();
    cur_hour = hour;
    cur_minute = minute;
  }

  switched = false;
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

void switch_wake() {
  // TODO:
  clock_mode = (clock_mode + 1) % NUM_MODES;
  Serial.print("Clock mode:");
  Serial.println(clock_mode);
  // if (clock_mode == SLEEP) {
  //   sleep_enable();
  //   set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  //   sleep_cpu();
  // } else {
  //   sleep_disable();
  //   switched = true;
  // }
}
