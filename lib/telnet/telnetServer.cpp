//
// telnetServer.cpp -- telnet server for debugger
//
// BSla, 18 jun 2026
//
#include <Arduino.h>
#include <WiFi.h>
#include "hsi.h"

#ifndef TELNET_DEBUG
#define TELNET_DEBUG 0
#endif

#define _DEBUG TELNET_DEBUG
#include "debug.h"
#include "telnetServer.h"

Telnet telnet;
WiFiServer telnetServer (23);
WiFiClient telnetClient;

static bool justConnected = false;

static void telnetDebugPrinter (const char *buffer)
{
    if (telnetClient && telnetClient.connected ()) telnetClient.print (buffer);
}


void Telnet::setup ()
{
   telnetServer.begin();
   telnetServer.setNoDelay (true); // Send data immediately
   DEBUGAddPrintFunction (telnetDebugPrinter);
   LOG (">< Telnet::setup: Telnet server started on port 23\n");
}

void Telnet::loop ()
{
  // Ensure only a single client
  if (telnetServer.hasClient()) {
    // Reject if we already have a client
    if (telnetClient && telnetClient.connected()) {
      WiFiClient rejectedClient = telnetServer.available ();
      rejectedClient.println ("System in use. Max. 1 connection allowed.");
      rejectedClient.stop ();
      ERROR ("Telnet: rejected new connection. We already have an active client\n");
    } 
    else {
      // Accept the new client
      telnetClient = telnetServer.available ();
      telnetClient.println ("Welcome to the telnet debug printer");
      LOG ("   Telnet::connected new client\n");
      justConnected = true;
    }
  }

  // Process data from the Telnet client
  if (telnetClient && telnetClient.connected ()) {
    size_t len = telnetClient.available ();
    if (len) {
      if (justConnected) {
         while (telnetClient.available ()) {
            char buffer [10];
            telnetClient.readBytes (buffer, 1);
         }
         justConnected = false;
      }
      else {
         uint8_t buffer [len];
         telnetClient.readBytes (buffer, len);
    
         // Do something useful with the data....
         ERROR ("telnetClient: '%s' ??\n", buffer);
      }
    }
  }
}

