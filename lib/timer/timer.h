//-------------------
// timer.h -- header file for timer module
//
// BSla, 8 oct 2021
//
// Arduino based implementation using millis () function
//

#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>
#include <Arduino.h>

typedef uint32_t timeType;

#define TICKS_PER_SECOND (1000UL) // we are using millis () to count ticks

// Macros to express time intervals
// e.g. time = 10 SECONDS;
#define MILLISECONDS *TICKS_PER_SECOND / 1000UL
#define MILLISECOND MILLISECONDS
#define SECONDS *TICKS_PER_SECOND
#define SECOND SECONDS
#define MINUTES *60UL SECONDS
#define MINUTE MINUTES
#define HOURS *60UL MINUTES
#define HOUR HOURS
#define DAYS *24UL HOURS
#define DAY DAYS

//-------------------
// Class Timer implements interval timers.
//
// Time is expressed in ticks. The macro TICKS_PER_SECOND defines the basic rate.
//
//  Timers have states: - idle:      not active, not expired
//                      - active:    counting, but not yet expired
//                      - expired:   the expiry time has been exceeded
//                      -
//
//
// Timer           ()                constructs a timer, but does not start it
// Timer           (timeType period) constructs AND STARTS a timer with the given period
// timeType now    ()                returns the current time expressed in ticks
// void start      (timeType period) starts a timer with a period time of period.
// void start      ()                starts the timer again with the same period time, from now.
// void restart    ()                restarts an expired timer without running late (for fixed interval timers)
// void stop       ()                stops a Timer, irrespective of its state. Clear expired bit
// void isActive   ()                tests if the timer is started but not expired
// bool hasExpired ()                tests if the timer has expired.
// void delay      (timeType period) delays (busy form of waiting)
// timeType timeSinceStart ()        returns expired time since the timer was started
//
// You can instantiate as many Timers as needed. The basic tick count is shared by all Timers.
// A single Timer can only measure 1 period at a time.
// The time overflows after 24.85 days. (= 2e9 milliseconds, the max bit value of a signed 32 bit integer
//

class Timer
{
public:
  Timer() { init(); }                           // constructor, no start
  Timer(timeType period);                       // construct AND START timer with period
  timeType now() { return timeType(millis()); } // return current time
  void start(timeType period);                  // start a timer with this period
  void start();                                 // start with a predefined period
  void restart();                               // restart expired timer without running late
  void stop();                                  // stop a timer, clear expired
  bool hasExpired();                            // has the timer expired?
  bool isActive();                              // is the timer active (started and not expired)?
  void delay(timeType period);                  // delay this period
  timeType timeSinceStart();                    // return time since start of timer

private:
  static bool inited;
  timeType startAt; // this Timer started at
  timeType pPeriod; // Timer period time
  bool pExpired;    // has this Timer expired
  bool pActive;     // is this Timer active
  void init();
  void setPeriod(timeType period) { pPeriod = period; } // set timer's period, no start
};

#endif
