/*
 * TinyStation - A Small TableStation Device for info communication
 * Lee Yongjae, setup74@gmail.com, 2017-05-10.
 */

/* 
 * FROM: WeatherStationDemo.ino: https://github.com/squix78/esp8266-weather-station 
 */
/**The MIT License (MIT)

Copyright (c) 2016 by Daniel Eichhorn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at http://blog.squix.ch
*/

#include <time.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <JsonListener.h>

#include "SSD1306Wire.h"
#include "OLEDDisplayUiAux.h"
#include "Wire.h"
#include "WundergroundClient.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"
#include "NTPClient.h"   // within ESP8266_Weather_Station/
#include "NcodeFontDraw.h"
#include "HelveticaFont.h"
#include "NewPinetreeFont.h"

// defined in mk_gmtime.c and gmtime_r.c
extern "C" {
time_t mk_gmtime(const struct tm * timeptr);
struct tm *gmtime_r(const time_t * timer, struct tm * timeptr);
}

// RGB LED Strip
#include <NeoPixelBus.h>
const uint16_t PixelCount = 4; // this example assumes 4 pixels, making it smaller will cause a failure
const uint8_t PixelPin = 2;  // make sure to set this to the correct pin, ignored for Esp8266
// Uart method is good for the Esp-01 or other pin restricted modules
// NOTE: These will ignore the PIN and use GPI02 pin (D4 in WeMos d1 mini)
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> strip(PixelCount, PixelPin);
RgbColor rgb_black(0, 0, 0);
RgbColor rgb_connecting(2, 2, 2);
RgbColor rgb_progress(0, 3, 3);
RgbColor rgb_frame_index(0, 3, 3);
RgbColor rgb_mqtt_noti(8, 8, 8);

// AQI Client
#include "AqiCnClient.h"

// MQTT Client
#include <PubSubClient.h>


/***************************
 * Begin Settings
 **************************/
// Please read http://blog.squix.org/weatherstation-getting-code-adapting-it
// for setup instructions

// _SiteConfig.h file is site specific list not to be in git repository, containing:
#if 0
// Wifi List
struct { char *ssid; char *pwd; } wifi[] = {
  { "your_ssid1", "your_pass_phrase1" },
  { "your_ssid2", "your_pass_phrase2" },
  ...
  { NULL, NULL }  // list end mark
};
// Wunderground Api
const boolean WUNDERGROUND_IS_METRIC = true;
const String WUNDERGROUND_API_KEY = "your api key";
const String WUNDERGROUND_LANGUAGE = "your lang";
const String WUNDERGROUND_COUNTRY = "your country";
const String WUNDERGROUND_CITY = "your city";
// MQTT Messaging
const char *mqttServer = "your mqtt server";
int mqttPort = 1883;
const char *mqttUser = "your mqtt client id";
const char *mqttPwd = "your mqtt client passwd";
const char *mqttTopic = "your mqtt topping for messages to this client";
// ntpClient settings
const float UTC_OFFSET = 9;
const char NTP_SERVER = "0.pool.ntp.org";
#endif
#include "_SiteConfig.h"

// Setup
const int UPDATE_INTERVAL_SECS = 10 * 60; // Update every 10 minutes

// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
const int SDA_PIN = D2;
const int SDC_PIN = D1;

// Initialize the oled display for address 0x3c
// sda-pin=14 and sdc-pin=12
SSD1306Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUiAux   ui( &display );

/***************************
 * End Settings
 **************************/

NTPClient ntpClient(NTP_SERVER, UTC_OFFSET * 3600L);

WundergroundClient wunderground(WUNDERGROUND_IS_METRIC);


// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

String lastUpdate = "--";

Ticker ticker;

//declaring prototypes
void drawProgress(OLEDDisplay *display, int percentage, String label1, String label2);
void updateData(OLEDDisplay *display);

