//
// debug.cpp - a simple debugger
//
// Note that if _Debug is 0, the arguments of LOG are NOT evaluated.
//   e.g. LOG ("The value of i = %d\n", i++);
//   If _LOG is defined as 0, i is NOT increased.
//
// BSla, 10 october 2023
//       31 october 2024 Add multi-level, multi output channel
//        2 december 2024 Add NOTE
//       10 december 2025 For AVR, format string must be a flash string , e.g. F("Hello")

#include <stdarg.h>
#include <stdio.h>
#include "debug.h"
#include <string.h>

static const int N_PFS = 10;            // max number of debug print functions
static const int LOG_P_BUF_SIZE = 200;  // size of buffer for formatted log message

static DebugPrintFunction debugPrintFunctions[N_PFS];
static int pfIndex = 0; // first EMPTY entry in debugPrintFunctions

static int debugCurrentLevel = 10; // log everything

void DebugSetLevel (int level)
{
   if (level >= 0 && level <= 10)
      debugCurrentLevel = level;

   //  loglevels are from 0..10:
   //  0 = ERROR
   //  1 = WARNING
   //  2 = NOTE
   //  8 = LOG
   // 10 = DEBUG decreasing level of importance
}

static void myPrint (const char *buffer)
{
   for (int i = 0; i < pfIndex; i++)
   {
      debugPrintFunctions[i](buffer);
   }
}

bool DebugAddPrintFunction (DebugPrintFunction pf)
{
   bool r = false;

   if (pfIndex < N_PFS)
   {
      debugPrintFunctions [pfIndex++] = pf;
      r = true;
   }
   return r;
}

bool DebugRmPrintFunction (DebugPrintFunction pf)
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
            debugPrintFunctions [j] = debugPrintFunctions [j+1];
         }
         --pfIndex;
         break;
      }
   }
   return r;
}

static int DebugInsertPrefix (const int level, char buffer [])
{
   buffer [0] = '\0';
   if (level == 0) strcpy (buffer, "ERROR: ");
   else if (level == 1) strcpy (buffer, "WARNING: ");
   return (strlen (buffer));
}

// for AVR, the format string must be a Flash string (e.g. F("Hello") )

#ifdef __AVR__
static int my_vsnprintf (char *buf, size_t bufsize, const __FlashStringHelper *fmtFlash, va_list ap);
void DebugReal(const int level, const __FlashStringHelper *fmt, ...)
#else
void DebugReal(const int level, const char *fmt, ...)
#endif
{
   if (level <= debugCurrentLevel)
   {
      va_list args;
      char buffer[LOG_P_BUF_SIZE];
      char *bp = buffer;
      int used = DebugInsertPrefix (level, bp);
      bp = &buffer [used];
      size_t size = LOG_P_BUF_SIZE - used; 

      va_start(args, fmt);
#ifdef __AVR__
      my_vsnprintf (bp, size, fmt, args);
#else
      vsnprintf(bp, size, fmt, args);
#endif
      va_end(args);
      myPrint(buffer);
   }
}


const char *toCCP (bool b)
{
   static const char *True  = "true";
   static const char *False = "false";
   return b ? True : False;
}

const char *sfToCCP (bool b)
{
   static const char *success = "success";
   static const char *fail    = "FAIL";
   return b ? success: fail;
}

const String ttToString (uint32_t t)
{
    uint32_t seconds = t / 1000;
    uint32_t millis = t%1000;
    uint32_t minutes = seconds / 60;
    seconds = seconds % 60;
    uint32_t hours = minutes / 60;
    minutes = minutes % 60;
    char buffer [20];
    snprintf (buffer, sizeof(buffer), "%2d:%02d:%02d.%03d", 
             (int)hours, (int)minutes, (int)seconds, (int)millis);
    String s (buffer);
    return s;
}


#ifdef __AVR__

// minimal_vsnprintf (RAM + PROGMEM formats) - AVR friendly
#include <Arduino.h>

// Append one char to buffer (like vsnprintf semantics).
// totalWritten = aantal tekens dat tot nu toe *zou* zijn geschreven (excl. nul).
static void buf_put_char(char *buf, size_t bufsize, size_t &totalWritten, char c) {
  if (bufsize > 0 && totalWritten < bufsize - 1) {
    buf[totalWritten] = c;
  }
  totalWritten++;
}

// Append a RAM string to buffer
static void buf_put_str_ram(char *buf, size_t bufsize, size_t &totalWritten, const char *s) {
  if (!s) {
    const char *nullstr = "(null)";
    while (*nullstr) buf_put_char(buf, bufsize, totalWritten, *nullstr++);
    return;
  }
  while (*s) buf_put_char(buf, bufsize, totalWritten, *s++);
}

// Append a PROGMEM string to buffer
static void buf_put_str_flash(char *buf, size_t bufsize, size_t &totalWritten, const __FlashStringHelper *fs) {
  if (!fs) {
    const char *nullstr = "(null)";
    while (*nullstr) buf_put_char(buf, bufsize, totalWritten, *nullstr++);
    return;
  }
  const char *p = (const char*)fs;
  char c;
  while ((c = (char)pgm_read_byte(p++))) buf_put_char(buf, bufsize, totalWritten, c);
}

