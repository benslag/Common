//
// main.cpp -- common main module
//
// BSla, 21 dec 2025
//

#ifndef MAIN_DEBUG
#define MAIN_DEBUG 1
#endif

#define _DEBUG MAIN_DEBUG
#include "debug.h"
#include "timer.h"

#include "mainapp.h"

static uint32_t startTime;
static uint32_t previousTime;

static void loopTime();
static bool doReport = true;


//
// convert the current time into a string
// format hh:mm:ss.mmm:  ss is seconds, mmm is milliseconds
// Is used by debugPrinter to show current time in seconds since startup
//
String mainTimeString ()
{
   uint32_t now = millis();
   int ms  = now % 1000;
   int sec = now / 1000;
   int min = sec / 60;
   sec     = sec % 60;
   int hrs = min / 60;
   min     = min % 60;
   char buffer [50];
   snprintf (buffer, sizeof (buffer),"%d:%02d:%02d.%03d ", hrs, min, sec, ms); 
   return String(buffer);
}

// 
// print a timestamp and the log buffer to the serial port. This function is
// installed as the debug printer in setup() and is called by the debug module whenever a debug message is printed.
// 
static void myDebugPrinter(const char *buffer)
{
   static bool lastWasNL = true;
   int l = strlen (buffer);
   if (l > 0) {
      if (lastWasNL) {
         Serial .print (mainTimeString());
      }
      Serial .print (buffer);
      lastWasNL = (buffer [l-1] == '\n');
   }
   else lastWasNL = false;
}

//
// the global setup() function is called once when the program starts.
// It initializes the serial port, sets up the debug printer, and calls 
//   mainApp.setup() to initialize the application.
//
void setup()
{
   Serial.begin (115200);
   DEBUGAddPrintFunction (myDebugPrinter);

   // DEBUGSetLevel (3);
   mainApp.setup();

   startTime = millis();
   previousTime = micros();
   //DEBUG ("   Main: setup done\n");
}

//
// the global loop() function is called repeatedly in an infinite loop.
//
void loop()
{
   mainApp.loop();
   loopTime();
}

// switch loop time reporting on or off. 
// If on, the loop time is reported every minute
//
void mainReportLoopTime (bool report)
{
   doReport = report;
}

static void loopTime()
{
#if _DEBUG == 1
   static int uptime = 1;
   static int loopCount = 0;
   static uint32_t maximumTime = 0;
   static Timer reportTimer(1 MINUTE);

   uint32_t now = micros();
   uint32_t thisLoopTime = now - previousTime;
   previousTime = now;
   if (thisLoopTime > maximumTime)
   {
      maximumTime = thisLoopTime;
   }

   if (reportTimer.hasExpired())
   {
      reportTimer.restart();

      uint32_t m = millis();
      uint32_t expired = m - startTime;

      float usPerLoop = float(expired) * 1000 / loopCount;
      if (doReport) {
         LOG("main loop: uptime = %d minutes; avg loop time = %5.1f us, max loop time = %d us\n",
             uptime, usPerLoop, maximumTime);
      }
      loopCount = 0;
      maximumTime = 0;
      uptime++;
      startTime = m;
   }
   loopCount++;
#endif
}
