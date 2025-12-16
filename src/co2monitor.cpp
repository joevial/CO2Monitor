#include <GxEPD2_BW.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
//#include <AsyncTCP.h>
#include <WebServer.h>
//#include <AsyncElegantOTA.h>
  #include <WiFiClient.h>
#include <ArduinoOTA.h>

#include <HTTPClient.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <WiFiClientSecure.h>
#include <SensirionI2CScd4x.h>
#include "kirby.h"
#include <Preferences.h>
#include "RTClib.h"
#include <Open_Sans_Condensed_Bold_36.h> 
#include <Open_Sans_Condensed_Bold_60.h> 
#include <Open_Sans_Condensed_Bold_100.h>
#include <FreeSans9pt7b.h> 
#include <U8g2_for_Adafruit_GFX.h>
#include "esp_sleep.h"
#include <ElegantOTA.h>
#include <WiFiManager.h>      
#include "driver/periph_ctrl.h"
#include<Wire.h>


U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
#define FONT1 0 //default
#define FONT2 &FreeSans9pt7b
#define FONT3 &Open_Sans_Condensed_Bold_36
#define FONT4 &Open_Sans_Condensed_Bold_100
RTC_DS3231 rtc;

Preferences preferences;
uint16_t calibfactor;
int calibdiff;
SensirionI2cScd4x scd4x;

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;
sensors_event_t humidity, temp;

#define I2C_ADDRESS 0x48

int GPIO_reason;

 WebServer server2(8080);


#define ENABLE_GxEPD2_GFX 1
#define TEMP_OFFSET 0.8
#define sleeptimeSecs 300
#define maxArray 501
// Add these variable declarations near the top with other RTC_DATA_ATTR variables
 bool editHour = false;
 bool editMinute = false;
 int adjustHour = 0;
 int adjustMinute = 0;

// Update MENU_MAX constant
#define MENU_MAX 11
#define TIME_TIMEOUT 20000
#define ELEGANTOTA_USE_ASYNC_WEBSERVER 0
#define button0 0
#define button1 1
#define button2 2
#define button3 5
#define button5 3

//second build:
/*#define button0 3
#define button1 0
#define button2 2
#define button3 1
#define button5 5*/

//First build:
/*#define button0 0
#define button1 1
#define button2 2
#define button3 5
#define button5 3*/

void gotosleep();
void cbSyncTime(struct timeval *tv);
void initTime(String timezone);
void drawBusy();
void measureCO2();
void startMenu();
void displayMenu();
void wipeScreen();
void batCheck();
void setupChart();
void setupChart2();
double mapf(float x, float in_min, float in_max, float out_min, float out_max);
void doTempChart();
void doCO2Chart();
void drawMain();
void doHumDisplay();
void doPresDisplay();
void doBatChart();
void recordData();

void drawMain();
void setup();
void loop();

RTC_DATA_ATTR float array1[maxArray];
RTC_DATA_ATTR float array2[maxArray];
RTC_DATA_ATTR float array3[maxArray];
RTC_DATA_ATTR float array4[maxArray];
RTC_DATA_ATTR float windspeed, windgust, fridgetemp, outtemp;
 int  timetosleep = 5;
 bool isSetNtp = false;  

RTC_DATA_ATTR bool lowbatdrawn = false;
bool wifisaved = false;
bool rotateDisplay = false;
bool editrotate = false; 
bool antiFlicker = false;
bool editFlicker = false;
bool connected = false;
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;  //Replace with your GMT offset (secs)
const int daylightOffset_sec = 0;   //Replace with your daylight offset (secs)
  float t, h, pres, barx;
  float v41_value, v42_value, v62_value;
char timeString[10]; // "12:34 PM" is 8 chars + null terminator
 RTC_DATA_ATTR   int firstrun = 100;
 RTC_DATA_ATTR   int page = 0;
 int calibTarget = 430;
float abshum;
 float minVal = 3.9;
 float maxVal = 4.2;
RTC_DATA_ATTR int readingCount = 0; // Counter for the number of readings
int readingTime;
int menusel = 1;
uint16_t co2;
float temp2, hum;
bool editinterval = false;
bool editcalib = false;
bool calibrated = false;
bool facreset = false;
bool wifireset = false;




#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(/*CS=5*/ SS, /*DC=*/ 21, /*RES=*/ 20, /*BUSY=*/ 10)); // GDEH0154D67 200x200, SSD1681 (was 122x250)