// integer -> ascii into buffer using buf_put_char (unsigned long input)
static void buf_put_unsigned_int_base(char *buf, size_t bufsize, size_t &totalWritten, unsigned long value, int base, bool uppercase=false) {
  char tmp[33];
  int i=0;
  if (value == 0) {
    buf_put_char(buf, bufsize, totalWritten, '0');
    return;
  }
  while (value && i < (int)sizeof(tmp)-1) {
    unsigned digit = value % base;
    if (digit < 10) tmp[i++] = '0' + digit;
    else tmp[i++] = (uppercase ? 'A' : 'a') + (digit - 10);
    value /= base;
  }
  while (i--) buf_put_char(buf, bufsize, totalWritten, tmp[i]);
}



// ---------------- PROGMEM-formatting core (va_list) ----------------
static int my_vsnprintf (char *buf, size_t bufsize, const __FlashStringHelper *fmtFlash, va_list ap) 
{
  size_t totalWritten = 0;
  size_t idx = 0;
  if (!fmtFlash) {
    const __FlashStringHelper *nullfmt = (const __FlashStringHelper*) ("<null>");
    return my_vsnprintf (buf, bufsize, nullfmt, ap);
  }

  const char *pf = (const char*)fmtFlash;
  while (true) {
    char c = (char)pgm_read_byte(pf + idx++);
    if (c == '\0') break;
    if (c != '%') { buf_put_char(buf, bufsize, totalWritten, c); continue; }

    // parse length 'l'
    int lcount = 0;
    char next = (char)pgm_read_byte(pf + idx);
    while (next == 'l') {
      lcount++;
      idx++;
      next = (char)pgm_read_byte(pf + idx);
    }
    // consume specifier
    char spec = (char)pgm_read_byte(pf + idx++);
    switch (spec) {
      case 'd': {
        if (lcount >= 1) {
          long v = va_arg(ap, long);
          bool neg = v < 0;
          unsigned long uv = (unsigned long)(neg ? - (long) v : v);
          if (neg) buf_put_char(buf, bufsize, totalWritten, '-');
          buf_put_unsigned_int_base(buf, bufsize, totalWritten, uv, 10);
        } else {
          int v = va_arg(ap, int);
          bool neg = v < 0;
          unsigned long uv = (unsigned long)(neg ? - (int) v : v);
          if (neg) buf_put_char(buf, bufsize, totalWritten, '-');
          buf_put_unsigned_int_base(buf, bufsize, totalWritten, uv, 10);
        }
        break;
      }

      case 'u': {
        if (lcount >= 1) {
          unsigned long v = va_arg(ap, unsigned long);
          buf_put_unsigned_int_base(buf, bufsize, totalWritten, v, 10);
        } else {
          unsigned int v = va_arg(ap, unsigned int);
          buf_put_unsigned_int_base(buf, bufsize, totalWritten, v, 10);
        }
        break;
      }

      case 'x':
      case 'X': {
        bool up = (spec == 'X');
        if (lcount >= 1) {
          unsigned long v = va_arg(ap, unsigned long);
          buf_put_unsigned_int_base(buf, bufsize, totalWritten, v, 16, up);
        } else {
          unsigned int v = va_arg(ap, unsigned int);
          buf_put_unsigned_int_base(buf, bufsize, totalWritten, v, 16, up);
        }
        break;
      }

      case 'c': {
        int ch = va_arg(ap, int);
        buf_put_char(buf, bufsize, totalWritten, (char)ch);
        break;
      }

      case 's': {
        const char *s = va_arg(ap, const char *);
        buf_put_str_ram(buf, bufsize, totalWritten, s);
        break;
      }

      case 'S': {
        const __FlashStringHelper *fs = va_arg(ap, const __FlashStringHelper *);
        buf_put_str_flash(buf, bufsize, totalWritten, fs);
        break;
      }

      case '%': {
        buf_put_char(buf, bufsize, totalWritten, '%');
        break;
      }

      default: {
        buf_put_char(buf, bufsize, totalWritten, '%');
        for (int k=0;k<lcount;k++) buf_put_char(buf, bufsize, totalWritten, 'l');
        buf_put_char(buf, bufsize, totalWritten, spec ? spec : '?');
        break;
      }
    } // switch
  } // while

  if (bufsize > 0) {
    size_t termPos = (totalWritten < bufsize - 1) ? totalWritten : (bufsize - 1);
    buf[termPos] = '\0';
  }
  return (int) totalWritten;
}


#ifdef _NOTUSED

// Convenience wrappers (like snprintf / vsnprintf)
int my_snprintf_ram(char *buf, size_t bufsize, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = my_vsnprintf_ram(buf, bufsize, fmt, ap);
  va_end(ap);
  return r;
}

int my_vsnprintf_ram_wrapper(char *buf, size_t bufsize, const char *fmt, va_list ap) {
  // simple wrapper name compatibility
  return my_vsnprintf_ram(buf, bufsize, fmt, ap);
}

int my_snprintf_flash(char *buf, size_t bufsize, const __FlashStringHelper *fmtFlash, ...) {
  va_list ap;
  va_start(ap, fmtFlash);
  int r = my_vsnprintf_flash(buf, bufsize, fmtFlash, ap);
  va_end(ap);
  return r;
}

int my_vsnprintf_flash_wrapper(char *buf, size_t bufsize, const __FlashStringHelper *fmtFlash, va_list ap) {
  return my_vsnprintf_flash(buf, bufsize, fmtFlash, ap);
}

#endif
#endif
