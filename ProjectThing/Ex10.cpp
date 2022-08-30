// Ex10.cpp/.ino
// OTA update from build/Thing.bin; see also magic.sh ota-httpd

#include "Thing.h"

#include <HttpClient.h>
#include <Adafruit_MotorShield.h>

using namespace std;

// what version of the firmware are we? (used to calculate need for updates)
// see firmwareVersion in main.cpp

// IP address and port number: CHANGE THE IP ADDRESS!
#define FIRMWARE_SERVER_IP_ADDR "192.168.0.37"
#define FIRMWARE_SERVER_PORT    "8000"

// Init MotorShield & Motors:
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *leftMotor = AFMS.getMotor(4);
Adafruit_DCMotor *rightMotor = AFMS.getMotor(3);

enum Motors { LEFT, RIGHT };

const int DEFAULT_SPEED = 150;
const int DEFAULT_DURATION = 5;

const String targetFile = "instructions.txt";
String previousContents = "";

// Split String into String array by delimiter:
vector<String> splitString(String string, char delimiter){
  string.trim();

  vector<String> vect;
  
  int from = 0;
  int to = string.indexOf(delimiter);
  if (to == -1){
    vect.push_back(string);
    return vect;
  }

  int occurances = 1;

  for (int i = 0; i < string.length(); i++){
    if (string[i] == delimiter){
      occurances++;
    }
  }

  for (int i = 0; i < occurances; i++){
    vect.push_back(string.substring(from, to) + '\0');
    from = to + 1;
    to = string.indexOf(delimiter, to + 1);
    if (to == -1){
      to = string.length();
    }
  }

  return vect;
}

void doInstruction(String dir, int spd, Motors motor){
  dir.toLowerCase();
  
  if (motor == Motors::LEFT){
    leftMotor->setSpeed(spd);
    if (dir.startsWith("forward")){
        leftMotor->run(FORWARD);
        Serial.printf("Left motor moving %s at speed %d. \n", dir, spd);
      } else if (dir.startsWith("backward")){
        leftMotor->run(BACKWARD);
        Serial.printf("Left motor moving %s at speed %d. \n", dir, spd);
      } else if (dir.equals("stop") or dir.equals("release")){
        leftMotor->run(RELEASE);
        Serial.println("Left motor stopped.");
      } else{
        leftMotor->run(RELEASE);
        Serial.println("Invalid input for direction, left motor will stop.");
      }
      return;
  } if (motor == Motors::RIGHT){
    rightMotor->setSpeed(spd);
      if (dir.startsWith("forward")){
        rightMotor->run(FORWARD);
        Serial.printf("Right motor moving %s at speed %d. \n", dir, spd);
      } else if (dir.startsWith("backward")){
        rightMotor->run(BACKWARD);
        Serial.printf("Right motor moving %s at speed %d. \n", dir, spd);
      } else if (dir.equals("stop") or dir.equals("release")){
        rightMotor->run(RELEASE);
        Serial.println("Right motor stopped.");
      } else{
        rightMotor->run(RELEASE);
        Serial.println("Invalid input for direction, right motor will stop.");
      }
      return;
  }
   
  Serial.println("Invalid motor value provided. Motors will not be instructed.");
}

// Clamp a value between a minimum & a maximum:
int clampValue(int value, int _min, int _max){
  if (value > _max){
    return _max;
  } else if (value < _min){
    return _min;
  } else{
    return value;
  }
}

// Get instruction and output valid entries for speed & direction:
vector<String> readCommand(String string){
  // Get motor instructions:
  vector<String> vals = splitString(string, '-');
  // Split into direction and speed:
  String dir = vals[0];
  // If direction not provided:
  if (dir.toInt() != 0){
    // Assume only value is speed, add another value so spd call below will default.
    // Direction is defaulted in doInstruction().
    vals.push_back(dir);
  }
  int spd;
  // If no speed value inputted:
  if (vals.size() == 1){
    spd = DEFAULT_SPEED;
    Serial.printf("No speed value detected in command [ %s ], defaulting to %d.\n", string.c_str(), DEFAULT_SPEED);
  }
  else
    spd = vals[1].toInt();
  // Validate speed value:
  spd = clampValue(spd, 0, 255);

  vector<String> output;
  output.push_back(dir);
  output.push_back(String(spd));

  return output;
}

