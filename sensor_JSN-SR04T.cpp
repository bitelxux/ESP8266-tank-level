#include <Arduino.h>
#include "sensor_JSN-SR04T.h"

// all distances in meters
float TANK_RADIUS = 0.6;
float TANK_EMPTY_DISTANCE = 1.170;
float TANK_FULL_DISTANCE= 0.21;
float REMAINING_WATER_HEIGHT= 0.19;

// pre-calculated
float piR2=3.141516*TANK_RADIUS*TANK_RADIUS;
float MAX_VOLUME = 1000 * piR2 * (TANK_EMPTY_DISTANCE + REMAINING_WATER_HEIGHT);

// circular buffer
int sum = 0;
int elementCount = 0;
const int windowSize = 5;
int circularBuffer[windowSize];
int* circularBufferAccessor = circularBuffer;

unsigned char dataBuffer[4] = {0};
unsigned char CS;
int distance =  0;
int previous_distance = 0;

void appendToBuffer(short int value)
{
  *circularBufferAccessor = value;
  circularBufferAccessor++;
  if (circularBufferAccessor >= circularBuffer + windowSize) 
    circularBufferAccessor = circularBuffer;
}

int mobileAverage(int value)
{
  // try not to do movile average
  // it's done in grafana

  return value;

  sum -= *circularBufferAccessor;
  sum += value;
  appendToBuffer(value);
  if (elementCount < windowSize)
    ++elementCount;
  return (int) sum / elementCount;
}


SR04T_sensor::SR04T_sensor(App* app){
    this->app = app;
    this->sensor = new SoftwareSerial(RX, TX);
}

void SR04T_sensor::init(){
    // mode 2
    this->sensor->begin(9600);
    /*
    // mode 1
    pinMode(TRIGGER_PIN, OUTPUT); // Initializing Trigger Output and Echo Input
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIGGER_PIN, LOW); // Reset the trigger pin and wait a half a second
    delayMicroseconds(500);    
    */
}

int SR04T_sensor::read(){
    // use readSensor_mode1 or readSensor_mode2
    // depending on the model/sensor setup
    // Mode1: use trigger
    // Mode2: serial comm
    return this->readSensor_mode2();
}

int SR04T_sensor::calcLitres(short int distance){

    float fd = distance/1000.0; // meters
    float h = max((float)0.0, TANK_EMPTY_DISTANCE - fd);
    float v = piR2 * h * 1000;

    // There is some remaining water under the floating switch
    // that is not usable because the pumb is off at this level.
    // Yet, it is water in the tank
    v += piR2 * REMAINING_WATER_HEIGHT * 1000;

    return (int) v;
}


// Trigger + Echo mode
int SR04T_sensor::readSensor_mode1(){
    long t = 0; // Measure: Put up Trigger...
    digitalWrite(TRIGGER_PIN, HIGH); // Wait for 11 µs ...
    delayMicroseconds(11); // Put the trigger down ...
    digitalWrite(TRIGGER_PIN, LOW); // Wait for the echo ...
    t = pulseIn(ECHO_PIN, HIGH);

    int mm = t/(29.2*2)*10;

    //sprintf(buffer, "duration was %d. mm was %d", t, mm);
    //app->log(buffer);

    // wrong reading
    if (t == 0){
        return -1;
    }

    return(mobileAverage(mm));
}

// read from serial. Requires a 47K resistor soldered on the board.
int SR04T_sensor::readSensor_mode2(){

   char buffer[100];
   short int distance = 0;

   // Flush old readings from sensor
   while (this->sensor->available()){
     delay(5);
     this->sensor->read();
   }

   delay(20);

   if (this->sensor->available() > 0){
       delay(4);
    
       if (this->sensor->read() == 0xFF){
          dataBuffer[0] = 0xFF;
          for (int i=1; i<4; i++){
            dataBuffer[i] = this->sensor->read();
          }
       }
    
       //sprintf(buffer, "lectura: %02X,%02X,%02X,%02X", dataBuffer[0], dataBuffer[1], dataBuffer[2], dataBuffer[3]);
       //app->log(buffer);
    
       CS = dataBuffer[0] + dataBuffer[1] + dataBuffer[2];
    
       if (dataBuffer[3] == CS){
         distance = (dataBuffer[1] << 8) + dataBuffer[2];
         //sprintf(buffer, "lectura: %d", distance);
         //app->log(buffer);
         return mobileAverage(distance);
       }
       else{
         sprintf(buffer, "CS Error: %02X,%02X,%02X,%02X", dataBuffer[0], dataBuffer[1], dataBuffer[2], dataBuffer[3]);
         this->app->log(buffer);
         return -1;
       }
   }
   return -1;
}

void SR04T_sensor::draw(Adafruit_SSD1306* display, int value){
  int outerX = 90;
  int outerY = 18;
  int outerWidth = 37;
  int outerHeight = 46;
  display->drawRoundRect(outerX, outerY, outerWidth, outerHeight, 4, 1);

  int innerWidth = outerWidth - 4;
  int innerHeight = (value * outerHeight)/MAX_VOLUME;
  int innerX = outerX + 2;
  int innerY = outerY + 2 + outerHeight - innerHeight - 4;
  display->fillRoundRect(innerX, innerY, innerWidth, innerHeight, 4, 1);

  // flat surface
  display->fillRect(innerX, innerY, innerWidth, 4, 1);
}