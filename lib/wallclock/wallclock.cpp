//
// wallclock.cpp -- maintain wall clock time
//
#include <time.h>

#ifndef WALLCLOCK_DEBUG
#define WALLCLOCK_DEBUG 0
#endif

#define _DEBUG WALLCLOCK_DEBUG
#include "debug.h"
#include "timer.h"

#include "wallclock.h"

const char *primaryNTPServer   = "homeTicker";
const char *secondaryNTPServer = "pool.ntp.org";
const int  gmtOffsetSec      = 3600;
const int  daylightOffsetSec = 3600;


Wallclock wallclock;

bool Wallclock::isDST (bool currentDST, struct tm &tm) 
{
   bool dst = false;
   int month   = tm.tm_mon;
   int day     = tm.tm_mday;
   int hour    = tm.tm_hour;
   int weekday = tm.tm_wday;
   int dstSwitchHour = currentDST?3:2; // at which hour do we switch

   // DST starts: Last Sunday of March at 2:00 AM
   if (month > 3 && month < 10) {
      LOG ("   Wallclock::isDST: month = %d, apr to sept -> true\n", month);
      dst = true;  // April to September: DST
   }

   int lastSunday = 31 - (weekday - (day % 7 + 7) % 7);


   // March: Check if last Sunday has passed
   if (month == 3) 
   {
      LOG ("   Wallclock::isDST: march: day %d, lastSunday = %d", day, lastSunday);
      if (day > lastSunday) {
         dst = true;
         LOG (" ==> dst = true\n");
      }
      else if (day == lastSunday) {
         if (hour >= dstSwitchHour) {
            dst = true;
            LOG (" %d hour >= %d: dst = true\n", hour, dstSwitchHour);
         }
      }
   }

   // October: Check if last Sunday has passed
   if (month == 10) 
   {
      LOG ("   Wallclock::isDST: october: day %d, lastSunday = %d", day, lastSunday);
      if (day < lastSunday) {
         dst = true;
         LOG (" ==> dst = true\n");
      }
      else if (day == lastSunday) {
         if (hour < dstSwitchHour) {
            dst = true;
            LOG (" %d hour < %d: dst = true\n", hour, dstSwitchHour);
         }
      }
   }
   return dst;
}


void Wallclock::setup ()
{
   configTime (gmtOffsetSec, dst?daylightOffsetSec:0, primaryNTPServer, secondaryNTPServer);
   setupDone = true;
}


bool Wallclock::wGetLocalTime (struct tm &tm)
{
    if (!setupDone) {
        setup ();
    }
    bool r = getLocalTime (&tm);
    if (r) {
        int hr = tm.tm_hour;
        if (hr != oldHour) {
            oldHour = hr;
            bool newDST = isDST (dst, tm);
            if (newDST != dst) {
                dst = newDST;
                LOG ("   Wallclock: we switched DST %s\n", dst?"ON":"OFF");
                configTime (gmtOffsetSec, dst?daylightOffsetSec:0, primaryNTPServer, secondaryNTPServer);
                if ((r = getLocalTime (&tm))) {oldHour = tm.tm_hour;}
            }
        }
    }
    else {
        ERROR ("Wallclock::getLocalTime failed %d\n");
    }

    return r;
}

