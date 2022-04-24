#ifndef cnn_h
#define cnn_h

#include <Arduino.h>
//#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#include <EEPROM.h> //https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.h
#include <NTPClient.h> // install NTPClient from manage libraries

void imAlive();
void handleOTA();
void log(char* msg);
bool send(String what);
void readEEPROM();
void resetEEPROM();
unsigned short int readEEPROMCounter();
void initNTP();
class App;

struct Timer
{
    unsigned long millis;
    //void (*function)();
    void (App:: *function)();
    char* functionName;
};

struct TimerNode
{
    unsigned long lastRun;
    Timer* timer;
    TimerNode* next;
};

class Log{
    public:
        const char* server;
        const char* ID;

        Log(const char* ID, const char* server);
        void log(char* msg);
};

class App{
    public:
        const char* ID;
        const char* server;
        const char* SSID;
        const char* password;
	unsigned long epochTime = 0;

	TimerNode* timers = NULL;
	Timer t0 {1000, &App::imAlive, "imAlive"};

	//Log logger;

	App(const char* SSID,
   	    const char* password,
	    const char* ID,
	    const char* server);
	void initNTP();
	void addTimer(Timer* timer);
	void attendTimers();
	void imAlive();
};


#endif
