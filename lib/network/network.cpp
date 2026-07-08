//
// network.cpp -- build and maintain network link
//
//
// In PlatformIO.ini, define either NETWORK_USING_WIFI or NETWORK_USING_ETH to select the network interface to use. Do not define both.
// To debug this module, define NETWORK_DEBUG to 1 in platformio.ini. This will print debug messages to the serial port.
// Use build_flags = -D NETWORK_USING_WIFI -D NETWORK_DEBUG=1 in platformio.ini to enable debug messages for this module.
//
// Add the following library to platformio.ini:
// lib_deps =
//    tzapu/WiFiManager @ ^2.0.17
//
// BSla, 2 Aug 2024
// 24 may 2026: copied from BrokerLink.cpp and adapted for general use
// 30-Jun-2026: added OTA functionality
//
#include <Arduino.h>
#include <string.h>


#if defined(NETWORK_USING_WIFI)
#if defined(NETWORK_USING_ETH)
#error "You must define either NETWORK_USING_WIFI or NETWORK_USING_ETH in platformio.ini, but not both"
#endif // NETWORK_USING_ETH
#elif defined(NETWORK_USING_ETH)
#else
#error "You must define either NETWORK_USING_WIFI or NETWORK_USING_ETH in platformio.ini (build_flags =...)"
#endif


#ifndef NETWORK_DEBUG
#define NETWORK_DEBUG 0
#endif

#define _DEBUG NETWORK_DEBUG
#include "debug.h"


#if defined(NETWORK_USING_WIFI)
#include <WiFi.h>
#elif defined(NETWORK_USING_ETH)
#include <ETH.h>
#endif // NETWORK_USING_WIFI

#include <WiFiClient.h>
#include <ArduinoOTA.h>

#include "network.h" // own definition module

// callbacks for WiFi/Ethernet events
static void onEvent (arduino_event_id_t event);
static void connectCallback ();
static void disconnectCallback ();


static const timeType StartupTime      (3 MINUTES); // time to wait for the link to come up before rebooting


Network network;


// Arduino-like setup function. This function is called once at startup to setup the network link
// It returns when the link is up. If it fails, it will reboot the ESP, so if it returns, the link is up.
//
void Network::setup (const char *hostname)
{

   LOG (">  Network::setup %s\n", hostname);
   this->hostname = String (hostname);
   this->ssid = String ("");
   this->password = String ("");
   linkState = AllDown;
   useWiFiManager = true; // we are using WiFiManager to connect to WiFi
   bringUp (true);

   LOG ("<  Network::setup done\n");
}

void Network::setup (const char *hostname, const char *ssid, const char *password)
{
   LOG (">  Network::setup hostname=%s, ssid=%s, password=********\n", hostname, ssid);
   this->hostname = String (hostname);
   this->ssid = String (ssid);
   this->password = String (password);
   linkState = AllDown;
   useWiFiManager = false; // we are not using WiFiManager to connect to WiFi
   bringUp (true);

   LOG ("<  Network::setup\n");
}

// Arduino-like loop function. Once every ... loops, check the link state and recover, and
// handle OTA requests.
//
void Network::loop ()
{
   if (checkCount++ % 1000 == 0) {
      checkState (false);
      if (isConnected ()) {
         ArduinoOTA.handle ();
      }     
   }
}

// 
// perform a reboot
//
void Network::reboot (const char *reason)
{
  if (rebootAllowed) {
    ERROR ("Network::reboot: %s; restarting ESP\n", reason);
    delay (5 SECONDS);
    ESP.restart ();
    while (true);
  }
  else {
     rebootPending = true;
     rebootReason = String (reason);
  }
}

//
// allow or disallow rebooting. We do not want to reboot while
// any critical operation is in progress
//
void Network::allowReboot (bool allow)
{
  rebootAllowed = allow;
  if (allow && rebootPending) {
     reboot (rebootReason.c_str ());
  }
}


const char *Network::ls2str (LinkState ls)
{
  static const char *ad  = "AllDown";
  static const char *lcu = "LinkComingUp";
  static const char *lu  = "LinkUp";

  return (ls == AllDown          ? ad
          : ls == LinkComingUp   ? lcu
          :                        lu);

}

// private methods


