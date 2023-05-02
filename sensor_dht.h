#ifndef sensor_dht_h
#define sensor_dht_h

#include <DHT.h>

#include "cnn.h"

#define DHTTYPE DHT11
#define DHTPIN  12

#define DHT11_ERROR 2147483647 // this value is a read error

class DHT11_sensor{
    public:
        App* app = NULL;
        DHT* dht = NULL;

        DHT11_sensor(App* app);
        int read();
        int readTemperature();
        int readHumidity();
        void init();
};

#endif