#include <Arduino.h>
#include "sensor_dht.h"

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
