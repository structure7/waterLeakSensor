#include <SimpleTimer.h>
#define BLYNK_PRINT Serial      // Comment this out to disable prints and save space
#include <BlynkSimpleEsp8266.h>

#include <TimeLib.h>            // Used by WidgetRTC.h
#include <WidgetRTC.h>          // Blynk's RTC

#include <ESP8266mDNS.h>        // Required for OTA
#include <WiFiUdp.h>            // Required for OTA
#include <ArduinoOTA.h>         // Required for OTA

const char auth[] = "fromBlynkApp";
char ssid[] = "ssid";
char pass[] = "pw";

SimpleTimer timer;

WidgetTerminal terminal(V26);
WidgetRTC rtc;

bool alarmFlag = 0;
String currentTimeDate;
bool flag1, flag2;

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);

  //WiFi.softAPdisconnect(true); // Per https://github.com/esp8266/Arduino/issues/676 this turns off AP

  while (Blynk.connect() == false) {
    // Wait until connected
  }

  // START OTA ROUTINE
  ArduinoOTA.setHostname("LeakDetect-GuestBath");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  // END OTA ROUTINE

  rtc.begin();

  timer.setInterval(1000L, pushAlarm);            // Temperature sensor reporting to app display

  Blynk.notify("Water detected guest bath/laundry room!");
}

void loop()
{
  Blynk.run();
  timer.run();
  ArduinoOTA.handle();

  if (millis() > 60000 && flag1 == 0)
  {
    Blynk.notify("After 60 seconds, water still guest bath/laundry room!");
    flag1 = 1;
  }

  
  if (millis() > 300000 && flag2 == 0)
  {
    Blynk.notify("After 5 minutes, water still in guest bath/laundry room!");
    flag2 = 1;
  }
}

void pushAlarm() {
  if (alarmFlag == 0 && year() != 1970) {
    alarmFlag = 1;
    // Below gives me leading zeros on minutes and AM/PM.
    if (minute() > 9 && hour() > 11) {
      currentTimeDate = String(hourFormat12()) + ":" + minute() + "pm on " + month() + "/" + day();
    }
    else if (minute() < 10 && hour() > 11) {
      currentTimeDate = String(hourFormat12()) + ":0" + minute() + "pm on " + month() + "/" + day();
    }
    else if (minute() > 9 && hour() < 12) {
      currentTimeDate = String(hourFormat12()) + ":" + minute() + "am on " + month() + "/" + day();
    }
    else if (minute() < 10 && hour() < 12) {
      currentTimeDate = String(hourFormat12()) + ":0" + minute() + "am on " + month() + "/" + day();
    }

    timer.setTimeout(2000, terminalAlarm);
  }
}

void terminalAlarm() {
  if (alarmFlag == 1) {
    terminal.println(" ");
    terminal.println(String(currentTimeDate) + ": Water detected in guest bath/laundry room!");
    terminal.println(" ");
    terminal.flush();
  }
}
