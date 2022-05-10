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

//EEPROM
#define EEPROM_SIZE 4 * 1024 * 1024
#define RESERVED_BYTES 10
#define COUNTER_SLOTS 10

#define CURRENT_COUNTER_SLOT_ADDRESS 0   // 1 byte
#define MAX_WRITES 100000                // theorically 10000
#define WARNINGS_ADDRESS 1               // bitmap supporting 8 predefined warnings

#define WARNING_COUNTER_SLOTS_ROTATED  1 // counter slots were exhaested and ressetted to 1
#define WARNING_1 2
#define WARNING_2 4
#define WARNING_3 8
#define WARNING_4 16
#define WARNING_5 32
#define WARNING_6 64
#define WARNING_7 128

#define FLUSH_BATCH_SIZE 50

// 10 bytes are reserver for general data
// we reserve space for 10 counters, 4 byte each
// for each counter, first two bytes is the number
// of write operations on the counter
// the second two bytes is the counter itself
// #define RECORDS_BASE_ADDRESS RESERVED_BYTES + 4 * COUNTER_SLOTS 
//\\ EEPROM

#define TX 14
#define RX 12

unsigned short int RECORDS_BASE_ADDRESS = RESERVED_BYTES + 4 * COUNTER_SLOTS;

char buffer[200];
unsigned char dataBuffer[4] = {0};
unsigned char CS;
int distance;
int counter = 0; // number of registers in EEPROM
int lastReading = 0;

SoftwareSerial sensor(RX, TX);

// prototipes
void flushStoredData();
void registerNewReading();

#define ID "tank.level"

const char* ssid = "Starlink";
const char* password = "82111847";
const char* log_server = "http://192.168.1.162:8888";
const char* baseURL = "http://192.168.1.162:8889";

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
float MAX_VOLUME = piR2 * (TANK_EMPTY_DISTANCE + REMAINING_WATER_HEIGHT) * 1000;

int previous_distance = 0;

typedef struct
{
  short flag;
  unsigned long timestamp;
  short int value;
} Reading;

App app = App(ssid, password, ID, log_server);

