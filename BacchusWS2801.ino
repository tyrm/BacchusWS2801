#include <SPI.h>
#include <Adafruit_WS2801.h>

// The Bacchus Protocol is Loosely defined here
// https://docs.google.com/spreadsheet/ccc?key=0AuwhwA-ghDeydEtWOUZZdTVKaDM5c211U210QV8ySWc&usp=sharing

//Configuration
int devAddress = 1;      // Device Address
uint8_t numOfPixels = 40;    // Number of Pixels this is controlling
uint8_t dataPin  = 2;    // Yellow wire on Adafruit Pixels
uint8_t clockPin = 3;    // Green wire on Adafruit Pixels

Adafruit_WS2801 pixels = Adafruit_WS2801(numOfPixels, dataPin, clockPin, WS2801_GRB);
byte serBuffer[512];          // Buffer for incoming unprocessed serial data
int bufCursor = 0;            // Current Location within the Buffer
byte incomingByte = 0;        // Temporary holder for incoming serial byte
boolean listening = false;    // Wether Bacchus is currently listening for serial DATA

void setup() {
  //Open Serial Port
  Serial.begin(115200); 
  while (!Serial) {} //Leonardo Fix
  
  
}

void loop() {
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    if ((incomingByte & 0xE0) == 0xE0) {
      // This is an EXECUTABLE Byte
      switch (incomingByte) {
        case 0xE8:
          // Recieved Master Start
          listening = true;
          bufCursor = 0;
          break;
        case 0xEE:
          // Received End
          processSerialBuffer(false);
          listening = false;
          break;
        case 0xEF:
          // Received End and Execute
          processSerialBuffer(true);
          listening = false;
          break;
        default:
          break;
      }
    } else if (listening) {
      // This is a DATA byte
      
      serBuffer[bufCursor] = incomingByte; // Copy Data to Serial Buffer
      bufCursor++;
      
      // If first 2 bytes do not equal device address then stop listening.
      if (bufCursor == 2) {
        if (addBytes(serBuffer[0],serBuffer[1]) != devAddress) {
          bufCursor = 0;
          listening = false;
        }
      }
    }
  }
}

void processSerialBuffer(boolean executeNow) {
  
}

int addBytes(byte byteA, byte byteB) {
  // Joins 2 Data Bytes into a single number
  
  int total = byteA;
  total <<= 7;
  total |= byteB;
  return total;
}

uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}
