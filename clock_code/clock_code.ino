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
#define DIG1_PIN  1
#define DIG2_PIN  2
#define DIG3_PIN  3
#define DIG4_PIN  4
// - button pins
#define MODE_PIN  0
#define SET_PIN   5
#define UP_PIN    6
#define DOWN_PIN  7

// - neopixels
#define PIX_PIN   A1 
#define NUM_PIX   84

#define MIC_IN    A0 

// constants for timers
#define HOUR_SEC    (60 * 60)
#define MINUTE_SEC  (60)

// TODO: look into interrupts for the buttons

void disp_hour(int hour);
void disp_minute(int minute);
unsigned long set_time();

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
  0x18
};

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
  // TODO: microphone
  /*
  analogRead(MIC_IN);
  */
  // TODO: neopixels
  // TODO: buttons

  unsigned long time = (millis() - last_twelve) / 1000;
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
    Serial.print(time / HOUR_SEC);
    Serial.print(":");
    Serial.println((time % HOUR_SEC) / MINUTE_SEC);
    disp_hour(time / HOUR_SEC);
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
  previous_set = set;

}


void disp_hour(int hour) {
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, digits[hour / 10]);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(DIG1_PIN, HIGH);
  delay(50);
  digitalWrite(DIG1_PIN, LOW);

  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, digits[hour % 10]);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(DIG2_PIN, HIGH);
  delay(50);
  digitalWrite(DIG2_PIN, LOW);
}

void disp_minute(int minute) {
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, digits[minute / 10]);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(DIG3_PIN, HIGH);
  delay(50);
  digitalWrite(DIG3_PIN, LOW);

  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, digits[minute % 10]);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(DIG4_PIN, HIGH);
  delay(50);
  digitalWrite(DIG4_PIN, LOW);
}
