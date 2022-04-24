#include <Arduino.h>
#include "cnn.h"

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

Log::Log(const char* ID, const char* server){
    this->ID = ID;
    this->server = server;
}

void App::imAlive(){
  Serial.println("I'm alive");
}

App::App(const char* SSID,
         const char* password,
	 const char* ID, 
	 const char* server){
    this->ID = ID;
    this->server = server;
    this->SSID = SSID;
    this->password = password;
    this->timers = NULL;

    this->addTimer(&this->t0);

}

void App::addTimer(Timer* timer){
    TimerNode* pointer = this->timers;

    TimerNode* newTimerNode = new TimerNode();

    newTimerNode->lastRun = 0;
    newTimerNode->timer = timer;
    newTimerNode->next = NULL;


    if (pointer == NULL){
       this->timers = newTimerNode;
       return;
    }

    while (pointer->next != NULL){
        pointer = pointer->next;
    }

    pointer->next = newTimerNode;
}

void App::attendTimers(){

    TimerNode* timerNode = this->timers;

    if (!timerNode){
	    return;
    }

    Serial.println("This is attend timers");

    while (timerNode != NULL){
        Serial.println("There's a timer!");
        if (millis() - timerNode->lastRun >= timerNode->timer->millis){
            Serial.println("Running timer");
            // timerNode->timer->function;
	    (*this.*timerNode->timer->function)();
            timerNode->lastRun =  millis();
	}
        timerNode = timerNode->next;
    }
}


void App::initNTP(){
  // Initialize a NTPClient to get time
  //logger.log("[NTP_UPDATE] Updating NTP time");
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(7200);
  timeClient.update();
  this->epochTime = timeClient.getEpochTime();
  //sprintf(buffer, "epochTime set to %d", epochTime);
  //logger.log(buffer);
}


void Log::log(char* msg){
    char buffer[100];
    sprintf(buffer, "%s/log/[%s] %s", this->server, this->ID, msg);
    String toSend = buffer;
    toSend.replace(" ", "%20");
    send(toSend);
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

void readEEPROM(int regSize){

  unsigned short int counter = readEEPROMCounter();
  unsigned short int cursor = 0;
  int regAddress;
  char buffer[100];

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

