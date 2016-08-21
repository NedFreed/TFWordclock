/*
   WORD CLOCK - 8x8 For use with Tempus Fugit board
   By David Saul https://meanderingpi.wordpress.com/

   GPS modifications by Ned Freed, ned.freed@mrochek.com, August 2016

   Release Version 1 - previuos development version  = rev 6 
   
   Inspired by the work of Andy Doro - NeoPixel WordClock from the Adafruit website

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   Software:
   This code requires the following external libraries:
   - RTClib https://github.com/adafruit/RTClib
   - LedControl https://github.com/wayoda/LedControl/releases

   GPS support requires the TinyGPS library by Mikal Hart:
   - TinyGPS http://arduiniana.org/libraries/TinyGPS/

   Arduino pin usage

   D0   - USB serial TX
   D1   - USB serial RX
   D2   - LDR source voltage
   D3   - Debug output
   D4   - GPS Tx
   D5   - GPS Rx
   D6
   D7   - Button 3 [down]
   D8   - Button 2 [up]
   D9   - Button 1 [select]
   D10  - LedControl (Max 7219) SPI LOAD
   D11  - LedControl (Max 7219) SPI CLOCK 
   D12  - LedControl (Max 7219) SPI DATA
   D13  - (onboard) LED output
   A0
   A1
   A2
   A3
   A4   - I2C SDA
   A5   - I2C SCL
   A6
   A7   - LDR sense

   We have only a single MAX72XX.

   Most pin assignments can be changed by editing the constants in setup.h.

   Wiring:


   This code is designed to work with the Tempus Fugit board - a simple wordclock for Arduino and the PiZero
   - add Github ref in here

   grid pattern

    H A T W E N T Y
    F I F V T E E N
    L F * P A S T O
    N I N E I G H T
    O N E T H R E E
    T W E L E V E N
    F O U R F I V E
    S I X S E V E N
*/

#define INCLUDE_DSTINFO

#include "setup.h"

// include the library code:
#include <Wire.h>
#include <RTClib.h>
#include <LedControl.h>
#include <EEPROM.h>
#ifdef __GPS
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#endif

LedControl lc = LedControl(12, 11, 10, 1);

RTC_DS1307 RTC; // Establish clock object
DateTime theTime; // Holds current clock time

const int brlim_add = 4; // EEPROM address for stored bright lim figure

int bright = 0;   // global variable for Max7219 brightness
int raw_bright = 0; // raw LDR value
int raw_bright_old = 1023; // old raw value used for averaging
int uncor = 0;
int c_flag = false;
int bright_lim = 15;  // brightness range limiting variable

// working hour and minute global variables
int min_x = 0;
int hour_x = 0;

#ifdef __GPS

TinyGPS gps;
SoftwareSerial ss(GPSRx, GPSTx);
byte lasthour = 25;
byte lastsecond = 70;
static bool isDST = false;          // Currently in DST
static long int lastuj = 0;
static int doDST = 0;               // DST transition pending today

#endif