void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawThingspeak(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void setReadyForWeatherUpdate();

// LED strip display set
void stripProgress(int cur, int max, RgbColor c);
void stripTrying(int cur, int max, RgbColor c);
void stripFrameIndex(int frameIndex, int frameCount);
void stripAQI(int frameIndex, int frameCount);
void stripMQTT(int frameIndex, int frameCount);

// AQI Client
AqiCnClient aqi = AqiCnClient("seoul", "e5347327ca989ce719b85002dedceda4707df6e5");
void drawAQI(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);

// Event Days
#define EVENT_BASE_DAY    31
#define EVENT_BASE_MONTH  5
#define EVENT_BASE_YEAR   2017
void drawEventDay(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);

// MQTT Client
WiFiClient espClient;
PubSubClient mqttClient(espClient);
// received msg
char mqttMsg[50] = "";
long lastMsg = 0;
int value = 0;
void drawMQTT(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void mqttCallback(char* topic, byte* payload, unsigned int length);

// NcodeFont
NcodeFontDraw nfd(Helvetica_18, NewPinetree_18, 1);


// Add frames
// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
#if 1
FrameCallback frames[] = { drawDateTime, drawCurrentWeather, drawEventDay, drawMQTT };
AuxCallback auxes[]    = { stripFrameIndex, stripFrameIndex, stripFrameIndex, stripFrameIndex };
int numberOfFrames = 4;
#else
FrameCallback frames[] = { drawDateTime, drawCurrentWeather, drawForecast, drawAQI,  drawMQTT };
AuxCallback auxes[]    = { stripAQI,     stripAQI,           stripAQI,     stripAQI, stripMQTT };
int numberOfFrames = 5;
#endif


OverlayCallback overlays[] = { drawHeaderOverlay };
int numberOfOverlays = 1;


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // this resets all the neopixels to an off state
  strip.Begin();
  strip.Show();

  // initialize dispaly
  display.init();
  display.clear();
  display.display();

  //display.flipScreenVertically();
  //display.setFont(ArialMT_Plain_10);
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);

  int wi = 0;
  int counter = 0;
  int counter_max = 24;
  WiFi.begin(wifi[wi].ssid, wifi[wi].pwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.clear();
    nfd.setFont(Helvetica_14, NewPinetree_14);
    nfd.drawStringMaxWidth(&display, 64, 4, nfd.TEXT_ALIGN_CENTER, 128, "Connecting");
    nfd.setFont(Helvetica_18, NewPinetree_18);
    nfd.drawStringMaxWidth(&display, 64, 30, nfd.TEXT_ALIGN_CENTER, 128, wifi[wi].ssid);
    display.display();

    stripTrying(counter % 4 + 1, 4, rgb_connecting);
    
    counter++;
    if (counter % counter_max == 0) {  // try next wifi item
      wi = (wifi[wi + 1].ssid == NULL)? 0 : wi + 1;
      WiFi.begin(wifi[wi].ssid, wifi[wi].pwd);
    }
  }

  ntpClient.begin();

  // MQTT Client Setup
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  ui.setTargetFPS(30);
  
  ui.setActiveSymbol(activeSymbole);
  ui.setInactiveSymbol(inactiveSymbole);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  ui.setFrames(frames, numberOfFrames);
  ui.setAuxes(auxes, numberOfFrames);
  ui.disableAllIndicators();

  //ui.setOverlays(overlays, numberOfOverlays);

  // Inital UI takes care of initalising the display too.
  ui.init();

  Serial.println("");

  updateData(&display);

  ticker.attach(UPDATE_INTERVAL_SECS, setReadyForWeatherUpdate);
}

void loop() {
  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED)
    updateData(&display);

  if (mqttClient.connected())
    mqttClient.loop();

  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    delay(remainingTimeBudget);
  }
}

void drawProgress(OLEDDisplay *display, int percentage, String label1, String label2) {
  char s[100];
  
  display->clear();
  label1.toCharArray(s, sizeof(s));
  nfd.setFont(Helvetica_14, NewPinetree_14);
  nfd.drawStringMaxWidth(display, 64, 4, nfd.TEXT_ALIGN_CENTER, 128, s);

  label2.toCharArray(s, sizeof(s));
  nfd.setFont(Helvetica_18, NewPinetree_18);
  nfd.drawStringMaxWidth(display, 64, 30, nfd.TEXT_ALIGN_CENTER, 128, s);
  display->display();
  
  stripProgress(percentage, 100, rgb_progress);
}

void updateData(OLEDDisplay *display) {
  drawProgress(display, 10, "Updating", "Time");
  ntpClient.update();

  drawProgress(display, 30, "Updating", "Weather");
  wunderground.updateConditions(WUNDERGROUND_API_KEY, WUNDERGROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);

  //drawProgress(display, 50, "Updating", "Forecasts");
  //wunderground.updateForecast(WUNDERGROUND_API_KEY, WUNDERGROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);

  //drawProgress(display, 60, "Updating", "AQI Data");
  //aqi.doUpdate();

  lastUpdate = ntpClient.getFormattedTime();
  readyForWeatherUpdate = false;

  // MQTT Reconnect Check
  if (!mqttClient.connected()) { 
    drawProgress(display, 70, "Reconnecting", "MQTT Broker");
    if (mqttClient.connect("ESP8266Client", mqttUser, mqttPwd)) {
      Serial.println("connected");
      mqttClient.publish(mqttTopic, "hello world");
      mqttClient.subscribe(mqttTopic);
    }
    else {
      strcpy(mqttMsg, "(not connected)");
    }
    delay(1000);
  }
  
  drawProgress(display, 100, "Updating", "Done");
  delay(1000);
}

