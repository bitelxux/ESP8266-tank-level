/* Testest with
 -  Arduino IDE 1.8.15
    - Arduino AVR board 1.8.3
    - ESP8266 2.7.0

https://arduino.esp8266.com/stable/package_esp8266com_index.json
board: NodeMCU1.0 (ESP-12E Module)
*/

//Libraries
#include <cnn.h>
#include <SoftwareSerial.h>

// This is for each variable to use it's real size when stored
// in the EEPROM
#pragma pack(push, 1)

//Constants
#define EEPROM_SIZE 4 * 1024 * 1024

#define LED_BLUE 0
#define LED_RED 4
#define LED_GREEN 5
#define TX 14
#define RX 12

char buffer[100];
unsigned char dataBuffer[4] = {0};
unsigned char CS;
int distance;
int counter = 0; // number of registers in EEPROM

SoftwareSerial sensor(RX, TX);

// prototipes
void FlushStoredData();
void registerNewReading();

#define ID "tank.level"

const char* ssid = "Starlink";
const char* password = "82111847";
const char* log_server = "http://192.168.1.162:8888";
const char* baseURL = "http://192.168.1.162:8889/";

// all distances in meters
float TANK_RADIUS = 0.6;
float TANK_EMPTY_DISTANCE = 1.170;
float TANK_FULL_DISTANCE= 0.21;
float REMAINING_WATER_HEIGHT= 0.19;

// circular buffer
int sum = 0;
int elementCount = 0;
const int windowSize = 5;
int circularBuffer[windowSize];
int* circularBufferAccessor = circularBuffer;

// pre-calculated
float piR2=3.141516*TANK_RADIUS*TANK_RADIUS;

int previous_distance = 0;

typedef struct
{
  unsigned long timestamp;
  short int value;
} Reading;

App app = App(ssid, password, ID, log_server);

int calcLitres(short int distance){

    float fd = distance/1000.0; // meters
    float h = max((float)0.0, TANK_EMPTY_DISTANCE - fd);
    float v = piR2 * h * 1000;

    // There is some remaining water under the floating switch
    // that is not usable because the pumb is off at this level.
    // Yet, it is water in the tank
    v += piR2 * REMAINING_WATER_HEIGHT * 1000;

    return (int) v;
}

void appendToBuffer(short int value)
{
  *circularBufferAccessor = value;
  circularBufferAccessor++;
  if (circularBufferAccessor >= circularBuffer + windowSize) 
    circularBufferAccessor = circularBuffer;
}

int mobileAverage(int value)
{
  sum -= *circularBufferAccessor;
  sum += value;
  appendToBuffer(value);
  if (elementCount < windowSize)
    ++elementCount;
  return (int) sum / elementCount;
}

bool isServerAlive(){
    sprintf(buffer, "%s/ping", baseURL);
    Serial.println(buffer);
    return (app.send(buffer));
}

void ledError(){
    digitalWrite(LED_RED, HIGH);
    delay(20);
    digitalWrite(LED_RED, LOW);  
}

void ledOK(){
    digitalWrite(LED_GREEN, HIGH);
    delay(20);
    digitalWrite(LED_GREEN, LOW);  
}

void registerNewReading(){
  int distance;
  char buffer[100];

  distance = readSensor();

  if (distance == -1){
    app.log("Error reading value from sensor");
    ledError();
    return;
  }

  if (app.epochTime){
    unsigned long now = app.epochTime + int(millis()/1000);

    int litres = calcLitres(distance);
    sprintf(buffer, "%s/add/%d:%d", baseURL, now, litres);

    // try to send to the server
    // if fails, store locally for further retrying
    if (app.send(buffer)){
      ledOK();
      sprintf(buffer, "Sent %d:%d", now, litres);
      app.log(buffer);
    }
    else{
      ledError();
      writeReading(now, distance);
      sprintf(buffer, "Locally stored %d:%d", now, litres);
      app.log(buffer);
    }
        
  }

}


// returns the mobile average of current reading
int readSensor(){

   short int distance = 0;

   // Flush old readings from sensor
   while (sensor.available()){
     delay(5);
     sensor.read();
   }
   delay(20);

   if (sensor.available() > 0){
       delay(4);
    
       if (sensor.read() == 0xFF){
          dataBuffer[0] = 0xFF;
          for (int i=1; i<4; i++){
            dataBuffer[i] = sensor.read();
          }
       }
    
       //sprintf(buffer, "lectura: %02X,%02X,%02X,%02X", dataBuffer[0], dataBuffer[1], dataBuffer[2], dataBuffer[3]);
       //app.log(buffer);
    
       CS = dataBuffer[0] + dataBuffer[1] + dataBuffer[2];
    
       if (dataBuffer[3] == CS){
         distance = (dataBuffer[1] << 8) + dataBuffer[2];
         return mobileAverage(distance);
       }
       else{
         sprintf(buffer, "CS Error: %02X,%02X,%02X,%02X", dataBuffer[0], dataBuffer[1], dataBuffer[2], dataBuffer[3]);
         app.log(buffer);
         return -1;
       }
   }
}

