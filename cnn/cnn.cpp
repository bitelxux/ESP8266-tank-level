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
  this->logger->log("I'm alive!!");
}

App::App(const char* SSID,
         const char* password,
	 const char* ID, 
	 const char* log_server){
    this->ID = ID;
    this->log_server = log_server;
    this->SSID = SSID;
    this->password = password;
    this->timers = NULL;
    this->logger = new Log(this->ID, this->log_server);

    this->addTimer(&this->t0, APP_TIMER);
    this->addTimer(&this->t1, APP_TIMER);
    this->addTimer(&this->t2, APP_TIMER);

}

void App::addTimer(void* timer, int type){
    TimerNode* pointer = this->timers;

    TimerNode* newTimerNode = new TimerNode();

    Serial.print("Add timer of type ");
    Serial.println(type);

    newTimerNode->type = type;
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

void App::attendUserTimer(TimerNode* timerNode){
    Timer* timer = (Timer*) timerNode->timer;
    if (millis() - timerNode->lastRun >= timer->millis){
        timer->function();
    }
}

void App::attendAppTimer(TimerNode* timerNode){
    AppTimer* timer = (AppTimer*) timerNode->timer;
    if (millis() - timerNode->lastRun >= timer->millis){
        (*this.*timer->function)();
    }
}

void App::attendTimers(){

    TimerNode* timerNode = this->timers;

    if (!timerNode){
	    return;
    }

    while (timerNode != NULL){


	if (timerNode->type == USER_TIMER){
    	    this->attendUserTimer(timerNode); 
        }

	if (timerNode->type == APP_TIMER){
    	    this->attendAppTimer(timerNode); 
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

void App::log(char* msg){
    this->logger->log(msg);
}


void App::connectIfNeeded(){
  // If millis() < 30000L is the first boot so it will try to connect
  // for further attempts it will try with spaces of 60 seconds

  if (WiFi.status() != WL_CONNECTED && (millis() < 30000L || millis() - this->tLastConnectionAttempt > 60000L)){
    Serial.println("Trying to connect");
    this->connect();
  }

  // also if we don't have time, try to update
  if (!this->epochTime){
    Serial.println("NTP time not updated. Trying to");
    this->initNTP();
  }
}

void App::connect(){

  char buffer[100];

  Serial.println("");
  Serial.println("Connecting");

  WiFi.begin(this->SSID, this->password);

  this->tLastConnectionAttempt = millis();
  this->tConnect =  millis();
  while(WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED, !digitalRead(LED));
    delay(100);
    if ((millis() - this->tConnect) > 500){
      Serial.print(".");
      this->tConnect = millis();
    }

    // If it doesn't connect, let the thing continue
    // in the case that in a previous connection epochTime was
    // initizalized, it will store readings for future send
    if (millis() - this->tLastConnectionAttempt >= 30000L){      
      break;
    }    
  }

  Serial.println("");

  if (WiFi.status() == WL_CONNECTED) { 
    IPAddress ip = WiFi.localIP();
    sprintf(this->IP, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    
    sprintf(buffer, "Connected to %s with IP %s", this->SSID, IP);
    this->log(buffer);

    ArduinoOTA.begin();
  }
  else
  {
    Serial.println("Failed to connect");
  }

  this->tLastConnectionAttempt = millis();
  
}



void Log::log(char* msg){
    char buffer[100];
    sprintf(buffer, "%s/log/[%s] %s", this->server, this->ID, msg);
    //Serial.println(buffer);
    String toSend = buffer;
    toSend.replace(" ", "%20");
    send(toSend);
}

void App::handleOTA(){
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
