/**The MIT License (MIT)

Copyright (c) 2015 by Daniel Eichhorn

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

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "AqiCnClient.h"


AqiCnClient::AqiCnClient(String city, String token) {
  query_url = String("/feed/") + city + "/?token=" + token;
}

void AqiCnClient::doUpdate(void) {
  JsonStreamingParser parser;
  parser.setListener(this);

  const char *host = "api.waqi.info";
  WiFiClient client;
  if (!client.connect(host, 80)) {
    Serial.println("connection failed");
    return;
  }

  Serial.print("Requesting URL: ");
  Serial.println(query_url);

  // This will send the request to the server
  client.print(String("GET ") + query_url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  int retryCounter = 0;
  while(!client.available()) {
    delay(1000);
    retryCounter++;
    if (retryCounter > 10) {
      return;
    }
  }

  int pos = 0;
  boolean isBody = false;
  char c;

  int size = 0;
  client.setNoDelay(false);
  while(client.connected()) {
    while((size = client.available()) > 0) {
      c = client.read();
      if (c == '{' || c == '[') {
        isBody = true;
      }
      if (isBody) {
        parser.parse(c);
      }
    }
  }
}


// JSON Parser for AQI Response —————————————————

void AqiCnClient::whitespace(char c) {
}

void AqiCnClient::startDocument() {
}

void AqiCnClient::key(String key) {
  curKey = String(key);
}

void AqiCnClient::value(String value) {
  if (curKey == "aqi") {
    val_s = value;
    val = val_s.toInt();
    if      (val <= 50 ) { r = 0;   g = 153; b = 102; level = "Good";          }
    else if (val <= 100) { r = 255; g = 222; b = 51;  level = "Moderate";      }
    else if (val <= 150) { r = 255; g = 153; b = 51;  level = "Sensitive";     }
    else if (val <= 200) { r = 204; g = 0;   b = 51;  level = "Unhealthy";     }
    else if (val <= 300) { r = 102; g = 0;   b = 153; level = "Very Unhealty"; }
    else                 { r = 126; g = 0;   b = 35;  level = "Hazrddous";     }
  }
  else if (curKey == "idx") { idx = value; }
  else if (curPar == "city" && curKey == "name") { city_name = value; }
  else if (curKey == "dominentpol") { dominentpol = value; }
  // iaqi
  else if (curPar == "co"   && curKey == "v") { iaqi.co   = value; }
  else if (curPar == "h"    && curKey == "v") { iaqi.h    = value; }
  else if (curPar == "no2"  && curKey == "v") { iaqi.no2  = value; }
  else if (curPar == "o3"   && curKey == "v") { iaqi.o3   = value; }
  else if (curPar == "p"    && curKey == "v") { iaqi.p    = value; }
  else if (curPar == "pm10" && curKey == "v") { iaqi.pm10 = value; }
  else if (curPar == "pm25" && curKey == "v") { iaqi.pm25 = value; }
  else if (curPar == "r"    && curKey == "v") { iaqi.r    = value; }
  else if (curPar == "so2"  && curKey == "v") { iaqi.so2  = value; }
  else if (curPar == "t"    && curKey == "v") { iaqi.t    = value; }
  else if (curPar == "w"    && curKey == "v") { iaqi.w    = value; }
  else if (curPar == "wd"   && curKey == "v") { iaqi.wd   = value; }
  // time
  else if (curPar == "time" && curKey == "s" ) { time_s  = value; }
  else if (curPar == "time" && curKey == "tz") { time_tz = value; }
  else if (curPar == "time" && curKey == "v" ) { time_v  = value; }
}

void AqiCnClient::endArray() {
}

void AqiCnClient::startObject() {
  curPar = curKey;
}

void AqiCnClient::endObject() {
  curPar = "";
}

void AqiCnClient::endDocument() {
}

void AqiCnClient::startArray() {
}
