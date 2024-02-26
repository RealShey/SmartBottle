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
const int buttonPinA = 4;
int buttonPinAState;
int lastButtonPinAState = LOW;
int sensorState = 1;

// button pin B -> reset to system to 0
const int buttonPinB = 6;
int buttonPinBState;
int lastButtonPinBState = LOW;

// read and write address
const uint8_t ADDRESS = 0x74;

// sensor reading storage
unsigned char data[4] = {};
uint8_t buf[2] = { 0 };
uint8_t dat = 0xB0;
float distance;

// debounce
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;

void setup() {
  // put your setup code here, to run once:

  pinMode(buttonPinA, INPUT);
  pinMode(buttonPinB, INPUT);

  Serial.begin(57600);
  mySerial.begin(9600);
  Wire.begin();

  // display 1 (Ultrasonic Sensor Display)
  display1.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  delay(2000);
  display1.clearDisplay();
  display1.setTextColor(WHITE);
  delay(1000);

}

void loop() {
  // put your main code here, to run repeatedly:

  // check button pin A
  int reading = digitalRead(buttonPinA);
  if (reading != lastButtonPinAState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay && reading != buttonPinAState) {
    {
      buttonPinAState = reading;
      if (buttonPinAState == HIGH) {
        // switch between sensors
        sensorState = -sensorState;
      }
    }
  }
  lastButtonPinAState = reading;

  // check button pin B
  reading = digitalRead(buttonPinB);
  if (reading != lastButtonPinBState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay && reading != buttonPinBState) {
    {
      buttonPinBState = reading;
      if (buttonPinBState == HIGH) {
        // do something
      }
    }
  }
  lastButtonPinBState = reading;

  Serial.println(sensorState);

  if (sensorState == 1) {
    // ultrasonic sensor
    measureUSS();
  }
  if (sensorState == -1) {
    // lidar sensor
    measureLDR();
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


        // ALEX
        //
        // insert distance to volume calculation here:
        //
      convertDistancesToVolume(distance);

        // display to OLED
        display1.clearDisplay();
        display1.setCursor(10, 0);
        display1.setTextSize(2);
        display1.setTextColor(WHITE);
        display1.print("USS Dist.");
        display1.setCursor(10, 30);
        display1.setTextSize(2);
        display1.print(String(distance) + " mm");
        display1.display();
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


    // ALEX
    //
    // insert distance to volume calculation here:
    //


    // display to OLED
    display1.clearDisplay();
    display1.setCursor(10, 0);
    display1.setTextSize(2);
    display1.setTextColor(WHITE);
    display1.print("LDR Dist.");
    display1.setCursor(10, 30);
    display1.setTextSize(2);
    display1.print(String(distance) + " mm");
    display1.display();
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
float convertDistancesToVolume(float distance) {
  // Replace this with your actual conversion formula
  // This is a placeholder, adjust as needed
  float radius = 2.5;  // Replace with your cylinder's radius in centimeters
  float CurrentVolume = PI * radius * radius *(distance);
  return CurrentVolume;
}