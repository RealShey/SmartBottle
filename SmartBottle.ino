#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

// OLED display definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SoftwareSerial mySerial(11, 10);

const uint8_t ADDRESS = 0x74;

// sensor reading storage
unsigned char ussData[4] = {};
uint8_t buf[2] = { 0 };
uint8_t dat = 0xB0;
float distance1 = 0;
int distance2 = 0;

void setup() {
  // put your setup code here, to run once:
  delay(1000);
  
  Serial.begin(115200);
  mySerial.begin(9600);
  Wire.begin();

  // display 1 (Ultrasonic Sensor Display)
  // display1.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // delay(2000);
  // display1.clearDisplay();
  // display1.setTextColor(WHITE);
  // delay(1000);

  // display 2 (LiDAR Rangefinder Display)
  display2.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  delay(2000);
  display2.clearDisplay();
  display2.setTextColor(WHITE);
  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:

  // lidar sensor
  measureLDR(distance2);
  delay(1000);

  // ultrasonic sensor
  do {
    for (int i = 0; i < 4; i++) {
      ussData[i] = mySerial.read();
    }
  } while (mySerial.read() == 0xff);

  mySerial.flush();

  if (ussData[0] == 0xff) {
    int sum;
    sum = (ussData[0] + ussData[1] + ussData[2]) & 0x00FF;
    if (sum == ussData[3]) {
      distance1 = (ussData[1] << 8) + ussData[2];
      distance1 = distance1 / 10;
      Serial.println(distance1);
      display1.clearDisplay();
      display1.setCursor(10, 0);
      display1.setTextSize(2);
      display1.setTextColor(WHITE);
      display1.print("Distance");
      display1.setCursor(10, 30);
      display1.setTextSize(2);
      display1.print(String(distance1) + " cm");
      display1.display();
    }
  }
  delay(1000);
}

// measureLDR() function
void measureLDR(int distance) {
  // obtain sensor readings
  writeReg(0x10, &dat, 1);
  delay(50);
  readReg(0x02, buf, 2);

  // calculate distance
  distance = buf[0] * 0x100 + buf[1] + 10;

  // display result to OLED
  Serial.println(distance);
  display2.clearDisplay();
  display2.setCursor(10, 0);
  display2.setTextSize(2);
  display2.setTextColor(WHITE);
  display2.print("Distance2");
  display2.setCursor(10, 30);
  display2.setTextSize(2);
  display2.print(String(distance) + " mm");
  display2.display();
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
