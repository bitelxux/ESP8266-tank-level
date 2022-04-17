
/* Testest with
 -  Arduino IDE 1.8.15
    - Arduino AVR board 1.8.3
    - ESP8266 2.7.0

https://arduino.esp8266.com/stable/package_esp8266com_index.json
board: NodeMCU1.0 (ESP-12E Module)

Install NTPClient from manage libraries
    
*/

//Libraries
#include <EEPROM.h>//https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.h
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>
#include <ArduinoOTA.h> 

// This is for each variable to use it's real size when stored
// in the EEPROM
#pragma pack(push, 1)

// pins mapping

/*
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;x``
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;
*/

//Constants
#define EEPROM_SIZE 4 * 1024 * 1024

#define LED 2
#define LED_BLUE 0
#define LED_RED 4

#define LED_GREEN 5
#define TX 14
#define RX 12

char buffer[100];
unsigned char dataBuffer[4] = {0};
unsigned char CS;
int distance;

char IP[16];

SoftwareSerial sensor(RX, TX);

// prototipes
void initNtp();
void FlushStoredData();
void readEEPROM();
void registerNewReading();
void blinkLed();
void fakeWrite();
void connectIfNeeded();
void handleOTA();
void imAlive();

#define ID "tank.level"

const char* ssid = "Starlink";
const char* password = "82111847";
const char* log_server = "http://192.168.1.162:8888";
const char* baseURL = "http://192.168.1.162:8889/";

unsigned int address = 0;
unsigned int tConnect = millis();
unsigned long tLastConnectionAttempt = 0;
unsigned long tBoot = millis();


// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

int counter = 0; // number of registers in EEPROM
unsigned long epochTime = 0;

struct
{
    boolean enabled;
    unsigned long timer;
    unsigned long lastRun;
    void (*function)();
    char* functionName;
} TIMERS[] = {
  { true, 60*1000, 0, &imAlive, "imAlive" },
  { true, 1*1000, 0, &handleOTA, "handleOTA" },
  { true, 30*1000, 0, &FlushStoredData, "FlushStoredData" },
  { false, 20*1000, 0, &readEEPROM, "readEEPROM" },
  { true, 3600*1000, 0, &initNtp, "initNtp" },
  { true, 1*1000, 0, &registerNewReading, "registerNewReading" },  
  { true, 1*1000, 0, &blinkLed, "blinkLed" },  
  { true, 5*1000, 0, &connectIfNeeded, "connectIfNeeded" },  
};

typedef struct
{
  unsigned long timestamp;
  short int value;
} Reading;

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  
  Serial.begin(115200); 
  EEPROM.begin(EEPROM_SIZE);
  
  // resetEEPROM();
  
  // sensor
  sensor.begin(9600);
}

void handleOTA(){
  ArduinoOTA.handle();
}

bool send(String what){

  if (WiFi.status() != WL_CONNECTED){
    return false;
  }

  bool result;
  WiFiClient client;
  HTTPClient http;
  //Serial.print("sending ");
  //Serial.println(what.c_str());
  http.begin(client, what.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200){
        result = true;
      }
      else {
        Serial.print("[send] Error code: ");
        Serial.println(httpResponseCode);
        result = false;
      }
      // Free resources
      http.end();

      return result;
}

void ledError(){
    digitalWrite(LED_RED, HIGH);
    delay(20);
    digitalWrite(LED_RED, LOW);  
}

void ledOK(){
    digitalWrite(LED_GREEN, HIGH);
    delay(20);
    digitalWrite(LED_GREEN, LOW);  
}

void registerNewReading(){
  short int value;
  char buffer[100];

  // Flush old readings from sensor
  while (sensor.available()){
    delay(5);
    sensor.read();
  }
  delay(20);

  value = readSensor();
  if (value == -1){
    log("Error reading value from sensor");
    ledError();
    return;
  }

  if (epochTime){
    unsigned long now = epochTime + int(millis()/1000);
    sprintf(buffer, "%s/add/%d:%d", baseURL, now, value);

    // try to send to the server
    // if fails, store locally for further retrying
    if (send(buffer)){
      ledOK();
      sprintf(buffer, "Sent %d:%d", now, value);
      log(buffer);
    }
    else{
      ledError();
      writeReading(now, value);
      sprintf(buffer, "Locally stored %d:%d", now, value);
      log(buffer);
    }
        
  }

}