//OLED

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static const unsigned char PROGMEM wifi_bmp[] = 
{
0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x1f, 0xf8, 0x70, 0x0e, 0xe0, 0x07, 0x07, 0xe0, 0x1e, 0x78,
0x18, 0x18, 0x03, 0xc0, 0x07, 0xe0, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char PROGMEM nowifi_bmp[] = 
{
0x00, 0x01, 0x00, 0x3e, 0x01, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x01, 0xfe, 0x03, 0x3c, 0x20, 0x3c,
0x30, 0x64, 0x38, 0x40, 0x7c, 0x00, 0x7e, 0x00, 0x7f, 0x00, 0x7f, 0x80, 0x7c, 0x00, 0x80, 0x00
};

static const unsigned char PROGMEM send_bmp[] = 
{
0x00, 0x00, 0x09, 0x00, 0x04, 0x00, 0x00, 0x80, 0x02, 0x40, 0x01, 0x20, 0x00, 0x80, 0x00, 0x90,
0x00, 0x90, 0x00, 0x80, 0x01, 0x20, 0x02, 0x40, 0x00, 0x80, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00
};

static const unsigned char PROGMEM store_bmp[] = 
{
0x7f, 0xfc, 0xe0, 0x6e, 0xe0, 0x6f, 0xe0, 0x6f, 0xe0, 0x6f, 0xe0, 0x2f, 0xff, 0xff, 0xff, 0xff,
0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0x7f, 0xfe
};

static const unsigned char PROGMEM disconnected_bmp[] = 
{
0x00, 0x01, 0x00, 0x3e, 0x01, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x01, 0xfe, 0x03, 0x3c, 0x20, 0x3c,
0x30, 0x64, 0x38, 0x40, 0x7c, 0x00, 0x7e, 0x00, 0x7f, 0x00, 0x7f, 0x80, 0x7c, 0x00, 0x80, 0x00
};

static const unsigned char PROGMEM connected_bmp[] = 
{
0x00, 0x01, 0x00, 0x02, 0x00, 0x64, 0x00, 0xf8, 0x07, 0xf8, 0x0b, 0xfc, 0x0d, 0xfc, 0x1e, 0xf8,
0x1f, 0x78, 0x3f, 0xb0, 0x3f, 0xd0, 0x1f, 0xe0, 0x1f, 0x00, 0x26, 0x00, 0x40, 0x00, 0x80, 0x00
};

static const unsigned char PROGMEM warning_bmp[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x02, 0x40, 0x06, 0x60, 0x06, 0x60, 
	0x0e, 0x70, 0x1f, 0xf8, 0x1e, 0x78, 0x3e, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void clearSection(int x, int y, int x1, int y1){
  display.fillRect(x, y, x1, y1, 0);
  display.display();
}

void drawTank(){
  int outerX = 100;
  int outerY = 18;
  int outerWidth = 20;
  int outerHeight = 46;
  display.drawRoundRect(outerX, outerY, outerWidth, outerHeight, 4, 1);

  int innerWidth = outerWidth - 4;
  int innerHeight = (lastReading * outerHeight)/MAX_VOLUME;
  int innerX = outerX + 2;
  int innerY = outerY + 2 + outerHeight - innerHeight - 4;
  display.fillRoundRect(innerX, innerY, innerWidth, innerHeight, 4, 1);
}

void drawStore(){
  display.drawBitmap(94, 0, store_bmp, 16, 16, 1);
  display.display();
}

void drawSend(){
  display.drawBitmap(94, 0, send_bmp, 16, 16, 1);
  display.display();
}

void updateDisplay(){
  display.clearDisplay();

  drawTank();

  if (readWarnings()){
      display.drawBitmap(18, 0, warning_bmp, 16, 16, 1);
  }

  if (isServerAlive()){
      display.drawBitmap(0, 0, connected_bmp, 16, 16, 1);
  }
  else
  {
      display.drawBitmap(0, 0, disconnected_bmp, 16, 16, 1);
  }

  if (WiFi.status() == WL_CONNECTED){
      display.drawBitmap(112, 0, wifi_bmp, 16, 16, 1);
  }
  else
  {
      display.drawBitmap(112, 0, nowifi_bmp, 16, 16, 1);
  }

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,18);            
  display.print("Litros: ");
  display.setCursor(46,18);            
  display.print(lastReading);

  unsigned short int counter = readCounter();
  display.setCursor(0,26);            
  display.print("Local: ");
  display.setCursor(46,26);            
  display.print(counter);

  display.setCursor(0,56);            
  display.print(app.IP);

  display.display();
}

void initOLED(){

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  //display.display();
  //delay(2000); // Pause for 2 seconds
}

// OLED

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

void registerNewReading(){
  int distance;
  char buffer[100];

  // purge some possible noise
  for (int i=0; i<10; i++){
    readSensor();
    delay(50);
  }

  distance = readSensor();

  if (distance == -1){
    app.log("Error reading value from sensor");
    return;
  }

  if (app.epochTime){
    unsigned long now = app.epochTime + int(millis()/1000);

    int litres = calcLitres(distance);
    lastReading = litres;
    sprintf(buffer, "%s/add/%d:%d", baseURL, now, litres);

    // try to send to the server
    // if fails, store locally for further retrying
    if (app.send(buffer)){
      drawSend();
      sprintf(buffer, "Sent %d:%d", now, litres);
      app.log(buffer);
    }
    else{
      drawStore();
      writeReading(now, litres);
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

void flushStoredData(){

  if (!isServerAlive()){
    sprintf(buffer, "[FLUSH_STORED_DATA] Server doesn't respond. I'll try later.");
    app.log(buffer);
    return;
  }

  // You have to start server.py at BASE_URL
  unsigned short int sent = 0;
  
  if (WiFi.status() != WL_CONNECTED){
    app.log("[FLUSH_STORED_DATA] Skipping. I'm not connected to the WIFI :-/.");
    return;
  }

  Reading reading;  
  unsigned short int counter = readCounter();
  unsigned short int cursor = 0;
  unsigned short int regAddress;
  short int value;
  bool errors = false;
  short flag;
  unsigned short int recNum;

  if (counter == 0){
      app.log("[FLUSH_STORED_DATA] Nothing to send");
      return;
  }

  int regSize = sizeof(reading);

  char buffer[100];
  
  regAddress = RECORDS_BASE_ADDRESS - regSize;
  while( regAddress <= EEPROM_SIZE - 2*regSize){

      regAddress += regSize;

      if (sent >= FLUSH_BATCH_SIZE){
         break;
      }
      
      EEPROM.get(regAddress, reading);

      // if value is -1 that register was already sent
      // 0 the position hasn't been used yet
      if (reading.flag == -1){
        continue;
      }

      // flag 0 means that position or higher have never been used
      if (reading.flag == 0){
        return;
      }

      recNum = (regAddress - RECORDS_BASE_ADDRESS)/regSize;

      sprintf(buffer, "%s/add/%d:%d", baseURL, reading.timestamp, reading.value);

      if (!app.send(buffer)){
        sprintf(buffer, "[FLUSH_STORED_DATA] Error sending record [%d]", recNum);
        app.log(buffer);
        errors = true;
        if (!isServerAlive()){
          sprintf(buffer, "[FLUSH_STORED_DATA] Server doesn't respond. I'll try later.");
          app.log(buffer);
          break;
        }

      }
      else
      {
        drawSend();
        decCounter();
        sent ++;
        sprintf(buffer, "[FLUSH_STORED_DATA] Success sending record [%d] (%d left)", recNum, readCounter());
        app.log(buffer);

        // We don't want to write the whole struct to save write cycles
        flag = -1;
        EEPROM.put(regAddress, flag);
        EEPROM.commit();

        updateDisplay();
      }

  }

  if (!errors){
      sprintf(buffer, "[FLUSH_STORED_DATA] %d records sent", sent);
      // to correct a buggy counter out of sync error that sometimes happens
      if (sent == 0){
        writeCounter(0);
      }
  } else {
      sprintf(buffer, "[FLUSH_STORED_DATA] %d records sent [errors]", sent);    
  }
  
  app.log(buffer);

}

unsigned short int incCounter(){
  counter = readCounter() + 1;
  writeCounter(counter);
  return counter;
}

unsigned short int decCounter(){
  counter = readCounter() - 1;
  writeCounter(counter);
  return counter;
}

byte readWarnings(){
  byte warnings;   
  warnings = EEPROM.read(WARNINGS_ADDRESS);
  return warnings;
}

byte addWarning(byte warning){
  byte warnings = readWarnings();
  warnings |= warning;
  EEPROM.write(WARNINGS_ADDRESS, warnings);
  EEPROM.commit();

  return warnings;
}

unsigned short int readCounter(){
  byte counterAddress;   
  unsigned short int counter;
  EEPROM.get(CURRENT_COUNTER_SLOT_ADDRESS, counterAddress);
  EEPROM.get(counterAddress + 2, counter); // skip counter writes: 2 bytes
  return counter;
}

int getCounterSlot(){
  byte counterAddress;
  int counterSlot;

  EEPROM.get(CURRENT_COUNTER_SLOT_ADDRESS, counterAddress);
  counterSlot = (counterAddress-RESERVED_BYTES-1)/4 + 1;
  return counterSlot;
}

void writeCounter(unsigned short int counter){
  byte counterAddress;
  int counterSlot;
  unsigned short int counterWrites;

  EEPROM.get(CURRENT_COUNTER_SLOT_ADDRESS, counterAddress);
  EEPROM.get(counterAddress, counterWrites);

  counterSlot = getCounterSlot();

  if (counterWrites >= MAX_WRITES){
    // We are about to reach the 10K writes limit ...
    // moving to the next slot

    //sprintf(buffer, "Counter slot %d reaching the limit of %d writings", counterSlot, MAX_WRITES);
    //app.log(buffer);

    if (counterSlot <  COUNTER_SLOTS){
        counterSlot ++;
        counterAddress += 4; // each counter is 2 byte for n of writes + 2 bytes for the counter
        sprintf(buffer, "Counter slot moved to slot %d", counterSlot);
        app.log(buffer);
    }
    else{
        sprintf(buffer, "WARNING. Counter slots exhausted. Resetting to slot 1. Writes might start to fail soon");
        addWarning(WARNING_COUNTER_SLOTS_ROTATED);
        app.log(buffer);
        counterSlot = 1;
        counterAddress = 1;
    }

    counterWrites = 0;

    EEPROM.write(CURRENT_COUNTER_SLOT_ADDRESS, counterAddress);
  }

  EEPROM.put(counterAddress, counterWrites+1);
  EEPROM.put(counterAddress + 2, counter);
  
  //sprintf(buffer, "counter slot: %d, numwrites: %d", counterSlot, counterWrites+1);
  //app.log(buffer);

  if (!EEPROM.commit()) {
    app.log("Commit failed writing counter changes");
  }
}

void writeReading(unsigned long in_timestamp, short int in_value){
  
  int address;
  int regAddress;
  unsigned short int counter = readCounter();
 
  Reading newReading;
  Reading record;
  Reading currentReading;
  
  newReading.timestamp = in_timestamp;
  newReading.value = in_value;
  newReading.flag = 1;

  int regSize = sizeof(newReading);
  short flag;

  for (address=RECORDS_BASE_ADDRESS; address < EEPROM_SIZE - regSize; address += regSize){
      EEPROM.get(address, flag);
      // flag 1 is a used position
      if (flag == -1 || flag == 0){
        EEPROM.put(address, newReading);
        EEPROM.commit();
        counter = incCounter();
        break;
      }      
  }  
}

void resetEEPROM(){
  int t0 = millis();
  for (int i=0; i < EEPROM_SIZE; i++){
    EEPROM.write(i, 0);
  }

  EEPROM.write(CURRENT_COUNTER_SLOT_ADDRESS, RESERVED_BYTES);

  EEPROM.commit();
  int t2=millis();

  sprintf(buffer, "EEPROM deleted in %d milliseconds", t2 - t0);
  app.log(buffer);
}

void setup() {

  initOLED();
  
  Serial.begin(115200); 
  EEPROM.begin(EEPROM_SIZE);
  // resetEEPROM();
  sensor.begin(9600);

  app.addTimer(30 * 1000, flushStoredData, "flushStoredData");
  app.addTimer(1000, registerNewReading, "registerNewReading");
  app.addTimer(1000, updateDisplay, "updateDisplay");
  app.addTimer(1000, todo, "todo");
}

void todo(){
  sprintf(buffer, "%s/todo", baseURL);
  String todo = app.get(buffer);

  if (todo == "reset eeprom") resetEEPROM();
}

void loop() {
  app.attendTimers();
}
