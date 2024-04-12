// general libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <MedianFilterLib.h>

// LED strip definitions
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#define PIXELPIN 6
#define NUMPIXELS 17
Adafruit_NeoPixel pixels(NUMPIXELS, PIXELPIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 10

// OLED display definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define GREEN 0x07E0
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// USS pins
SoftwareSerial mySerial(10, 11);  // RX, TX

// button pin A -> switch between sensors
const int buttonPinA = 5;
int buttonPinAState;
int lastButtonPinAState = LOW;
int sensorState = 1;

// button pin B -> pause system
const int buttonPinB = 4;
int buttonPinBState;
int lastButtonPinBState = LOW;
int pause = 1;

// button pin C -> increment volume goal
const int buttonPinC = 3;
int buttonPinCState;
int lastButtonPinCState = LOW;

// read and write address for LiDAR
const uint8_t ADDRESS = 0x74;

// sensor reading storage
unsigned char data[4] = {};
uint8_t buf[2] = { 0 };
uint8_t dat = 0xB0;

// volume variables
float lastVolume = 0;
float curVolume = 0;
float totVolume = 0;
float maxVolume = 0;

// median filter algortithm
MedianFilter<float> medianFilter(5);
float median = 0;

void setup() {
  delay(1000);

  // set button pin modes
  pinMode(buttonPinA, INPUT);
  pinMode(buttonPinB, INPUT);
  pinMode(buttonPinC, INPUT);

  // begin serial communications
  Serial.begin(115200);
  mySerial.begin(9600);
  Wire.begin();

  // initialize OLED display
  display1.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  delay(2000);
  display1.clearDisplay();
  display1.setTextColor(WHITE);
  display1.display();
  delay(1000);

// initialize LED strip
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  pixels.begin();
}

void loop() {
  // check button pin A
  int reading = digitalRead(buttonPinA);
  if (reading != buttonPinAState) {
    buttonPinAState = reading;
    if (buttonPinAState == HIGH) {
      // switch between sensors
      sensorState = -sensorState;
    }
  }
  lastButtonPinAState = reading;

  // check button pin B
  reading = digitalRead(buttonPinB);
  if (reading != buttonPinBState) {
    buttonPinBState = reading;
    if (buttonPinBState == HIGH) {
      // pause sensors
      pause = -pause;
    }
  }
  lastButtonPinBState = reading;

  // check button pin C
  reading = digitalRead(buttonPinC);
  if (reading != buttonPinCState) {
    buttonPinCState = reading;
    if (buttonPinCState == HIGH) {
      // increment volume goal by 8oz
      maxVolume += 8.0;
    }
  }
  lastButtonPinCState = reading;

  Serial.println(sensorState);

  if (maxVolume > 1 && pause == -1) {
    if (sensorState == 1) {
      // ultrasonic sensor
      measureUSS();
    }
    if (sensorState == -1) {
      // lidar sensor
      measureLDR();
    }
  }
}

// measureUSS() function
// read data being sent by USS
void measureUSS() {
  // obtain sensor readings
  float distance;
  do {
    for (int i = 0; i < 4; i++) {
      data[i] = mySerial.read();
    }
  } while (mySerial.read() == 0xff);
  mySerial.flush();

  // calculate distance
  if (data[0] == 0xff) {
    int sum;
    sum = (data[0] + data[1] + data[2]) & 0x00FF;
    if (sum == data[3]) {
      distance = (data[1] << 8) + data[2];
      displayMeas(distance, "USS");
    }
    delay(100);
  }
}


// measureLDR() function
// read data being sent by LiDAR
void measureLDR() {
  // obtain sensor readings
  float distance;
  writeReg(0x10, &dat, 1);
  delay(50);
  readReg(0x02, buf, 2);

  // calculate distance
  distance = buf[0] * 0x100 + buf[1] + 10;
  displayMeas(distance, "LDR");

  delay(100);
}


// readReg() function
// code provided by (SEN0590) datasheet
uint8_t readReg(uint8_t reg, const void* pBuf, size_t size) {
  if (pBuf == NULL) {
    Serial.println("pBuf ERROR!! : null pointer");
  }
  uint8_t* _pBuf = (uint8_t*)pBuf;
  Wire.beginTransmission(ADDRESS);
  Wire.write(&reg, 1);
  if (Wire.endTransmission() != 0) {
    return 0;
  }
  delay(20);
  Wire.requestFrom(ADDRESS, (uint8_t)size);
  for (uint16_t i = 0; i < size; i++) {
    _pBuf[i] = Wire.read();
  }
  return size;
}


// writeReg() function
// code provided by LiDAR (SEN0590) datasheet
bool writeReg(uint8_t reg, const void* pBuf, size_t size) {
  if (pBuf == NULL) {
    Serial.println("pBuf ERROR!! : null pointer");
  }
  uint8_t* _pBuf = (uint8_t*)pBuf;
  Wire.beginTransmission(ADDRESS);
  Wire.write(&reg, 1);
  for (uint16_t i = 0; i < size; i++) {
    Wire.write(_pBuf[i]);
  }
  if (Wire.endTransmission() != 0) {
    return 0;
  } else {
    return 1;
  }
}


// displayMeas() function
// takes in distance and displays volume measurements
void displayMeas(float distance, char* sensor) {
  if (distance > 30 && distance < 250) {
    Serial.print(F("distance="));
    Serial.print(distance);
    Serial.println(F("mm"));

    // convert distance reading to volume measurement
    curVolume = medianFilter.AddValue(convertDistancesToVolume(distance));
    Serial.print(F("curVolume="));
    Serial.print(curVolume);
    Serial.println(F("oz"));

    // calculate total based on current and last
    if (curVolume >= (lastVolume * 1.25)) {
      lastVolume = curVolume;
    }
    if (curVolume <= (lastVolume * 0.75)) {
      totVolume = totVolume + (lastVolume - curVolume);
      lastVolume = curVolume;
    }
    Serial.print(F("totVolume="));
    Serial.print(totVolume);
    Serial.println(F("oz"));

    // light LED strip according to goal progress
    float ratio = totVolume / (maxVolume + 1);
    lightPixels(ratio);

    // display info to OLED
    display1.clearDisplay();
    display1.setTextSize(1);
    display1.setTextColor(WHITE);
    // line 1
    display1.setCursor(0, 0);
    display1.print(F("Sensor: "));
    display1.print(sensor);
    // line 2
    display1.setCursor(0, 25);
    display1.print(F("Volume: "));
    display1.print(curVolume);
    display1.print(F("oz"));
    // line 3
    display1.setCursor(0, 50);
    display1.print(F("Total/Goal: "));
    if (totVolume >= maxVolume) {
      display1.setTextColor(GREEN);
    }
    display1.print(totVolume);
    display1.print(F("/"));
    display1.print(maxVolume);
    display1.print(F("oz"));

    display1.display();
  } else {
    Serial.println(F("Limit"));
  }
}


// convertDistancesToVolume() function
// takes in distance and returns volume
float convertDistancesToVolume(float distance) {
  // bottle measurements and data conversions
  float radius = 31.75;
  float distanceWater = (PI * radius * radius * (distance)) * 0.00003381;
  float totVolume = 24.50;
  float CurrentVolume = totVolume - distanceWater;

  // throw out negative readings (ERRORS)
  if (CurrentVolume < 0) {
    CurrentVolume = 0;
  }
  return CurrentVolume;
}


// lightPixels() function
// takes in volume ratio and lights LED strip
void lightPixels(float ratio) {
  if (ratio < .066) {
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .066) {
    pixels.setPixelColor(1, pixels.Color(255, 0, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .132) {
    pixels.setPixelColor(2, pixels.Color(255, 0, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .198) {
    pixels.setPixelColor(3, pixels.Color(255, 0, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .264) {
    pixels.setPixelColor(4, pixels.Color(255, 0, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .33) {
    pixels.setPixelColor(5, pixels.Color(0, 0, 255));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .396) {
    pixels.setPixelColor(6, pixels.Color(0, 0, 255));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .462) {
    pixels.setPixelColor(7, pixels.Color(0, 0, 255));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .528) {
    pixels.setPixelColor(8, pixels.Color(0, 0, 255));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .594) {
    pixels.setPixelColor(9, pixels.Color(0, 0, 255));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .66) {
    pixels.setPixelColor(10, pixels.Color(0, 255, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .726) {
    pixels.setPixelColor(11, pixels.Color(0, 255, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .792) {
    pixels.setPixelColor(12, pixels.Color(0, 255, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .858) {
    pixels.setPixelColor(13, pixels.Color(0, 255, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= .924) {
    pixels.setPixelColor(14, pixels.Color(0, 255, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (ratio >= 1) {
    pixels.setPixelColor(15, pixels.Color(255, 255, 255));
    pixels.show();
    delay(DELAYVAL);
  }
  pixels.setBrightness(30);
}