void setup() {
  // Setup code

  //Serial for debugging
  Serial.begin(9600);

  Serial.println("Tempus Fugit WordClock Application ENGLISH");
  Serial.println();

  // initialize the pushbutton pins as an inputs:
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(button3, INPUT);

  // initialize the board LED pin as an output - for seconds indicator
  pinMode(ledPin, OUTPUT);

  // initialze LDR +5 drive
  pinMode(LDR_drv, OUTPUT);    // this is the LDR drive voltage
  digitalWrite(LDR_drv, HIGH);

  // pinMode(3,OUTPUT);    // for debug connect LED to D3 to check operation

  /*
    Setup the MX7219
    The MAX7219 is in power-saving mode on startup we have to do a wakeup call
  */
  lc.shutdown(0, false);
  /* Set the brightness to a max values - for green display */
  lc.setIntensity(0, 15);
  /* and clear the display */
  lc.clearDisplay(0);

  // start clock
  Wire.begin();  // Begin I2C
  RTC.begin();   // begin clock


  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running; set RTC to compile time");
    // display Error message on display
    lc.setColumn(0, 2, B00101010); // display E E E
    delay(1000);

    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
    Serial.println("RTC Time set.");
  }

  // display INIT for 1 second
  // this is the window to force a RTC time reset
  lc.setColumn(0, 4, B01101001); // display INIT
  delay(1500);

  // If buttons 2 & 3 held down at power up for approx 5 seconds
  // attempt to set clock to original compile time
  if (digitalRead(button2) == LOW && digitalRead(button3) == LOW) {
    int count = 0;
    lc.clearDisplay(0);   // clear display
    // wait until the 2 buttons have been held for approx 5 seconds
    // before reseting the time
    // display diagnol line
    for (count = 0; count < 8; count++)
    {
      lc.setRow(0, count, 1 << count);
      delay(500);
      if (digitalRead(button2) == HIGH || digitalRead(button3) == HIGH) {
        Serial.println("RTC update ABORTED.");
        Serial.println();
        count = 9999;     // force count to invalid number
      }
    }
    // if button held down for complete time update RTC
    if (count == 8) {
      RTC.adjust(DateTime(__DATE__, __TIME__));
      Serial.println("RTC Time reset to compile time.");
      lc.clearDisplay(0);   // clear display
      // wait for buttons to be released
      while (digitalRead(button2) == LOW || digitalRead(button3) == LOW)
        delay(20);

    }
  }

  // Display startup sequence

  bright_lim = EEPROM.read(brlim_add);
#ifdef __BRIGHTDEBUG
  Serial.print("Stored bright limit ");
  Serial.println(bright_lim);
#endif
  if (bright_lim > 14)     // check to if EEPROM has been written prviously
  {
    bright_lim = 15;       // if not force to 15
  }
  adjustBrightness();      // set initial display brightness

  st_display();            // display rolling screen

#ifdef __GPS
  ss.begin(GPSBaudRate);   // Wait until after display is stable to start GPS
#endif

  Serial.println("Initialisation complete, clock running");
  Serial.println();
  Serial.println();

}

  // serial print theTime variable - for debug

void PrintTime(void) {
  Serial.print(theTime.year(), DEC);
  Serial.print('/');
  Serial.print(theTime.month(), DEC);
  Serial.print('/');
  Serial.print(theTime.day(), DEC);
  Serial.print(' ');
  Serial.print(theTime.hour(), DEC);
  Serial.print(':');
  Serial.print(theTime.minute(), DEC);
  Serial.print(':');
  Serial.print(theTime.second(), DEC);
  Serial.print(' ');
}

#ifdef __GPS

// Modified version of CACM Algorithm 199, returns days since January 1, 1970

static long int
modjulday(int year, int month, int day)
{
  long int c, ya;

  if (month > 2)
      month -= 3;
  else {
    month += 9;
    year--;
  }
  c = year / 100;
  ya = year - c * 100;
  return c * 146097L / 4 + ya * 1461L / 4 + (month * 153L + 2L) / 5L + day +
         (1721119L - 2440588L);
}

// Quick and dirty version of mktime

static unsigned long int 
modmktime(int year, byte month, byte day, byte hour, byte minute, byte second)
{
  return modjulday(year, month, day) * (24L * 60L * 60L) +
         hour * 60L * 60L + minute * 60L + second;
}

// Inverse conversions

static void
modjuldate(unsigned long int j, int *year, byte *month, byte *day)
{
  long int y, m, d;

  j -= (1721119L - 2440588L);
  y = (j * 4 - 1) / 146097L;
  j = j * 4 - y * 146097L - 1;
  d = j / 4;
  j = (d * 4 + 3) / 1461L;
  d = d * 4 - j * 1461L + 3;
  d = (d + 4) / 4;
  m = (d * 5 - 3) / 153L;
  d = d * 5 - m * 153L - 3;
  *day = (byte)((d + 5) / 5);
  *year = (int)(y * 100 + j);
  if (m < 10)
    *month = (byte)(m + 3);
  else {
    *month = (byte)(m - 9);
    *year += 1;
  }
}