// bring up the link. If anything goes wrong, reboot the ESP. This method has no return value,
// if it fails, it will reboot, so if it returns, the link is up.
// 
void Network::bringUp (bool verbose)
{
   startupTimer.start (StartupTime); // should be long enough to bring up the link
   bool r = true;
   while ((linkState != LinkUp) && (!startupTimer.hasExpired ())) {
      if (verbose) {LOG (".");}
      switch (linkState) {
       case AllDown:
         if ((r = startNetwork (verbose))) linkState = LinkComingUp;
         break;

       case LinkComingUp:
         if ((r = linkIsUp (false))) linkState = LinkUp;
         break;

       case LinkUp:
         if (!(r = linkIsUp (verbose))) {
            ERROR ("Network::bringUp: lost link in linkUp\n");
            linkState = AllDown;
         }
         else {
            if (r && verbose) LOG ("   Network::bringUp: network is up\n");
         }
         break;

      }
      if (!r) delay (5 SECONDS); // wait a bit before retrying
   }
   LOG ("\n");

   if (startupTimer.hasExpired ()) {
      LOG ("   Network::bringUp: startup timer expired; state = %s\n", ls2str (linkState));
      r = false;
      reboot ("Network::bringUp: startup takes too long: timeout");
   }
   startupTimer.stop ();
   if (verbose) {LOG ("<  Network::bringUp: network is up\n");}
}

//
// check the link state. Reboot if not what we want
void Network::checkState (bool verbose)
{
   if (linkState == LinkUp && !linkIsUp (false)) {
      ERROR ("Network::checkState: link is not up\n");
      linkState = AllDown;
   }

   if (linkState != LinkUp) {
      if (verbose) {LOG (">  Network::checkState: state = %s; bringing up\n", ls2str (linkState));}
      bringUp ();  
   }
}

