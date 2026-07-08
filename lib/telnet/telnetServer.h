//
// telnetServer.h -- telnet server for Serial1
//
// BSla, 18 jun 2026
//
#ifndef _TELNETSERVER_H
#define _TELNETSERVER_H

class Telnet {
public:
   void setup ();
   void loop ();
};

extern Telnet telnet;

#endif