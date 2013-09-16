#include <SPI.h>
#include <Adafruit_WS2801.h>

// The Bacchus Protocol is Loosely defined here
// https://docs.google.com/spreadsheet/ccc?key=0AuwhwA-ghDeydEtWOUZZdTVKaDM5c211U210QV8ySWc&usp=sharing

//Configuration
const int devAddress       = 1;     // Device Address
const int numOfPixels      = 20;    // Number of Pixels this is controlling
const int procLED          = 13;    //
const uint8_t dataPin      = 6;     // Yellow wire on Adafruit Pixels
const uint8_t clockPin     = 7;     // Green wire on Adafruit Pixels

Adafruit_WS2801 pixels = Adafruit_WS2801(numOfPixels, dataPin, clockPin, WS2801_GRB);
byte serBuffer[512];          // Buffer for incoming unprocessed serial data
int bufCursor        = 0;     // Current Location within the Buffer
byte incomingByte    = 0;     // Temporary holder for incoming serial byte
boolean listening    = false; // Wether Bacchus is currently listening for serial DATA
int executeOn        = -1;    // Which control signal to respond to (0-15). -1 = none.
boolean blackout     = false;

boolean stateBuf[numOfPixels];     // Pixel State
uint32_t colorBuf[numOfPixels];    // Pixel Color

void setup() {
  //Open Serial Port
  Serial.begin(115200); 
  while (!Serial) {} //Leonardo Fix
  
  pixels.show();
}

void loop() {
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    if ((incomingByte & 0xF0) == 0xF0) {
      // This is a SIGNAL Byte
      if ((incomingByte & 0x0F) == executeOn) {
        pixels.show();
        executeOn = -1;
      }
    } else if ((incomingByte & 0xF8) == 0xE8) {
      // This is an EXECUTABLE Byte
      switch (incomingByte) {
        case 0xE8: // Master Start
          listening = true;
          bufCursor = 0;
          break;
        case 0xEA: // Blackout On
          blackout = true;
          for (int i=0;i<numOfPixels;i++) {
            pixels.setPixelColor(i, Color(0,0,0));
          }
          pixels.show();
          break;
        case 0xEB: // Blackout Off
          blackout = false;
          pixels.show();
          break;
        case 0xEE: // End
          processSerialBuffer(false);
          listening = false;
          break;
        case 0xEF: // End and Execute
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
          listening = false;
        }
      }
    }
  }
}

void processSerialBuffer(boolean executeNow) {
  byte curCommand = 0x00;
  int cmdOffset = 0;
  int cmdIndex = 0;
  
  for (int i=2;i<bufCursor;i++) {
    if (serBuffer[i] & 0x80 == 0x80) {
      curCommand = serBuffer[i];
      
      cmdOffset = i+1;
      cmdIndex = 0;
    } else {
      switch (curCommand) {
        case 0x80: // lightState
          if (i == cmdOffset) {
            cmdIndex = addBytes(serBuffer[i],serBuffer[i+1]);
            i++; // Because we used 2 bytes
          } else {
            if (serBuffer[i] == 1) {
              stateBuf[cmdIndex] = true;
            } else {
              stateBuf[cmdIndex] = false;
            }
            
            updatePixel(cmdIndex);
            
            cmdIndex++;
          }
          break;
        case 0x81: // lightStateAll
          if (i == cmdOffset) {
            // Get State
            boolean setTo = false;
            if (serBuffer[i] == 1) {
              setTo = true;
            }
            
            // Apply to All Pixels
            for (int idx;idx<numOfPixels;idx++) {
              stateBuf[idx] = setTo;
              updatePixel(idx);
            }
          }
          break;
        case 0x82: // lightColor
          if (i == cmdOffset) {
            cmdIndex = addBytes(serBuffer[i],serBuffer[i+1]);
            i++; // Because we used 2 bytes
          } else {
            colorBuf[cmdIndex] = Color(serBuffer[i]*2,serBuffer[i+1]*2,serBuffer[i+2]*2);
            
            updatePixel(cmdIndex);
            
            cmdIndex++;
            i += 2; //Because we used 3 bytes
          }
          break;
        case 0x83: // lightColorAll
          if (i == cmdOffset) {
            // Get Color
            uint32_t setTo = Color(serBuffer[i]*2,serBuffer[i+1]*2,serBuffer[i+2]*2);
            
            // Apply to All Pixels
            for (int idx;idx<numOfPixels;idx++) {
              colorBuf[idx] = setTo;
              updatePixel(idx);
            }
            i += 2; //Because we used 3 bytes
          }
          break;
        case 0xC0: // Use Signal
          if (i == cmdOffset) {
            executeOn = serBuffer[i];
          }
          break;
      }
    }
  }
  
  if (executeNow) {
    if (!blackout) {
      pixels.show();
    }
    executeOn = -1;
  }
}

void updatePixel(int i) {
  if (stateBuf[i]) {
    pixels.setPixelColor(i, colorBuf[i]);
  } else {
    pixels.setPixelColor(i, Color(0,0,0));
  }
}

int addBytes(byte byteA, byte byteB) {
  // Joins 2 Data Bytes into a single number
  
  int total = byteA;
  total <<= 7;
  total |= byteB;
  return total;
}

uint32_t Color(byte r, byte g, byte b) {
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}