const char *wday_str[7] = {
  "일", "월", "화", "수", "목", "금", "토"
};

void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  char s[100];

  //String date = wunderground.getDate();
  //date.toCharArray(s, sizeof(s));

  time_t curTime = ntpClient.getRawTime() - 946684800L/*UNIX_OFFSET*/;  // change epoch: 1970->2000
  struct tm tv;
  gmtime_r(&curTime, &tv);
  sprintf(s, "%d.%d.%d (%s)", tv.tm_year + 1900, tv.tm_mon + 1, tv.tm_mday, wday_str[tv.tm_wday]);
  nfd.setFont(Helvetica_Bold_14, NewPinetree_Bold_14);
  nfd.drawStringMaxWidth(display, 64 + x, 6 + y, nfd.TEXT_ALIGN_CENTER, 128, s);
  
  String time = ntpClient.getFormattedTime();
  time.toCharArray(s, sizeof(s));
  nfd.setFont(Helvetica_Bold_24, NewPinetree_Bold_24);
  nfd.drawStringMaxWidth(display, 64 + x, 30 + y, nfd.TEXT_ALIGN_CENTER, 128, s);
}

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  char s[100];
    
  String weather = wunderground.getWeatherText();
  weather.toCharArray(s, sizeof(s));
  nfd.setFont(Helvetica_Bold_14, NewPinetree_Bold_14);
  nfd.drawStringMaxWidth(display, 58 + x, 10 + y, nfd.TEXT_ALIGN_LEFT, 128, s);

  String temp = wunderground.getCurrentTemp() + "°C";
  temp.toCharArray(s, sizeof(s));
  nfd.setFont(Helvetica_Bold_24, NewPinetree_Bold_24);
  nfd.drawStringMaxWidth(display, 58 + x, 30 + y, nfd.TEXT_ALIGN_LEFT, 128, s);

  display->setFont(Meteocons_Plain_42);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  String weatherIcon = wunderground.getTodayIcon();
  int weatherIconWidth = display->getStringWidth(weatherIcon);
  display->drawString(29 + x, 12 + y, weatherIcon);
}

void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawForecastDetails(display, x, y, 0);
  drawForecastDetails(display, x + 44, y, 2);
  drawForecastDetails(display, x + 88, y, 4);
}

void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  char s[100];

  String day = wunderground.getForecastTitle(dayIndex).substring(0, 3);
  day.toUpperCase();
  day.toCharArray(s, sizeof(s));
  nfd.setFont(Helvetica_14, NewPinetree_14);
  nfd.drawStringMaxWidth(display, 20 + x, 2 + y, nfd.TEXT_ALIGN_CENTER, 128, s);

  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 20, 24 + y, wunderground.getForecastIcon(dayIndex));

  (wunderground.getForecastLowTemp(dayIndex) + "/" + wunderground.getForecastHighTemp(dayIndex)).toCharArray(s, sizeof(s));
  nfd.setFont(Helvetica_14, NewPinetree_14);
  nfd.drawStringMaxWidth(display, 20 + x, 48 + y, nfd.TEXT_ALIGN_CENTER, 128, s);
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  String time = ntpClient.getFormattedTime().substring(0, 5);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 54, time);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  String temp = wunderground.getCurrentTemp() + "°C";
  display->drawString(128, 54, temp);
  display->drawHorizontalLine(0, 52, 128);
}

void setReadyForWeatherUpdate() {
  Serial.println("Setting readyForUpdate to true");
  readyForWeatherUpdate = true;
}


// LED Strip

void stripProgress(int cur, int max, RgbColor c) {
  int i, num_led = 4;
  for (i = 0; i < num_led; i++) {
    // (i / num_led >= cur / max ) * (num_led * max) to remove divider part
    if (i * max  <= cur * num_led)
      strip.SetPixelColor(i, c);
    else
      strip.SetPixelColor(i, RgbColor(0, 0, 0));
  }
  strip.Show();
}

void stripTrying(int cur, int max, RgbColor c) {
  int i, num_led = 4;
  for (i = 0; i < num_led; i++) {
    if (i == cur * num_led / (max + 1))
      strip.SetPixelColor(i, c);
    else
      strip.SetPixelColor(i, RgbColor(0, 0, 0));
  }
  strip.Show();
}