short int mmToLitres(int milimetres){
  return milimetres;
}

int readSensor(){
   if (sensor.available() > 0){
    delay(4);

    if (sensor.read() == 0xFF){
      dataBuffer[0] = 0xFF;
      for (int i=1; i<4; i++){
        dataBuffer[i] = sensor.read();
      }
    }

    //sprintf(buffer, "lectura: %02X,%02X,%02X,%02X", dataBuffer[0], dataBuffer[1], dataBuffer[2], dataBuffer[3]);
    //log(buffer);

    CS = dataBuffer[0] + dataBuffer[1] + dataBuffer[2];
    if (dataBuffer[3] == CS){
      distance = (dataBuffer[1] << 8) + dataBuffer[2];
      return distance;
    }
    else{
      sprintf(buffer, "CS Error: %02X,%02X,%02X,%02X", dataBuffer[0], dataBuffer[1], dataBuffer[2], dataBuffer[3]);
      log(buffer);
      return -1;
    }
   }
     
}


void FlushStoredData(){

  // You have to start server.py at BASE_URL
  int sent = 0;
  
  if (WiFi.status() != WL_CONNECTED){
    log("[FLUSH_STORED_DATA] Skipping. I'm not connected to the internet :-/.");
    return;
  }

  Reading reading;  
  unsigned short int counter = readEEPROMCounter();
  unsigned short int cursor = 0;
  int regAddress;
  short int value;
  bool errors = false;

  int regSize = sizeof(reading);

  char buffer[100];
  //sprintf(buffer, "[FLUSH_STORED_DATA] %d registers", counter);
  //Serial.println(buffer);
  
  for (cursor = 0; cursor < counter; cursor++){
      regAddress = 2 + cursor*regSize; // +2 is to skip the counter bytes
      EEPROM.get(regAddress, reading);

      // if value is -1 that register was already sent
      if (reading.value == -1){
        continue;
      }
      
      sprintf(buffer, "%s/add/%d:%d", baseURL, reading.timestamp, reading.value);

      if (!send(buffer)){
        //sprintf(buffer, "[FLUSH_STORED_DATA] Error sending register [%d]", cursor);
        //log(buffer);
        errors = true;
      }
      else
      {
        ledOK();
        sent ++;
        sprintf(buffer, "[FLUSH_STORED_DATA] Success sending record [%d]", cursor);
        log(buffer);
        // We don't want to write the whole struct to save write cycles
        value = -1;
        EEPROM.put(regAddress + sizeof(reading.timestamp), value);
        EEPROM.commit();            
      }
  }

  if (!errors){
      sprintf(buffer, "[FLUSH_STORED_DATA] %d records sent", sent);
  } else {
      sprintf(buffer, "[FLUSH_STORED_DATA] %d records sent [errors]", sent);    
  }
  
  log(buffer);

}

void initNtp(){
  // Initialize a NTPClient to get time
  log("[NTP_UPDATE] Updating NTP time");
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(7200);  
  timeClient.update();
  epochTime = timeClient.getEpochTime();
  sprintf(buffer, "epochTime set to %d", epochTime);
  log(buffer);
}


void readEEPROM(){
  
  unsigned short int counter = readEEPROMCounter();
  unsigned short int cursor = 0;
  int regAddress;
  Reading reading;

  int regSize = sizeof(reading);

  sprintf(buffer, "%d registers currently stored in EEPROM", counter);
  log(buffer);
  
  /*
  for (cursor = 0; cursor < counter; cursor++){
    regAddress = 2 + cursor*regSize; // +2 is to skip the counter bytes
    EEPROM.get(regAddress, reading);
    sprintf(buffer, "Address %d: %d:%d", regAddress, reading.timestamp, reading.value);
    log(buffer);
  }
  */
    
}