// Quick and dirty version of gmtime

static void
modgmtime(unsigned long int t, int *year, byte *month, byte *day,
          byte *hour, byte *minute, byte *second)
{
  modjuldate(t / 86400L, year, month, day);
  *hour = (byte)(t / 3600L % 24);
  *minute = (byte)(t / 60L % 60); 
  *second = (byte)(t % 60);
}

void delayprocessGPS(int ms) {
reread:
  while (ss.available()) {
    char c = ss.read();
    // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
    if (gps.encode(c)) { // Did a new valid sentence come in?
      int year;
      byte month, day, hour, minute, second;
      unsigned long int now;
      long int uj;
      
      gps.crack_datetime(&year, &month, &day, &hour, &minute, &second);
      // On startup TinyGPS produces some invalid stuff
      if (year < 2016)
        continue;
      // Once per second max
      if (lastsecond == second)
        continue;
      lastsecond = second;
      // Toggle the LED - this now acts as an indicator that GPS is working
      digitalWrite(ledPin, !digitalRead(ledPin)); // toggle led https://www.baldengineer.com/arduino-toggling-outputs.html
      now = modmktime(year, month, day, hour, minute, second);
      now += (TIME_OFFSET + (isDST ? DST_OFFSET : 0)) * 60L;
      modgmtime(now, &year, &month, &day, &hour, &minute, &second);
      uj = now / 86400L;
      if (uj != lastuj) {
        int j;
        // Day change, DST checks needed
        for (j = 0; j < DSTLENGTH - 1 && uj >= DSTINFO(j); j++);
        if ( --j >= 0 ) {
#ifdef __GPS_DEBUG
          Serial.print("DST table i/v ");
          Serial.print(j, DEC);
          Serial.print('/');
          Serial.println(DSTINFO(j));
#endif
          if (25 == lasthour) {
            // First GPS sets DST indicator
            int jj = j;
            if (uj == DSTINFO(j) && hour < DST_TRANSITION_TIME) jj++;
            isDST = (jj % 2) == 0;
            if (isDST) {
              now +=  DST_OFFSET * 60;
              modgmtime(now, &year, &month, &day, &hour, &minute, &second);
            }
          }
          if ( uj == DSTINFO(j) && hour < DST_TRANSITION_TIME ) {
            // Today is the day, set things up to do DST switch
            doDST = (j % 2) ? -1 : 1;
#ifdef __GPS_DEBUG
            Serial.print("doDST ");
            Serial.println(doDST, DEC);
#endif
          }
        }
        lastuj = uj;
      }
      if (lasthour != hour) {
        if (doDST != 0 && hour >= DST_TRANSITION_TIME) {
          // Time to actually do the DST transition
          Serial.println("*DST*");
          if (isDST != (doDST > 0)) {
            if (isDST)
              now -=  DST_OFFSET * 60;
            else
              now += DST_OFFSET * 60;
            modgmtime(now, &year, &month, &day, &hour, &minute, &second);
          }
          isDST = doDST > 0;
          doDST = 0;
        }
#ifdef __GPS_DEBUG
        Serial.print("Adjusted GPS time ");
        Serial.print(year, DEC);
        Serial.print('/');
        Serial.print(month, DEC);
        Serial.print('/');
        Serial.print(day, DEC);
        Serial.print(' ');
        Serial.print(hour, DEC);
        Serial.print(':');
        Serial.print(minute, DEC);
        Serial.print(':');
        Serial.print(second, DEC);
        Serial.print(' ');
        Serial.print(now);
        Serial.print(' ');
        Serial.println(isDST, DEC);
#endif
        RTC.adjust(DateTime(year, month, day, hour, minute, second));
        Serial.print("RTC set from GPS ");
        theTime = RTC.now();
        PrintTime();
        Serial.println();
        lasthour = hour;
      }
    }
  }
  if (ms > 10) {
    delay(10);
    ms -= 10;
    goto reread;
  }
  if (ms > 0)
    delay(ms);
}

