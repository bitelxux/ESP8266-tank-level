#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"


#include <Arduino.h>
#include "sensor_dht.h"

#define MAX_TEMPERATURE 80

DHT11_sensor::DHT11_sensor(App* app){
    this->app = app;
    this->dht = new DHT(DHTPIN, DHTTYPE, 100);
}

void DHT11_sensor::init(){
    pinMode(DHTPIN, INPUT);
    this->dht->begin();
}

int DHT11_sensor::read(){
    return this->readTemperature();
}

int DHT11_sensor::readTemperature(){
    // DHT11 only reads full degrees anyways
    int read = int(this->dht->readTemperature());
    if (isnan(read) || read == DHT11_ERROR || read <= 0){
        this->app->log("Error reading temperature");
        read = 666;
    }    
    return read;
}

int DHT11_sensor::readHumidity(){
    int read = int(this->dht->readHumidity());
    if (isnan(read) || read == DHT11_ERROR || read <= 0){
        this->app->log("Error reading humidity");
        read = 666;
    }    
    return read;
}

void DHT11_sensor::draw(Adafruit_SSD1306* display, int value){
  int outerX = 108;
  int outerY = 18;
  int outerWidth = 20;
  int outerHeight = 46;
  display->drawRoundRect(outerX, outerY, outerWidth, outerHeight, 4, 1);

  int innerWidth = outerWidth - 4;
  int innerHeight = (value * outerHeight)/MAX_TEMPERATURE;
  int innerX = outerX + 2;
  int innerY = outerY + 2 + outerHeight - innerHeight - 4;
  display->fillRoundRect(innerX, innerY, innerWidth, innerHeight, 4, 1);

  // flat surface
  display->fillRect(innerX, innerY, innerWidth, 4, 1);
}

#pragma GCC diagnostic pop
