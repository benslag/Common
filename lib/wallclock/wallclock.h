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
    bool wGetLocalTime (struct tm &tm);

 private:
    bool setupDone = false;
    bool dst = false;        // preset: no DST
    timeType oldSec = 0;
    int oldHour = 0;

    bool isDST (bool currentDST, struct tm &tm);
};

extern Wallclock wallclock;
#endif