void stripFrameIndex(int frameIndex, int frameCount) {
  // led strip to show pending message counts
  for (int i = 0; i < 4; i++) 
      strip.SetPixelColor(i, (i + 1 == 4 * (frameIndex + 1) / frameCount)? rgb_frame_index : rgb_black);
  strip.Show();
}

void stripAQI(int frameIndex, int frameCount) {
  RgbColor c = RgbColor(aqi.r * 4 / 255, aqi.g * 4 / 255, aqi.b * 4 / 255);
  strip.SetPixelColor(0, (aqi.val >= 0)?   c : rgb_black);
  strip.SetPixelColor(1, (aqi.val >= 50)?  c : rgb_black);
  strip.SetPixelColor(2, (aqi.val >= 100)? c : rgb_black);
  strip.SetPixelColor(3, (aqi.val >= 150)? c : rgb_black);
  strip.Show();
}

void stripMQTT(int frameIndex, int frameCount) {
  strip.SetPixelColor(0, rgb_mqtt_noti);
  strip.SetPixelColor(1, rgb_black);
  strip.SetPixelColor(2, rgb_black);
  strip.SetPixelColor(3, rgb_mqtt_noti);
  strip.Show();
}


// AQI Client

void drawAQI(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  char s[100];
    
  aqi.level.toCharArray(s, sizeof(s));
  nfd.setFont(Helvetica_14, NewPinetree_14);
  nfd.drawStringMaxWidth(display, 64 + x, 2 + y, nfd.TEXT_ALIGN_CENTER, 128, s);

  ("AQI " + aqi.val_s).toCharArray(s, sizeof(s));
  nfd.setFont(Helvetica_Bold_24, NewPinetree_Bold_24);
  nfd.drawStringMaxWidth(display, 64 + x, 20 + y, nfd.TEXT_ALIGN_CENTER, 128, s);

  aqi.dominentpol.toCharArray(s, sizeof(s));
  nfd.setFont(Helvetica_14, NewPinetree_14);
  nfd.drawStringMaxWidth(display, 64 + x, 46 + y, nfd.TEXT_ALIGN_CENTER, 128, s);
}


// Event Day Display

void drawEventDay(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  nfd.setFont(Helvetica_12, NewPinetree_12);
  nfd.drawStringMaxWidth(display, x + 64, 2 + y, nfd.TEXT_ALIGN_CENTER, 128, "Event Days");
  
  struct tm tv;
  memset(&tv, 0, sizeof(tv));
  tv.tm_mday = EVENT_BASE_DAY;
  tv.tm_mon = EVENT_BASE_MONTH - 1;
  tv.tm_year = EVENT_BASE_YEAR - 1900;
  tv.tm_isdst = 0;
  time_t baseTime = mk_gmtime(&tv) + UTC_OFFSET * 3600L;/*epoch=year2000*/;
  time_t curTime = ntpClient.getRawTime() - 946684800L/*UNIX_OFFSET*/;  // change epoch: 1970->2000
  int dayCount =  curTime / 86400L - baseTime / 86400UL;  // dayCount starts at 0
  
  char eventMsg[32];
  sprintf(eventMsg, "산이 %d 일째", dayCount + 1);  // start at 1
  nfd.setFont(Helvetica_18, NewPinetree_18);
  nfd.drawStringMaxWidth(display, x + 64, 20 + y, nfd.TEXT_ALIGN_CENTER, 128, eventMsg);

  sprintf(eventMsg, "%d개월+%d : %d주+%d", dayCount / 30, dayCount % 30, dayCount / 7, dayCount % 7);
  nfd.setFont(Helvetica_Bold_14, NewPinetree_Bold_14);
  nfd.drawStringMaxWidth(display, x + 64, 42 + y, nfd.TEXT_ALIGN_CENTER, 128, eventMsg);
}


// MQTT Client

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Switch on the LED if an 1 was received as first character
  if (length >= sizeof(mqttMsg))
    length = sizeof(mqttMsg) - 1;
  if (length > 0)
    memcpy(mqttMsg, payload, length);
  mqttMsg[length] = '\0';
  ui.switchToFrame(4);
}

void drawMQTT(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  nfd.setFont(Helvetica_12, NewPinetree_12);
  nfd.drawStringMaxWidth(display, x + 64, 2 + y, nfd.TEXT_ALIGN_CENTER, 128, "Message");
  nfd.setFont(Helvetica_18, NewPinetree_18);
  nfd.drawStringMaxWidth(display, x + 64, 20 + y, nfd.TEXT_ALIGN_CENTER, 128, mqttMsg);
}

