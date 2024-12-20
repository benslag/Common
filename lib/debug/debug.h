//
// debug.h - a simple debugger
//
// The following macros exist:
//
// DebugAddPrintFunction (PrintFunction x) 
//                     adds a log print function (need at least 1)
// LOG     (fmt, ...)  logs a message using a printf style format
// WARNING (fmt, ...)  logs a warning message, ...
// ERROR   (fmt, ...)  logs an error message, ...
//
// Note that if _D is 0, the arguments of any of the macros are NOT evaluated.
//   e.g. LOG ("The value of i = %d\n", i++);
//   If _LOG is defined as 0, i is NOT increased.
//
// Macro toCCP (x) can be used to print the value of a boolean.
//   e.g. LOG ("Boolean x = %s\n", toCCP (x));
//   prints either "Boolean x = true" or "Boolean x = false"
//   (note: CCP is const char ptr)
// 
// BSla, 10 october 2023
//       31 october 2024 Add multi-level, multi output channel. Decouple from Serial.print
//
#ifndef __DEBUG_H
#define __DEBUG_H

#ifndef _DEBUG
#define _DEBUG 1
#endif

typedef void (*DebugPrintFunction) (const char *buffer);

extern bool  DebugAddPrintFunction (DebugPrintFunction pf);
extern void  DebugSetLevel (int level);
extern void  DebugReal (const int level, const char *fmt, ...);
extern const char* toCCP (bool b);



#define DEBUGSetLevel(x)         DebugSetLevel (x)
#define DEBUGAddPrintFunction(x) DebugAddPrintFunction (x)
#define DEBUGRmPrintFunction(x)  DebugRmPrintFunction (x)
#define WARNING(...) DebugReal (1,__VA_ARGS__)
#define ERROR(...)   DebugReal (0,__VA_ARGS__)

#if _DEBUG == 1
#define DEBUG(...) DebugReal (10,__VA_ARGS__)
#define LOG(...)   DebugReal (8,__VA_ARGS__)


#define DEBUGAddPrintFunction(x) DebugAddPrintFunction (x)
#define DEBUGRmPrintFunction(x)  DebugRmPrintFunction (x)

#elif _DEBUG == 0
#define DEBUG(...) {}
#define LOG(...) {}
#else
#error Debug.h: _DEBUG is not defined as 0 or 1
#endif  // if _DEBUG==1

#endif  // __DEBUG_H
