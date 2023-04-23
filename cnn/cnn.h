#ifndef cnn_h
#define cnn_h

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#include <EEPROM.h> //https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.h
#include <NTPClient.h> // install NTPClient from manage libraries

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager


/*
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;
*/

// for debug
#define DEBUG_WIFI   0b00000001
#define DEBUG_SERIAL 0b00000010
#define DEBUG_ALL    0b00000100

class App;

typedef void (*function_callback)();
typedef void (App::*AppCallback)();

#define USER_TIMER 0
#define APP_TIMER 1

class Timer{
    public:
	int millis;
	AppCallback appFunction;
	function_callback  function;
	char* name;
	int type;
	unsigned long lastRun;
	Timer* next;

	Timer();
};

class Log{
    public:
        App* app;
        const char* server;
        const char* ID;

        Log(App* app, const char* ID, const char* server);
        void log(char* msg);
};

class App{
    public:

    const char* ID = NULL;
    const char* log_server = NULL;
    const char* SSID = NULL;
    const char* password = NULL;
    int LED = 2;
    char IP[16];
    unsigned long tLastConnectionAttempt = 0;
    unsigned long tConnect = 0;

    // for NTP
    unsigned long tEpoch = 0;
    unsigned long tEpochOffset = 0;

    Timer* timers = NULL;
    Log* logger;
    App(const char* ID, const char* server);
    WiFiManager *wifiManager = NULL;
    
    void initNTP();
    void addTimer(int millis, AppCallback function, char* name);
    void addTimer(int millis, function_callback function, char* name);
    void attendTimers();
    void imAlive();
    void log(char* msg);
    void debug(char* level, unsigned short int channels, char* message);
    bool send(String what);
    String get(String what);
    void handleOTA();
    void blinkLED();
    void startWiFiManager();
    void updateNTP();
    unsigned long getEpochSeconds();
};


#endif