#define every(interval) \
    static uint32_t __every__##interval = millis(); \
    if (millis() - __every__##interval >= interval && (__every__##interval = millis()))


const char* blynkserver = "192.168.50.197:9443";
const char* bedroomauth = "8_-CN2rm4ki9P3i_NkPhxIbCiKd5RXhK";  //hubert
//const char* fridgeauth = "VnFlJdW3V0uZQaqslqPJi6WPA9LaG1Pk";

// Virtual Pins
const char* v41_pin = "V41";
const char* v62_pin = "V62";

float vBat;
float pres2;




void gotosleep() {
      scd4x.stopPeriodicMeasurement();
      delay(10);
      scd4x.powerDown();
      delay(10);
      WiFi.disconnect();
      display.hibernate();
      //SPI.end();
      Wire.end();
      pinMode(SS, INPUT );
      pinMode(6, INPUT );
      pinMode(4, INPUT );
      pinMode(8, INPUT );
      pinMode(9, INPUT );
      pinMode(button1, INPUT_PULLUP );
      pinMode(button2, INPUT_PULLUP );
      pinMode(button3, INPUT_PULLUP );
      pinMode(button0, INPUT_PULLUP );
      pinMode(button5, INPUT_PULLUP );


      uint64_t bitmask = BUTTON_PIN_BITMASK(GPIO_NUM_0) | BUTTON_PIN_BITMASK(GPIO_NUM_1) | BUTTON_PIN_BITMASK(GPIO_NUM_2) | BUTTON_PIN_BITMASK(GPIO_NUM_3) |  BUTTON_PIN_BITMASK(GPIO_NUM_5);

      esp_deep_sleep_enable_gpio_wakeup(bitmask, ESP_GPIO_WAKEUP_GPIO_LOW);
      esp_sleep_enable_timer_wakeup((timetosleep * 60) * 1000000ULL);
      delay(1);
      esp_deep_sleep_start();
      //esp_light_sleep_start();
      delay(1000);
}
void gotolongsleep();

void gotolongsleep() {
      scd4x.stopPeriodicMeasurement();
      delay(10);
      scd4x.powerDown();
      delay(10);
      WiFi.disconnect();
      display.hibernate();
      //SPI.end();
      Wire.end();
      pinMode(SS, INPUT );
      pinMode(6, INPUT );
      pinMode(4, INPUT );
      pinMode(8, INPUT );
      pinMode(9, INPUT );
      pinMode(button1, INPUT_PULLUP );
      pinMode(button2, INPUT_PULLUP );
      pinMode(button3, INPUT_PULLUP );
      pinMode(button0, INPUT_PULLUP );
      pinMode(button5, INPUT_PULLUP );



      uint64_t bitmask = BUTTON_PIN_BITMASK(GPIO_NUM_0) | BUTTON_PIN_BITMASK(GPIO_NUM_1) | BUTTON_PIN_BITMASK(GPIO_NUM_2) | BUTTON_PIN_BITMASK(GPIO_NUM_3) |  BUTTON_PIN_BITMASK(GPIO_NUM_5);

      esp_deep_sleep_enable_gpio_wakeup(bitmask, ESP_GPIO_WAKEUP_GPIO_LOW);
      esp_sleep_enable_timer_wakeup((30ULL * 60ULL) * 1000000ULL);
      delay(1);
      esp_deep_sleep_start();
      //esp_light_sleep_start();
      delay(1000);
}

#include "esp_sntp.h"
void cbSyncTime(struct timeval *tv) { // callback function to show when NTP was synchronized
  Serial.println("NTP time synched");
  isSetNtp = true;
}

void initTime(String timezone){
  configTzTime(timezone.c_str(), "moeburn.mooo.com", "pool.ntp.org", "time.nist.gov");

  /*while ((!isSetNtp) && (millis() < TIME_TIMEOUT)) {
        delay(1000);
        display.print("@");
        display.display(true);
        }*/

}



void measureCO2(){


  scd4x.measureSingleShot();
  scd4x.stopPeriodicMeasurement();
  scd4x.reinit();
  scd4x.startPeriodicMeasurement();
    bool isDataReady = false;
  
   
   while (!isDataReady) {scd4x.getDataReadyStatus(isDataReady); delay(100);}
   scd4x.setAmbientPressure(pres2);
    scd4x.readMeasurement(co2, temp2, hum);
  DateTime now = rtc.now();
      

      // Format the time string
      if (now.minute() < 10) {
        snprintf(timeString, sizeof(timeString), "%d:0%d %s", now.hour() % 12 == 0 ? 12 : now.hour() % 12, now.minute(), now.hour() < 12 ? "AM" : "PM");
      } else {
        snprintf(timeString, sizeof(timeString), "%d:%d %s", now.hour() % 12 == 0 ? 12 : now.hour() % 12, now.minute(), now.hour() < 12 ? "AM" : "PM");
      }


}

void startMenu(){
  //scd4x.measureSingleShot();
  scd4x.stopPeriodicMeasurement();
  scd4x.reinit();
  scd4x.startPeriodicMeasurement();
  pinMode(button3, INPUT_PULLUP );



  WiFi.mode(WIFI_STA);
  WiFi.setTxPower (WIFI_POWER_8_5dBm);
  if (wifisaved) {
    WiFiManager wm;
      WiFi.begin(wm.getWiFiSSID(), wm.getWiFiPass());
  }
  //WiFi.mode(WIFI_STA);

  WiFi.setTxPower (WIFI_POWER_8_5dBm);

  
  pinMode(button0, INPUT);
  vBat = analogReadMilliVolts(0) / 500.0;
  pinMode(button0, INPUT_PULLUP);
  bmp.takeForcedMeasurement();
  aht.getEvent(&humidity, &temp);
  t = temp.temperature;
  h = humidity.relative_humidity;
  pres = bmp.readPressure() / 100.0;
 // scd4x.startPeriodicMeasurement();
  /*abshum = (6.112 * pow(2.71828, ((17.67 * temp.temperature)/(temp.temperature + 243.5))) * humidity.relative_humidity * 2.1674)/(273.15 + temp.temperature);
  bool isDataReady = false;
  scd4x.getDataReadyFlag(isDataReady);
  if (isDataReady) {
  scd4x.readMeasurement(co2, temp2, hum);}*/
  displayMenu();
}

void displayMenu(){
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextSize(1);
  display.fillScreen(GxEPD_WHITE);
  display.setCursor(0, 0);
  display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
  
  // Read current time if not in edit mode
  if (!editHour && !editMinute) {
    DateTime now = rtc.now();
    adjustHour = now.hour();
    adjustMinute = now.minute();
  }
  
  // Measure a sample text to get line height
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  u8g2Fonts.setFont(u8g2_font_6x13_tf);
  int fontAscent = u8g2Fonts.getFontAscent();
  int lineHeight = fontAscent + 2; // Add small padding between lines
  int cursorY = fontAscent; // Start at font ascent so text is visible

  if (WiFi.status() == WL_CONNECTED) {
    u8g2Fonts.setCursor(0, cursorY);
    u8g2Fonts.print("Connected! to: ");
    u8g2Fonts.println(WiFi.localIP());
    cursorY += lineHeight;
    
    u8g2Fonts.setCursor(0, cursorY);
    u8g2Fonts.print("RSSI: ");
    u8g2Fonts.println(WiFi.RSSI());
    cursorY += lineHeight;
    
    cursorY += lineHeight; // Empty line
  } else {
    cursorY += lineHeight * 3; // Skip 3 lines
  }
  
  // Menu items with dynamic positioning
  const char* menuLabels[] = {
    "Connect WiFi",
    "Change Interval",
    "Set Calibration Value",
    "Calibrate Now",
    "Factory Reset SCD41",
    "Forget Wifi",
    "Rotate?",
    "Anti-flicker?",
    "Adjust Hour",
    "Adjust Minute",
    "Exit"
  };
  
  for (int i = 1; i <= 11; i++) {
    if (menusel == i) {
      //display.setTextColor(GxEPD_WHITE, GxEPD_BLACK);
      u8g2Fonts.setForegroundColor(GxEPD_WHITE);
      u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
      // Draw black background rectangle for selected menu item
      uint16_t textWidth = u8g2Fonts.getUTF8Width(menuLabels[i - 1]);
      display.fillRect(0, cursorY - fontAscent, textWidth, fontAscent + 2, GxEPD_BLACK);
    } else {
      u8g2Fonts.setForegroundColor(GxEPD_BLACK);
      u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    }
    u8g2Fonts.setCursor(0, cursorY);
    u8g2Fonts.println(menuLabels[i - 1]);
    cursorY += lineHeight;
  }
  
  // Right-side values - reset cursor to start of menu items
  cursorY = fontAscent + lineHeight * 3;
  
  // Interval value (menu item 2)
  cursorY += lineHeight;
  if (editinterval) {
      u8g2Fonts.setForegroundColor(GxEPD_WHITE);
      u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
    } else {
      u8g2Fonts.setForegroundColor(GxEPD_BLACK);
      u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    }
  char intervalStr[16];
  snprintf(intervalStr, sizeof(intervalStr), "%d mins", timetosleep);
  
  tbw = u8g2Fonts.getUTF8Width(intervalStr);
  if (editinterval) {
    display.fillRect(display.width() - tbw - 2, cursorY - fontAscent, tbw + 2, fontAscent + 2, GxEPD_BLACK);
  }
  u8g2Fonts.setCursor(display.width() - tbw - 2, cursorY);
  u8g2Fonts.print(intervalStr);

  // Calibration target value (menu item 3)
  cursorY += lineHeight;
  if (editcalib) {
      u8g2Fonts.setForegroundColor(GxEPD_WHITE);
      u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
    } else {
      u8g2Fonts.setForegroundColor(GxEPD_BLACK);
      u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    }
  char calibStr[16];
  snprintf(calibStr, sizeof(calibStr), "%d ppm", calibTarget);
  tbw = u8g2Fonts.getUTF8Width(calibStr);
  if (editcalib) {
    display.fillRect(display.width() - tbw - 2, cursorY - fontAscent, tbw + 2, fontAscent + 2, GxEPD_BLACK);
  }
  u8g2Fonts.setCursor(display.width() - tbw - 2, cursorY);
  u8g2Fonts.print(calibStr);

  // Calibration status (menu item 4)
  cursorY += lineHeight;
  if (calibrated) {
    char calibDoneStr[32];
    calibdiff = 32768 - calibfactor;
    snprintf(calibDoneStr, sizeof(calibDoneStr), "Adj by: %u", calibdiff);
    tbw = u8g2Fonts.getUTF8Width(calibDoneStr);
    u8g2Fonts.setCursor(display.width() - tbw - 2, cursorY);
    u8g2Fonts.print(calibDoneStr);
  }
  
  // Factory reset status (menu item 5)
  cursorY += lineHeight;
  if (facreset) {
    const char *r = "Reset!";
    tbw = u8g2Fonts.getUTF8Width(r);
    u8g2Fonts.setCursor(display.width() - tbw - 2, cursorY);
    u8g2Fonts.print(r);
  }
  
  // WiFi reset status (menu item 6)
  cursorY += lineHeight;
  if (wifireset) {
    const char *r2 = "Reset!";
    tbw = u8g2Fonts.getUTF8Width(r2);
    u8g2Fonts.setCursor(display.width() - tbw - 2, cursorY);
    u8g2Fonts.print(r2);
  }
  
  // Rotate setting (menu item 7)
  cursorY += lineHeight;
  if (editrotate) {
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);
    u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
  } else {
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  }
  const char* rotateStr = rotateDisplay ? "Yes" : "No";
  tbw = u8g2Fonts.getUTF8Width(rotateStr);
  if (editrotate) {
    display.fillRect(display.width() - tbw - 2, cursorY - fontAscent, tbw + 2, fontAscent + 2, GxEPD_BLACK);
  }
  u8g2Fonts.setCursor(display.width() - tbw - 2, cursorY);
  u8g2Fonts.print(rotateStr);
  
  // Anti-flicker setting (menu item 8)
  cursorY += lineHeight;
  if (editFlicker) {
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);
    u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
  } else {
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  }
  const char* flickerStr = antiFlicker ? "Yes" : "No";
  tbw = u8g2Fonts.getUTF8Width(flickerStr);
  if (editFlicker) {
    display.fillRect(display.width() - tbw - 2, cursorY - fontAscent, tbw + 2, fontAscent + 2, GxEPD_BLACK);
  }
  u8g2Fonts.setCursor(display.width() - tbw - 2, cursorY);
  u8g2Fonts.print(flickerStr);
  
  // Hour setting (menu item 9)
  cursorY += lineHeight;
  if (editHour) {
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);
    u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
  } else {
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  }
  char hourStr[8];
  snprintf(hourStr, sizeof(hourStr), "%02d", adjustHour);
  tbw = u8g2Fonts.getUTF8Width(hourStr);
  if (editHour) {
    display.fillRect(display.width() - tbw - 2, cursorY - fontAscent, tbw + 2, fontAscent + 2, GxEPD_BLACK);
  }
  u8g2Fonts.setCursor(display.width() - tbw - 2, cursorY);
  u8g2Fonts.print(hourStr);
  
  // Minute setting (menu item 10)
  cursorY += lineHeight;
  if (editMinute) {
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);
    u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
  } else {
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  }
  char minuteStr[8];
  snprintf(minuteStr, sizeof(minuteStr), "%02d", adjustMinute);
  tbw = u8g2Fonts.getUTF8Width(minuteStr);
  if (editMinute) {
    display.fillRect(display.width() - tbw - 2, cursorY - fontAscent, tbw + 2, fontAscent + 2, GxEPD_BLACK);
  }
  u8g2Fonts.setCursor(display.width() - tbw - 2, cursorY);
  u8g2Fonts.print(minuteStr);
  
  // Bottom status lines (use u8g2Fonts for consistent metrics)
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  u8g2Fonts.setCursor(0, display.height() - lineHeight * 3 + fontAscent);
  char buf[64];
  snprintf(buf, sizeof(buf), "CO2: %u ppm | Temp: %.1f%cC", co2, t, 247);
  u8g2Fonts.print(buf);
  u8g2Fonts.setCursor(0, display.height() - lineHeight * 2 + fontAscent);
  snprintf(buf, sizeof(buf), "Hum: %.0f%% | Pres: %.1f mbar", h, pres);
  u8g2Fonts.print(buf);
  u8g2Fonts.setCursor(0, display.height() - lineHeight + fontAscent);
  int batPct = mapf(vBat, 3.3, 4.05, 0, 100);
  snprintf(buf, sizeof(buf), "vBat: %.3fv %d%%", vBat, batPct);
  u8g2Fonts.print(buf);


  display.display(true);
}



