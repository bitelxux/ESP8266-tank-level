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

App::App(const char* SSID,
         const char* password,
	 const char* ID, 
	 const char* log_server){
    this->ID = ID;
    this->log_server = log_server;
    this->SSID = SSID;
    this->password = password;
    this->timers = NULL;
    this->logger = new Log(this, this->ID, this->log_server);

    this->addTimer(60000, &App::imAlive, "imAlive");
    this->addTimer(5000, &App::connectIfNeeded, "connectIfNeeded");
    this->addTimer(1000, &App::handleOTA, "handleOTA");
    this->addTimer(1000, &App::blinkLED, "blinkLED");

    pinMode(LED, OUTPUT);

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

void App::handleOTA(){
  ArduinoOTA.handle();
}

bool App::send(String what){

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

