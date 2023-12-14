/*******************************************************************
    DemoApp for the ESP32 Cheap Yellow Display.

    Written by Rick Hale
 *******************************************************************/

#include <mutex>
#include "pauser.h"

extern SemaphoreHandle_t mutex;

void Pauser::wait() {
  if (_useTouch) {
    int xp = 320 / 2;
    int yp = 230;
    int fontSize = 1;
    int16_t x, y; // screen coordinates
    int waitCount = 0;
    String prompt = "Touch screen to continue";
    if (_firstTime) {
      prompt = "Touch or Wait (to disable touch)";
    }
    xSemaphoreTake(mutex, portMAX_DELAY); // enter critical section
    tft.setFreeFont(NULL); // Set font to GLCD
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawCentreString(prompt, xp, yp, fontSize);
    xSemaphoreGive(mutex); // exit critical section  
    while (true) {
      if (touch.getXY(x, y)) {
        // Serial.printf("touched at (%d, %d)\n", x, y);
        delay(500);
        _firstTime = false;
        break;
      }
      else {
        delay(100);
        waitCount++;
        if (_firstTime && waitCount > 50) {
          _useTouch = false;
          _firstTime = false;
          break;
        }
      }
    }
  }
  else {
    delay(5000);
  }
}
