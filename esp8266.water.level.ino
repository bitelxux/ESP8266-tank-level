//Libraries
#include <FS.h>
#include <cnn.h>
#include <ArduinoJson.h>     // 5.13.5 !!
#include <SoftwareSerial.h>

#include <ESP8266WebServer.h>

// This is for each variable to use it's real size when stored
// in the EEPROM
#pragma pack(push, 1)
#define SENSOR_MODE 2

#define BOARD_ID "caseta.A"
#define VERSION "20230423.48"

//EEPROM
#define EEPROM_SIZE 4096
#define RESERVED_BYTES 10
#define COUNTER_SLOTS 10

#define CURRENT_COUNTER_SLOT_ADDRESS 0   // 1 byte
#define MAX_WRITES 100000                // theorically 10000
#define WARNINGS_ADDRESS 1               // bitmap supporting 8 predefined warnings
#define BOOTS_ADDRESS 2                  // 2 bytes

#define WARNING_COUNTER_SLOTS_ROTATED  1 // counter slots were exhaested and ressetted to 1
#define WARNING_STORAGE_IS_FULL 2
#define WARNING_2 4
#define WARNING_3 8
#define WARNING_4 16
#define WARNING_5 32
#define WARNING_6 64
#define WARNING_7 128

#define FLUSH_BATCH_SIZE 50

#define REC_NEVER_USED 0
#define REC_IN_USE 1
#define REC_FLUSHED 2

// 10 bytes are reserver for general data
// we reserve space for 10 counters, 4 byte each
// for each counter, first two bytes is the number
// of write operations on the counter
// the second two bytes is the counter itself
// #define RECORDS_BASE_ADDRESS RESERVED_BYTES + 4 * COUNTER_SLOTS 
//\\ EEPROM

// for sensor mode 0
#define TRIGGER_PIN 14
#define ECHO_PIN 12
#define USONIC_DIV 58.0

// for sensor mode 1
// requires to solder a 47K resistor
#define TX 14
#define RX 12

#if (SENSOR_MODE == 1)
  int (*readSensor)() = &readSensor_mode1;
#else
  int (*readSensor)() = &readSensor_mode2;
#endif

#define RSSI_MAX -50  // maximum strength of signal in dBm
#define RSSI_MIN -100 // minimum strength of signal in dBm

// WiFiManager parameteres
#define SERVER_LABEL "SERVER_IP"

// Reset Button
#define RESET_BUTTON 14
unsigned long rb_last_change = 0;
int rb_required_time = 3;
boolean time_to_reset = false;

App *app = NULL;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

typedef struct
{
  byte flag;
  unsigned long timestamp;
  unsigned short value;
} Reading;

unsigned short int RECORDS_BASE_ADDRESS = RESERVED_BYTES + 4 * COUNTER_SLOTS;
unsigned short int MAX_RECORDS = (EEPROM_SIZE - RECORDS_BASE_ADDRESS)/sizeof(Reading) -1;
// TODO review that -1. It's not the best approach

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
float MAX_VOLUME = 1000 * piR2 * (TANK_EMPTY_DISTANCE + REMAINING_WATER_HEIGHT);

char buffer[200];
unsigned char dataBuffer[4] = {0};
unsigned char CS;
int counter = 0; // number of registers in EEPROM
int lastReading = 0;
int distance =  0;
int previous_distance = 0;

SoftwareSerial sensor(RX, TX);

// prototipes
void flushStoredData();
void registerNewReading();

ESP8266WebServer restServer(80);

// This values  will depend on what the user configures
// on the  WifiManager on the first connection
char server[16];
char log_server[30];
char baseURL[30];


//OLED
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

boolean displayInitialized = false;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

