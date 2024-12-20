//-------------------
// timer.cpp -- code for timer module
//
// BSla, 9 oct 2021
//
// Arduino based implementation using millis () function

#include "timer.h"

//--------------------
Timer::Timer(timeType period)
//
// construct a timer and start it with given period
{
  init();
  start(period);
}

//------------------
void Timer::init()
// init the Timer instance
{
  pPeriod = 1;
  pExpired = false;
  pActive = false;
  startAt = millis();
}

//------------------
void Timer::start(timeType period)
// start a timer with this period
// irrespective of the timer's current state
{
  if (period > 0)
  {
    setPeriod(period);
    start();
  }
  else
  {
    start();
    stop();
  }
}

//---------------------
void Timer::start()
//
// start a timer with the current period
{
  pExpired = false;
  pActive = true;
  startAt = millis();
}

//---------------------
void Timer::stop()
//
// stop a timer, irrespective of its state
{
  pExpired = false;
  pActive = false;
}

//----------------------------
timeType Timer::timeSinceStart()
//
// return time since most recent start ()
// Deal with tick counter overflow:
// - numbers are unsigned
// - As long as the timer is active, millis() is never < startAt:
//     upon starting, startAt = millis.
// So as long as the timer is active, timeSinceStart ()
//  always returns a positive number < half number range
//
//
{
  return millis() - startAt;
}

//------------------------
bool Timer::hasExpired()
//
// return true if timer has expired
{
  bool result = pExpired;
  if (pActive)
  {
    if (timeSinceStart() > pPeriod)
    { // see note at timeSinceStart
      result = pExpired = true;
      pActive = false;
    }
  }
  return result;
}

//------------------------
void Timer::restart()
//
// restart the timer:
//    when expired, make sure that it does not run late
//    when still running, do the same as start ()
//
{
  if (hasExpired())
  {
    while (timeSinceStart() >= pPeriod)
    {
      startAt += pPeriod;
    }
    pExpired = false;
    pActive = true;
  }
  else
    start();
}

//------------------------
bool Timer::isActive()
//
// return true if timer is active
// i.e. started and not yet expired
{
  hasExpired();
  return pActive;
}

//--------------------------
void Timer::delay(timeType period)
//
// delay a fixed period, burning CPU cycles
{
  start(period);
  while (!hasExpired())
  {
    // delay
  }
}
