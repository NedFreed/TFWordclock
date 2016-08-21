/*
   WORD CLOCK - 8x8 For use with Tempus Fugit board
   By David Saul https://meanderingpi.wordpress.com/

   GPS and DST modifications by Ned Freed, ned.freed@mrochek.com

   Global configuration settings - setup.h
*/

#ifndef __SETUP

#define __SETUP

// Define for GPS support

#define __GPS 1

#ifdef __GPS

#define __GPS_DEBUG 1

// Local offset from GMT in minutes (US/Pacific/DST)

#define TIME_OFFSET (-8*60)

void delayprocessGPS(int ms);

#else

#define delayprocessGPS delay

#endif

// Define for brightness debug

// #define __BRIGHTDEBUG 1


// setup button and LED addresses

#define button1 9
#define button2 8
#define button3 7

#define ledPin 13      // use for seconds indicator

// setup for LDR connection
#define analogInPin   A7  // Analog input connected to LDR
#define analogOutPin   3  // Analog output pin that the LED is attached to  - for debug only
#define LDR_drv        2  // +5v drive for LDR - this is needed for compatability with Pi Zero circuit

#ifdef __GPS

#define GPSTx 4
#define GPSRx 5

#define GPSBaudRate 9600

#endif

// EEPROM memory allocations

#define wr_no0 0
#define wr_no1 1
#define wr_no2 2
#define wr_no3 3

#ifdef INCLUDE_DSTINFO

#include <avr/pgmspace.h>

// DST transitions at 2AM

#define DST_TRANSITION_TIME 2

// DST offset in minutes

#define DST_OFFSET 60

// DST change dates, expressed as # days since Jan 1 1970

#define DSTLENGTH 44

// United States DST transition dates - store in flash to save RAM
// Note that the code assumes the first transition is to DST, second from DST, etc.

const uint16_t dstinfo[DSTLENGTH] PROGMEM = {
  16873, // Sun Mar 13 03:00:00 2016 PDT isdst=1
  17111, // Sun Nov  6 01:00:00 2016 PST isdst=0
  17237, // Sun Mar 12 03:00:00 2017 PDT isdst=1
  17475, // Sun Nov  5 01:00:00 2017 PST isdst=0
  17601, // Sun Mar 11 03:00:00 2018 PDT isdst=1
  17839, // Sun Nov  4 01:00:00 2018 PST isdst=0
  17965, // Sun Mar 10 03:00:00 2019 PDT isdst=1
  18203, // Sun Nov  3 01:00:00 2019 PST isdst=0
  18329, // Sun Mar  8 03:00:00 2020 PDT isdst=1
  18567, // Sun Nov  1 01:00:00 2020 PST isdst=0
  18700, // Sun Mar 14 03:00:00 2021 PDT isdst=1
  18938, // Sun Nov  7 01:00:00 2021 PST isdst=0
  19064, // Sun Mar 13 03:00:00 2022 PDT isdst=1
  19302, // Sun Nov  6 01:00:00 2022 PST isdst=0
  19428, // Sun Mar 12 03:00:00 2023 PDT isdst=1
  19666, // Sun Nov  5 01:00:00 2023 PST isdst=0
  19792, // Sun Mar 10 03:00:00 2024 PDT isdst=1
  20030, // Sun Nov  3 01:00:00 2024 PST isdst=0
  20156, // Sun Mar  9 03:00:00 2025 PDT isdst=1
  20394, // Sun Nov  2 01:00:00 2025 PST isdst=0
  20520, // Sun Mar  8 03:00:00 2026 PDT isdst=1
  20758, // Sun Nov  1 01:00:00 2026 PST isdst=0
  20891, // Sun Mar 14 03:00:00 2027 PDT isdst=1
  21129, // Sun Nov  7 01:00:00 2027 PST isdst=0
  21255, // Sun Mar 12 03:00:00 2028 PDT isdst=1
  21493, // Sun Nov  5 01:00:00 2028 PST isdst=0
  21619, // Sun Mar 11 03:00:00 2029 PDT isdst=1
  21857, // Sun Nov  4 01:00:00 2029 PST isdst=0
  21983, // Sun Mar 10 03:00:00 2030 PDT isdst=1
  22221, // Sun Nov  3 01:00:00 2030 PST isdst=0
  22347, // Sun Mar  9 03:00:00 2031 PDT isdst=1
  22585, // Sun Nov  2 01:00:00 2031 PST isdst=0
  22718, // Sun Mar 14 03:00:00 2032 PDT isdst=1
  22956, // Sun Nov  7 01:00:00 2032 PST isdst=0
  23082, // Sun Mar 13 03:00:00 2033 PDT isdst=1
  23320, // Sun Nov  6 01:00:00 2033 PST isdst=0
  23446, // Sun Mar 12 03:00:00 2034 PDT isdst=1
  23684, // Sun Nov  5 01:00:00 2034 PST isdst=0
  23810, // Sun Mar 11 03:00:00 2035 PDT isdst=1
  24048, // Sun Nov  4 01:00:00 2035 PST isdst=0
  24174, // Sun Mar  9 03:00:00 2036 PDT isdst=1
  24412, // Sun Nov  2 01:00:00 2036 PST isdst=0
  24538, // Sun Mar  8 03:00:00 2037 PDT isdst=1
  24776, // Sun Nov  1 01:00:00 2037 PST isdst=0
};

#define DSTINFO(k) pgm_read_word_near(dstinfo + k)

#endif

#endif
