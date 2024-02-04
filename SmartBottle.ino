#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

#define ADDRESS 0x74

// OLED display definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SoftwareSerial mySerial(11, 10);

// sensor reading storage
unsigned char ussData[4] = {};
uint8_t buf[2] = { 0 };
uint8_t dat = 0xB0;
float distance1;
int distance2;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  mySerial.begin(9600);
  Wire.begin();

  // display 1 (Ultrasonic Sensor Display)
  display1.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(2000);
  display1.clearDisplay();
  display1.setTextColor(WHITE);
  delay(1000);

  // display 2 (LiDAR Rangefinder Display)
  display2.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(2000);
  display2.clearDisplay();
  display2.setTextColor(WHITE);
  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:

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

  // lidar rangefinder
  writeReg(0x10, &dat, 1);
  delay(50);
  readReg(0x02, buf, 2);
  distance2 = buf[0] * 0x100 + buf[1] + 10;
  Serial.print("distance2=");
  Serial.print(distance2);
  Serial.print("mm");
  Serial.println("\t");
  delay(100);
}

// readReg() function
uint8_t readReg(uint8_t reg, const void* pBuf, size_t size) {
  if (pBuf == NULL) {
    Serial.println("pBuf ERROR!! : null pointer");
  }
  uint8_t* _pBuf = (uint8_t*)pBuf;
  Wire.beginTransmission(address);
  Wire.write(&reg, 1);
  if (Wire.endTransmission() != 0) {
    return 0;
  }
  delay(20);
  Wire.requestFrom(address, (uint8_t)size);
  for (uint16_t i = 0; i < size; i++) {
    _pBuf[i] = Wire.read();
  }
  return size;
}

// writeReg() function
bool writeReg(uint8_t reg, const void* pBuf, size_t size) {
  if (pBuf == NULL) {
    Serial.println("pBuf ERROR!! : null pointer");
  }
  uint8_t* _pBuf = (uint8_t*)pBuf;
  Wire.beginTransmission(address);
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
