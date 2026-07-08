//
// network.h -- build and maintain networklink
//
// BSla, 2 Aug 2024
// 24 may 2026: copied from BrokerLink.h and adapted for NTP server use
// 30 jun 2026: now includes OTA handling
// 
// 
#ifndef _NETWORK_H
#define _NETWORK_H


#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
#include "timer.h" 

enum LinkState {AllDown, LinkComingUp, LinkUp};

class Network {
  public:

   Network () {}
   ~Network () {}
   void setup    (const char *hostname); // no ethernet? Use wiFiManager to connect to WiFi
   void setup    (const char *hostname, const char *ssid, const char *password); // connect to WiFi with credentials
   void loop ();     // Arduino style loop
   bool linkIsUp () {return (linkState == LinkUp);}
   void reboot (const char *reason);  // print reason and reboot
   void allowReboot (bool allow); // allow or disallow rebooting
   const char *ls2str (LinkState ls);  // convert link state to string
   const char *getHostname () const { return hostname.c_str (); }
   void setConnected (bool connected) { ethConnected = connected; }
   bool isConnected () { return (linkState == LinkUp); }

 private:
   String hostname;
   String ssid;
   String password;
   bool   useWiFiManager = false;
   LinkState linkState;
   bool   linkWasUp = false;
   bool   clientWasUp = false;
   bool   rebootAllowed = false;
   bool   rebootPending = false;
   String rebootReason;
   WiFiManager wm;
   Timer  startupTimer;
   uint32_t checkCount;
   bool   ethConnected = false; 
   
   void   setupOTA             ();                    // setup over-the-air download  
   void   bringUp              (bool verbose = true); // bring up all
   void   checkState           (bool verbose = true); // check the state of the link and recover
   bool   startNetwork         (bool verbose = true); // setup link
   bool   linkIsUp             (bool verbose = true); // check if link is up
   const  IPAddress* getOwnIP  (bool verbose = true); // what is my IP
   String ip2str               (IPAddress ip);                    // convert an IP to a string
};

extern Network network;
#endif