void wipeScreen() {
  if (!antiFlicker) {
    display.setPartialWindow(0, 0, display.width(), display.height());

    display.firstPage();
    do {
      display.fillRect(0, 0, display.width(), display.height(), GxEPD_BLACK);
    } while (display.nextPage());
    delay(10);
    display.firstPage();
    do {
      display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);
    } while (display.nextPage());
    display.firstPage();
    display.fillScreen(GxEPD_WHITE);
  }
  else {
    display.fillScreen(GxEPD_WHITE);
    }
}

void batCheck() {
  if (vBat < 3.4){
    if (!lowbatdrawn) {
    wipeScreen();
    display.fillRect(0, 0, display.width(), display.height(), GxEPD_BLACK);
    display.setTextColor(GxEPD_WHITE, GxEPD_BLACK);
    display.setCursor(10,60);
    display.setFont(FONT3);
    display.setTextSize(1);
    display.print("LOW BATTERY");
    display.display(true);
    lowbatdrawn = true;
    while (millis() < 5000) {
      delay(100);
    }
    }
    gotosleep();
 }
}



void setupChart() {
  display.setTextSize(1);
  display.setFont();
    display.setCursor(0, 0);
    display.print("<");
    display.print(maxVal, 3);
    
    // Adjusted for bottom of 200x200 display
    display.setCursor(0, 193);
    display.print("<");
    display.print(minVal, 3);

    // Adjusted for horizontal placement of the additional text
    display.setCursor(100, 193);
    display.print("<#");
    display.print(readingCount - 1, 0);
    display.print("*");
    display.print(timetosleep, 0);
    display.print("mins>");
    int barx = mapf(vBat, 3.3, 4.05, 0, 19); // Map battery value to progress bar width
    if (barx > 19) { barx = 19; }
    if (barx < 0) { barx = 0; }
    // Adjusted rectangle and progress bar to fit within bounds
    display.drawRect(179, 192, 19, 7, GxEPD_BLACK); // Rectangle moved to fit fully within 200x200
    display.fillRect(179, 192, barx, 7, GxEPD_BLACK); // Progress bar inside the rectangle

    // Adjusted marker lines to stay within bounds
    display.drawLine(198, 193, 198, 198, GxEPD_BLACK); 
    display.drawLine(199, 193, 199, 198, GxEPD_BLACK);

    // Set the cursor for additional chart decorations
    display.setCursor(80, 0); 
}



