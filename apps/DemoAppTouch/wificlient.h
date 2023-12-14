#if !defined(__wificlient_h)
#define __wificlient_h

#include <Arduino_JSON.h>

class WifiClient {
private:
  JSONVar config;
  char timezone[32];
  TaskHandle_t TimeTempTask;
  double _temperature = -100;
  bool _firstTime = true;
  unsigned long _lastTime = 0;

  static void showDateAndTemp(void * pvParameters);
  void checkWeather();

public:
  WifiClient(JSONVar config);

  void start();
  bool connectWifi();
  const char * get_timezone();
  double temperature() { return _temperature; }
};

#endif
