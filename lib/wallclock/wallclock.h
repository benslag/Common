//
// wallclock.h -- maintain wall clock time
//
#ifndef _WALLCLOCK_H
#define _WALLCLOCK_h

#include <Arduino.h>
#include "timer.h"
#include "time.h"

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
    int oldHour = 0;
};

extern Wallclock wallclock;
#endif