#endif

void loop() {
  // Main Clock code:

  int count = 0;      // setup temp count variable

  // get time from the RTC
  theTime = RTC.now();
  // theTime = calculateTime(); // takes into account DST   - comment out until sorted out

  // save time as simple variable
  min_x = theTime.minute();
  hour_x = theTime.hour();

  // Print the time
  PrintTime();

  // Update time display
  displayTime();

#ifdef __BRIGHTDEBUG
  // output LDR info - for debug only
  Serial.print("Raw = ");
  Serial.print(raw_bright);
  Serial.print(" Max =  ");
  Serial.print(bright);
  Serial.print(" bright_lim = ");
  Serial.println(bright_lim);
#endif

  // check bright_lim var needs to be updated in EEPROM
  update_EEPROM();

  Serial.println();   // - for debug only

  // key check  & general delay loop, setup to give display update time of about once every 2 minutes
  while (count < 600)  // setup for 1 min delay
  {
    count++;
    delayprocessGPS(100);
    // check the state of the button1 [select]
    if (digitalRead(button1) == LOW) {
      while (digitalRead(button1) == LOW) // wait for key to be released
      {
        delayprocessGPS(20);
      }
      // valid select button press detected - jump to keypress
      keypress();
      count = 600;    // force immediate display update
    }

    /*
        The following section of code allows you the set a range variable between 1 & 15
        This is used to limit the bightness of the display
        15 = no limit full 7219 brightness setting range available
        1 = Max 7219 brighness will be limited to 0 or 1 [depending on ambient brighness]
        5 = Max 7219 brighness will be limited to 0 or 5 [depending on ambient brighness]
    */


    // check the state of the button 3 [inc display limit range]
    if (digitalRead(button3) == LOW) {
      while (digitalRead(button3) == LOW) // wait for key to be released
      {
        delayprocessGPS(20);
      }
      bright_lim++;
      if (bright_lim > 15) {
        bright_lim = 15;
        lc.setLed(0, 0, 7, true);     // flash H
        delayprocessGPS(250);
        lc.setLed(0, 0, 7, false);
      }
      lc.setLed(0, 2, 5, true);     // flash * with to indicate every push or S2 or S3
      delayprocessGPS(250);
      lc.setLed(0, 2, 5, false);

      // update display brighness
      adjustBrightness();

      // for debug only
#ifdef __BRIGHTDEBUG
      Serial.print("Range = ");
      Serial.println(bright_lim);
#endif
    }

    // check the state of the button 2 [dec display limit range]
    if (digitalRead(button2) == LOW) {
      while (digitalRead(button2) == LOW) // wait for key to be released
      {
        delayprocessGPS(20);
      }
      bright_lim--;
      if (bright_lim < 1) {
        bright_lim = 1;
        lc.setLed(0, 0, 5, true);     // flash H
        delayprocessGPS(250);
        lc.setLed(0, 0, 5, false);
      }
      lc.setLed(0, 2, 5, true);     // flash * with to indicate every push or S2 or S3
      delayprocessGPS(250);
      lc.setLed(0, 2, 5, false);

      // update display brighness
      adjustBrightness();

#ifdef __BRIGHTDEBUG
      // for debug only
      Serial.print("Range = ");
      Serial.println(bright_lim);
#endif
    }

    // check ambient light every 4 seconds
    if (count % 40 == 0)
      adjustBrightness();

#ifndef __GPS
    // Flash onboard LED every second - GPS, if enabled, takes care of this
    if (count % 10 == 0)
    {
      digitalWrite(ledPin, !digitalRead(ledPin)); // toggle LED https://www.baldengineer.com/arduino-toggling-outputs.html
    }
#endif
  }
}

