//
// wallclock.h -- maintain wall clock time
//
#ifndef _WALLCLOCK_H
#define _WALLCLOCK_h

#include <Arduino.h>
#include "timer.h"
#include "time.h"

// Formatting characters for strfTime ()
// strfTime (struct tm, format, dutch, capital)
//
// %A -- full weekday name (dutch, capital)
// %a -- abbreviated weekday name (dutch, capital)
// %B -- full month name (dutch, capital)
// %b -- abbreviated month name (dutch, capital)
// %c -- local d/t representation: 'maandag 23 augustus 2026 14:13:12'
// %d -- date of month (01..31)
// %D -- date of month as a single digit (not '09' but '9')
// %H -- hour (24 hour clock)
// %I -- hour (12 hour clock)
// %j -- day of the year (1..366)
// %m -- month (01-12)
// %M -- minute (00-59)
// %p -- in Dutch: 'v.m.' or 'n.m.'
// %S -- second (00-61)
// %U -- week number of the year (Sunday as 1st day of week) (00..53)
// %w -- weekday (0-6, sunday == 0)
// %W -- week number of the year (Monday as 1st day of week) (00-53)
// %x -- local date representation
// %X -- local time representation
// %y -- year without century (00..99)
// %Y -- year with century
// %Z -- time zone name, if any
//

class Wallclock 
{
 public:
    Wallclock () {}
    void setup ();
    bool getLocalTime (struct tm &tm);
    bool getLocalTime (time_t &tt);
    String strfTime (const struct tm &tm, const char *format, bool dutch=false, bool capital=false);

 private:
    bool gltSuccess = false;
    bool setupDone = false;
    bool dst = false;        // preset: no DST
    timeType oldSec = 0;
    int oldHour = -1;
};

extern Wallclock wallclock;
#endif