/*******************************************************************
    DemoApp for the ESP32 Cheap Yellow Display.

    Written by Rick Hale
 *******************************************************************/

#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_sntp.h"
#include "time.h"
#include <mutex>
#include <string.h>
#include "Free_Fonts.h"

extern SemaphoreHandle_t mutex;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec =  -8 * 60 * 60;
const int   daylightOffset_sec = 3600;

// refresh the weather only every 15 minutes...
const unsigned long timerDelay = 15 * 60 * 1000;
const unsigned char wifiicon[] ={ // wifi icon
  0x00, 0xff, 0x00, 0x7e, 0x00, 0x18,0x00, 0x00
};

WifiClient::WifiClient(JSONVar config) : config(config) {
  strcpy(timezone, (const char *) config["timezone"]);
}

const char *WifiClient::get_timezone() {
  return timezone;
}

bool WifiClient::connectWifi() {
  bool connected = false;
  int hotspot_count = config["wifi"].length();
  int hotspot_number = 0;
  String ssid;
  String password;
  while (!connected && hotspot_number < hotspot_count) {
    ssid = (const char*) config["wifi"][hotspot_number]["ssid"];
    password = (const char*) config["wifi"][hotspot_number]["password"];
    Serial.printf("Connecting to WiFi network, ssid: %s\n", ssid);
    WiFi.begin(ssid, password);
    int retries = 0;
    while(WiFi.status() != WL_CONNECTED && retries++ < 20) {
      delay(1000);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }
    else {
      WiFi.disconnect();
      hotspot_number++;
    }
  }
  if (connected) {
    Serial.println("");
    Serial.printf("Connected to WiFi network, ssid: %s, with IP Address: %s\n", ssid, WiFi.localIP().toString());
    // from https://lastminuteengineers.com/esp32-ntp-server-date-time-tutorial/
    delay(1000);
    Serial.printf("Syncing to ntp server: %s\n",ntpServer);
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(1000);
  }
  else {
    Serial.println("");
    Serial.printf("Unable to connect to wifi\n");
  }
  return connected;
}

void WifiClient::start() {
  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
    this->showDateAndTemp,   /* Task function. */
    "DateTemp",     /* name of task. */
    10000,       /* Stack size of task */
    this,        /* parameter of the task */
    1,           /* priority of the task */
    &TimeTempTask,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
  vTaskDelay(2000);
}

void WifiClient::checkWeather() {
  // Send an HTTP GET request to openweathermap.org
  if (_firstTime || (millis() - _lastTime) > timerDelay) {
    _firstTime = false;
    // Check WiFi connection status
    if(WiFi.status() == WL_CONNECTED){
      HTTPClient http;
      double latitude = (double) config["openweathermap"]["latitude"];
      double longitude = (double) config["openweathermap"]["longitude"];
      String apiKey = (const char *) config["openweathermap"]["ApiKey"];

      String serverPath = "http://api.openweathermap.org/data/2.5/weather?lat=" + (String) latitude  +
        "&lon=" + (String) longitude +
        "&APPID=" + apiKey + "&units=Imperial";
      Serial.printf("checking weather, serverPath=%s\n", serverPath.c_str());

      http.begin(serverPath.c_str());

      // Send HTTP GET request
      int httpResponseCode = http.GET();

      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String jsonBuffer = http.getString();
        Serial.println(jsonBuffer);
        JSONVar myObject = JSON.parse(jsonBuffer);

        // JSON.typeof(jsonVar) can be used to get the type of the var
        if (JSON.typeof(myObject) == "undefined") {
          Serial.println("Parsing input failed!");
          return;
        }
        Serial.print("JSON object = ");
        Serial.println(myObject);
        Serial.print("Temperature: ");
        _temperature = (double) myObject["main"]["temp"];
        Serial.println(_temperature);
        Serial.print("Pressure: ");
        Serial.println(myObject["main"]["pressure"]);
        Serial.print("Humidity: ");
        Serial.println(myObject["main"]["humidity"]);
        Serial.print("Wind Speed: ");
        Serial.println(myObject["wind"]["speed"]);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();

    }
    else {
      Serial.println("WiFi Disconnected");
    }
    _lastTime = millis();
  }
}

void WifiClient::showDateAndTemp(void * pvParameters) {
  WifiClient *l_pThis = (WifiClient *) pvParameters;
  Serial.printf("ShowDateTemp running on core %d\n", xPortGetCoreID());
  String temperatureStr = "??";
  String curDateTime;
  long counter = 0;
  for (;;) {
    l_pThis->checkWeather();
    if (l_pThis->temperature() != -100) {
      temperatureStr = String(l_pThis->temperature(), 0) + "F";
    }

    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    time(&now);
    // Set timezone to Pacific Standard Time
    setenv("TZ", l_pThis->get_timezone(), 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%H:%M:%S", &timeinfo);

    String newDateTime = temperatureStr + " " + strftime_buf;
    if (newDateTime != curDateTime) {
      if (counter++ % 100 == 0) {
        Serial.printf("showDateAndTemp(), temperature: %s, current date/time: %s\n", temperatureStr, strftime_buf);
      }
      curDateTime = newDateTime;
      int x = 240;
      int y = 0;
      int fontSize = 1;
      bool connected = WiFi.status() == WL_CONNECTED;
      xSemaphoreTake(mutex, portMAX_DELAY); // enter critical section
      tft.setFreeFont(NULL); // Set font to GLCD
      tft.setTextColor(TFT_RED, TFT_BLACK);
      int iconX = 225;
      if (connected) {
        tft.drawBitmap(iconX,0,wifiicon,8,8,TFT_WHITE);
      }
      else {
        tft.drawString("X", iconX, 0, fontSize);
      }
      tft.drawString(curDateTime, x, y, fontSize);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setFreeFont(FM12);
      xSemaphoreGive(mutex); // exit critical section
    }
    vTaskDelay(250);
  }
}