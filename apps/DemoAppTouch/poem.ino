/*******************************************************************
    DemoApp for the ESP32 Cheap Yellow Display.

    https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

    Written by Rick Hale
 *******************************************************************/

#include <mutex>
#include "poem.h"

extern SemaphoreHandle_t mutex;

int Poem::showLine(String line, int x, int y) {
  xSemaphoreTake(mutex, portMAX_DELAY); // enter critical section
  tft.setTextColor(_text_color, _background_color);
  tft.setFreeFont(FSB9);
  tft.drawCentreString(line, x, y, GFXFF); // Centered
  int retval = tft.fontHeight(GFXFF);;
  tft.setFreeFont(NULL); // Set font to GLCD
  xSemaphoreGive(mutex); // exit critical section
  return retval;
}

void Poem::showPoem(int poemNumber) {
  String title = (const char *) config["poems"][poemNumber]["title"];
  String author = (const char *) config["poems"][poemNumber]["author"];
  Serial.printf("poem #%d: title: %s, author: %s\n", poemNumber, title.c_str(), author.c_str());
  _text_color = (int) config["poems"][poemNumber]["text_color"];
  _background_color = (int) config["poems"][poemNumber]["background_color"];
  eraseScreen(_background_color);
  int x = 320 / 2;
  int y = 240 / 2 - 20;
  int fontSize = 2;
  xSemaphoreTake(mutex, portMAX_DELAY); // enter critical section
  tft.setFreeFont(NULL); // Set font to GLCD
  tft.setTextColor(_text_color, _background_color);
  tft.drawCentreString(title, x, y, fontSize);
  y += 30;
  tft.drawCentreString(author, x, y, 1);
  xSemaphoreGive(mutex); // exit critical section
  _pauser->wait();

  // read the poem from the SD card...
  String poem_text = readFile(SD, (const char *) config["poems"][poemNumber]["path"]);
  int start = 0;
  int index = poem_text.indexOf("\r\n\r\n");
  int verse = 0;
  while (index != -1) {
    showVerse((const char *) config["poems"][poemNumber]["title"], verse++, poem_text.substring(start, index));
    start = index + 4;
    index = poem_text.indexOf("\r\n\r\n", start);
  }
  showVerse((const char *) config["poems"][poemNumber]["title"], verse++, poem_text.substring(start), true);
}

void Poem::showVerse(const String title, int verse_number, String verse, bool lastVerse) {
  eraseScreen(_background_color);
  int x = 320 /2;
  int y = 80;
  int fontSize = 4;
  int rowSize = 30;
  int start = 0;
  int index = verse.indexOf("\r\n");
  while (index != -1) {
    String row = verse.substring(start, index);
    y += showLine(row, x, y);
    delay(20);
    start = index + 2;
    index = verse.indexOf("\r\n", start);
  }
  showLine(verse.substring(start), x, y);
  if (!lastVerse) {
    _pauser->wait();
  }
}