//
// bring up the link, or reboot if it fails
bool Network::startNetwork (bool verbose)
{
   bool r = true;

#if defined(NETWORK_USING_WIFI)   

   if (verbose) {LOG (">  Network::startNetwork: logging in on wifi network\n");}
   //WiFi.config (*getOwnIP (), gateway, subnet, dns);
   WiFi.mode (WIFI_STA);
   WiFi.onEvent (onEvent);
   if (!useWiFiManager) {
      if (verbose) {
         LOG ("   Network::startNetwork: connecting to WiFi network %s using credentials\n", 
            ssid.c_str ());
      }
      WiFi.begin (ssid.c_str (), password.c_str ());
   }
   else {
      if (verbose) {LOG ("   Network::startNetwork: connecting to WiFi using WiFimanager\n");}
      wm.setHostname (getHostname ());
      String s = String (getHostname ()) + " Setup";
      if (!wm.autoConnect (s.c_str ())) {
         ERROR ("Network::startNetwork: failed to connect\n");
         delay (3000);
         ESP.restart (); // reboot to set wifi config      
      }
   }
   if (verbose) {LOG ("<  Network::startNetwork: success\n");}

#elif defined(NETWORK_USING_ETH)
   if (verbose) {LOG ("   Network::startNetwork: configuring ETH\n");}

   WiFi.onEvent (onEvent);
   if (verbose) {LOG ("   Network::startNetwork: connecting to Ethernet: ETH.begin()\n");}
   ETH.begin ();
   if (verbose) {LOG ("   Network::startNetwork: waiting for Ethernet to connect..");}
   while (!ethConnected) { //!ETH.linkUp ()) {
      delay (500);
      if (verbose) {LOG (".");}
   }
#endif
   if (verbose) {LOG ("   Network::startNetwork: waiting for DHCP\n");}
   Timer t (10 SECONDS);
#if defined (NETWORK_USING_WIFI)
   while (WiFi.localIP () == IPAddress (0, 0, 0, 0)) {
#elif defined (NETWORK_USING_ETH)
   while (ETH.localIP () == IPAddress (0, 0, 0, 0)) {
#endif
      if (t.hasExpired ()) {
         ERROR ("Did not get IP address from DHCP server\n");
         r = false;
         break;
      } 
   }

   if (verbose) {
#if defined (NETWORK_USING_WIFI)
     String mac = String (WiFi.macAddress ());
     String ip = ip2str (WiFi.localIP ());
#elif defined (NETWORK_USING_ETH)
     String mac = String (ETH.macAddress ());
     String ip = ip2str (ETH.localIP ());
#endif
     NOTE ("   Network::startNetwork: MAC address = %s, my IP = %s\n", mac.c_str (), ip.c_str ());
   }

   if (r) {
      setupOTA ();
      if (verbose) {LOG ("<  Network::startNetwork: success\n");}
   }
   else ERROR ("Network::startNetwork FAILED\n");
   return r;
}



//
// check if link is up
bool Network::linkIsUp (bool verbose)
{
   //if (verbose) {LOG (">  Network::linkIsUp ()\n");}
   bool isUp = true;
#ifdef NETWORK_USING_WIFI
   isUp = (WiFi.status () == WL_CONNECTED);
#elif defined(NETWORK_USING_ETH)
   isUp = ethConnected;//ETH.linkUp ();
#endif

   if (isUp != linkWasUp) {
      if (verbose) { LOG (">< Network::linkIsUp () = %s\n", toCCP (isUp));}
      linkWasUp = isUp;
   }
  
   return isUp;
}

//
// Setup Over The Air download
void Network::setupOTA ()
{
   LOG (">  Network::setupOTA ()\n");
   // port defaults to 3232
   //ArduinoOTA.setPort (3232);

   // set hostname
   ArduinoOTA.setHostname (getHostname ());
   ArduinoOTA.setPassword ("OtoOtata");
   ArduinoOTA.onStart ([]()
   {
      String type = (ArduinoOTA.getCommand () == U_FLASH)? "sketch":"filesystem";
      // NOTE: if updating SPIFFS/LittleFS, unmount the FS here
   });

   ArduinoOTA.onEnd ([]()
   {
      // Finished
   });

   ArduinoOTA.onProgress ([](unsigned int progress, unsigned int total)
   {
       // Log progress here if a serial debug connection is present
   });

   ArduinoOTA.onError ([](ota_error_t error)
   {
      // handle errors gently
   });

   ArduinoOTA.begin ();
   LOG ("<  Network::setupOTA\n");
}

//
// convert an IP to a string
String Network::ip2str (IPAddress ip)
{
   String s;
   for (int i = 0; i < 4; i++) {
      s += int (ip[i]);
      if (i < 3) s += '.';
   }
   return s;
}

//
// Event handler for Ethernet events
static void onEvent (arduino_event_id_t event) {
  switch (event) {

#if defined(NETWORK_USING_WIFI)
    case ARDUINO_EVENT_WIFI_READY:
      LOG ("   Network::onEvent: WiFi interface ready\n");
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      LOG ("   Network::onEvent: WiFi scan done\n");
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      //LOG ("   Network::onEvent: WiFi STA start\n");
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      LOG ("   Network::onEvent: WiFi STA stop\n");
      network.setConnected (false);
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      //LOG ("   Network::onEvent: WiFi STA connected\n");
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      LOG ("   Network::onEvent: WiFi STA disconnected\n");
      network.setConnected (false);
      break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
      LOG ("   Network::onEvent: WiFi STA auth mode change\n");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      LOG ("   Network::onEvent: WiFi STA got IP\n");
      network.setConnected (true);
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
      LOG ("   Network::onEvent: WiFi STA got IPv6\n");
      network.setConnected (true);
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      LOG ("   Network::onEvent: WiFi STA lost IP\n");
      network.setConnected (false);
      break;
    case ARDUINO_EVENT_WIFI_AP_START:
      LOG ("   Network::onEvent: WiFi AP start\n");
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      LOG ("   Network::onEvent: WiFi AP stop\n");
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      LOG ("   Network::onEvent: WiFi AP station connected\n");
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      LOG ("   Network::onEvent: WiFi AP station disconnected\n");
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      LOG ("   Network::onEvent: WiFi AP station IP assigned\n");
      break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
      LOG ("   Network::onEvent: WiFi AP probe request received\n");
      break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
      LOG ("   Network::onEvent: WiFi AP got IPv6\n");
      break;
    case ARDUINO_EVENT_WIFI_FTM_REPORT:
      LOG ("   Network::onEvent: WiFi FTM report\n");
      break;
#endif
#ifdef NETWORK_USING_ETH
    case ARDUINO_EVENT_ETH_START:
      //LOG ("   Network::onEvent: ETH Started\n");
      // The hostname must be set after the interface is started, but needs
      // to be set before DHCP, so set it from the event handler thread.
      ETH.setHostname (network.getHostname ());
      break;

    case ARDUINO_EVENT_ETH_STOP:
      LOG ("   Network::onEvent: ETH Stopped\n");
      network.setConnected (false);
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      //LOG ("   Network::onEvent: ETH Connected\n");
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      LOG ("   Network::onEvent: ETH Disconnected\n");
      network.setConnected (false);
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      //LOG ("   Network::onEvent: ETH Got IP Address %s\n", ETH.localIP().toString().c_str());
      network.setConnected (true);
      break;

#endif      
    default: 
      LOG ("   Network::onEvent: received ETH unknown event %d\n", event);
      break;
  }
}
