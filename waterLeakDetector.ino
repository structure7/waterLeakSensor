#include <SimpleTimer.h>
#define BLYNK_PRINT Serial      // Comment this out to disable prints and save space
#include <BlynkSimpleEsp8266.h>

#include <TimeLib.h>            // Used by WidgetRTC.h
#include <WidgetRTC.h>          // Blynk's RTC

#include <ESP8266mDNS.h>        // Required for OTA
#include <WiFiUdp.h>            // Required for OTA
#include <ArduinoOTA.h>         // Required for OTA

#include <PubSubClient.h>       // Required for MQTT
#include <ESP8266WiFi.h>

const char* auth = "fromBlynkApp";
const char* ssid = "ssid";
const char* pass = "pw";
const char* mqtt_server = "server";

WiFiClient espClient;
PubSubClient client(espClient);
char msg[50];

SimpleTimer timer;

WidgetTerminal terminal(V101);
WidgetRTC rtc;

bool alarmFlag = 0;         // Holds terminal notification until rtc is set. Text notification doesn't wait.
String currentTimeDate;
bool flag1, flag2;          // 60s and 5m notification lab

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);

  client.setServer(mqtt_server, 1883);

  //WiFi.softAPdisconnect(true); // Per https://github.com/esp8266/Arduino/issues/676 this turns off AP

  while (Blynk.connect() == false) {
    // Wait until connected
  }

  // START OTA ROUTINE
  ArduinoOTA.setHostname("LeakDetect-KitchenSink");

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

  timer.setInterval(2000L, setTimeDate);
  timer.setTimeout(1000, mqttPub);

  Blynk.notify("Water detected under kitchen sink!");
}

void loop()
{
  Blynk.run();
  timer.run();
  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (millis() > 60000 && flag1 == 0)
  {
    Blynk.notify("After 60 seconds, water still under kitchen sink!");
    terminal.println(" ");
    terminal.println(String(currentTimeDate) + ": After 60 seconds, water still under kitchen sink!");
    terminal.println(" ");
    terminal.flush();
    flag1 = 1;
  }


  if (millis() > 300000 && flag2 == 0)
  {
    Blynk.notify("After 5 minutes, water still in Master Bathroom!");
    terminal.println(" ");
    terminal.println(String(currentTimeDate) + ": After 5 minutes, water still under kitchen sink!");
    terminal.println(" ");
    terminal.flush();
    flag2 = 1;
  }
}

void setTimeDate() {
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
    terminal.println(String(currentTimeDate) + ": Water detected under kitchen sink!");
    terminal.println(" ");
    terminal.flush();
  }
}

void mqttPub() {
  String mailNotify = String("true");
  mailNotify.toCharArray(msg, 50);
  client.publish("house/leaks/kitchen/sink", msg);
  Serial.println("MQTT msg sent");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
