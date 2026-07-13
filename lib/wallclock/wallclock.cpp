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



// see if struct tm contains a date/time that is within the DST range.
// Note that tm must contain a valid date/time
static bool isDST (bool currentDST, struct tm &tm) 
{
   bool dst    = false;
   int month   = tm.tm_mon+1;
   int day     = tm.tm_mday;
   int hour    = tm.tm_hour;
   int weekday = tm.tm_wday;
   int dstSwitchHour = currentDST?3:2; // at which hour do we switch

   LOG (">  Wallclock::isDST \n");

   // DST starts: Last Sunday of March at 2:00 AM
   if (month > 3 && month < 10) {
      LOG ("   isDST: month = %d, apr to sept -> true\n", month);
      dst = true;  // April to September: DST
   }

   int lastSunday = 31 - (weekday - (day % 7 + 7) % 7);


   // March: Check if last Sunday has passed
   if (month == 3) 
   {
      LOG ("   isDST: march: day %d, lastSunday = %d", day, lastSunday);
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
      LOG ("   isDST: october: day %d, lastSunday = %d", day, lastSunday);
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
   LOG ("<  Wallclock::isDST = %s\n", toCCP (dst));
   return dst;
}

static bool myGetLocalTime (struct tm &tm)
{
   return getLocalTime (&tm);
}


Wallclock wallclock;

void Wallclock::setup ()
{
   configTime (gmtOffsetSec, dst?daylightOffsetSec:0, primaryNTPServer, secondaryNTPServer);
   setupDone = true;
}

static const char *weekdays [] = {
   "zondag","maandag","dinsdag","woensdag","donderdag","vrijdag","zaterdag"
};

static const char *months [] = {
   "januari", "februari", "maart","april", "mei", "juni",
   "juli", "augustus", "september", "oktober", "november", "december"
};

static String getDay (int day, bool full, bool capital)
{
   String s;
   const char *dn = weekdays [day%7];
   s = String (dn);
   if (capital) s[0] = toupper (s[0]);
   if (!full) s = s.substring (0, 3);
   return s;
}

static String getMonth (int month, bool full, bool capital)
{
   String s;
   const char *dn = months [month%12]; 
   s = String (dn);
   if (capital) s[0] = toupper (s[0]);
   if (!full) s = s.substring (0, 3);
   return s;
}

String Wallclock::strfTime (const struct tm &tm, const char *format, bool dutch, bool capitals)
{
   String s = "";
   char buffer [100];
   if (!dutch) {
      int i = strftime (buffer, 99, format, &tm);
      if (i > 98) {
         ERROR ("Wallclock::strfTime: buffer overflow, format = '%s'\n", format);
      }
      else s = String (buffer);
   }
   else {
      const char *fp = format;
      char c;
      bool meta = false;
      while ((c = *fp++) != '\0') {
         if (!meta) {
            if (c == '%') {
               meta = true;
            }
            else {
               s += String (c);
            }
         }
         else {
            // we just read a '%'; what is the next char?
            switch (c) {
             case '%':
               s += String (c);   // '%%' -> '%'
               break;
             case 'a': // abbreviated weekday name
               s += getDay (tm.tm_wday, false, capitals);
               break;
             case 'A': // full weekday name
               s += getDay (tm.tm_wday, true, capitals);
               break;
             case 'b': // short month name
               s += getMonth (tm.tm_mon, false, capitals);
               break;
             case 'B':
               s += getMonth (tm.tm_mon, true, capitals);
               break;
             case 'c': // local date and time representation
               sprintf (buffer, "%s %d %s %d %02d:%02d:%02d",
                        getDay (tm.tm_wday, true, capitals), tm.tm_mday, 
                        getMonth (tm.tm_mon, true, capitals), 
                        tm.tm_year+1900,
                        tm.tm_hour, tm.tm_min, tm.tm_sec);
               s += String (buffer);
               break;
             case 'D': // date as SINGLE digit
               s += String (tm.tm_mday);
               break;
             case 'p': // am or pm
               if (tm.tm_hour >= 12) s += "n.m.";
               else s += "v.m.";
               break;

              default:
               char fmt [3]; fmt[0] = '%'; fmt [1] = c; fmt[2] = '\0';
               int i = strftime (buffer, 99, fmt, &tm);
               if (i > 98) {
                  ERROR ("Wallclock::strfTime: buffer overflow, format = '%s'\n", fmt);
               }
               else s += String (buffer);
               break;
            }
            meta = false; // finished processing meta char
         }
      }
   }
   return s;
}


bool Wallclock::getLocalTime (struct tm &tm)
{
    //LOG (">  Wallclock::getLocalTime ()");
    if (!setupDone) {
        setup ();
    }
    bool r = myGetLocalTime (tm);
    if (r) {
        int hr = tm.tm_hour;
        if (hr != oldHour) {
            oldHour = hr;
            bool newDST = isDST (dst, tm);
            if (newDST != dst) {
                dst = newDST;
                LOG ("   Wallclock: we switched DST %s\n", dst?"ON":"OFF");
                configTime (gmtOffsetSec, dst?daylightOffsetSec:0, primaryNTPServer, secondaryNTPServer);
                if ((r = myGetLocalTime (tm))) {
                   oldHour = tm.tm_hour;
                }
            }
        }
        tm.tm_isdst = dst?1:0;
    }
    else {
        ERROR ("Wallclock::getLocalTime failed\n");
    }
    if (r && (gltSuccess == false)) {
       String s = strfTime (tm, "%A, %d %B %Y  %H:%M:%S");
       s += String (" MET");
       if (dst) s += String (" DST");
       NOTE (">< Wallclock::getLocalTime: %s\n", s.c_str ());
    }
    gltSuccess = r;

    return r;
}

bool Wallclock::getLocalTime (time_t &tt)
{
   struct tm tm;
   bool r = getLocalTime (tm);
   if (r) {
      tt = mktime (&tm);
   }
   else tt = 0;
   return r;
}
