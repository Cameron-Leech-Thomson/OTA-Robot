// Ex08.cpp/.ino
// become a web client and send your email to our cloud server over HTTPS

#include "Thing.h"

// globals and utilities for connecting to COM3505 cloud server
WiFiClientSecure com3505Client; // the TLS web client
const char *com3505Addr    = "SERVER IP ADDRESS";  // server IP
const int   com3505Port   = 9194;                  // server port
const char* wifiSsid      = "CHANGE_ME";           // wifi SSID
const char* wifiPassword  = "CHANGE_ME_TOO";       // wifi PSK
const char* myEmail       = "AND_CHANGE_ME_AS_WELL@sheffield.ac.uk";

// x509 certificate for the server
const char* cert = \
  "-----BEGIN CERTIFICATE-----\n" \
  "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" \
  "tTubn3rv/Zry2T4q0l8JQiziE6Zq5+BEMhj5ipo=" \
  "-----END CERTIFICATE-----\n";

void setup08() {
  setup07(); dln(startupDBG, "\nsetup08...");

  // a URI for the request
  String url;
  url = "/com3505-2021?email=";
  url += myEmail;
  url += "&mac=";
  url += MAC_ADDRESS;

  // conect to the uni network
  // (if your device is already provisioned you can use "WiFi.begin();" here)
  dbg(netDBG, "connecting to wifi...");
  WiFi.begin();
//  WiFi.begin(wifiSsid, wifiPassword);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    dbg(netDBG, ".");
  }

  dbg(netDBG, "wifi connected; IP address: ");
  dln(netDBG, WiFi.localIP());

  // now we'll try and talk to the "cloud" server; this is
  // in setup instead of loop so we don't send too much data to the server...
  //
  // first connect to the cloud server or reset after pause
  if(! cloudConnect()) {
    Serial.println("** OOOPS! no connection to server; restarting in 10 **");
    delay(10000);
    ESP.restart();
  }
    
  // send the request to the server
  cloudGet(url);

  // read all the lines of the reply from server and print to serial
  while(cloudAvailable()) dbg(netDBG, cloudRead());

  // job's a goodun
  dln(netDBG, "closing connection...");
  cloudStop();
} // setup08()

void loop08() {
  // serve pending web requests
  webServer.handleClient();
} // loop08()

// use WiFiClient class to create TCP connection to cloud
bool cloudConnect() {
  dbg(netDBG, "\nconnecting to ");
  dbg(netDBG, com3505Addr);
  dbg(netDBG, ":");
  dln(netDBG, com3505Port);

  // configure the TLS client for connection; ordinarily we would do
  // "setCACert(...)" but the server is using a self-signed certificate so the
  // CA chain doesn't exist; instead we just give the client certificate and
  // do "setInsecure" - the connection should still be encrypted, but more
  // vulnerable to a MitM attack
  com3505Client.setCertificate(cert); // client verification only
  // setInsecure doesn't exist in some of the Arduino core versions that we
  // may be using, so this is commented here; try uncommenting, and see the
  // notes: https://iot.unphone.net/#patch-the-core-for-ex08
  com3505Client.setInsecure(); // tell mbedTLS to ignore the CA chain

  // open the connection, or punt
  if(com3505Client.connect(com3505Addr, com3505Port)) {
    dln(netDBG, "connected to com3505 server; doing GET");
  } else {
    dbg(netDBG, com3505Addr);
    dln(netDBG, " - no com3505 server");
    return false;
  }
  return true;
} // cloudConnect()

// do a GET request on com3505Client
void cloudGet(String url) {
  dbg(netDBG, "requesting URL: ");
  dln(netDBG, url);
  com3505Client.print(
    String("GET ") + url + " HTTP/1.1\r\n" +
    "Host: " + com3505Addr + "\r\n" + "Connection: close\r\n\r\n"
  );
  unsigned long timeout = millis();
  while(com3505Client.available() == 0) {
    if(millis() - timeout > 5000) {
      dln(netDBG, ">>> client timeout !");
      cloudStop();
      return;
    }
  }
} // cloudGet()

// close cloud connection
void cloudStop() { com3505Client.stop(); }

// read a line from the cloud client after a request
String cloudRead() { return com3505Client.readStringUntil('\r'); }

// is there more to read from the response?
bool cloudAvailable() { return com3505Client.available(); }