void resetEEPROM(){
  
  int address = 0;
  unsigned short int counter=0;
  EEPROM.put(address, counter);
  EEPROM.commit();  
  int a = 1/0; // I don't want to continue
}

unsigned short int readEEPROMCounter(){
  unsigned short int counter;
  EEPROM.get(0, counter);
  return counter;
}

void writeReading(unsigned long in_timestamp, short int in_value){
  
  int address;
  int regAddress;
  unsigned short int counter = readEEPROMCounter();
  unsigned short int cursor;
 
  Reading newReading;
  Reading currentReading;
  
  newReading.timestamp = in_timestamp;
  newReading.value = in_value;

  int regSize = sizeof(newReading);
  
  for (cursor = 0; cursor < counter; cursor++){
    regAddress = sizeof(counter) + cursor*regSize; // +sizeof counter is to skip the counter bytes
    EEPROM.get(regAddress, currentReading);

    if (currentReading.value == -1){ // Re-use old register
      sprintf(buffer, "Reused position %d", regAddress);
      log(buffer);
      EEPROM.put(regAddress, newReading);
      if (!EEPROM.commit()) {
        log("Commit failed on re-use");
      }
      break;
    }
  }

  if (cursor == counter){ // new register
      regAddress = sizeof(counter) + cursor*regSize; // +sizeof counter is to skip the counter bytes

      // Serial.print("New register at ");
      // Serial.println(regAddress);

      EEPROM.put(regAddress, newReading);
      counter += 1;
      EEPROM.put(0, counter); 
      if (!EEPROM.commit()) {
        log("Commit failed on new register");
      }
  }
  
}

void attendTimers(){
    byte NUM_TIMERS = (sizeof(TIMERS) / sizeof(TIMERS[0]));
    for (int i=0; i<NUM_TIMERS; i++){
      if (TIMERS[i].enabled && millis() - TIMERS[i].lastRun >= TIMERS[i].timer) {
        TIMERS[i].function();
        TIMERS[i].lastRun = millis();
      }
    }
}

void connect(){

  char buffer[100];
  
  Serial.println("");
  Serial.println("Connecting");

  WiFi.begin(ssid, password);

  tLastConnectionAttempt = millis();
  tConnect =  millis();
  while(WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED, !digitalRead(LED));
    delay(100);
    if ((millis() - tConnect) > 500){
      Serial.print(".");
      tConnect = millis();
    }

    // If it doesn't connect, let the thing continue
    // in the case that in a previous connection epochTime was
    // initizalized, it will store readings for future send
    if (millis() - tLastConnectionAttempt >= 30000L){      
      break;
    }    
  }

  Serial.println("");

  if (WiFi.status() == WL_CONNECTED) { 
    IPAddress ip = WiFi.localIP();
    sprintf(IP, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    
    sprintf(buffer, "Connected to %s with IP %s", ssid, IP);
    log(buffer);

    ArduinoOTA.begin();
  }
  else
  {
    Serial.println("Failed to connect");
  }

  tLastConnectionAttempt = millis();
  
}

void blinkLed(){
  digitalWrite(LED, !digitalRead(LED));
}

void log(char* msg){
  char buffer[100];
  sprintf(buffer, "%s/log/[%s] %s", log_server, ID, msg);
  String toSend = buffer;
  toSend.replace(" ", "%20");
  send(toSend);
}

void imAlive(){
  IPAddress ip = WiFi.localIP();
  char msg[50];
  static long cont = 0;
  sprintf(msg, "I'm Alive!! IP: %s [%d]", IP, ++cont);
  log(msg);

  sprintf(buffer, "size of record: %d", sizeof(Reading));
  log(buffer);
}

void connectIfNeeded(){
  // If millis() < 30000L is the first boot so it will try to connect
  // for further attempts it will try with spaces of 60 seconds

  if (WiFi.status() != WL_CONNECTED && (millis() < 30000L || millis() - tLastConnectionAttempt > 60000L)){
    Serial.println("Trying to connect");
    connect();
  }  

  // also if we don't have time, try to update
  if (!epochTime){
    Serial.println("NTP time not updated. Trying to");
    initNtp();
  }
}

void loop() {
  attendTimers();
  delay(20);
}
