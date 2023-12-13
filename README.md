# ESP32-CYD-Apps

Apps for the [Cheap Yellow Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)

## Sample Configuration and Data

The sdcard folder contains sample configuration and data for the apps to use. You can copy its contents to the root of your CYD's SD card.

### Configuration

`SD/config.json` is used to store configuration information. `sdcard/config.json` is a sample that will need to be modified to add your own network credentials and [OpenWeatherMap API key](https://openweathermap.org/api), along with city, state, etc. The applications will try all of the wifi access points until it successfully connects. If none of them work, it will continue in offline mode, but the date and weather won't be available.

### Data

The pictures were copied from the [TFT_eSPI examples](https://github.com/Bodmer/TFT_eSPI/tree/master/examples/Generic/ESP32_SDcard_jpeg/Data)
