/*
 Integrated Datalogger Code for ESP32-C3 Supermini with E-Paper, RTC, and SD Card
 */

#include <SPI.h> 
#include <Wire.h>

#include "FS.h"
#include <SD.h> 
#include <RTClib.h> 

// Libraries for E-paper display
#include <GxEPD2_BW.h>      // Using Black & White driver
#include <Fonts/FreeMonoBold9pt7b.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SHT31.h>

// ** NEW: Battery Monitor Library (Corrected name) **
#include <Adafruit_MAX1704X.h>

// PIN DEFINITIONS FOR USER'S CUSTOM WIRING ON ESP32-C3 SUPERMINI
// --------------------------------------------------------------------------------
// Shared SPI Bus Pins (Used by both E-Paper and SD Card)
#define PIN_CLK 20  // SCK (E-Paper CLK)
#define PIN_MISO 4  // MISO (SD Card data input)
#define PIN_MOSI 5  // MOSI (E-Paper SDA/MOSI)

// SD Card Chip Select
#define PIN_CS_CARD 3 

// E-Paper display pins (Control)
#define PIN_CS_DISPLAY 10 // E-Paper CS
#define PIN_DC 7        // E-Paper DC
#define PIN_RST 6       // E-Paper RST
#define PIN_BUSY 21     // E-Paper BUSY

// I2C pins (SDA=1, SCL=2)
#define I2C_SDA 1
#define I2C_SCL 2


#define PIN_BOOSTER_EN 8 // GPIO8 is used for the MCP1640 EN pin (controls V_OUT)

// SENSOR & DISPLAY OBJECTS
RTC_DS3231 rtc; 
Adafruit_BME280 bme; 
Adafruit_SHT31 sht31 = Adafruit_SHT31(); 
Adafruit_MAX17048 maxlipo; // MAX17048 Fuel Gauge Object

// E-Paper display driver for 2.9" B&W - GxEPD2_290_BS is the confirmed working driver
GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display(GxEPD2_290_BS(PIN_CS_DISPLAY, PIN_DC, PIN_RST, PIN_BUSY));

// GLOBAL VARIABLES
RTC_DATA_ATTR bool rtcTimeIsSet = false; 
RTC_DATA_ATTR int sensor_data = 0; 
bool sdOK = false; 

// Sensor and Battery variables
float temp, hum, TEMP, HUM; // TEMP/HUM for BME280 and SHT31, respectively
float lipoVoltage = 0.0;
float lipoSOC = 0.0;

// Sleep duration: Set to 1800 seconds (30 minutes)
uint64_t uS_TO_S_FACTOR = 1000000; 
uint64_t TIME_TO_SLEEP = 1800; // 180 seconds (3 minutes)

// FUNCTION PROTOTYPES
void appendFile(fs::FS &fs, const char * path, const char * message);
void writeFile(fs::FS &fs, const char * path, const char * message); 
void displayReadings();
void logSDCard();
void Read_Sensors_and_Power(); // Function name updated to reflect new readings
void print_wakeup_reason();

// -----------------------------------------------------------------------------
// HELPER FUNCTIONS
// -----------------------------------------------------------------------------
void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    // Keep this only for initial setup diagnosis.
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
        default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
    }
}

void Read_Sensors_and_Power() { 
    DateTime now = rtc.now();

    // Read BME280 values
    temp = bme.readTemperature();
    hum = bme.readHumidity();
    
    // Read SHT31 values 
    float sht_t = sht31.readTemperature();
    float sht_h = sht31.readHumidity();
    
    // Read MAX17048 values 
    lipoVoltage = maxlipo.cellVoltage(); // Correct method: cellVoltage()
    lipoSOC = maxlipo.cellPercent();     // **FIXED**: Correct method for SoC is cellPercent()

    // Only update global variables if readings are valid (not NaN)
    if (!isnan(sht_t)) TEMP = sht_t;
    if (!isnan(sht_h)) HUM = sht_h;

    // Only print the run metadata and temperature/humidity values that will be logged
    Serial.println("------------------------------------");
    Serial.printf("RUN: %d | Date: %02d/%02d/%04d Time: %02d:%02d:%02d\n", 
                  sensor_data, now.month(), now.day(), now.year(), now.hour(), now.minute(), now.second());
    Serial.printf("Temp_BME280 = %.2f C | Hum_BME280 = %.2f %%\n", temp, hum); 
    Serial.printf("Temp_SHT31 = %.2f C | Hum_SHT31 = %.2f %%\n", TEMP, HUM); 
    
    // Print Battery Data 
    Serial.printf("LiPo V: %.3f V | LiPo SOC: %.1f %%\n", lipoVoltage, lipoSOC);
    
    Serial.println("------------------------------------");
}

