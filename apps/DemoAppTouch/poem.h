#if !defined(__poem_h)
#define __poem_h

#include <Arduino_JSON.h>
#include "pauser.h"

class Poem {
private:
  int _poemNumber;
  int _text_color;
  int _background_color;
  JSONVar _config;
  Pauser *_pauser;

  int showLine(String line, int x, int y);
  void showVerse(const String title, int verse_number, String verse, bool lastVerse=false);
public:
  Poem(JSONVar config, Pauser *pauser) : _config(config), _pauser(pauser) {}
  void showPoem(int poemNumber);
};

#endif
