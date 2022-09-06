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
  SOUND,
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

#define MIC_IN    A0 

// constants for timers
#define HOUR_SEC    (60 * 60)
#define MINUTE_SEC  (60)

// TODO: look into interrupts for the buttons

void disp_digit(int digit, int pin);
void disp_hour(int hour);
void disp_minute(int minute);
unsigned long set_time();

unsigned long last_twelve;
// clock setting variables
int clock_setting = SET;
int set_hour = 0;
int set_minute = 0;
bool previous_set = false;

int clock_mode = CLOCK;
bool previous_mode = false;

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
  // TODO: set up the neopixels
}

void loop() {
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
  // TODO: neopixels
  // TODO: buttons
  
  // TODO: this is ugly, put it in a function
  if (clock_setting == HOURS) {
    Serial.println("Setting hour");
    Serial.print(set_hour);
    Serial.print(":");
    Serial.println(set_minute);
    disp_hour(set_hour);
    disp_minute(set_minute);
  } else if (clock_setting == MINUTES) {
    Serial.println("Setting minute");
    Serial.print(set_hour);
    Serial.print(":");
    Serial.println(set_minute);
    disp_hour(set_hour);
    disp_minute(set_minute);
  } else {
    int hour = time / HOUR_SEC;
    if (hour == 0) hour = 12;
    Serial.print(hour);
    Serial.print(":");
    Serial.println((time % HOUR_SEC) / MINUTE_SEC);
    disp_hour(hour);
    disp_minute((time % HOUR_SEC) / MINUTE_SEC);
  }

  // TODO: set up the clock time if needed, ensure this only happens with _separate_ presses, not holds
  bool set = (digitalRead(SET_PIN) == LOW);
  if (set && !previous_set) {
    // set last_twelve, or the starting numbers
    clock_setting = (clock_setting + 1) % NUM_SETTINGS;
    if (clock_setting == SET) {
      last_twelve = millis() - (set_hour * HOUR_SEC + set_minute * MINUTE_SEC) * 1000;
    } else if (clock_setting == HOURS) {
      set_hour = time / HOUR_SEC;
      if (set_hour == 0) set_hour = 12;
      set_minute = (time % HOUR_SEC) / MINUTE_SEC;
    }
  }
  previous_set = set;

  bool mode = (digitalRead(MODE_PIN) == LOW);
  if (mode && !previous_mode) {
    // TODO: make the modes meaningful, go to sleep if it changes to sleep, otherwise, different lighting mode
    clock_mode = (clock_mode + 1) % NUM_MODES;
    if (clock_setting == SET) {
      last_twelve = millis() - (set_hour * HOUR_SEC + set_minute * MINUTE_SEC) * 1000;
    } else if (clock_setting == HOURS) {
      set_hour = time / HOUR_SEC;
      set_minute = (time % HOUR_SEC) / MINUTE_SEC;
    }
  }
  previous_mode = mode;

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

void disp_hour(int hour) {
  disp_digit(hour / 10, DIG1_PIN);
  disp_digit(hour % 10, DIG2_PIN);
}

void disp_minute(int minute) {
  disp_digit(minute / 10, DIG3_PIN);
  disp_digit(minute % 10, DIG4_PIN);
}
