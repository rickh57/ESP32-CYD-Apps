/*******************************************************************
    DemoApp for the ESP32 Cheap Yellow Display.

    https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

    Written by Rick Hale
 *******************************************************************/

#include <TFT_eTouch.h>
#include <TFT_eSPI.h>
#include "Free_Fonts.h"
#include <SPI.h>
#include <SafeString.h>
#include "wificlient.h"
#include <mutex>
#include "poem.h"
#include "fractal.h"
#include "cardfs.h"
#include "pauser.h"

cSF(my_SS,64);

#include "FS.h"
#include "SD.h"

// define constants for customized SPI-Pins
#define SCK   18
#define MISO  19
#define MOSI  23

SemaphoreHandle_t mutex = xSemaphoreCreateMutex();

void PrintFileNameDateTime() {
  Serial.println( F("Code running comes from file ") );
  Serial.println( F(__FILE__) );
  Serial.print( F("  compiled ") );
  Serial.print( F(__DATE__) );
  Serial.print( F(" ") );
  Serial.println( F(__TIME__) );
}

/*
  ESP32-2432S028-320x240-TFT_eTouch-Demo-Code-001.ino
  ------------------------------------------------
  This sketch tests and demonstrates the use of touchscreen functions of the ESP32 Smart Display
  ESP32-2432S028 using the TFT_eTouch.h library available on GitHub: https://github.com/achillhasler/TFT_eTouch. The smart display uses different SPI pins for the touchscreen controller and for the the  TFT display controller ILI9341. This means that the touch capabilities of the TFT_eSPI library cannot be used as this library assumes same SPI pins for touchscreen and display.

  Using the TFT_eTouch library allows the touch controller to use different SPI pins from the display.

  This sketch reads the touch display displays the screen location touched.

  The calibration settings were obtained from running the calibrate.ino sketch
  with the ESP32-2432S028 display.
  ------------------------------------------------------------------------------------------------------
*/

// TFT display defines are in the TFT_eSPI UserSetup.h

//defines related to ESP32 Smart Display
//--------- TFT_eTouch defines --------
// pins used by ESP32-2432S028 display touch controller
#define ETOUCH_MOSI 32
#define ETOUCH_MISO 39
#define ETOUCH_SCK 25
#define ETOUCH_CS 33
#define ETOUCH_IRQ 0xff // not used in this sketch
//
//---- TFT defines not defined in TFT_eSPI.h : User_Setup.h
#define SCREEN_ROTATION 1
#define LCD_BACK_LIGHT_PIN 21

SPIClass hSPI(HSPI);//define SPI port to be used by TFT_eTouch
TFT_eSPI tft; // TFT_eSPI instance
TFT_eTouch<TFT_eSPI> touch(tft, ETOUCH_CS, ETOUCH_IRQ, hSPI);

//variables used by Sketch
int tft_width;
int tft_height;

WifiClient *client = NULL;
JSONVar config;
Pauser *pauser = new Pauser(true);

void eraseScreen(int background_color) {
  xSemaphoreTake(mutex, portMAX_DELAY); // enter critical section
  tft.fillScreen(background_color);
  xSemaphoreGive(mutex); // exit critical section
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  while(!Serial);
  delay(1000);
  Serial.println("Setup-Start");
  PrintFileNameDateTime();

  hSPI.begin(ETOUCH_SCK, ETOUCH_MISO, ETOUCH_MOSI, ETOUCH_CS);// Touch controller pins
  tft.begin();
  delay(500);

  mountSdcard();
  showCardInfo();
  config = readJsonFile(SD, "/etc/config.json");

  touch.init();

  pinMode(LCD_BACK_LIGHT_PIN, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.setRotation(SCREEN_ROTATION);
  tft.fillScreen(TFT_BLACK);
  tft_width = tft.width();
  tft_height = tft.height();
  // the following Calibration was obtained using calibrate.ino in TFT_eTouch.h examples
  TFT_eTouchBase::Calibation calibrationData = { 249, 3859, 3738, 164, 2 };
  //#define TOUCH_DEFAULT_CALIBRATION { 236, 3831, 3731, 178, 2 }
  //#define TOUCH_DEFAULT_CALIBRATION { 249, 3859, 3738, 164, 2 }

  client = new WifiClient(config);
  client->connectWifi();

  touch.setCalibration(calibrationData);
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  int x = 320 / 2;
  int y = 220 / 2;
  tft.drawCentreString("Rick's DemoApp (Touch)", x, y, 4);
  y += 20;
  tft.drawCentreString(__DATE__, x, y, 2);

  // starting background task...
  client->Start();
  pauser->wait();
}

void loop() {
  static unsigned long repeat_count = 0;
  Serial.printf("Starting iteration #%ld\n", ++repeat_count);
  Fractal fractal;
  Poem poem(config, pauser);
  for (int i = 0; i < config["poems"].length(); i++) {
    poem.showPoem(i);
    pauser->wait();
    fractal.drawMandelbrot();
    pauser->wait();
  }
  Serial.printf("Finished iteration #%ld\n", repeat_count);
}
