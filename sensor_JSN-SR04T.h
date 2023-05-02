#ifndef sensor_sr04t_h
#define sensor_sr04t_h

#include <SoftwareSerial.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "cnn.h"

#define SENSOR_MODE 2

// for sensor mode 1
#define TRIGGER_PIN 14
#define ECHO_PIN 12
#define USONIC_DIV 58.0

// for sensor mode 2
// requires to solder a 47K resistor
#define TX 14
#define RX 12

class SR04T_sensor{
    public:
        App* app = NULL;
        SoftwareSerial* sensor = NULL;

        SR04T_sensor(App* app);
        int read();
        int calcLitres(short int distance);
        int readSensor_mode1();
        int readSensor_mode2();
        void init();

        //overwrite drawing in display
        void draw(Adafruit_SSD1306* display, int value);
};

#endif sensor_sr04t_h
