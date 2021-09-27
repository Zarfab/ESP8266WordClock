#include <ESP8266WiFi.h>
#include <time.h>
#include "RTClib.h"


#define STASSID "your-ssid"
#define STAPSK  "your-password"

#define NTP_SERVER  "pool.ntp.org"


RTC_DS1307 rtc;

String tzInfo = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"; // Paris
tm timeinfo;
time_t now;

String timeString;


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  delay(10000);

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(STASSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  configTime(0, 0, NTP_SERVER);
  setenv("TZ", "GMT0", 1);
  if(!getNTPtime(10)) {
    Serial.println("[NTP] Time not set");
    ESP.restart();
  }
  Serial.print("[NTP] "); showTimeFromNTP(timeinfo);

  if (!rtc.begin()) {
    Serial.println("[RTC] Couldn't find RTC");
    ESP.restart();
  }
  DateTime nowdt = rtc.now();
  Serial.printf("[RTC] RTC begun ! (%i)\n", nowdt.unixtime());
  Serial.print("[RTC] "); showTimeFromRTC(nowdt);

  
  Serial.printf("[RTC] Adjust time to UTC : %i\n", now);
  rtc.adjust(DateTime(now));
  Serial.print("[RTC] "); showTimeFromRTC(rtc.now());

  setenv("TZ", tzInfo.c_str(), 1);
  timeString.reserve(64);
}



void loop() {
  now = rtc.now().unixtime();
  timeinfo = *localtime(&now);
  
  Serial.printf("[TIME] it is %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  updateTimeString();
  Serial.println(timeString);
  delay(10000);
}


bool getNTPtime(int sec) {
  uint32_t start = millis();
  do {
    time(&now);
    localtime_r(&now, &timeinfo);
    delay(10);
  } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
  if (timeinfo.tm_year <= (2016 - 1900)) return false;  // the NTP call was not successful
  Serial.print("now ");  Serial.println(now);
  char time_output[30];
  strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now));
  Serial.println(time_output);
  Serial.println();
  return true;
}

void showTimeFromNTP(tm localTime) {
  Serial.print(localTime.tm_mday);
  Serial.print('/');
  Serial.print(localTime.tm_mon + 1);
  Serial.print('/');
  Serial.print(localTime.tm_year - 100);
  Serial.print('-');
  Serial.print(localTime.tm_hour);
  Serial.print(':');
  Serial.print(localTime.tm_min);
  Serial.print(':');
  Serial.print(localTime.tm_sec);
  Serial.println();
}


void showTimeFromRTC(DateTime dt) {
  Serial.print(dt.day(), DEC);
  Serial.print('/');
  Serial.print(dt.month(), DEC);
  Serial.print('/');
  Serial.print(dt.year(), DEC);
  Serial.print('-');
  Serial.print(dt.hour(), DEC);
  Serial.print(':');
  Serial.print(dt.minute(), DEC);
  Serial.print(':');
  Serial.print(dt.second(), DEC);
  Serial.println();
}



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
}
