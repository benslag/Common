//
// debug.h - a simple printf style debugger
//
// The following macros exist:
//
// DebugAddPrintFunction (PrintFunction x) 
//                     adds a log print function (need at least 1)
// DEBUG   (fmt, ...)  logs a message using a printf style format
// LOG     (fmt, ...)  logs a message using a printf style format
// NOTE    (fmt, ...)  logs a note, will always expand
// WARNING (fmt, ...)  logs a warning message, wil always expand
// ERROR   (fmt, ...)  logs an error message, will always expand
//
// Note that if _DEBUG is 0, the arguments of any of the macros are NOT evaluated.
//   e.g. LOG ("The value of i = %d\n", i++);
//   If _LOG is defined as 0, i is NOT increased.
//
// Macro toCCP (x) can be used to print the value of a boolean.
//   e.g. LOG ("Boolean x = %s\n", toCCP (x));
//   prints either "true" or "false"
//   (note: CCP is Const Char Ptr)
// 
// BSla, 10 october 2023
//       31 october 2024 Add multi-level, multi output channel. Decouple from Serial.print
//        2 december 2025 Add NOTE macro
//
#ifndef __DEBUG_H
#define __DEBUG_H

#ifndef _DEBUG      // to avoid DEBUG and LOG expansion, explicitly #define _DEBUG as 0
#define _DEBUG 1
#endif

typedef void (*DebugPrintFunction) (const char *buffer);

extern bool  DebugAddPrintFunction (DebugPrintFunction pf);
extern void  DebugSetLevel (int level);
extern void  DebugReal (const int level, const char *fmt, ...);
extern const char* toCCP (bool b);

// DEBUG statements have a level. With DEBUGSetLevel, you can control if statements of this level
// will be expanded or not. ERROR has level 0, WARNING has level 1, NOTE has level 2
// DEBUGSetLevel (3) will print these categories, but not LOGs and DEBUGs.



#define DEBUGSetLevel(x)         DebugSetLevel (x)
#define DEBUGAddPrintFunction(x) DebugAddPrintFunction (x)
#define DEBUGRmPrintFunction(x)  DebugRmPrintFunction (x)
#define NOTE(...)    DebugReal (2,__VA_ARGS__)
#define WARNING(...) DebugReal (1,__VA_ARGS__)
#define ERROR(...)   DebugReal (0,__VA_ARGS__)

// NOTEs, WARNINGs and ERRORs always expand into DebugReal calls.
// but DEBUG and LOG only expand if _DEBUG is 1.

#if _DEBUG == 1
#define DEBUG(...) DebugReal (10,__VA_ARGS__)
#define LOG(...)   DebugReal (8,__VA_ARGS__)

#define DEBUGAddPrintFunction(x) DebugAddPrintFunction (x)
#define DEBUGRmPrintFunction(x)  DebugRmPrintFunction (x)

#elif _DEBUG == 0
#define DEBUG(...) {}
#define LOG(...) {}
#endif  // if _DEBUG==1

#endif  // __DEBUG_H
