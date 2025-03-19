#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SoftwareSerial pmsSerial(2, 3);

void setup() {
  Serial.begin(115200);
  pmsSerial.begin(9600);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};

struct pms5003data data;

uint32_t startTime = 0;
const uint32_t interval = 5000; // 5 seconds

void loop() {
  if (readPMSdata(&pmsSerial)) {
    int AQI_PM25 = calculateAQI(data.pm25_standard, true);
    int AQI_PM10 = calculateAQI(data.pm100_standard, false);
    
    int maxAQI = max(AQI_PM25, AQI_PM10);

    Serial.print("AQI (PM2.5): ");
    Serial.println(AQI_PM25);
    Serial.print("AQI (PM10): ");
    Serial.println(AQI_PM10);
    Serial.print("Max AQI: ");
    Serial.println(maxAQI);

    displayAQI(maxAQI);
  }
}

int calculateAQI(float concentration, bool isPM25) {
  int breakpoints[6][2] = {
    {0, 50}, {51, 100}, {101, 150}, {151, 200}, {201, 300}, {301, 500}
  };

  int pm25Limits[6] = {12, 35, 55, 150, 250, 500};
  int pm10Limits[6] = {54, 154, 254, 354, 424, 604};

  int *limits = isPM25 ? pm25Limits : pm10Limits;

  for (int i = 0; i < 6; i++) {
    if (concentration <= limits[i]) {
      return map(concentration, i == 0 ? 0 : limits[i - 1], limits[i], breakpoints[i][0], breakpoints[i][1]);
    }
  }
  return 500;  // Max AQI
}

void displayAQI(int aqi) {
  display.clearDisplay();
  display.setCursor(10, 10);
  display.setTextSize(2);
  display.print("AQI: ");
  display.println(aqi);
  display.display();
}

boolean readPMSdata(Stream *s) {
  if (!s->available()) {
    return false;
  }
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }
  if (s->available() < 32) {
    return false;
  }
  uint8_t buffer[32];
  uint16_t sum = 0;
  s->readBytes(buffer, 32);
  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }
  uint16_t buffer_u16[15];
  for (uint8_t i = 0; i < 15; i++) {
    buffer_u16[i] = buffer[2 + i * 2 + 1];
    buffer_u16[i] += (buffer[2 + i * 2] << 8);
  }
  memcpy((void *)&data, (void *)buffer_u16, 30);
  if (sum != data.checksum) {
    Serial.println("Checksum failure");
    return false;
  }
  return true;
}
