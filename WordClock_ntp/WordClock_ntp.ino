#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <time.h>
#include <ArduinoJson.h>
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERNAL // Disable version number message in FastLED library (looks like an error)
#include <FastLED.h>
#include <FS.h>
#include <MedianFilter.h>

#include "display_utils.h"

#define SERIAL_DEBUG Serial

#define CONFIG_FILE "/last_config.json"
#define NTP_SERVER  "pool.ntp.org"

#define REQUEST_EVERY 1200 // request time 20 minutes (1200 seconds)
#define REFRESH_TIMESTRING_EVERY 30 // refresh time string every 30 seconds
#define IGNORE_SYNC_ERROR_FOR 86400 // ignore sync error for 24h (24 * 3600)
#define PHOTORESISTOR_LOW 40
#define PHOTORESISTOR_HIGH 900

#define LED_PIN D5
#define NB_LEDS 104
#define STATUS_LED 2
#define BRIGHTNESS_FILTER_SIZE 127
#define FASTLED_REFRESH 100 // refresh display every 100 ms
#define POWER_SUPPLY_I_MAX 2000 // maximum intensity from power supply in mA, 500mA if powering from computer USB
const int MAX_BRIGHTNESS = constrain(floor(255.0 / ((NB_LEDS * 30.0) / POWER_SUPPLY_I_MAX)), 1, 255);
const int MIN_BRIGHTNESS = 2;



ESP8266WebServer server(80);

CRGB leds[NB_LEDS];
bool ledState[NB_LEDS];
CRGB color = CRGB::White;
String paletteName = "";
CRGBPalette16 palette;
uint8_t colorIndex = 0;
int brightnessLevel = MAX_BRIGHTNESS / 2;
int brightnessLevelTarget = brightnessLevel;
bool brightnessAuto = false;
MedianFilter brightnessFilter(BRIGHTNESS_FILTER_SIZE, brightnessLevel);

String zone = "Europe";
String city = "Paris";
String tzInfo = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
tm timeinfo;
time_t now;
long unsigned lastNTPtime;
unsigned long lastEntryTime;
unsigned int refreshTimeStringTimer;
String timeString;
int32_t lastTimeSync = 0;


/**************************************************************/
/****        Server Utils                              ********/
/**************************************************************/
String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}


bool handleFileRead(String path) {
  #ifdef SERIAL_DEBUG
  SERIAL_DEBUG.println("handleFileRead: " + path);
  #endif
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz)) {
      path += ".gz";
    }
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}


void handleRoot() {
  if (!handleFileRead("/index.html")) {
    server.send(404, "text/plain", "FileNotFound");
  }
}


void sendConfigData() {
  StaticJsonDocument<800> doc;
  doc["color"] = crgbToHtmlString(color);
  doc["palette"] = paletteName;
  JsonArray paletteValues = doc.createNestedArray("palette_values");
  for(int i = 0; i < 16; i++) {
    paletteValues.add(crgbToHtmlString(palette[i]));
  }
  JsonObject brightnessObj = doc.createNestedObject("brightness");
  brightnessObj["auto"] = brightnessAuto;
  brightnessObj["level"] = map(brightnessLevelTarget, MIN_BRIGHTNESS, MAX_BRIGHTNESS, 0, 100);
  JsonObject locationObj = doc.createNestedObject("location");
  locationObj["zone"] = zone;
  locationObj["city"] = city;
  String data_str;
  serializeJson(doc, data_str);
  File f = SPIFFS.open(CONFIG_FILE, "w");
  if(f) {
    serializeJson(doc, f);
    #ifdef SERIAL_DEBUG
    SERIAL_DEBUG.println("Saving config file : ");
    SERIAL_DEBUG.println(data_str);
    #endif
  }
  f.close();
  server.send(200, "text/plain", data_str);
}


