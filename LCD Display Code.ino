#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// I2C Pin Definitions for ESP32
#define SDA_PIN 21
#define SCL_PIN 22
#define LCD_COLUMNS 20
#define LCD_ROWS 4

// Dynamic LCD pointer object
LiquidCrystal_I2C *lcd = nullptr;

// Function to auto-detect I2C address (0x27 or 0x3F)
uint8_t scanI2CAddress() {
  uint8_t detectedAddress = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      if (addr == 0x27 || addr == 0x3F) {
        detectedAddress = addr;
        break;
      }
    }
  }
  return detectedAddress;
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(500);

  Serial.println("\n==========================================");
  Serial.println("     ESP32 & I2C LCD DIRECT SYSTEM        ");
  Serial.println("==========================================");

  // Initialize I2C Bus on GPIO 21 (SDA) and GPIO 22 (SCL)
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(150);

  // Auto-detect LCD I2C Address
  uint8_t lcdAddr = scanI2CAddress();
  if (lcdAddr == 0) {
    Serial.println("[WARNING] Auto-detect failed! Using default 0x27.");
    lcdAddr = 0x27;
  } else {
    Serial.printf("[SUCCESS] LCD Detected at Address: 0x%02X\n", lcdAddr);
  }

  // Initialize LCD Display
  lcd = new LiquidCrystal_I2C(lcdAddr, LCD_COLUMNS, LCD_ROWS);
  lcd->init();
  lcd->backlight();
  lcd->clear();

  // Print Project and Team Details
  lcd->setCursor(0, 0);
  lcd->print("SMART ENERGY METER  ");

  lcd->setCursor(0, 1);
  lcd->print("TEAM: TRIO TITANS   ");

  lcd->setCursor(0, 2);
  lcd->print("SANJITH,VIKRAM,VISHN");

  lcd->setCursor(0, 3);
  lcd->print("SYSTEM ACTIVE:   00s");

  Serial.println("[STATUS] Text displayed on LCD successfully.");
}

int liveCounter = 0;

void loop() {
  if (lcd != nullptr) {
    // Update live counter on Row 4 (00 to 59 seconds)
    lcd->setCursor(17, 3);
    lcd->printf("%02ds", liveCounter % 60);

    // Print status in Serial Monitor
    Serial.printf("LCD Status: Running OK | Count: %d sec\n", liveCounter);
  }

  liveCounter++;
  delay(1000);
}