double mapf(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void doTempChart() {
    // Recalculate min and max values
    minVal = array1[maxArray - readingCount];
    maxVal = array1[maxArray - readingCount];

    for (int i = maxArray - readingCount + 1; i < maxArray; i++) {
        if (array1[i] != 0) {  // Only consider non-zero values
            if (array1[i] < minVal) {
                minVal = array1[i];
            }
            if (array1[i] > maxVal) {
                maxVal = array1[i];
            }
        }
    }

    // Calculate scaling factors for full 200x200 display
    float yScale = 199.0 / (maxVal - minVal); // Adjusted for full vertical range
    float xStep = 200.0 / (readingCount - 1); // Adjusted for full horizontal range

    wipeScreen();

    do {
        display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);

        for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
            int x0 = (i - (maxArray - readingCount)) * xStep;
            int y0 = 199 - ((array1[i] - minVal) * yScale); // Full vertical range
            int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
            int y1 = 199 - ((array1[i + 1] - minVal) * yScale); // Full vertical range

            // Only draw a line for valid (non-zero) values
            if (array1[i] != 0) {
                display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
            }
        }

        // Call setupChart to draw chart decorations
        setupChart();

        // Display temperature information at the bottom
        //display.setCursor(10, 190); // Positioned near the bottom for readability
        display.print("[");
        display.print("Temp: ");
        display.print(array1[maxArray - 1], 3);
        display.print("c");
        display.print("]");
    } while (display.nextPage());

    display.setFullWindow();
    //gotosleep();
}

