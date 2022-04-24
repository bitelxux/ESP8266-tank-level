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

#define LED 2
#define USER_TIMER 0
#define APP_TIMER 1

struct Timer
{
    unsigned long millis;
    void (*function)();
    char* functionName;
};

struct AppTimer
{
    unsigned long millis;
    void (App:: *function)();
    char* functionName;
};

struct TimerNode
{
    int type; 
    unsigned long lastRun;
    void* timer;
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
        const char* log_server;
        const char* SSID;
        const char* password;
	char IP[16];
	unsigned long epochTime = 0;
	unsigned long tLastConnectionAttempt = 0;
	unsigned long tConnect = 0;

	TimerNode* timers = NULL;
	AppTimer t0 {1000, &App::imAlive, "imAlive"};
	AppTimer t1 {5000, &App::connectIfNeeded, "connectIfNeeded"};
	AppTimer t2 {1000, &App::handleOTA, "handleOTA"};

	Log* logger;

	App(const char* SSID,
   	    const char* log_password,
	    const char* ID,
	    const char* server);
	void initNTP();
	void addTimer(void* timer, int type = USER_TIMER);
	void attendTimers();
	void attendUserTimer(TimerNode* timerNode);
	void attendAppTimer(TimerNode* timerNode);
	void imAlive();
	void log(char* msg);
	void connectIfNeeded();
	void connect();
	void handleOTA();
};


#endif
