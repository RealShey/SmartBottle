#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

// OLED display definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// USS pins
SoftwareSerial mySerial(11, 10);  // RX, TX

// button pin A -> switch between sensors
const int buttonPinA = 5;
int buttonPinAState;
int lastButtonPinAState = LOW;
int sensorState = 1;

// button pin B -> set volume goal or reset to system to 0
const int buttonPinB = 4;
int buttonPinBState;
int lastButtonPinBState = LOW;

// button pin C -> increment volume goal
const int buttonPinC = 3;
int buttonPinCState;
int lastButtonPinCState = LOW;

// read and write address
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
  Serial.begin(57600);
  mySerial.begin(9600);
  Wire.begin();

  // display 1
  display1.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  delay(2000);
  display1.clearDisplay();
  display1.setTextColor(WHITE);
  delay(1000);
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
      // reset to system to 0
      maxVolume = 0;
      totVolume = 0;
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
      Serial.print("Water goal: ");
      Serial.print(maxVolume);
      Serial.println(" oz");
    }
  }
  lastButtonPinCState = reading;

  Serial.println(sensorState);

  if (maxVolume != 0) {
    if (sensorState == -1) {
      // ultrasonic sensor
      measureUSS();
    }
    if (sensorState == 1) {
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
      if (distance > 30) {
        Serial.print("distance=");
        Serial.print(distance);
        Serial.println("mm");

        // convert distance reading to volume measurement
        curVolume = convertDistancesToVolume(distance);
        Serial.print("curVolume=");
        Serial.print(curVolume);
        Serial.println("oz");

        // calculate total based on current and last
        if (curVolume >= (lastVolume * 1.05)) {
          lastVolume = curVolume;
        }
        if (curVolume <= (lastVolume * 0.95)) {
          totVolume = totVolume + (lastVolume - curVolume);
          lastVolume = curVolume;
        }
        Serial.print("totVolume=");
        Serial.print(totVolume);
        Serial.println("oz");

        // Miguel add yours here
        if (totVolume >= maxVolume) {
          display1.clearDisplay();
          display1.setCursor(10, 0);
          display1.setTextSize(2);
          display1.setTextColor(WHITE);
          display1.print("Goal Reached!");
          display1.display();
        } else {
          display1.clearDisplay();
          display1.setCursor(10, 0);
          display1.setTextSize(2);
          display1.setTextColor(WHITE);
          display1.print("Water Drank:");
          display1.print(totVolume);
          display1.print("oz");
          display1.display();
        }
      } else {
        Serial.println("Below the lower limit");

        // display to OLED
        display1.clearDisplay();
        display1.setCursor(10, 0);
        display1.setTextSize(2);
        display1.setTextColor(WHITE);
        display1.print("USS Dist.");
        display1.setCursor(10, 30);
        display1.setTextSize(2);
        display1.print("Low Limit");
        display1.display();
      }
    } else Serial.println("ERROR");
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

  if (distance > 30) {
    Serial.print("distance=");
    Serial.print(distance);
    Serial.println("mm");

    // convert distance reading to volume measurement
    curVolume = convertDistancesToVolume(distance);
    Serial.print("curVolume=");
    Serial.print(curVolume);
    Serial.println("oz");

    // calculate total based on current and last
    if (curVolume >= (lastVolume * 1.05)) {
      lastVolume = curVolume;
    }
    if (curVolume <= (lastVolume * 0.95)) {
      totVolume = totVolume + (lastVolume - curVolume);
      lastVolume = curVolume;
    }
    Serial.print("totVolume=");
    Serial.print(totVolume);
    Serial.println("oz");

    if (totVolume >= maxVolume) {
      display1.clearDisplay();
      display1.setCursor(10, 0);
      display1.setTextSize(2);
      display1.setTextColor(WHITE);
      display1.print("Goal Reached!");
      display1.display();
    } else {
      display1.clearDisplay();
      display1.setCursor(10, 0);
      display1.setTextSize(2);
      display1.setTextColor(WHITE);
      display1.print("Water Drank:");
      display1.print(totVolume);
      display1.print("oz");
      display1.display();
    }
  } else {
    Serial.println("Below the lower limit");

    // display to OLED
    display1.clearDisplay();
    display1.setCursor(10, 0);
    display1.setTextSize(2);
    display1.setTextColor(WHITE);
    display1.print("LDR Dist.");
    display1.setCursor(10, 30);
    display1.setTextSize(2);
    display1.print("Low Limit");
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
  float totVolume = 24.00;
  float CurrentVolume = totVolume - distanceWater;
  if (CurrentVolume < 0) {
    CurrentVolume = 0;
  }
  return CurrentVolume;
}