void doCO2Chart() {
    // Recalculate min and max values
    minVal = array2[maxArray - readingCount];
    maxVal = array2[maxArray - readingCount];

    for (int i = maxArray - readingCount + 1; i < maxArray; i++) {
        if (array2[i] != 0) {  // Only consider non-zero values
            if (array2[i] < minVal) {
                minVal = array2[i];
            }
            if (array2[i] > maxVal) {
                maxVal = array2[i];
            }
        }
    }

    // Calculate scaling factors for full 200x200 display
    float yScale = 199.0 / (maxVal - minVal); // Adjusted for full vertical range
    float xStep = 200.0 / (readingCount - 1); // Adjusted for full horizontal range

    wipeScreen();

    do {
        display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);

        for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
            int x0 = (i - (maxArray - readingCount)) * xStep;
            int y0 = 199 - ((array2[i] - minVal) * yScale); // Full vertical range
            int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
            int y1 = 199 - ((array2[i + 1] - minVal) * yScale); // Full vertical range

            // Only draw a line for valid (non-zero) values
            if (array2[i] != 0) {
                display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
            }
        }
        setupChart();
        display.print("[");
        display.print("CO2: ");
        display.print(array2[(maxArray - 1)], 0);
        display.print("ppm");
        display.print("]");
    } while (display.nextPage());

    display.setFullWindow();

}




void doPresDisplay() {
    // Recalculate min and max values
    minVal = array3[maxArray - readingCount];
    maxVal = array3[maxArray - readingCount];

    for (int i = maxArray - readingCount + 1; i < maxArray; i++) {
        if ((array3[i] < minVal) && (array3[i] > 0)) {
            minVal = array3[i];
        }
        if (array3[i] > maxVal) {
            maxVal = array3[i];
        }
    }

    // Adjust scaling factors
    float yScale = 199.0 / (maxVal - minVal); // Adjusted for full vertical range
    float xStep = 200.0 / (readingCount - 1); // Adjusted for full horizontal range

    wipeScreen();

    do {
        display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);

        for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
            int x0 = (i - (maxArray - readingCount)) * xStep;
            int y0 = 199 - ((array3[i] - minVal) * yScale); // Adjusted for vertical range
            int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
            int y1 = 199 - ((array3[i + 1] - minVal) * yScale); // Adjusted for vertical range
            if (array3[i] > 0) {
                display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
            }
        }

        setupChart();
        display.print("[");
        display.print("Pres: ");
        display.print(array3[(maxArray - 1)], 2);
        display.print("mb");
        display.print("]");
    } while (display.nextPage());

    display.setFullWindow();
    //gotosleep();
}

void doHumDisplay() {
    // Recalculate min and max values
    minVal = array4[maxArray - readingCount];
    maxVal = array4[maxArray - readingCount];

    for (int i = maxArray - readingCount + 1; i < maxArray; i++) {
        if ((array4[i] < minVal) && (array4[i] > 0)) {
            minVal = array4[i];
        }
        if (array4[i] > maxVal) {
            maxVal = array4[i];
        }
    }

    // Adjust scaling factors
    float yScale = 199.0 / (maxVal - minVal); // Adjusted for full vertical range
    float xStep = 200.0 / (readingCount - 1); // Adjusted for full horizontal range

    wipeScreen();

    do {
        display.fillRect(0, 0, display.width(), display.height(), GxEPD_WHITE);

        for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
            int x0 = (i - (maxArray - readingCount)) * xStep;
            int y0 = 199 - ((array4[i] - minVal) * yScale); // Adjusted for vertical range
            int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
            int y1 = 199 - ((array4[i + 1] - minVal) * yScale); // Adjusted for vertical range
            if (array4[i] > 0) {
                display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
            }
        }

        setupChart();
        display.print("[");
        display.print("Hum: ");
        display.print(array4[(maxArray - 1)], 2);
        display.print("g");
        display.print("]");
    } while (display.nextPage());

    display.setFullWindow();
    gotosleep();
}

void doBatChart() {
    // Recalculate min and max values
     minVal = array4[maxArray - readingCount];
     maxVal = array4[maxArray - readingCount];

    for (int i = maxArray - readingCount + 1; i < maxArray; i++) {
        if ((array4[i] < minVal) && (array4[i] > 0)) {
            minVal = array4[i];
        }
        if (array4[i] > maxVal) {
            maxVal = array4[i];
        }
    }

    // Calculate scaling factors
    float yScale = 199.0 / (maxVal - minVal);
    float xStep = 200.0 / (readingCount - 1);

    wipeScreen();
    
    
    do {
        display.fillRect(0,0,display.width(),display.height(),GxEPD_WHITE);

        
        for (int i = maxArray - readingCount; i < (maxArray - 1); i++) {
            int x0 = (i - (maxArray - readingCount)) * xStep;
            int y0 = yScale - ((array4[i] - minVal) * yScale);
            int x1 = (i + 1 - (maxArray - readingCount)) * xStep;
            int y1 = yScale - ((array4[i + 1] - minVal) * yScale);
            if (array4[i] > 0) {
                display.drawLine(x0, y0, x1, y1, GxEPD_BLACK);
            }
        }
        display.setCursor(0, 0);
        display.print("<");
        display.print(maxVal, 3);
        display.setCursor(0, 114);
        display.print("<");
        display.print(minVal, 3);
        display.setCursor(120, 114);
        display.print("<#");
        display.print(readingCount - 1, 0);
        display.print("*");
        display.print(timetosleep * 60, 0);
        display.print("mins>");
        display.setCursor(175, 114);

  vBat = analogReadMilliVolts(0) / 500.0;

        int batPct = mapf(vBat, 3.3, 4.05, 0, 100);
        display.setCursor(125, 0);
        display.print("[vBat: ");
        display.print(vBat, 3);
        display.print("v/");
        display.print(batPct, 1);
        display.print("%]");
    } while (display.nextPage());

    display.setFullWindow();

}


void recordData(){
        for (int i = 0; i < (maxArray - 1); i++) {
            array3[i] = array3[i + 1];
        }
        array3[(maxArray - 1)] = pres;

        if (readingCount < maxArray) {
            readingCount++;
        }

        for (int i = 0; i < (maxArray - 1); i++) {
            array1[i] = array1[i + 1];
        }
        array1[(maxArray - 1)] = t;

        for (int i = 0; i < (maxArray - 1); i++) {
            array2[i] = array2[i + 1];
        }
        array2[(maxArray - 1)] = co2;

        for (int i = 0; i < (maxArray - 1); i++) {
            array4[i] = array4[i + 1];
        }
        array4[(maxArray - 1)] = hum;
}