// SCL is GPI05
// SDA is GPI04
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

  int outerX = 90;
  int outerY = 18;
  int outerWidth = 37;
  int outerHeight = 46;
  display.drawRoundRect(outerX, outerY, outerWidth, outerHeight, 4, 1);

  int innerWidth = outerWidth - 4;
  int innerHeight = (lastReading * outerHeight)/MAX_VOLUME;
  int innerX = outerX + 2;
  int innerY = outerY + 2 + outerHeight - innerHeight - 4;
  display.fillRoundRect(innerX, innerY, innerWidth, innerHeight, 4, 1);

  // flat surface
  display.fillRect(innerX, innerY, innerWidth, 4, 1);

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

  if (!displayInitialized) {
    Serial.println("Display not initialized");
    return;
  }

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

  display.setCursor(40,5);            
  display.print(BOARD_ID);

  display.setCursor(0,18);
  display.print("Litros: ");
  display.setCursor(46,18);            
  display.print(lastReading);

  unsigned short int counter = readCounter();
  display.setCursor(0,26);            
  display.print("Local: ");
  display.setCursor(46,26);            
  display.print(counter);

  display.setCursor(0,48);            
  display.print(VERSION);

  display.setCursor(0,56);            
  display.print(app->IP);

  display.display();
}

void initOLED(){

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("SSD1306 found and intialized");
    displayInitialized = true;
  }
  else {
    Serial.println("SSD1306 allocation failed");
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

bool isServerAlive(){
    sprintf(buffer, "%s/ping", baseURL);
    //Serial.println(buffer);
    return (app->send(buffer));
}

void registerNewReading(){
  int distance;
  int counter;
  char buffer[100];

  // some attempts to read a value from sensor
  for (int i=0; i<10; i++){
      distance = readSensor();
      if (distance != -1){
        break;
      }
      delay(20);
  }

  if (distance <= 0){
    app->log("Error reading value from sensor");
    return;
  }

  sprintf(buffer, "LOG: read distance is %d", distance);
  app->log(buffer);

  int litres = calcLitres(distance);
  lastReading = litres;

  unsigned long now = app->getEpochSeconds();
  if (now){
    sprintf(buffer, "%s/add/%d:%d", baseURL, now, litres);

    // try to send to the server
    // if fails, store locally for further retrying
    if (app->send(buffer)){
      drawSend();
      sprintf(buffer, "Sent %d:%d", now, litres);
      app->log(buffer);
    }
    else{
      drawStore();
      counter = writeReading(now, litres);
    }
        
  }
  else{
      app->log("Warning: Not handling reading as time is not synced");
  }

}

// Trigger + Echo mode
int readSensor_mode1(){
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
int readSensor_mode2(){

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
         app->log(buffer);
         return -1;
       }
   }
}

