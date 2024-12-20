//
// debug.cpp - a simple debugger
//
// Note that if _Debug is 0, the arguments of LOG are NOT evaluated.
//   e.g. LOG ("The value of i = %d\n", i++);
//   If _LOG is defined as 0, i is NOT increased.
//
// BSla, 10 october 2023
//       31 october 2024 Add multi-level, multi output channel

#include <stdarg.h>
#include <stdio.h>
#include "debug.h"
#include <string.h>

static const int N_PFS = 10;
static const int LOG_P_BUF_SIZE = 200;

static DebugPrintFunction debugPrintFunctions[N_PFS];
static int pfIndex = 0; // first EMPTY entry in debugPrintFunctions

static int debugCurrentLevel = 10; // log everything

void DebugSetLevel(int level)
{
   if (level >= 0 && level <= 10)
      debugCurrentLevel = level;

   //  loglevels are from 0..10:
   //  0 = ERROR
   //  1 = WARNING
   //  2..10 = LOG, decreasing level of importance
}

static void myPrint(const char *buffer)
{
   for (int i = 0; i < pfIndex; i++)
   {
      debugPrintFunctions[i](buffer);
   }
}

bool DebugAddPrintFunction(DebugPrintFunction pf)
{
   bool r = false;

   if (pfIndex < N_PFS)
   {
      debugPrintFunctions[pfIndex++] = pf;
      r = true;
   }
   return r;
}

bool DebugRmPrintFunction(DebugPrintFunction pf)
// remove the first added print function pf
{
   bool r = false;
   for (int i = 0; i < pfIndex; i++)
   {
      if (debugPrintFunctions[i] == pf)
      {
         r = true;
         for (int j = i; j < pfIndex - 1; j++)
         {
            debugPrintFunctions[j] = debugPrintFunctions[j + 1];
         }
         --pfIndex;
         break;
      }
   }
   return r;
}

void DebugReal(const int level, const char *fmt, ...)
{
   if (level <= debugCurrentLevel)
   {
      va_list args;
      char buffer[LOG_P_BUF_SIZE];
      buffer[0] = '\0';
      if (level == 0)
         strcpy(buffer, "ERROR: ");
      else if (level == 1)
         strcpy(buffer, "WARNING: ");

      char *bp = &buffer[strlen(buffer)];
      va_start(args, fmt);
      vsnprintf(bp, LOG_P_BUF_SIZE, fmt, args);
      va_end(args);
      myPrint(buffer);
   }
}

const char *toCCP(bool b)
{
   static const char *True = "true";
   static const char *False = "false";
   return b ? True : False;
}