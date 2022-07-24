#include <Arduino.h>
#include "cnn.h"

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

Timer::Timer(){
}

Log::Log(App* app, const char* ID, const char* server){
    this->app = app;
    this->ID = ID;
    this->server = server;
}

void Log::log(char *msg){
    char buffer[100];
    sprintf(buffer, "%s/log/[%s] %s", this->server, this->ID, msg);
    //Serial.println(buffer);
    String toSend = buffer;
    toSend.replace(" ", "%20");
    this->app->send(toSend);
}

void App::imAlive(){
  char buffer[30];
  sprintf(buffer, "[%s] I'm alive!!", this->IP);
  this->logger->log(buffer);
}

App::App(const char* ID, const char* log_server){
    this->ID = ID;
    this->log_server = log_server;
    this->SSID = SSID;
    this->password = password;
    this->timers = NULL;
    this->logger = new Log(this, this->ID, this->log_server);

    this->addTimer(60000, &App::imAlive, "imAlive");
    this->addTimer(1000, &App::handleOTA, "handleOTA");
    this->addTimer(1000, &App::blinkLED, "blinkLED");
    this->addTimer(60 * 1000, &App::voidUpdateNTP, "voidUpdateNTP");

    pinMode(LED, OUTPUT);

    // WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it around
    this->wifiManager = new WiFiManager();

}

void App::startWiFiManager(){
  //this->wifiManager->resetSettings();
  
  this->wifiManager->autoConnect("TankLevel");
  
  IPAddress ip = WiFi.localIP();
  sprintf(this->IP, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    
  this->initNTP();
  ArduinoOTA.begin();
}


void App::blinkLED(){
     digitalWrite(this->LED, !digitalRead(this->LED));
}

void App::addTimer(int millis, AppCallback function, char*name){

	Timer* newTimer = new Timer();

	newTimer->type = APP_TIMER;
	newTimer->millis = millis;
	newTimer->appFunction = function;
	newTimer->name = name;
	newTimer->lastRun = 0;
	newTimer->next = NULL;

	Timer* timer = this->timers;
	if(!timer){
	    this->timers = newTimer;
	    return;
	}

	while(timer->next){
            timer = timer->next;
	}

	timer->next = newTimer;
}


void App::addTimer(int millis, function_callback function, char*name){

	Timer* newTimer = new Timer();

	newTimer->type = USER_TIMER;
	newTimer->millis = millis;
	newTimer->function = function;
	newTimer->name = name;
	newTimer->lastRun = 0;
	newTimer->next = NULL;

	Timer* timer = this->timers;
	if(!timer){
	    this->timers = newTimer;
	    return;
	}

	while(timer->next){
            timer = timer->next;
	}

	timer->next = newTimer;
}


void App::attendTimers(){

    Timer* timer = this->timers;

    if (!timer){
	    return;
    }

    while (timer != NULL){

        if (millis() - timer->lastRun > timer->millis){
	    if (timer->type == USER_TIMER){
    	       timer->function();
            }

	    if (timer->type == APP_TIMER){
    	       (*this.*timer->appFunction)();
            }
	    timer->lastRun = millis();
        }

        timer = timer->next;
    }
}

void App::initNTP(){

  char buffer[50];
  // Initialize a NTPClient to get time
  //logger.log("[NTP_UPDATE] Updating NTP time");
  // Set offset time in seconds to adjust for your timezone, for example:
  timeClient.begin();
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(0);
  this->updateNTP();
    
}

bool App::updateNTP(){
  char buffer[100];
  if (timeClient.update()){
      this->tEpoch = timeClient.getEpochTime();
      this->tEpochOffset = millis()/1000;
      return true;
  }
  else
  {
      this->log("Error updating NTP time");
      return false;
  }
}

// For timer
void App::voidUpdateNTP(){
  char buffer[100];
  if (timeClient.update()){
      this->tEpoch = timeClient.getEpochTime();
      this->tEpochOffset = millis()/1000;
  }
  else
  {
      this->log("Error updating NTP time");
  }
}

unsigned long App::getEpochSeconds(){
  if (this->tEpoch){
      return this->tEpoch + millis()/1000 - this->tEpochOffset;
  }
  else{
      return 0;
  }
}

void App::log(char* msg){
    this->logger->log(msg);
}

void App::handleOTA(){
  ArduinoOTA.handle();
}

bool App::send(String what){

  bool result;

  if (WiFi.status() != WL_CONNECTED){
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  //Serial.print("sending ");
  //Serial.println(what.c_str());
  http.begin(client, what.c_str());
  http.addHeader("Content-Type", "application/json");
  http.addHeader("device_id", this->ID);

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

String App::get(String what){

  if (WiFi.status() != WL_CONNECTED){
    return "NOWIFI";
  }

  bool result;
  WiFiClient client;
  HTTPClient http;
  http.begin(client, what.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200){
    return http.getString();
  }
  else
  {
    return "ERROR";
  }
}