void flushStoredData(){


  Reading reading;  
  unsigned short int counter = readCounter();
  unsigned short int cursor = 0;
  unsigned short int regAddress;
  short int value;
  bool errors = false;
  byte flag;
  unsigned short int recNum;
  unsigned short int sent = 0;
  int regSize = sizeof(reading);
  char buffer[100];
 
  // You have to start server.py at BASE_URL
  if (!isServerAlive()){
    sprintf(buffer, "[FLUSH_STORED_DATA] Server doesn't respond. I'll try later.");
    app->log(buffer);
    return;
  }

  if (WiFi.status() != WL_CONNECTED){
    app->log("[FLUSH_STORED_DATA] Skipping. I'm not connected to the WIFI :-/.");
    return;
  }

  if (counter == 0){
      app->log("[FLUSH_STORED_DATA] Nothing to send");
      return;
  }

  for (regAddress = RECORDS_BASE_ADDRESS; regAddress < EEPROM_SIZE; regAddress += regSize){

      if (counter == 0 || sent >= FLUSH_BATCH_SIZE){
         break;
      }
      
      EEPROM.get(regAddress, reading);

      // if value is REC_FLUSHED that register was already sent
      // 0 the position hasn't been used yet
      if (reading.flag == REC_FLUSHED){
        continue;
      }

      // flag 0 means that position or higher have never been used
      if (reading.flag == REC_NEVER_USED){
        return;
      }

      recNum = (regAddress - RECORDS_BASE_ADDRESS)/regSize;
      sprintf(buffer, "%s/add/%d:%d", baseURL, reading.timestamp, reading.value);

      if (!app->send(buffer)){
        sprintf(buffer, "[FLUSH_STORED_DATA] Error sending record [%d]", recNum);
        app->log(buffer);
        errors = true;
        if (!isServerAlive()){
          sprintf(buffer, "[FLUSH_STORED_DATA] Server doesn't respond. I'll try later.");
          app->log(buffer);
          break;
        }

      }
      else
      {
        drawSend();
        counter = decCounter();
        sent ++;
        
        sprintf(buffer, "[FLUSH_STORED_DATA] Success sending record [%d] (%d left)", recNum, readCounter());
        app->log(buffer);

        // We don't want to write the whole struct to save write cycles
        flag = REC_FLUSHED;
        EEPROM.write(regAddress, flag);
        EEPROM.commit();

        removeWarning(WARNING_STORAGE_IS_FULL);

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
  
  app->log(buffer);

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

void removeWarnings(){
  EEPROM.write(WARNINGS_ADDRESS, 0);
  EEPROM.commit();
}

byte removeWarning(byte warning){
  byte warnings = readWarnings();
  if (!(warnings && warning)){
    // warning was not set. nothing to remove
    return warnings;
  }
  warnings ^= warning;
  EEPROM.write(WARNINGS_ADDRESS, warnings);
  EEPROM.commit();
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
    //app->log(buffer);

    if (counterSlot <  COUNTER_SLOTS){
        counterSlot ++;
        counterAddress += 4; // each counter is 2 byte for n of writes + 2 bytes for the counter
        sprintf(buffer, "Counter slot moved to slot %d", counterSlot);
        app->log(buffer);
    }
    else{
        sprintf(buffer, "WARNING. Counter slots exhausted. Resetting to slot 1. Writes might start to fail soon");
        addWarning(WARNING_COUNTER_SLOTS_ROTATED);
        app->log(buffer);
        counterSlot = 1;
        counterAddress = 1;
    }

    counterWrites = 0;

    EEPROM.write(CURRENT_COUNTER_SLOT_ADDRESS, counterAddress);
  }

  EEPROM.put(counterAddress, counterWrites+1);
  EEPROM.put(counterAddress + 2, counter);
  
  //sprintf(buffer, "counter slot: %d, numwrites: %d", counterSlot, counterWrites+1);
  //app->log(buffer);

  if (!EEPROM.commit()) {
    app->log("Commit failed writing counter changes");
  }
}

int writeReading(unsigned long in_timestamp, short int in_value){
  
  int address;
  int regAddress;
  unsigned short int counter = readCounter();
 
  //sprintf(buffer, "writeReading %d %d", counter, MAX_RECORDS);
  //app->log(buffer);

  if (counter >= MAX_RECORDS){
    addWarning(WARNING_STORAGE_IS_FULL);
    app->log("WARNING. Local storage is full");
    return -1;
  }

  Reading newReading;
  Reading record;
  Reading currentReading;
  
  newReading.timestamp = in_timestamp;
  newReading.value = in_value;
  newReading.flag = REC_IN_USE;

  int regSize = sizeof(newReading);
  byte flag;

  for (address=RECORDS_BASE_ADDRESS; address < EEPROM_SIZE - regSize; address += regSize){
      EEPROM.get(address, flag);
      // flag is a used position
      if (flag != REC_IN_USE){
        EEPROM.put(address, newReading);
        EEPROM.commit();
        counter = incCounter();        
        sprintf(buffer, "Locally stored [%d of %d] %d:%d", counter, MAX_RECORDS, in_timestamp, in_value);
        app->log(buffer);
        break;
      }      
  }  

  return counter;
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
  app->log(buffer);

  restServer.send(200, "text/plain", buffer);
}

void resetWIFI(){

  sprintf(buffer, "WIFI networks will be reset and board rebooted NOW");
  app->log(buffer);
  restServer.send(200, "text/plain", buffer);
  _resetWifi();
}

void readConfigFile(){
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(server, json["server"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
}

ICACHE_RAM_ATTR void resetButtonPushed() {

    char buffer[20];
    static int t0 = millis();
    static bool armed = false;
    
    if (millis() - t0 < 200){
       t0 = millis();
       return;
    }

    if (!armed && digitalRead(RESET_BUTTON) == 0){ // pressed
       t0 = millis();
       armed = true;
    }

    if (armed && millis() - t0 > 10000){
       armed = false;
       time_to_reset = true;
       return;
    }
    
    if (digitalRead(RESET_BUTTON) == 1){ //released too soon
       Serial.println("RESET not performed");      
       armed = false;
    }

}

void checkConnection()  {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print(millis());
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.reconnect();
    }
}

void isTimeToReset(){
  if (time_to_reset){
    app->log("RESET button has been pressed for more than 10 seconds. Resetting WIFI and  EEPROM");
    resetEEPROM();
    _resetWifi();
  }
}

char* millis_to_human(unsigned long millis)
{
    char* buffer = new char[100];

    int seconds = millis/1000;

    int days = int(seconds/86400);
    seconds = seconds % 86400;
    int hours = int(seconds/3600);
    seconds = seconds % 3600;
    int minutes = int(seconds/60);
    seconds = seconds % 60;

    sprintf(buffer, "%d days, %d hours, %d minutes, %d seconds", days, hours, minutes, seconds);
    return buffer;
}

// Serving Hello world
void getHelloWord() {
    restServer.send(200, "text/json", "{\"name\": \"Hello world\"}");
}

void boardID() {
    sprintf(buffer, "%s\n", BOARD_ID);
    restServer.send(200, "text/plain", buffer);
}

void scan_networks() {

  char *buffer = new char[1024];
  int n = WiFi.scanNetworks();
  char *connected = NULL;

  if (n == 0) {
    Serial.println("no networks found");
  } else {
    sprintf(buffer, "networks found:\n");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found

      if (WiFi.SSID(i) == WiFi.SSID()){
        connected = "*";
      } else {
        connected = " ";
      }
    
      sprintf(buffer + strlen(buffer), "%d [%s] %-32s dBM %d (%d%%)\n", 
                                       i+1,
                                       connected, 
                                       WiFi.SSID(i).c_str(), 
                                       WiFi.RSSI(i), 
                                       dBmtoPercentage(WiFi.RSSI(i)));
    }
  }
  restServer.send(200, "text/plain", buffer);
}

void get_readings() {
    sprintf(buffer, "%d\n", readCounter());
    restServer.send(200, "text/plain", buffer);
}

void wifi_signal() {
    sprintf(buffer, "%s [%d]\n", WiFi.SSID(), WiFi.RSSI());
    restServer.send(200, "text/plain", buffer);
}


void version() {
    sprintf(buffer, "%s\n", VERSION);
    restServer.send(200, "text/plain", buffer);
}

void uptime() {
    sprintf(buffer, "%s\n", millis_to_human(millis()));
    restServer.send(200, "text/plain", buffer);
}

void boots() {
    sprintf(buffer, "%d\n", readBoots());
    restServer.send(200, "text/plain", buffer);
}

void reboot() {
    app->log("Board is going to reboot");
    restServer.send(200, "text/plain", "OK\n");
    delay(2000);
    ESP.restart();
}

void  restReadWarnings(){
    byte warnings = readWarnings();
    sprintf(buffer, "%d\n", warnings);
    restServer.send(200, "text/plain", buffer);
}

void clearWarnings(){
    removeWarnings();
    restServer.send(200, "text/plain", "OK\n");
}

int dBmtoPercentage(int dBm)
{
  int quality;
    if(dBm <= RSSI_MIN)
    {
        quality = 0;
    }
    else if(dBm >= RSSI_MAX)
    {  
        quality = 100;
    }
    else
    {
        quality = 2 * (dBm + 100);
   }

   return quality;
}

void help() {
    // this buffer is bigger 
    char *buffer = new char[1024];
    sprintf(buffer, "TLM (Tank Level Monitor) %s\n"
                    "----------------------------------------------------------------\n"
                    "\n"
                    "help: API help\n"
                    "helloWorld: json payload test\n"
                    "boardID: gets board's ID\n"
                    "version: build version\n"
                    "uptime: gets uptime\n"
                    "boots: number of boots so long\n"
                    "resetBoots: resets number of boots to 0\n"
                    "readings: number of locally stored readings\n"
                    "reboot: reboots the board\n"
                    "resetEEPROM: clears all info in EEPROM, including local readings.\n"
                    "resetWIFI: deletes WIFI known networks\n"
                    "WIFISignal: WIFI RSSI\n"
                    "scanNetworks: info about WIFI networks\n"
                    "warnings: lists current active warnings\n"
                    "warnings/clear: clear current warnings\n\n", VERSION
            );
    restServer.send(200, "text/plain", buffer);
}

// Define routing
void restServerRouting() {
    restServer.on("/", HTTP_GET, []() {
        restServer.send(200, F("text/html"),
            F("Welcome to the REST Web Server"));
    });
    restServer.on(F("/help"), HTTP_GET, help);
    restServer.on(F("/helloWorld"), HTTP_GET, getHelloWord);
    restServer.on(F("/boardID"), HTTP_GET, boardID);
    restServer.on(F("/version"), HTTP_GET, version);
    restServer.on(F("/uptime"), HTTP_GET, uptime);
    restServer.on(F("/boots"), HTTP_GET, boots);
    restServer.on(F("/resetBoots"), HTTP_GET, resetBoots);
    restServer.on(F("/reboot"), HTTP_GET, reboot);
    restServer.on(F("/readings"), HTTP_GET, get_readings);
    restServer.on(F("/resetEEPROM"), HTTP_GET, resetEEPROM);
    restServer.on(F("/resetWIFI"), HTTP_GET, resetWIFI);
    restServer.on(F("/WIFISignal"), HTTP_GET, wifi_signal);
    restServer.on(F("/scanNetworks"), HTTP_GET, scan_networks);
    restServer.on(F("/warnings"), HTTP_GET, restReadWarnings);
    restServer.on(F("/warnings/clear"), HTTP_GET, clearWarnings);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += restServer.uri();
  message += "\nMethod: ";
  message += (restServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += restServer.args();
  message += "\n";
  for (uint8_t i = 0; i < restServer.args(); i++) {
    message += " " + restServer.argName(i) + ": " + restServer.arg(i) + "\n";
  }
  restServer.send(404, "text/plain", message);
}

unsigned short readBoots(){
    unsigned short boots;
    EEPROM.get(BOOTS_ADDRESS, boots);
    return boots;
}



void resetBoots(){
    unsigned short boots = 0;
    EEPROM.write(BOOTS_ADDRESS, boots);
    EEPROM.commit();
    restServer.send(200, "text/plain", "boots resetted\n");
}

int incBoots(){
  unsigned short boots = readBoots();
  boots ++;
  EEPROM.write(BOOTS_ADDRESS, boots);
  EEPROM.commit(); 
  return boots;
}

void setup() {

  app = new App(BOARD_ID, log_server);

  Serial.begin(115200); 

  /*
  app->wifiManager->resetSettings();
  Serial.println("Wifi reseted");
  delay(5000);
  return;
  */
  
  //set GPIO14 interrupt
  attachInterrupt(digitalPinToInterrupt(RESET_BUTTON), resetButtonPushed, CHANGE);

  initOLED();
  EEPROM.begin(EEPROM_SIZE);

  if (SENSOR_MODE == 2){
      sensor.begin(9600);
  }
  else
  {
      pinMode(TRIGGER_PIN, OUTPUT); // Initializing Trigger Output and Echo Input
      pinMode(ECHO_PIN, INPUT);
      digitalWrite(TRIGGER_PIN, LOW); // Reset the trigger pin and wait a half a second
      delayMicroseconds(500);  
  }

  app->addTimer(30 * 1000, flushStoredData, "flushStoredData");
  app->addTimer(60 * 1000, registerNewReading, "registerNewReading");
  app->addTimer(1000, updateDisplay, "updateDisplay");
  app->addTimer(1000, isTimeToReset, "isTimeToReset");
  app->addTimer(30*1000, checkConnection, "checkConnection");

  readConfigFile();
  WiFiManagerParameter pserver(SERVER_LABEL, "Server IP", server, 16);

  //set config save notify callback
  app->wifiManager->setSaveConfigCallback(saveConfigCallback);
  app->wifiManager->addParameter(&pserver);

  app->startWiFiManager();

  strcpy(server, pserver.getValue());

  sprintf(log_server, "http://%s:8888", server);
  sprintf(baseURL, "http://%s:8889", server);

  sprintf(buffer, "log_server is %s", log_server);
  Serial.println(buffer);
  sprintf(buffer, "app_server is %s", baseURL);
  Serial.println(buffer);

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["server"] = server;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  // Set server routing
  restServerRouting();
  // Set not found response
  restServer.onNotFound(handleNotFound);
  // Start server
  restServer.begin();
  Serial.println("HTTP server started");

  unsigned short boots = incBoots();
  sprintf(buffer, "boot: %d", boots);
  Serial.println(buffer);
  app->log(buffer);

  delay(5000); // wait for sensor to settle
  registerNewReading();
}

void _resetWifi(){
  app->log("reset WIFI networks");
  app->wifiManager->resetSettings();
  ESP.restart();
}

void loop() {
  app->attendTimers();
  restServer.handleClient();
}