void drawMain(){
    display.setPartialWindow(0, 0, display.width(), display.height());
    //u8g2Fonts.setFont(u8g2_font_7x14_tf);
    
    // Current values to draw
    float co2todraw = array2[(maxArray - 1)];
    float temptodraw = array1[(maxArray - 1)];
    
    // Clock parameters
    int centerX = 100;
    int centerY = 100;
    int radius = 98;
    
    // Get current time
    DateTime now = rtc.now();
    int hours = now.hour() % 12;
    int minutes = now.minute();
    //int seconds = now.second();
    
    // Begin partial-page drawing loop
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // Draw clock circle
        display.drawCircle(centerX, centerY, radius, GxEPD_BLACK);
        
        // Draw hour ticks
        for (int i = 0; i < 12; i++) {
            float angle = (i * 30.0) * M_PI / 180.0; // 30 degrees per hour
            int x1 = centerX + (radius - 8) * sin(angle);
            int y1 = centerY - (radius - 8) * cos(angle);
            int x2 = centerX + radius * sin(angle);
            int y2 = centerY - radius * cos(angle);
            display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
        }

        
// Draw hour hand as arrow with small filled circle
float hourAngle = ((hours + minutes / 60.0) * 30.0) * M_PI / 180.0;
int hourHandX = centerX + (radius - 45) * sin(hourAngle);  // Moved back 30 pixels
int hourHandY = centerY - (radius - 45) * cos(hourAngle);
int hourTipX = centerX + (radius - 25) * sin(hourAngle);  // Tip 10 pixels from edge
int hourTipY = centerY - (radius - 25) * cos(hourAngle);

// Calculate perpendicular to the line from circle to tip
float dx = hourTipX - hourHandX;
float dy = hourTipY - hourHandY;
float len = sqrt(dx*dx + dy*dy);
float perpX = -dy / len;  // Perpendicular vector
float perpY = dx / len;

int circleRadius = 3;
int tangent1X = hourHandX + circleRadius * perpX;
int tangent1Y = hourHandY + circleRadius * perpY;
int tangent2X = hourHandX - circleRadius * perpX;
int tangent2Y = hourHandY - circleRadius * perpY;

display.drawLine(100, 100, hourTipX, hourTipY, GxEPD_BLACK);
display.fillTriangle(tangent1X, tangent1Y, tangent2X, tangent2Y, hourTipX, hourTipY, GxEPD_BLACK);
display.fillCircle(hourHandX, hourHandY, 3, GxEPD_BLACK);

// Draw minute hand as arrow with larger hollow circle
float minuteAngle = (minutes * 6.0) * M_PI / 180.0;
int minuteHandX = centerX + (radius - 20) * sin(minuteAngle);  // Moved back 20 pixels
int minuteHandY = centerY - (radius - 20) * cos(minuteAngle);
int minuteTipX = centerX + radius * sin(minuteAngle);  // Arrow tip at edge
int minuteTipY = centerY - radius * cos(minuteAngle);

// Calculate perpendicular to the line from circle to tip
dx = minuteTipX - minuteHandX;
dy = minuteTipY - minuteHandY;
len = sqrt(dx*dx + dy*dy);
perpX = -dy / len;
perpY = dx / len;

circleRadius = 7;
tangent1X = minuteHandX + circleRadius * perpX;
tangent1Y = minuteHandY + circleRadius * perpY;
tangent2X = minuteHandX - circleRadius * perpX;
tangent2Y = minuteHandY - circleRadius * perpY;

display.drawLine(100, 100, minuteTipX, minuteTipY, GxEPD_BLACK);
display.fillTriangle(tangent1X, tangent1Y, tangent2X, tangent2Y, minuteTipX, minuteTipY, GxEPD_BLACK);
display.drawCircle(minuteHandX, minuteHandY, 6, GxEPD_BLACK);
display.drawCircle(minuteHandX, minuteHandY, 7, GxEPD_BLACK);
display.fillCircle(minuteHandX, minuteHandY, 5, GxEPD_WHITE);

        display.fillCircle(100, 100, 7, GxEPD_BLACK);
        // Draw CO2 label (small text above the value)
        display.setFont(FONT3);
        const char *lbl = "CO2";
        int16_t bx, by; uint16_t bw, bh;
        display.getTextBounds(lbl, 0, 0, &bx, &by, &bw, &bh);
        int16_t lbl_x = centerX - bw / 2 - bx;
        int16_t lbl_y = centerY - 55;
        display.setCursor(lbl_x, lbl_y);
        display.print(lbl);
        
        // Draw CO2 value (big text in center)
        display.setFont(FONT4);
        char co2buf[16];
        snprintf(co2buf, sizeof(co2buf), "%u", (unsigned)co2todraw);
        display.getTextBounds(co2buf, 0, 0, &bx, &by, &bw, &bh);
        int16_t val_x = centerX - bw / 2 - bx;
        int16_t val_y = centerY + 40;
        display.setCursor(val_x, val_y);
        display.print(co2buf);
        
        // Draw ppm label (small text below the value)
        display.setFont(FONT2);
        const char *units = "ppm";
        display.getTextBounds(units, 0, 0, &bx, &by, &bw, &bh);
        int16_t units_x = centerX - bw / 2 - bx;
        int16_t units_y = centerY + 65;
        display.setCursor(units_x, units_y);
        display.print(units);
        
        // Draw corner information in small font
        //display.setFont(FONT1);
        u8g2Fonts.setFont(u8g2_font_7x14_tf);

        
        // Top left: Temperature
        u8g2Fonts.setCursor(0, 11);
        //display.print("T:");
        u8g2Fonts.print(temptodraw, 1);
        u8g2Fonts.print("°C");
        
        // Top right: Humidity
        u8g2Fonts.setCursor(155, 11);
        //display.print("H:");
        u8g2Fonts.print(h, 0);
        u8g2Fonts.print("% RH");
        
        // Bottom left: Pressure
        u8g2Fonts.setCursor(0, 198);
        //display.print("P:");
        u8g2Fonts.print(pres, 1);
        u8g2Fonts.print("mbar");
        
        // Bottom right: Battery indicator (existing behavior)
        barx = mapf(vBat, 3.3, 4.05, 0, 19);
        if (barx > 19) { barx = 19; }
        display.drawRect(179, 192, 19, 7, GxEPD_BLACK);
        display.fillRect(179, 192, barx, 7, GxEPD_BLACK);
        display.drawLine(198, 193, 198, 198, GxEPD_BLACK);
        display.drawLine(199, 193, 199, 198, GxEPD_BLACK);
        
    } while (display.nextPage());
    
    display.setFullWindow();
}