void FlushStoredData(){

  if (!isServerAlive()){
    sprintf(buffer, "[FLUSH_STORED_DATA] Server doesn't respond. Skipping");
    app.log(buffer);
    return;
  }

  // send will be done in batches to allow other tasks to run
  unsigned short batch = 50;

  // You have to start server.py at BASE_URL
  int sent = 0;
  
  if (WiFi.status() != WL_CONNECTED){
    app.log("[FLUSH_STORED_DATA] Skipping. I'm not connected to the internet :-/.");
    return;
  }

  Reading reading;  
  unsigned short int counter = readEEPROMCounter();
  unsigned short int cursor = 0;
  int regAddress;
  short int value;
  bool errors = false;

  int regSize = sizeof(reading);

  char buffer[100];
  //sprintf(buffer, "[FLUSH_STORED_DATA] %d registers", counter);
  //Serial.println(buffer);
  
  for (cursor = 0; cursor < counter; cursor++){

      if (sent >= batch){
         break;
      }

      regAddress = 2 + cursor*regSize; // +2 is to skip the counter bytes
      EEPROM.get(regAddress, reading);

      // if value is -1 that register was already sent
      if (reading.value == -1){
        continue;
      }
      
      sprintf(buffer, "%s/add/%d:%d", baseURL, reading.timestamp, reading.value);

      if (!app.send(buffer)){
        //sprintf(buffer, "[FLUSH_STORED_DATA] Error sending register [%d]", cursor);
        //app.log(buffer);
        errors = true;
      }
      else
      {
        ledOK();
        sent ++;
        sprintf(buffer, "[FLUSH_STORED_DATA] Success sending record [%d]", cursor);
        app.log(buffer);
        // We don't want to write the whole struct to save write cycles
        value = -1;
        EEPROM.put(regAddress + sizeof(reading.timestamp), value);
        EEPROM.commit();            
      }
  }

  if (!errors){
      sprintf(buffer, "[FLUSH_STORED_DATA] %d records sent", sent);
      if (sent == 0){
          writeCounter(0);
      }
  } else {
      sprintf(buffer, "[FLUSH_STORED_DATA] %d records sent [errors]", sent);    
  }
  
  app.log(buffer);

}

void writeCounter(unsigned short int value){
  EEPROM.put(0, value); 
  if (!EEPROM.commit()) {
    app.log("Commit failed writing counter");
  }
}

void writeReading(unsigned long in_timestamp, short int in_value){
  
  int address;
  int regAddress;
  unsigned short int counter = readEEPROMCounter();
  unsigned short int cursor;
 
  Reading newReading;
  Reading currentReading;
  
  newReading.timestamp = in_timestamp;
  newReading.value = in_value;

  int regSize = sizeof(newReading);
  
  for (cursor = 0; cursor < counter; cursor++){
    regAddress = sizeof(counter) + cursor*regSize; // +sizeof counter is to skip the counter bytes
    EEPROM.get(regAddress, currentReading);

    if (currentReading.value == -1){ // Re-use old register
      sprintf(buffer, "Reused position %d", regAddress);
      app.log(buffer);
      EEPROM.put(regAddress, newReading);
      if (!EEPROM.commit()) {
        app.log("Commit failed on re-use");
      }
      break;
    }
  }

  if (cursor == counter){ // new register
      regAddress = sizeof(counter) + cursor*regSize; // +sizeof counter is to skip the counter bytes

      // Serial.print("New register at ");
      // Serial.println(regAddress);

      EEPROM.put(regAddress, newReading);
      counter += 1;
      writeCounter(counter);
  }
  
}

void readEEPROM(int regSize){

  unsigned short int counter = readEEPROMCounter();
  unsigned short int cursor = 0;
  int regAddress;
  char buffer[100];

  sprintf(buffer, "%d registers currently stored in EEPROM", counter);
  app.log(buffer);

  /*
  for (cursor = 0; cursor < counter; cursor++){
    regAddress = 2 + cursor*regSize; // +2 is to skip the counter bytes
    EEPROM.get(regAddress, reading);
    sprintf(buffer, "Address %d: %d:%d", regAddress, reading.timestamp, reading.value);
    log(buffer);
  }
  */

}

void resetEEPROM(){

  int address = 0;
  unsigned short int counter=0;
  EEPROM.put(address, counter);
  EEPROM.commit();
  int a = 1/0; // I don't want to continue
}

unsigned short int readEEPROMCounter(){
  unsigned short int counter;
  EEPROM.get(0, counter);
  return counter;
}

void setupLeds(){
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
}

void setup() {
  setupLeds();

  Serial.begin(115200); 
  EEPROM.begin(EEPROM_SIZE);
  // resetEEPROM();
  sensor.begin(9600);

  app.addTimer(30000, FlushStoredData, "FlushStoredData");
  app.addTimer(1000, registerNewReading, "registerNewReading");
}

void loop() {
  app.attendTimers();
}
