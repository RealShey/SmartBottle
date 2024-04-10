// general libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

// LED strip declerations
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
float distance;

// volume variables
float lastVolume = 0;
float curVolume = 0;
float totVolume = 0;
float maxVolume = 0;

void setup() {
  delay(1000);

  // set button pin mode as inputs
  pinMode(buttonPinA, INPUT);
  pinMode(buttonPinB, INPUT);
  pinMode(buttonPinC, INPUT);

  // initialize serial communications
  Serial.begin(115200);
  mySerial.begin(9600);
  Wire.begin();

  // display 1
  display1.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  delay(2000);
  display1.clearDisplay();
  display1.setTextColor(WHITE);
  delay(1000);

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
      // increment volume goal
      maxVolume += 8.0;  // Increment by 8 oz
      Serial.print(F("Water goal: "));
      Serial.print(maxVolume);
      Serial.println(F(" oz"));
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
void measureUSS() {
  // obtain sensor readings
  do {
    for (int i = 0; i < 4; i++) {
      data[i] = mySerial.read();
    }
  } while (mySerial.read() == 0xff);

  mySerial.flush();

  if (data[0] == 0xff) {
    // calculate distance
    int sum;
    sum = (data[0] + data[1] + data[2]) & 0x00FF;
    if (sum == data[3]) {
      distance = (data[1] << 8) + data[2];
      if (distance > 30 && distance < 270) {
        Serial.print(F("distance="));
        Serial.print(distance);
        Serial.println(F("mm"));

        // convert distance reading to volume measurement
        curVolume = convertDistancesToVolume(distance);
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

        float Ratio = totVolume / (maxVolume + 1);
        lightPixels(Ratio);

        display1.clearDisplay();
        display1.setCursor(10, 0);
        display1.setTextSize(2);
        display1.setTextColor(WHITE);
        display1.print(F("You Drank"));
        display1.print(totVolume);
        display1.print(F("oz"));
        display1.display();
        if (totVolume >= maxVolume) {
          // display1.clearDisplay();
          // display1.setCursor(32, 0);
          // display1.setTextSize(3);
          display1.setTextColor(WHITE);
          display1.print(F("Goal!"));
          display1.display();
        }
      } else {
        Serial.println(F("Limit"));

        // display to OLED
        display1.clearDisplay();
        display1.setCursor(10, 0);
        display1.setTextSize(2);
        display1.setTextColor(WHITE);
        display1.print(F("USS Dist."));
        display1.setCursor(10, 30);
        display1.setTextSize(2);
        display1.print(F("Limit"));
        display1.display();
      }
    } else Serial.println(F("ERROR"));
  }
  delay(100);
}


// measureLDR() function
void measureLDR() {
  // obtain sensor readings
  int distance;
  writeReg(0x10, &dat, 1);
  delay(50);
  readReg(0x02, buf, 2);

  // calculate distance
  distance = buf[0] * 0x100 + buf[1] + 10;

  if (distance > 30 && distance < 220) {
    Serial.print(F("distance="));
    Serial.print(distance);
    Serial.println(F("mm"));

    // convert distance reading to volume measurement
    curVolume = convertDistancesToVolume(distance);
    Serial.print(F("curVolume="));
    Serial.print(curVolume);
    Serial.println(F("oz"));

    // calculate total based on current and last
    if (curVolume >= (lastVolume * 1.50)) {
      lastVolume = curVolume;
    }
    if (curVolume <= (lastVolume * 0.50)) {
      totVolume = totVolume + (lastVolume - curVolume);
      lastVolume = curVolume;
    }
    Serial.print(F("totVolume="));
    Serial.print(totVolume);
    Serial.println(F("oz"));

    float Ratio = totVolume / (maxVolume + 1);
    lightPixels(Ratio);

    display1.clearDisplay();
    display1.setCursor(10, 0);
    display1.setTextSize(2);
    display1.setTextColor(WHITE);
    display1.print(F("You Drank"));
    display1.print(totVolume);
    display1.print(F("oz"));
    display1.display();
    if (totVolume >= maxVolume) {
      // display1.clearDisplay();
      // display1.setCursor(32, 0);
      // display1.setTextSize(3);
      display1.setTextColor(WHITE);
      display1.print(F("Goal!"));
      display1.display();
    }
  } else {
    Serial.println(F("Limit"));

    // display to OLED
    display1.clearDisplay();
    display1.setCursor(10, 0);
    display1.setTextSize(2);
    display1.setTextColor(WHITE);
    display1.print(F("LDR Dist."));
    display1.setCursor(10, 30);
    display1.setTextSize(2);
    display1.print(F("Limit"));
    display1.display();
  }
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


// convertDistancesToVolume() function
float convertDistancesToVolume(float distance) {
  float radius = 31.75;
  float distanceWater = (PI * radius * radius * (distance)) * 0.00003381;
  float totVolume = 24.50;
  float CurrentVolume = totVolume - distanceWater;
  if (CurrentVolume < 0) {
    CurrentVolume = 0;
  }
  return CurrentVolume;
}


// lightPixels() function
void lightPixels(float Ratio) {
  if (Ratio < .066) {
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .066) {
    pixels.setPixelColor(1, pixels.Color(255, 0, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .132) {
    pixels.setPixelColor(2, pixels.Color(255, 0, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .198) {
    pixels.setPixelColor(3, pixels.Color(255, 0, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .264) {
    pixels.setPixelColor(4, pixels.Color(255, 0, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .33) {
    pixels.setPixelColor(5, pixels.Color(0, 0, 255));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .396) {
    pixels.setPixelColor(6, pixels.Color(0, 0, 255));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .462) {
    pixels.setPixelColor(7, pixels.Color(0, 0, 255));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .528) {
    pixels.setPixelColor(8, pixels.Color(0, 0, 255));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .594) {
    pixels.setPixelColor(9, pixels.Color(0, 0, 255));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .66) {
    pixels.setPixelColor(10, pixels.Color(0, 255, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .726) {
    pixels.setPixelColor(11, pixels.Color(0, 255, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .792) {
    pixels.setPixelColor(12, pixels.Color(0, 255, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .858) {
    pixels.setPixelColor(13, pixels.Color(0, 255, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= .924) {
    pixels.setPixelColor(14, pixels.Color(0, 255, 0));
    pixels.show();
    delay(DELAYVAL);
  }
  if (Ratio >= 1) {
    pixels.setPixelColor(15, pixels.Color(255, 255, 255));
    pixels.show();
    delay(DELAYVAL);
  }
  pixels.setBrightness(30);
}
