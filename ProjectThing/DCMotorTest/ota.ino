// Ex10.cpp/.ino
// OTA update from build/Thing.bin; see also magic.sh ota-httpd

#include <esp_log.h>
#include <Arduino.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>


// what version of the firmware are we? (used to calculate need for updates)
// see firmwareVersion in main.cpp

// IP address and port number
#define FIRMWARE_SERVER_IP_ADDR "87.80.29.87"
#define FIRMWARE_SERVER_PORT    "8000"

int firmwareVersion = 1; // used to check for updates, see Ex10
int loopIteration = 0;
WebServer webServer(80);

// setup ////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  Serial.printf("firmware is at version %d\n", firmwareVersion);

  // get on the network
  WiFi.begin("NOWTV9VJGW", "qwU4e8LyJ6CE");
  uint16_t connectionTries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    if(connectionTries++ % 75 == 0) Serial.println("");
    delay(250);
  }
  Serial.println("\nConnected to network");

  // materials for doing an HTTPS GET on github from the BinFiles/ dir
  HTTPClient http;
  int respCode;
  int highestAvailableVersion = -1;

  // read the version file from the cloud
  respCode = doCloudGet(&http, "version");
  if(respCode > 0 && respCode != 404) {// check response code (-ve on failure)
    highestAvailableVersion = atoi(http.getString().c_str());
    Serial.print("Highest version: ");
    Serial.println(highestAvailableVersion);
  }
  else {
    Serial.printf("couldn't get version! rtn code: %d\n", respCode);
  }
  http.end(); // free resources

  // do we know the latest version, and does the firmware need updating?
  if(respCode < 0) {
    return;
  } else if(firmwareVersion >= highestAvailableVersion) {
    Serial.printf("firmware is up to date\n");
    return;
  }

  // do a firmware update
  Serial.printf(
    "upgrading firmware from version %d to version %d\n",
    firmwareVersion, highestAvailableVersion
  );

  // do a GET for the .bin
  String binName = String(highestAvailableVersion);
  binName += ".bin";
  respCode = doCloudGet(&http, binName);
  int updateLength = http.getSize();
// TODO if this isn't big enough, refuse
  if(respCode > 0 && respCode != 404) // check response code (-ve on failure)
    Serial.printf(".bin code/size: %d; %d\n\n", respCode, updateLength);
  else {
    Serial.printf("failed to get .bin! return code is: %d\n", respCode);
    http.end(); // free resources
    return;
  }


  // debug code, checks how much we can download of the bin file, then aborts
  /*if(Update.begin(updateLength)) {
    Serial.printf("starting OTA may take 2-5 mins to complete...\n");
  }
  WiFiClient s = http.getStream();
  int bytesRead = 0;
  while(s.available() > 0) {
    if(s.read() == -1)
      break;
    bytesRead++;
  }
  Serial.printf("read %d bytes from stream\n", bytesRead);
  Update.abort();
  Serial.printf("OTA aborted\n");
  s.flush();*/

  // write the new version of the firmware to flash
  WiFiClient stream = http.getStream();
  Update.onProgress(handleOTAProgress); // print out progress
  if(Update.begin(updateLength)) {
    Serial.print("starting OTA may take a minute or two...\n");
    Update.writeStream(stream);
    //Serial.println(stream);
    if(Update.end()) {
      Serial.printf("update done, now finishing...\n");
      Serial.flush();
      if(Update.isFinished()) {
        Serial.printf("update successfully finished; rebooting...");
        delay(2000);
        Serial.printf("now...\n\n");
        ESP.restart();
        Serial.printf("now...\n\n");
      } else {
        Serial.printf("update didn't finish correctly :(\n");
        Serial.flush();
      }
    } else {
      Serial.printf("an update error occurred, #: %d\n", Update.getError());
      Serial.flush();
    }
  } else {
    Serial.printf("not enough space to start OTA update :(\n");
    Serial.flush();
  }
  stream.flush();
}

// loop /////////////////////////////////////////////////////////////////////
void loop() {
  int sliceSize = 500000;
  loopIteration++;

  Serial.println("Hello");
  webServer.handleClient(); // serve pending web requests every loop, as Ex09
}

// helper for downloading from cloud firmware server; for experimental
// purposes just use a hard-coded IP address and port (defined above)
int doCloudGet(HTTPClient *http, String fileName) {
  // build up URL from components; for example:
  // http://192.168.4.2:8000/Thing.bin
  String url =
    String("http://") + FIRMWARE_SERVER_IP_ADDR + ":" +
    FIRMWARE_SERVER_PORT + "/" + fileName;
    
  // make GET request and return the response code
  http->begin(url);
  Serial.print("Get request to: ");
  Serial.println(url);
  http->addHeader("User-Agent", "ESP32");
  return http->GET();
}

// callback handler for tracking OTA progress ///////////////////////////////
void handleOTAProgress(size_t done, size_t total) {
  float progress = (float) done / (float) total;
  // dbf(otaDBG, "OTA written %d of %d, progress = %f\n", done, total, progress);

  int barWidth = 70;
  Serial.printf("[");
  int pos = barWidth * progress;
  for(int i = 0; i < barWidth; ++i) {
    if(i < pos)
      Serial.printf("=");
    else if(i == pos)
      Serial.printf(">");
    else
      Serial.printf(" ");
  }
  Serial.printf(
    "] %d %%%c", int(progress * 100.0), (progress == 1.0) ? '\n' : '\r'
  );
  Serial.flush();
}