// Read the instructions & feed them to the motors:
void readInstructions(String instr){
  Serial.println("Reading current instructions: [ " + instr + " ]");
  blink(1);
  // Split the instructions into *LEFTMOTOR-SPEED, RIGHTMOTOR-SPEED, DURATION*
  vector<String> instructions = splitString(instr, ' ');
  
  if (instructions.size() != 3){
    // If duration is the only missing value:
    if (instructions.size() == 2 and instructions[0].toInt() == instructions[1].toInt()){
      // Add a default value:
      instructions.push_back(String(DEFAULT_DURATION));
      Serial.println("Invalid Instructions...\nAssuming duration value missing, defaulting to " + String(DEFAULT_DURATION) + " seconds.");
    } else{
      Serial.println("Invalid instructions...\nSupply instructions in the following format:\nLEFTMOTOR-SPEED, RIGHTMOTOR-SPEED, DURATION");
      return;
    }    
  }
  
  int duration = instructions[2].toInt();  

  // Get left motor instructions:
  vector<String> vals = readCommand(instructions[0]);
  // Split into direction and speed:
  String leftDir = vals[0];
  int leftSpeed = vals[1].toInt();

  // Get right motor instructions:
  vals = readCommand(instructions[1]);
  // Split into direction and speed:
  String rightDir = vals[0];
  int rightSpeed = vals[1].toInt();

  // Carry out instructions:
  doInstruction(leftDir, leftSpeed, Motors::LEFT);
  doInstruction(rightDir, rightSpeed, Motors::RIGHT);

  delay(duration * 1000);
  leftMotor->run(RELEASE);
  rightMotor->run(RELEASE);

  Serial.println();
}

void checkAbort(){
  if (Serial.available() > 0){
    // If there is any input to the serial monitor, take it and format it:
    String input = Serial.readString();
    input.toLowerCase();
    input.trim();
    if (input.startsWith("\n")){
      input = input.substring(2, input.length());
    } else if (input.endsWith("\n")){
      input = input.substring(0, -2);
    }

    // Check if it is a valid abort message:
    if (input.equals("stop") or input.equals("exit") or input.equals("abort") or input.equals("quit")){
      Serial.println("Aborting execution...");
      for(int i = 0; i < 10; i++){
        blink(1);
        Serial.print(".");
        delay(0.5);
      }
      Serial.println("\nBye!");
      // Abort execution:
      esp_deep_sleep_start();
    }
  }
}

// Pull instructions from server & process them:
void requestInstructions(){
  blink(5);
  HTTPClient http;
  int respCode;
  
  respCode = doCloudGet(&http, targetFile);
  int updateLength = http.getSize();
  
  if(respCode > 0 && respCode != 404) { // check response code (-ve on failure)
    Serial.printf("Retrived %s. Return Code: %d; Size: %d\n\n", targetFile.c_str(), respCode, updateLength);
  } else {
    Serial.printf("Failed to get %s. Return code: %d\n", targetFile.c_str(), respCode);
    http.end(); // free resources
    return;
  }

  // Get contents of file:
  String contents = http.getString();
  if(!contents.equals(previousContents)){
    Serial.println("New instructions detected!");
    previousContents = contents;
  
    vector<String> lines = splitString(contents, '\n');
  
    for (int i = 1; i < lines.size(); i++){
      if (lines[i] != NULL){
        checkAbort();
        readInstructions(lines[i]);
      }
    }    
    Serial.print("Instructions complete!");
  } else{
    Serial.print("No new instructions detected.");  
  }
  int runtime = ceil(millis() / 1000);
  Serial.printf(" System running for %d seconds.\n\n", runtime);
  http.end();
}

// setup ////////////////////////////////////////////////////////////////////
void setupRobot() {
  setupServer(); // include the AP and network joining webserver stuff from Ex09
  dln(startupDBG, "\nRobot Setup..."); // debug printout

  Serial.print("Connecting to Motor Shield: ");
  // Setup AFMS:
  if (!AFMS.begin()) {
    Serial.println("Could not find Motor Shield :(");
    while (1);
  }
  Serial.println("Motor Shield found!");

  Serial.println("Attempting Network Connection...");
  // get on the network
  WiFi.begin(); // register MAC first! and add SSID/PSK details if needed
  uint16_t connectionTries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    if(connectionTries++ % 75 == 0) Serial.println("");
    delay(250);
  }
  Serial.println("Connected to Network!\n");

  rightMotor->run(RELEASE);
  leftMotor->run(RELEASE);

  // Get Instructions:
  requestInstructions();
}

// loop /////////////////////////////////////////////////////////////////////
void loopRobot() {
  int sliceSize = 500000;
  loopIteration++;
  if(loopIteration % sliceSize == 0){ // a slice every sliceSize iterations
    dln(otaDBG, "OTA loop");
  }

  int requestFreq = 10000;
  
  webServer.handleClient(); // serve pending web requests every loop, as Ex09
  if(loopIteration % requestFreq == 0){ // a slice every requestFreq iterations
    // Get instructions:
    requestInstructions();
  }

  // Abort message:
  checkAbort();
}

// helper for downloading from cloud firmware server; for experimental
// purposes just use a hard-coded IP address and port (defined above)
int doCloudGet(HTTPClient *http, String fileName) {
  // build up URL from components; for example:
  // http://192.168.4.2:8000/Thing.bin
  String url =
    String("http://") + FIRMWARE_SERVER_IP_ADDR + ":" +
    FIRMWARE_SERVER_PORT + "/" + fileName;
  Serial.printf("Getting %s...\n", url.c_str());

  // make GET request and return the response code
  http->begin(url);
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
  // Serial.flush();
}