void displayReadings() {
    DateTime now = rtc.now();
    char dateTimeBuffer[17]; 
    sprintf(dateTimeBuffer, "%02i/%02i %02i:%02i:%02i", now.day(), now.month(), now.hour(), now.minute(), now.second());

    // Initialize the display here
    display.init(0, true, 50, false); 
    delay(50); 
    
    display.setRotation(1);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setFullWindow();
    
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(0, 15); display.print("Temp BME: "); display.print(temp); display.println(" C");
        display.setCursor(0, 35); display.print("Temp SHT: "); display.print(TEMP); display.println(" C");
        display.setCursor(0, 55); display.print("Hum BME: "); display.print(hum); display.println(" %");
        display.setCursor(0, 75); display.print("Hum SHT: "); display.print(HUM); display.println(" %");
      
        display.setCursor(0, 95); 
        display.setTextColor(GxEPD_BLACK);
        display.printf("LiPo: %.2fV (%.0f%%)", lipoVoltage, lipoSOC); // Condensed output
        
        // Display Run count and Sleep Info 
        display.setCursor(0, 115); 
        display.setTextColor(GxEPD_BLACK); 
        display.print("Run # "); display.print(sensor_data);
        display.print(" (Next wake: 180 sec)"); 

        display.setCursor(0, 135); 
        display.setTextColor(GxEPD_BLACK);
        display.print(dateTimeBuffer);
    } while(display.nextPage());
    
    // display.update() is handled by display.nextPage() loop
}

void logSDCard() {
    DateTime now = rtc.now();
    char dateBuffer[11];
    char timeBuffer[9];
    sprintf(dateBuffer, "%02i/%02i/%04i", now.month(), now.day(), now.year());
    sprintf(timeBuffer, "%02i:%02i:%02i", now.hour(), now.minute(), now.second());

    String dataMessage = String(sensor_data) + ";" + 
                            String(dateBuffer) + ";" + 
                            String(timeBuffer) + ";" + 
                            String(temp) + ";" + 
                            String(hum) + ";" + 
                            String(TEMP) + ";" + 
                            String(HUM) + ";" +
                            String(lipoVoltage) + ";" + // NEW Battery Data
                            String(lipoSOC) + "\n";     // NEW Battery Data
    
    Serial.print("Saving data to SD: ");
    Serial.println(dataMessage);
    appendFile(SD, "/data.txt", dataMessage.c_str());
}