void handleConfig() {
  #ifdef SERIAL_DEBUG
  /*SERIAL_DEBUG.println("handle config");
  String message = "Number of args received:";
  message += server.args();            //Get number of parameters
  message += "\n";                            //Add a new line
  for (int i = 0; i < server.args(); i++) {
    message += "Arg nº" + (String)i + " –> ";   //Include the current iteration value
    message += server.argName(i) + ": ";     //Get the name of the parameter
    message += server.arg(i) + "\n";              //Get the value of the parameter 
  }
  SERIAL_DEBUG.print(message);*/
  #endif
  if(server.hasArg("color")) {
    color = strtol(server.arg("color").substring(1).c_str(), NULL, 16);
  }
  if(server.hasArg("use_palette") && server.hasArg("gradient_style")) {
    String paletteStr = server.arg("gradient_style");
    paletteName = paletteStr.substring(0, paletteStr.indexOf(':'));
    paletteStr = paletteStr.substring(paletteStr.indexOf('[') + 1);
    for(int c = 0; c < 16; c++) {
      String colorString = paletteStr.substring(0, paletteStr.indexOf(','));
      palette[c] = strtol(colorString.substring(1).c_str(), NULL, 16);
      paletteStr = paletteStr.substring(paletteStr.indexOf(',') + 1);
    }
    #ifdef SERIAL_DEBUG
    /*SERIAL_DEBUG.print("new palette : ");
    SERIAL_DEBUG.println(paletteName);
    for(int c = 0; c < 16; c++) {
      SERIAL_DEBUG.print(crgbToHtmlString(palette[c]) + ", ");
    }
    SERIAL_DEBUG.println();*/
    #endif
  }
  else {
    paletteName = "";
  }
  brightnessAuto = server.hasArg("brightness_auto");
  if(server.hasArg("brightness_level")) {
    brightnessLevelTarget = constrain(map(atoi(server.arg("brightness_level").c_str()),
                              0, 100, MIN_BRIGHTNESS, MAX_BRIGHTNESS), MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  }
  if(server.hasArg("city")) {
    city = server.arg("city");
    if(server.hasArg("zone")) {
      zone = server.arg("zone");
    }
    updateTzInfo();
    
    #ifdef SERIAL_DEBUG
    SERIAL_DEBUG.println("Change location to " + zone + "/" + city +
      ", TZ_INFO = " + tzInfo);
    #endif
    setenv("TZ", tzInfo.c_str(), 1);
    getNTPtime(10);
    #ifdef SERIAL_DEBUG
    SERIAL_DEBUG.printf("it is %02d:%02d:%02d at %s\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, city.c_str());
    #endif
    updateTimeString();
  }
  
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}


String crgbToHtmlString(CRGB color) {
  char buf[8];
  sprintf(buf, "#%02x%02x%02x", color.r, color.g, color.b);
  return String(buf);
}



/**************************************************************/
/****        Main functions                            ********/
/**************************************************************/
void setup() {
  #ifdef SERIAL_DEBUG
  Serial.begin(115200);
  #endif
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  Serial.println();

  timeString.reserve(64);

  SPIFFS.begin();
  // Parse stored values ///////////////////////////////////////////////////////////
  File f = SPIFFS.open(CONFIG_FILE, "r");
  if (!f) {
    #ifdef SERIAL_DEBUG
      SERIAL_DEBUG.println("file open failed");
    #endif
  }
  else {
    StaticJsonDocument<800> doc;
    deserializeJson(doc, f);
    // parse solid color //////////////////////////////////////////////////////////////
    String colorConfig = doc["color"];
    if(colorConfig)
      color = strtol(colorConfig.substring(1).c_str(), NULL, 16);
    // parse palette name (empty if not used) /////////////////////////////////////////
    String paletteNameConfig = doc["palette"];
    if(paletteNameConfig) paletteName = paletteNameConfig;
    // parse palette colors ///////////////////////////////////////////////////////////
    if(paletteName != "") {
      JsonArray paletteArrayConfig = doc["palette_values"];
      for(int i = 0; i < 16; i++) {
        String paletteColorConfig = paletteArrayConfig[i];
        if(paletteColorConfig)
          palette[i] = strtol(paletteColorConfig.substring(1).c_str(), NULL, 16);
      }
    }
    // parse brightness //////////////////////////////////////////////////////////////
    JsonObject brightnessConfig = doc["brightness"];
    brightnessAuto = brightnessConfig["auto"];
    int brightnessLevelConfig = brightnessConfig["level"];
    if(brightnessLevelConfig > 0) {
      brightnessLevel = constrain(map(brightnessLevelConfig, 0, 100, MIN_BRIGHTNESS, MAX_BRIGHTNESS),
                                  MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    }
    // parse location ///////////////////////////////////////////////////////////////
    JsonObject locationConfig = doc["location"];
    if(locationConfig) {
      String zoneConfig = locationConfig["zone"];
      if(zoneConfig) zone = zoneConfig;
      String cityConfig = locationConfig["city"];
      if(cityConfig) city = cityConfig;
      updateTzInfo();
    }
    f.close();
    #ifdef SERIAL_DEBUG
      serializeJsonPretty(doc, SERIAL_DEBUG);
    #endif
  }

  if(brightnessAuto) {
    for(int i = 0; i < BRIGHTNESS_FILTER_SIZE; i++) {
      brightnessFilter.in(constrain(map(analogRead(A0), PHOTORESISTOR_LOW, PHOTORESISTOR_HIGH, MIN_BRIGHTNESS, MAX_BRIGHTNESS),
                                MIN_BRIGHTNESS, MAX_BRIGHTNESS));
    }
    brightnessLevel = brightnessFilter.out();
  }
  brightnessLevelTarget = brightnessLevel;
  
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NB_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(brightnessLevel);
  fill_solid(leds, NB_LEDS, CRGB::Black);
  leds[STATUS_LED] = CRGB::Red;
  FastLED.show();
  FastLED.delay(500);

  WiFiManager wifiManager;
  wifiManager.autoConnect("WordClockAP");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/", HTTP_POST, handleConfig);
  server.on("/config_data.json", sendConfigData);
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });
  server.begin();
  #ifdef SERIAL_DEBUG
  SERIAL_DEBUG.println(F("HTTP server started"));
  #endif

  refreshTimeStringTimer = 1;

  leds[STATUS_LED] = CRGB::Green;
  FastLED.show();
  FastLED.delay(500);

  displayLocalIp();

  configTime(0, 0, NTP_SERVER);
  setenv("TZ", tzInfo.c_str(), 1);
  
  if(getNTPtime(10)) {
    #ifdef SERIAL_DEBUG
    SERIAL_DEBUG.printf("it is %02d:%02d:%02d at %s\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, city.c_str());
    #endif
    updateTimeString();
  }
  else {
    Serial.println("Time not set");
    ESP.restart();
  }
  lastNTPtime = time(&now);
  lastEntryTime = millis();
  refreshTimeStringTimer = millis();

  // switch off on board blue LED
  digitalWrite(LED_BUILTIN, HIGH);
}




void loop() {
  server.handleClient();

  getTimeReducedTraffic(REQUEST_EVERY);

  if(brightnessAuto) {
    brightnessFilter.in(constrain(map(analogRead(A0), PHOTORESISTOR_LOW, PHOTORESISTOR_HIGH, MIN_BRIGHTNESS, MAX_BRIGHTNESS),
                                MIN_BRIGHTNESS, MAX_BRIGHTNESS));
    brightnessLevelTarget = brightnessFilter.out();
  }  
  if(brightnessLevel != brightnessLevelTarget) {
    //Serial.printf("current = %i, target = %i\n", brightnessLevel, brightnessLevelTarget);
    brightnessLevel = brightnessLevelTarget;
    FastLED.setBrightness(brightnessLevel);
  }

  if((millis() - refreshTimeStringTimer) / 1000 >= REFRESH_TIMESTRING_EVERY) {
    refreshTimeStringTimer = millis();
    #ifdef SERIAL_DEBUG
    SERIAL_DEBUG.printf("it is %02d:%02d:%02d at %s\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, city.c_str());
    #endif
    updateTimeString();
  }

  colorIndex++;
  updateLedArray();
  
  FastLED.show();
  FastLED.delay(FASTLED_REFRESH);
}



/**************************************************************/
/****        Time sync functions                       ********/
/**************************************************************/
bool getNTPtime(int sec) {

  {
    uint32_t start = millis();
    do {
      time(&now);
      localtime_r(&now, &timeinfo);
      delay(10);
    } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
    if (timeinfo.tm_year <= (2016 - 1900)) return false;  // the NTP call was not successful
    char time_output[30];
    strftime(time_output, 30, "%a  %d-%m-%Y %T", localtime(&now));
    Serial.println(time_output);
    Serial.println();
  }
  return true;
}


void getTimeReducedTraffic(int sec) {
  tm *ptm;
  if ((millis() - lastEntryTime) < (1000 * sec)) {
    now = lastNTPtime + (int)(millis() - lastEntryTime) / 1000;
  } else {
    leds[STATUS_LED] = CRGB::White;
    FastLED.show();
    lastEntryTime = millis();
    lastNTPtime = time(&now);
    now = lastNTPtime;
    Serial.println("Get NTP time");
  }
  ptm = localtime(&now);
  timeinfo = *ptm;
}

bool requestTime() {
  getTimeReducedTraffic(REQUEST_EVERY);
}

void updateTzInfo() {
  File f = SPIFFS.open("/timezones.json", "r");
  if (!f) {
    #ifdef SERIAL_DEBUG
      SERIAL_DEBUG.println("/timezones.json file open failed");
    #endif
  }
  else {
    StaticJsonDocument<200> filter;
    filter[zone][city] = true;
    StaticJsonDocument<400> doc;
    DeserializationError error = deserializeJson(doc, f, DeserializationOption::Filter(filter));
    if (error) {
      #ifdef SERIAL_DEBUG
      SERIAL_DEBUG.println(F("Failed to read file, using default configuration"));
      #endif
    }
    else {
      String tzInfoStr = doc[zone][city];
      if(tzInfoStr) tzInfo = tzInfoStr;
    }
  }
  f.close();
}



/**************************************************************/
/****        display functions                         ********/
/**************************************************************/
void updateTimeString() {
  timeString = "il est ";
  int h = timeinfo.tm_hour;
  if(timeinfo.tm_min + timeinfo.tm_sec / 30 > 32)
    h++;
  switch(h) {
    case 0:
    case 24:
      timeString += "minuit ";
      break;
    case 1:
    case 13:
      timeString += "une heure ";
      break;
    case 2:
    case 14:
      timeString += "deux heures ";
      break;
    case 3:
    case 15:
      timeString += "trois heures ";
      break;
    case 4:
    case 16:
      timeString += "quatre heures ";
      break;
    case 5:
    case 17:
      timeString += "cinq heures ";
      break;
    case 6:
    case 18:
      timeString += "six heures ";
      break;
    case 7:
    case 19:
      timeString += "sept heures ";
      break;
    case 8:
    case 20:
      timeString += "huit heures ";
      break;
    case 9:
    case 21:
      timeString += "neuf heures ";
      break;
    case 10:
    case 22:
      timeString += "dix heures ";
      break;
    case 11:
    case 23:
      timeString += "onze heures ";
      break;
    case 12:
      timeString += "midi ";
      break;
    default:
      break;
  }
  
  if(timeinfo.tm_min + timeinfo.tm_sec / 30 > 32 && timeinfo.tm_min + timeinfo.tm_sec / 30 < 58) 
    timeString += "moins ";
    
  switch((timeinfo.tm_min + 2 + timeinfo.tm_sec / 30) / 5) {
    case 0: // between 0 and 2 minutes
      break;
    case 1: // between 3 and 7 minutes
    case 11: // between 53 and 57 minutes
      timeString += "cinq";
      break;
    case 2: // between 8 and 12 minutes
    case 10: // between 48 and 52 minutes
      timeString += "dix";
      break;
    case 3: // between 13 and 17 minutes
      timeString += "et quart";
      break;
    case 4: // between 18 and 22 minutes
    case 8: // between 38 and 42 minutes
      timeString += "vingt";
      break;
    case 5: // between 23 and 27 minutes
    case 7: // between 33 and 37 minutes
      timeString += "vingt cinq";
      break;
    case 6: // between 28 and 32 minutes
    #if (VERSION == 0)
      timeString += "et demie";
    #else
      timeString += "demie et"; // invert 'demie' and 'et' because 'et' is on the lower row
    #endif
      break;
    case 9: // between 43 and 47 minutes
      timeString += "le quart";
      break;
    case 12: // 58 minutes and above
      break;
    default:
      break;
  }
  Serial.println(timeString);
  // update boolean array with new words
  for(int i = 0; i < NB_LEDS; i++)
    ledState[i] = false;
    
  int lastIndex = 0;
  while(timeString.length() > 0) {
    // get first word
    String w = timeString.substring(0, timeString.indexOf(' '));
    // get word index
    int wordIndex = getStringIndex(w, lastIndex);
    if(wordIndex >= 0) {
      lastIndex = wordIndex + w.length();
      // update led states
      for(int i = 0; i < w.length(); i++) {
        ledState[wordIndex + i] = true;
      }
    }
    // remove first word and space
    timeString = timeString.substring(w.length());
    timeString.trim();
  }
}


void updateLedArray() {
  // reset display
  fill_solid(leds, NB_LEDS, CRGB::Black);
  CRGB toApply = paletteName == "" ? color : ColorFromPalette(palette, colorIndex);
  // fill led array with color value
  fill_solid(leds, NB_LEDS, toApply);
  // display time string,
  // light off unused letters
  for(int i = 0; i < NB_LEDS; i++) {
    if(!ledState[i]) {
      leds[ledIndex(i)] = CRGB::Black;
    }
  }
}


void displayLocalIp() {
    // reset display
  fill_solid(leds, NB_LEDS, CRGB::Black);
  IPAddress ip = WiFi.localIP();
  for(int i = 0; i < 4; i++) {
    for(int i = 0; i < NB_LEDS; i++)
      ledState[i] = false;
    uint8_t val = ip[i];
    uint8_t digits[3] = {val / 100, (val % 100) / 10, val % 10};
    int offset = 0;
    for(int d : digits) {
      if(offset > 5 || d > 0 || digits[0] > 0)
        displayDigitOnLedArray(d, ledState, offset);
      offset += 5;
    }
    for(int i = 0; i < NB_LEDS; i++) {
      if(ledState[i]) {
        leds[ledIndex(i)] = color;
      }
    }
    FastLED.show();
    FastLED.delay(1750);
    fill_solid(leds, NB_LEDS, CRGB::Black);
    FastLED.show();
    FastLED.delay(250);
  }
}

#ifdef SERIAL_DEBUG
void printDebugTable() {
  for(int i = 0; i < NB_LEDS; i++) {
    if(ledState[i])
      SERIAL_DEBUG.print(getChar(i));
    else
      SERIAL_DEBUG.print('.');
      
    if(i % LETTER_PER_LINE == LETTER_PER_LINE - 1)
      SERIAL_DEBUG.println();
  }
}
#endif