void setup(){
  vBat = analogReadMilliVolts(0) / 500.0;

  Wire.begin();  
  scd4x.begin(Wire, SCD41_I2C_ADDR_62);
  scd4x.wakeUp();

  preferences.begin("my-app", false);
    timetosleep = preferences.getUInt("timetosleep", 5);
    wifisaved = preferences.getBool("wifisaved", false);
    rotateDisplay = preferences.getBool("rotate", false);
    antiFlicker = preferences.getBool("antiflicker", false);
  preferences.end();
  display.init(115200, false, 10, false); // void init(uint32_t serial_diag_bitrate, bool initial, uint16_t reset_duration = 10, bool pulldown_rst_mode = false)
  
  u8g2Fonts.begin(display); 
  uint16_t bg = GxEPD_WHITE;
  uint16_t fg = GxEPD_BLACK;
  u8g2Fonts.setFontMode(1);                 // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);            // left to right (this is default)
  u8g2Fonts.setForegroundColor(fg);         // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(bg);         // apply Adafruit GFX color
  display.setRotation(rotateDisplay ? 3 : 1);
  display.setTextSize(1);
  //display.clearScreen();
  pinMode(button0, INPUT_PULLUP );
  pinMode(button1, INPUT_PULLUP );
  pinMode(button2, INPUT_PULLUP );
  //pinMode(button3, INPUT_PULLUP );
  //pinMode(4, INPUT_PULLUP );
  pinMode(button5, INPUT_PULLUP );
  barx = mapf (vBat, 3.3, 4.05, 0, 19);
  if (barx > 19) {barx = 19;}
  GPIO_reason = log(esp_sleep_get_gpio_wakeup_status())/log(2);
  if (GPIO_reason < 0) {
    //batCheck();
  }
  lowbatdrawn = false;
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }


  aht.begin();
  bmp.begin();
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500);
  bmp.takeForcedMeasurement();
  
  aht.getEvent(&humidity, &temp);
   t = temp.temperature;
   h = humidity.relative_humidity;

    pres2 = bmp.readPressure();
    pres = pres2 / 100.0;
   //scd4x.setAmbientPressureRaw((pres2));
   
    abshum = (6.112 * pow(2.71828, ((17.67 * temp.temperature)/(temp.temperature + 243.5))) * humidity.relative_humidity * 2.1674)/(273.15 + temp.temperature);

  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);



   



            
  //if (firstrun >= 1000) {display.clearScreen();
  //firstrun = 0;}
  //firstrun++;

  display.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
  



  if (GPIO_reason < 0) {
    
    measureCO2();
    recordData();
    
      switch (page){
        case 0: 
          wipeScreen();
          drawMain();
          break;
        case 1: //down
          doTempChart();  
          break;
        case 2: //towards
          doHumDisplay(); 
          break;
        case 3:  //away
          doCO2Chart(); 
          break;
        case 4: //up
          doPresDisplay(); 
          break;
      }    
      gotosleep();  
    }
  switch (GPIO_reason) {
    case button1: 
      page = 1;
      doTempChart();
      break;
    case button2: 
      page = 2;
      doHumDisplay();
      break;
    case button3: 
      page = 3;
      doCO2Chart();
      break;
    case button0: 
      page = 4;
      doPresDisplay();
      break;
    case button5: 
    delay(50);
      while (!digitalRead(button5))
        {
          delay(10);
          if (millis() > 2000) {
            display.clearScreen();
            startMenu();
            while (!digitalRead(button5)){delay(100);}

          return;}
        }
      wipeScreen();
      display.setPartialWindow(0, 0, display.width(), display.height());

      display.firstPage();
      display.setCursor(0, 20);
      do {
        display.println("Measuring CO2,");
        display.println("please wait 10 seconds...");
      } while (display.nextPage());
      measureCO2();
      recordData();
      page = 0;
drawMain();
      break;
  }
  gotosleep();  
  

}

