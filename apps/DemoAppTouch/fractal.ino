/*******************************************************************
    Based on https://github.com/adafruit/Adafruit_ILI9341/tree/master/examples/mandelbrot

    Written by Rick Hale
 *******************************************************************/

#include "fractal.h"

void Fractal::drawMandelbrot() {
  int64_t       n, a, b, a2, b2, posReal, posImag;
  uint32_t      startTime,elapsedTime;

  int32_t
    startReal   = (int64_t)((centerReal - rangeReal * 0.5)   * (float)(1 << bits)),
    startImag   = (int64_t)((centerImag + rangeImag * 0.5)   * (float)(1 << bits)),
    incReal     = (int64_t)((rangeReal / (float)pixelWidth)  * (float)(1 << bits)),
    incImag     = (int64_t)((rangeImag / (float)pixelHeight) * (float)(1 << bits));
  Serial.println("Mandelbrot set");
  eraseScreen(TFT_BLACK);

  startTime = millis();
  posImag = startImag;
  for (int y = 0; y < pixelHeight; y++) {
    posReal = startReal;
    for (int x = 0; x < pixelWidth; x++) {
      a = posReal;
      b = posImag;
      for (n = iterations; n > 0 ; n--) {
        a2 = (a * a) >> bits;
        b2 = (b * b) >> bits;
        if ((a2 + b2) >= (4 << bits))
          break;
        b  = posImag + ((a * b) >> (bits - 1));
        a  = posReal + a2 - b2;
      }
      xSemaphoreTake(mutex, portMAX_DELAY); // enter critical section
      tft.drawPixel(x, y, (n * 29)<<8 | (n * 67)); // takes 500ms with individual pixel writes
      xSemaphoreGive(mutex); // exit critical section
      posReal += incReal;
      yield();
    }
    posImag -= incImag;
  }
  elapsedTime = millis()-startTime;
  Serial.printf("Fractal took %d ms\n", elapsedTime);

  rangeReal *= 0.95;
  rangeImag *= 0.95;
  Serial.printf("adjusted rangeReal: %f\n", rangeReal);
  if (rangeReal < 1.5) {
    // reset to full range...
    Serial.println("Reset range");
    rangeReal   =  3.0;
    rangeImag   =  3.0;
  }
}