// Write to the SD card
void writeFile(fs::FS &fs, const char * path, const char * message) {
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        // Success
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

// Append to the SD card
void appendFile(fs::FS &fs, const char * path, const char * message) {
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message)) {
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

// -----------------------------------------------------------------------------
// MAIN FUNCTIONS
// -----------------------------------------------------------------------------

void setup() {
    //  Setup Serial and check wake-up reason
    Serial.begin(115200);
    while(!Serial) delay(10);
    delay(100); 
    
    print_wakeup_reason();

    // I2C Initialization
    Wire.begin(I2C_SDA, I2C_SCL); 
    delay(50); 
    
    // ** POWER BOOSTER CONTROL SETUP ** 
    pinMode(PIN_BOOSTER_EN, OUTPUT);
    digitalWrite(PIN_BOOSTER_EN, HIGH); // Turn ON Booster (LED will briefly light)
    delay(10); // Wait for 5V output to stabilize

    //  Initialize RTC
    Serial.println("Initializing DS3231 RTC...");
    if (!rtc.begin()) {
        Serial.println("FATAL: Couldn't find RTC. Check wiring!");
    } else {
        if (!rtcTimeIsSet || rtc.lostPower()) {
            Serial.println(rtc.lostPower() ? "RTC lost power, resetting the time." : "Setting RTC time for the first time.");
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
            rtcTimeIsSet = true;
        } 
        
        // RTC STABILITY CHECK
        DateTime now = rtc.now();
        int attempts = 0;
        while (now.year() < 2023 && attempts < 10) { 
            Serial.printf("RTC read attempt %d failed (year: %d). Retrying...\n", attempts + 1, now.year());
            delay(50);
            now = rtc.now();
            attempts++;
        }
        if (attempts > 0) {
            Serial.printf("RTC stabilized after %d attempts. Current time: %d/%d/%d %d:%d:%d\n", 
                          attempts, now.month(), now.day(), now.year(), now.hour(), now.minute(), now.second());
        }
    }
    
    //  Initialize MAX17048 (NEW - using MAX1704X library)
    Serial.println("Initializing MAX17048 Fuel Gauge...");
    if (!maxlipo.begin()) {
        Serial.println("FATAL: Couldn't find MAX17048. Check wiring!");
    } else {
        // Initialization successful
    }

    //  Initialize SPI bus pins for SD/E-Paper
    SPI.begin(PIN_CLK, PIN_MISO, PIN_MOSI);
    
    //  Initialize SD Card
    Serial.println("Initializing SD Card...");
    pinMode(PIN_CS_CARD, OUTPUT);
    digitalWrite(PIN_CS_CARD, HIGH);
    delay(50); 
    
    if (!SD.begin(PIN_CS_CARD)) {
        sdOK = false;
        Serial.println("FATAL: Card Mount Failed.");
    } else {
        sdOK = true;
        if (!SD.exists("/data.txt")) {
            Serial.println("Log file does not exist. Creating new log file with header.");
            writeFile(SD, "/data.txt", "Run;Date;Time;temp_BME;hum_BME;temp_SHT;hum_SHT;lipo_V;lipo_SOC\n"); // NEW header
        } 
    }
    
    //  Initialize Sensors
    if (!bme.begin(0x76)) {
        if (!bme.begin(0x77)) {
            Serial.println("FATAL: Could not find BME280 at 0x77. Check wiring!");
        } 
    } 

    if (!sht31.begin(0x44)) { 
        Serial.println("FATAL: Could not find SHT31 sensor. Check wiring!");
    } 
    
    //  Read sensor data
    Read_Sensors_and_Power(); 
    
    //  Log data to SD card.
    if (sdOK) {
        logSDCard(); 
        // CRITICAL FIX: Explicitly end SD interaction and release the SPI bus 
        Serial.println("SD.end() called to release SPI bus.");
        SD.end(); 
    } else {
        Serial.println("Skipping SD card log and SD.end() due to prior mount failure.");
    }

    // --- Prepare for E-Paper ---
    digitalWrite(PIN_CS_CARD, HIGH);
    delay(10); 

    // Use E-paper display last.
    displayReadings();

    // *** E-PAPER REFRESH WAIT ***
    delay(5000); 

    // . Go to deep sleep
    sensor_data++;
    Serial.println("Data processed. Entering deep sleep.");

    // ** POWER BOOSTER CONTROL SHUTDOWN ** digitalWrite(PIN_BOOSTER_EN, LOW); // Turn OFF Booster (LED turns off, enters < 1uA shutdown)
    
    // >>> DEEP SLEEP ENABLED <<< (Set to 1800 seconds)
    display.hibernate();
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start(); 
}

void loop() {
    // This part never executes in deep sleep cycle
    delay(5000); 
    Serial.println("MCU is awake and running loop().");
}
