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

#define TX 14
#define RX 12

char buffer[100];
unsigned char dataBuffer[4] = {0};
unsigned char CS;
int distance;
int counter = 0; // number of registers in EEPROM

int lastReading = 0;

SoftwareSerial sensor(RX, TX);


// prototipes
void FlushStoredData();
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

int previous_distance = 0;

typedef struct
{
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

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

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

void clearSection(int x, int y, int x1, int y1){
  display.fillRect(x, y, x1, y1, 0);
  display.display();
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

  unsigned short int counter = readEEPROMCounter();
  display.setCursor(0,26);            
  display.print("Local: ");
  display.setCursor(46,26);            
  display.print(counter);

  display.setCursor(0,56);            
  display.print("IP: ");
  display.setCursor(46,56);            
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
  display.display();
  delay(2000); // Pause for 2 seconds
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
        drawSend();
        sent ++;
        sprintf(buffer, "[FLUSH_STORED_DATA] Success sending record [%d]", cursor);
        app.log(buffer);
        // We don't want to write the whole struct to save write cycles
        value = -1;
        EEPROM.put(regAddress + sizeof(reading.timestamp), value);
        EEPROM.commit();            
        clearSection(94, 0, 16, 16);
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

void setup() {

  initOLED();
  
  Serial.begin(115200); 
  EEPROM.begin(EEPROM_SIZE);
  // resetEEPROM();
  sensor.begin(9600);

  app.addTimer(120*1000, FlushStoredData, "FlushStoredData");
  app.addTimer(1000, registerNewReading, "registerNewReading");
  app.addTimer(1000, updateDisplay, "updateDisplay");
}

void loop() {
  app.attendTimers();
}