void loop()
{
  if (WiFi.status() == WL_CONNECTED && connected) {
    ArduinoOTA.handle();
    server2.handleClient();
    ElegantOTA.loop();
  }
  if (WiFi.status() == WL_CONNECTED && !connected) {
    connected = true;
    ArduinoOTA.setHostname("co2monitor");
    ArduinoOTA.begin();

    server2.on("/", []() {
      server2.send(200, "text/plain", "Hi! This is ElegantOTA Demo.");
    });

    ElegantOTA.begin(&server2);    // Start ElegantOTA
  
    server2.begin();
    displayMenu();
  }



  if (!digitalRead(button5)) {
      switch (menusel) {
        case 1:
          { 
                display.setPartialWindow(0, 0, display.width(), display.height());
                wipeScreen();
                display.setCursor(0, 0);                                  //
                display.println("Use your mobile phone to connect to ");
                display.println("[CO2 Setup] then browse to");
                display.println("http://co2monitor.local to connect to WiFi");
                display.println("OR http://192.168.4.1");
                display.display(true);
                WiFi.mode(WIFI_STA);
                WiFi.setTxPower (WIFI_POWER_8_5dBm);
                WiFiManager wm;
                wm.setConfigPortalTimeout(300);
                  bool res;
                  res = wm.autoConnect("CO2 Setup");
                  if (res) {
                      wifisaved = true;
                      preferences.begin("my-app", false);
                      preferences.putBool("wifisaved", wifisaved);
                      preferences.end();
                    }
                  
                displayMenu();
                break; 
          }
        case 2: 
            editinterval = !editinterval;
            preferences.begin("my-app", false);
            preferences.putUInt("timetosleep", timetosleep);
            preferences.end();
            displayMenu();
            break; 
        case 3: 
            editcalib = !editcalib;
            displayMenu();
            break;
        case 4: 
            scd4x.stopPeriodicMeasurement();
            
            delay(500);
            scd4x.performForcedRecalibration(calibTarget, calibfactor);
            scd4x.startPeriodicMeasurement();
            calibrated = true;
            displayMenu();
            break; 
        case 5:    
            scd4x.stopPeriodicMeasurement();
            scd4x.performFactoryReset();
            scd4x.startPeriodicMeasurement();
            facreset = true;
            break;  
        case 6:    
            {WiFiManager wm;
            wm.resetSettings();
            wifireset = true;
            wifisaved = false;
            preferences.begin("my-app", false);
            preferences.putBool("wifisaved", wifisaved);
            preferences.end();
            
            break;  
            }
        case 7:
            editrotate = !editrotate;
            preferences.begin("my-app", false);
            preferences.putBool("rotate", rotateDisplay);
            preferences.end();
            displayMenu();
            break;
        case 8:
            editFlicker = !editFlicker;
            preferences.begin("my-app", false);
            preferences.putBool("antiflicker", antiFlicker);
            preferences.end();
            displayMenu();
            break;
        case 9:
            if (!editHour) {
                // First press - enter edit mode
                DateTime now = rtc.now();
                adjustHour = now.hour();
                editHour = true;
            } else {
                // Second press - save and exit
                DateTime now = rtc.now();
                rtc.adjust(DateTime(now.year(), now.month(), now.day(), adjustHour, now.minute(), 0));
                editHour = false;
            }
            displayMenu();
            break;
        case 10:
            if (!editMinute) {
                // First press - enter edit mode
                DateTime now = rtc.now();
                adjustMinute = now.minute();
                editMinute = true;
            } else {
                // Second press - save and exit
                DateTime now = rtc.now();
                rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), adjustMinute, 0));
                editMinute = false;
            }
            displayMenu();
            break;
        case 11: 
            wipeScreen();
            display.setPartialWindow(0, 0, display.width(), display.height());

            display.firstPage();
            display.setCursor(0, 20);
            do {
              display.println("Measuring CO2,");
              display.println("please wait 10 seconds...");
            } while (display.nextPage());
            ESP.restart();
            break; 
      }
    }
  every (100){

    if (!digitalRead(button0)) {
      calibrated = false;
      facreset = false;
      wifireset = false;
      if (editrotate) {rotateDisplay = !rotateDisplay;} else
      if (editFlicker) {antiFlicker = !antiFlicker;} else
      if (editinterval) {timetosleep--; if (timetosleep < 1) timetosleep = 1;} else
      if (editcalib) {calibTarget--; if (calibTarget < 300) calibTarget = 300;} else
      if (editHour) {adjustHour--; if (adjustHour < 0) adjustHour = 23;} else
      if (editMinute) {adjustMinute--; if (adjustMinute < 0) adjustMinute = 59;} 
      else {
          menusel += rotateDisplay ? -1 : 1;
      }
      if (menusel > MENU_MAX) {menusel = 1;} 
      if (menusel < 1) {menusel = MENU_MAX;}
      if (timetosleep < 1) {timetosleep = 1;}
      if (timetosleep > 999) {timetosleep = 999;}
      displayMenu();
    }
    if (!digitalRead(button3)) {
      calibrated = false;
      facreset = false;
      wifireset = false;
      if (editrotate) {rotateDisplay = !rotateDisplay;} else
      if (editFlicker) {antiFlicker = !antiFlicker;} else
      if (editinterval) {timetosleep++; if (timetosleep > 999) timetosleep = 999;} else
      if (editcalib) {calibTarget++; if (calibTarget > 2000) calibTarget = 2000;} else
      if (editHour) {adjustHour++; if (adjustHour > 23) adjustHour = 0;} else
      if (editMinute) {adjustMinute++; if (adjustMinute > 59) adjustMinute = 0;} 
      else {
          menusel += rotateDisplay ? 1 : -1;
      }
      if (menusel > MENU_MAX) {menusel = 1;} 
      if (menusel < 1) {menusel = MENU_MAX;}
      if (timetosleep < 1) {timetosleep = 1;}
      if (timetosleep > 999) {timetosleep = 999;}
      displayMenu();
    }
    if (!digitalRead(button2)) {
      if (menusel == 7) {
        display.fillScreen(GxEPD_WHITE);
        display.drawInvertedBitmap(0, 0, kirby, 250, 122, GxEPD_BLACK);
        display.display(true);
        delay(1000);
        gotosleep();
      }
    }
  }

  every (10000) {
    pinMode(button0, INPUT);
    vBat = analogReadMilliVolts(0) / 500.0;
    pinMode(button0, INPUT_PULLUP);
    bmp.takeForcedMeasurement();
    aht.getEvent(&humidity, &temp);
    t = temp.temperature;
    h = humidity.relative_humidity;
    abshum = (6.112 * pow(2.71828, ((17.67 * temp.temperature)/(temp.temperature + 243.5))) * humidity.relative_humidity * 2.1674)/(273.15 + temp.temperature);
    bool isDataReady = false;

    pres2 = bmp.readPressure();
    pres = pres2 / 100.0;

    scd4x.getDataReadyStatus(isDataReady);
    if (isDataReady) {
    scd4x.setAmbientPressure(pres2);
    scd4x.readMeasurement(co2, temp2, hum);}
    displayMenu();
  }

  if (millis() > 30*60*1000) {ESP.restart();} //30 minute timeout
}