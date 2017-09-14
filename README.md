# TinyStation

TinyStation - A Small Table Station Device for info communication
- Author: Lee Yongjae, setup74@gmail.com, 2017-05-10.
- Ardunio Program for ESP8266-WeMos module + SSD1306 OLED Display + NeoPixel LED Strip
- FROM: WeatherStationDemo.ino: https://github.com/squix78/esp8266-weather-station 
- has Hangul (Korean) display feature for mqtt message display


### Prepare

Following files should be created for local sensitive info

**_WifiList.h**
```c++
struct { char *ssid; char *pwd; } wifi[] = {
  { "your_ssid1", "your_pass_phrase1" },
  { "your_ssid2", "your_pass_phrase2" },
  ...
  { NULL, NULL }  // list end mark
};
```

**_WUnderGround.h**
```c++
const boolean WUNDERGROUND_IS_METRIC = true;
const String WUNDERGROUND_API_KEY = "your api key";
const String WUNDERGROUND_LANGUAGE = "your lang";
const String WUNDERGROUND_COUNTRY = "your country";
const String WUNDERGROUND_CITY = "your city";
```

