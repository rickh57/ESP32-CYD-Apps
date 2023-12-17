/*******************************************************************
    DemoApp for the ESP32 Cheap Yellow Display.

    Written by Rick Hale based on ideas from
    https://forum.arduino.cc/t/esp32-2432s028r-all-in-one-display-touch-spi-problems/1059746/33
 *******************************************************************/

#include <TFT_eTouch.h>
#include <TFT_eSPI.h>
#include "Free_Fonts.h"
#include <SPI.h>
#include "wificlient.h"
#include <mutex>
#include "poem.h"
#include "fractal.h"
#include "cardfs.h"
#include "pauser.h"
// JPEG decoder library
#include <JPEGDecoder.h>

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
  client->start();
  pauser->wait();
}

//##############################s######################################################################
// Draw a JPEG on the TFT pulled from SD Card
//####################################################################################################
// xpos, ypos is top left corner of plotted image
void drawSdJpeg(const char *filename, int xpos, int ypos) {

  // Open the named file (the Jpeg decoder library will close it)
  File jpegFile = SD.open( filename, FILE_READ);  // or, file handle reference for SD library

  if ( !jpegFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }

  Serial.println("===========================");
  Serial.print("Drawing file: "); Serial.println(filename);
  Serial.println("===========================");

  // Use one of the following methods to initialise the decoder:
  //bool decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
  bool decoded = JpegDec.decodeSdFile(filename);  // or pass the filename (String or character array)

  if (decoded) {
    // print information about the image to the serial port
    jpegInfo();
    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}

//####################################################################################################
// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
//####################################################################################################
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void jpegRender(int xpos, int ypos) {

  //jpegInfo(); // Print information from the JPEG file (could comment this line out)

  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  bool swapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
  uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // Fetch data from the file, decode and display
  while (JpegDec.read()) {    // While there is more data in the file
    pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

    // Calculate coordinates of top left corner of current MCU
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    else if ( (mcu_y + win_h) >= tft.height())
      JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  tft.setSwapBytes(swapBytes);

  showTime(millis() - drawTime); // These lines are for sketch testing only
}

//####################################################################################################
// Print image information to the serial port (optional)
//####################################################################################################
// JpegDec.decodeFile(...) or JpegDec.decodeArray(...) must be called before this info is available!
void jpegInfo() {

  // Print information extracted from the JPEG file
  Serial.println("JPEG image info");
  Serial.println("===============");
  Serial.print("Width      :");
  Serial.println(JpegDec.width);
  Serial.print("Height     :");
  Serial.println(JpegDec.height);
  Serial.print("Components :");
  Serial.println(JpegDec.comps);
  Serial.print("MCU / row  :");
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print("MCU / col  :");
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print("Scan type  :");
  Serial.println(JpegDec.scanType);
  Serial.print("MCU width  :");
  Serial.println(JpegDec.MCUWidth);
  Serial.print("MCU height :");
  Serial.println(JpegDec.MCUHeight);
  Serial.println("===============");
  Serial.println("");
}

//####################################################################################################
// Show the execution time (optional)
//####################################################################################################
// WARNING: for UNO/AVR legacy reasons printing text to the screen with the Mega might not work for
// sketch sizes greater than ~70KBytes because 16-bit address pointers are used in some libraries.

// The Due will work fine with the HX8357_Due library.

void showTime(uint32_t msTime) {
  //tft.setCursor(0, 0);
  //tft.setTextFont(1);
  //tft.setTextSize(2);
  //tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //tft.print(F(" JPEG drawn in "));
  //tft.print(msTime);
  //tft.println(F(" ms "));
  Serial.print(F(" JPEG drawn in "));
  Serial.print(msTime);
  Serial.println(F(" ms "));
}

void showPicture(const char *filename) {
  // The image is 300 x 300 pixels so we do some sums to position image in the middle of the screen!
  // Doing this by reading the image width and height from the jpeg info is left as an exercise!
  int x = 0; //(tft.width()  - 300) / 2 - 1;
  int y = 0; //(tft.height() - 300) / 2 - 1;

  Serial.printf("showPicture(%s)\n", filename);
  char fileNameBuffer[128];
  sprintf(fileNameBuffer, "/Documents/Pictures/%s", filename);
  eraseScreen(TFT_BLACK);
  xSemaphoreTake(mutex, portMAX_DELAY); // enter critical section
  drawSdJpeg(fileNameBuffer, x, y);     // This draws a jpeg pulled off the SD Card
  xSemaphoreGive(mutex); // exit critical section
  pauser->wait();
}

void loop() {
  static unsigned long repeat_count = 0;
  Serial.printf("Starting iteration #%ld\n", ++repeat_count);
  Fractal fractal;
  listDirWithCallback(SD, "/Documents/Pictures", 2, showPicture);
  Poem poem(config, pauser);
  for (int i = 0; i < config["poems"].length(); i++) {
    poem.showPoem(i);
    pauser->wait();
    fractal.drawMandelbrot();
    pauser->wait();
  }
  Serial.printf("Finished iteration #%ld\n", repeat_count);
}
