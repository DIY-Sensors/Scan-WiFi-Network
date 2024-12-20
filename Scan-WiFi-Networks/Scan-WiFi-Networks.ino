#include "ESP8266WiFi.h"
#include <PubSubClient.h>
#include <ESPping.h> // for ping test
#include "arduino_secrets.h"
// only needed for the OLED display
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "Icons.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

unsigned long lTime_Strength = 0;
unsigned long lTime_scan = 0;
unsigned long lTime_ping = 0;
unsigned long lTime_display = 0;
unsigned long lTime = 0;
const long lInterval_Strength = 5000; // 5 second
const long lInterval_scan = 60000; // 60 seconds
const long lInterval_ping = 60000; // 60 seconds
const long lInterval_display = 5000; // 5 second

int iWiFi_RSSI = -100;
byte bInternet = false;

int iWiFiTry = 0;          
int iMQTTTry = 0;
String sClient_id;

const char* ssid = YourSSID;
const char* password = YourWiFiPassWord;
const char* HostName = "WiFi-Scanner";  // make this unique!

const char* mqtt_broker = YourMQTTserver;
const char* mqtt_user = YourMQTTuser;
const char* mqtt_password = YourMQTTpassword;

WiFiClient espClient;
PubSubClient MQTTclient(espClient); // MQTT Client

void Connect2WiFi() { 
  //Connect to WiFi
  // WiFi.mode(WIFI_STA);  //in case of an ESP32
  iWiFiTry = 0;
  WiFi.begin(ssid, password);
  WiFi.setHostname(HostName);
  Serial.print("Connecting to WiFi ");
  while (WiFi.status() != WL_CONNECTED && iWiFiTry < 11) { //Try to connect to WiFi for 11 times
    ++iWiFiTry;
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  // ArduinoOTA.setPort(8266); // Port defaults to 8266
  Serial.println("Ready");

  //Unique MQTT Device name
  sClient_id = "esp-client-" + String(WiFi.macAddress());
  Serial.print("ESP Client name: "); Serial.println(sClient_id);
}

void Connect2MQTT() {
  // Connect to the MQTT server
  iMQTTTry=0;
  if (WiFi.status() != WL_CONNECTED) { 
    Connect2WiFi; 
  }

  Serial.print("Connecting to MQTT ");
  MQTTclient.setServer(mqtt_broker, 1883);
  while (!MQTTclient.connect(sClient_id.c_str(), mqtt_user, mqtt_password) && iMQTTTry < 11) { //Try to connect to MQTT for 11 times
    ++iMQTTTry;
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
}

void setup() {
  Serial.begin(115200);

  lTime_scan = millis();

  // setup OLED Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Set WiFi to station 
  WiFi.mode(WIFI_STA);
  Connect2WiFi();
  Connect2MQTT();

  Serial.println("Setup done");
}

void loop() {

  lTime = millis();

  // Signal Strength
  if ( (lTime - lTime_Strength) > lInterval_Strength) { 
    if (!MQTTclient.connect(sClient_id.c_str(), mqtt_user, mqtt_password)) { Connect2MQTT(); }
    lTime_Strength = lTime;
    iWiFi_RSSI = WiFi.RSSI();
    MQTTclient.publish("WiFi/Scanner/Signal", String(iWiFi_RSSI).c_str());
    Serial.print("WiFi Scanner Signal Strength: "); Serial.println(iWiFi_RSSI);

    MQTTclient.publish("WiFi/Scanner/IP", WiFi.localIP().toString().c_str());
    Serial.print("WiFi Scanner IP: "); Serial.println( WiFi.localIP());

    MQTTclient.publish("WiFi/Scanner/MAC", String(WiFi.macAddress()).c_str());
    Serial.print("WiFi Scanner MAC address: "); Serial.println(WiFi.macAddress());

    MQTTclient.publish("WiFi/Scanner/Channel", String(WiFi.channel()).c_str());
    Serial.print("WiFi Scanner Channel: "); Serial.println(WiFi.channel());

    Serial.println("");
  } 


  // Scan WiFi networks
  if ( (lTime - lTime_scan) > lInterval_scan) { 
    lTime_scan = lTime;
    WiFi.disconnect(); // Disconnect from an AP if it was previously connected
    delay(1000);
    Serial.println("WiFi disconnected");
    Serial.println("scan start");

    int iChannels[14] = {0,0,0,0,0,0,0,0,0,0,0,0,0.0};
    int iSignals[14] = {-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100};  
    
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0)
      Serial.println("no networks found");
    else
      {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i)
        {
            // Print SSID and RSSI for each network found
          Serial.print(i + 1); Serial.print(": "); Serial.print(WiFi.SSID(i)); Serial.print(" (");
          Serial.print(WiFi.RSSI(i)); Serial.print(")");
          Serial.print(" Channel: "); Serial.print(WiFi.channel(i));
          Serial.print(" Encryption: "); Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
          delay(10);

          iChannels[WiFi.channel(i)]++;
          if(WiFi.RSSI(i+1) > iSignals[WiFi.channel(i+1)]){ iSignals[WiFi.channel(i+1)] = WiFi.RSSI(i+1); }
        }
      }
      Serial.println("End of Scan");

      Connect2WiFi();
      Connect2MQTT();

      MQTTclient.publish("WiFi/Scanner/Channel1", String(iChannels[1]).c_str());
      MQTTclient.publish("WiFi/Scanner/Signal1", String(iSignals[1]).c_str());
      MQTTclient.publish("WiFi/Scanner/Channel2", String(iChannels[2]).c_str());
      MQTTclient.publish("WiFi/Scanner/Signal2", String(iSignals[2]).c_str());
      MQTTclient.publish("WiFi/Scanner/Channel3", String(iChannels[3]).c_str());
      MQTTclient.publish("WiFi/Scanner/Signal3", String(iSignals[3]).c_str());
      MQTTclient.publish("WiFi/Scanner/Channel4", String(iChannels[4]).c_str());
      MQTTclient.publish("WiFi/Scanner/Signal4", String(iSignals[4]).c_str());
      MQTTclient.publish("WiFi/Scanner/Channel5", String(iChannels[5]).c_str());
      MQTTclient.publish("WiFi/Scanner/Signal5", String(iSignals[5]).c_str());
      MQTTclient.publish("WiFi/Scanner/Channel6", String(iChannels[6]).c_str());
      MQTTclient.publish("WiFi/Scanner/Signal6", String(iSignals[6]).c_str());
      MQTTclient.publish("WiFi/Scanner/Channel7", String(iChannels[7]).c_str());
      MQTTclient.publish("WiFi/Scanner/Signal7", String(iSignals[7]).c_str());
      MQTTclient.publish("WiFi/Scanner/Channel8", String(iChannels[8]).c_str());
      MQTTclient.publish("WiFi/Scanner/Signal8", String(iSignals[8]).c_str());
      MQTTclient.publish("WiFi/Scanner/Channel9", String(iChannels[9]).c_str());
      MQTTclient.publish("WiFi/Scanner/Signal9", String(iSignals[9]).c_str());
      MQTTclient.publish("WiFi/Scanner/Channel10", String(iChannels[10]).c_str());
      MQTTclient.publish("WiFi/Scanner/Signal10", String(iSignals[10]).c_str());
      MQTTclient.publish("WiFi/Scanner/Channel11", String(iChannels[11]).c_str());
      MQTTclient.publish("WiFi/Scanner/Signal11", String(iSignals[11]).c_str());
      MQTTclient.publish("WiFi/Scanner/Channel12", String(iChannels[12]).c_str());
      MQTTclient.publish("WiFi/Scanner/Signal12", String(iSignals[12]).c_str());
      MQTTclient.publish("WiFi/Scanner/Channel13", String(iChannels[13]).c_str());
      MQTTclient.publish("WiFi/Scanner/Signal13", String(iSignals[13]).c_str());
  } // End of Scan WiFi networks

  // Ping servers
  if ( (lTime - lTime_ping) > lInterval_ping) { 
    
    String sHost;
    bInternet = false;

    if (!MQTTclient.connect(sClient_id.c_str(), mqtt_user, mqtt_password)) { Connect2MQTT(); } 
    // Ping gateway (router)
    Serial.print("Ping: "); Serial.println(WiFi.gatewayIP());    // Ping gateway (router)
    if (Ping.ping(WiFi.gatewayIP()) > 0){
      MQTTclient.publish("WiFi/Scanner/Ping/GateWay", String(Ping.averageTime()).c_str());
    } else {
      MQTTclient.publish("WiFi/Scanner/Ping/GateWay", String(1000).c_str());
    }    
 
    Serial.print("Ping: "); Serial.println(mqtt_broker);    // Ping MQTT Broker (Home Assistant)
    if (Ping.ping(mqtt_broker) > 0){
      MQTTclient.publish("WiFi/Scanner/Ping/HomeAssistant", String(Ping.averageTime()).c_str());
    } else {
      MQTTclient.publish("WiFi/Scanner/Ping/HomeAssistant", String(1000).c_str());
    }    

    sHost = "google.com";  // Test if the internet is working, can be any stable site.
    Serial.print("Ping: "); Serial.println(sHost);
    if (Ping.ping(sHost.c_str()) > 0){
      MQTTclient.publish("WiFi/Scanner/Ping/Google", String(Ping.averageTime()).c_str());
      bInternet = true;
    } else {
      MQTTclient.publish("WiFi/Scanner/Ping/Google", String(1000).c_str());
    }    
    Serial.println("");

    sHost = "outlook.office.com";  // Test if the internet is working, can be any stable site.
    Serial.print("Ping: "); Serial.println(sHost);
    if (Ping.ping(sHost.c_str()) > 0){
      MQTTclient.publish("WiFi/Scanner/Ping/MS_Outlook", String(Ping.averageTime()).c_str());
      bInternet = true;
    } else {
      MQTTclient.publish("WiFi/Scanner/Ping/MS_Outlook", String(1000).c_str());
    }    
    Serial.println(""); 

    sHost = "youtube.com";  // Test if the internet is working, can be any stable site.
    Serial.print("Ping: "); Serial.println(sHost);
    if (Ping.ping(sHost.c_str()) > 0){
      MQTTclient.publish("WiFi/Scanner/Ping/YouTube", String(Ping.averageTime()).c_str());
      bInternet = true;
    } else {
      MQTTclient.publish("WiFi/Scanner/Ping/YouTube", String(1000).c_str());
    }    
    Serial.println(""); 

    sHost = "ntp.pool.org";    // I sync my time servers with this.
    Serial.print("Ping: "); Serial.println(sHost);
    if (Ping.ping(sHost.c_str()) > 0){
         MQTTclient.publish("WiFi/Scanner/Ping/NTP", String(Ping.averageTime()).c_str());
    } else {
      MQTTclient.publish("WiFi/Scanner/Ping/NTP", String(1000).c_str());
    }    
    Serial.println(""); 

    // Ping IP
  
    IPAddress CloudFlareDNS(1,1,1,1); // Cloudflare's public DNS resolver
    Serial.print("Ping: "); Serial.println(CloudFlareDNS);
    if (Ping.ping(CloudFlareDNS) > 0){
      MQTTclient.publish("WiFi/Scanner/Ping/CloudFlareDNS", String(Ping.averageTime()).c_str());
    } else {
       MQTTclient.publish("WiFi/Scanner/Ping/CloudFlareDNS", String(1000).c_str());
    }
    Serial.println(""); 

    IPAddress Switch1(192,168,50,240); // Cloudflare's public DNS resolver
    Serial.print("Ping: "); Serial.println(Switch1);
    if (Ping.ping(Switch1) > 0){
      MQTTclient.publish("WiFi/Scanner/Ping/Switch1", String(Ping.averageTime()).c_str());
    } else {
       MQTTclient.publish("WiFi/Scanner/Ping/Switch1", String(1000).c_str());
    }
    Serial.println(""); 

    IPAddress Switch2(192,168,50,241); // Cloudflare's public DNS resolver
    Serial.print("Ping: "); Serial.println(Switch2);
    if (Ping.ping(Switch2) > 0){
      MQTTclient.publish("WiFi/Scanner/Ping/Switch2", String(Ping.averageTime()).c_str());
    } else {
       MQTTclient.publish("WiFi/Scanner/Ping/Switch2", String(1000).c_str());
    }
    Serial.println(""); 

    const IPAddress Switch3(192,168,50,242); // Cloudflare's public DNS resolver
    Serial.print("Ping: "); Serial.println(Switch3);
    if (Ping.ping(Switch3) > 0){
      MQTTclient.publish("WiFi/Scanner/Ping/Switch3", String(Ping.averageTime()).c_str());
    } else {
       MQTTclient.publish("WiFi/Scanner/Ping/Switch3", String(1000).c_str());
    }
    Serial.println(""); 

    const IPAddress WiFi_AP1(192,168,50,178); // Cloudflare's public DNS resolver
    Serial.print("Ping: "); Serial.println(WiFi_AP1);
    if (Ping.ping(WiFi_AP1) > 0){
      MQTTclient.publish("WiFi/Scanner/Ping/WiFi_AP1", String(Ping.averageTime()).c_str());
    } else {
       MQTTclient.publish("WiFi/Scanner/Ping/WiFi_AP1", String(1000).c_str());
    }
    Serial.println(""); 

    const IPAddress WiFi_AP2(192,168,50,199); // Cloudflare's public DNS resolver
    Serial.print("Ping: "); Serial.println(WiFi_AP2);
    if (Ping.ping(WiFi_AP2) > 0){
      MQTTclient.publish("WiFi/Scanner/Ping/WiFi_AP2", String(Ping.averageTime()).c_str());
    } else {
       MQTTclient.publish("WiFi/Scanner/Ping/WiFi_AP2", String(1000).c_str());
    }
    Serial.println(""); 

    const IPAddress WiFi_AP3(192,168,50,200); // Cloudflare's public DNS resolver
    Serial.print("Ping: "); Serial.println(WiFi_AP3);
    if (Ping.ping(WiFi_AP3) > 0){
      MQTTclient.publish("WiFi/Scanner/Ping/WiFi_AP3", String(Ping.averageTime()).c_str());
    } else {
       MQTTclient.publish("WiFi/Scanner/Ping/WiFi_AP3", String(1000).c_str());
    }
    Serial.println(""); 

    lTime_ping = millis();

  } // End of ping servers

  if ( (lTime - lTime_display) > lInterval_display) { 
      lTime_display = lTime;
      display.clearDisplay(); 
      display.setFont(&FreeMonoBold9pt7b);
      display.setCursor(0,10);
      display.print(" WiFi  Int.");
      
      int iWiFi_Level = 6;
      if (iWiFi_RSSI < -50) { iWiFi_Level = 1; }
      if ((iWiFi_RSSI >= -50) && (iWiFi_RSSI < -60)) { iWiFi_Level = 2; }
      if ((iWiFi_RSSI >= -60) && (iWiFi_RSSI < -70)) { iWiFi_Level = 3; }
      if ((iWiFi_RSSI >= -70) && (iWiFi_RSSI < -80)) { iWiFi_Level = 4; }
      if ((iWiFi_RSSI >= -80) && (iWiFi_RSSI < -85)) { iWiFi_Level = 5; }

      switch (iWiFi_Level) {
        case 6:
          display.drawBitmap(10,20, Icon_sick, 40, 40, WHITE);
          break;
        case 5:
          display.drawBitmap(10,20, Icon_Sad, 40, 40, WHITE);
          break;
        case 4:
          display.drawBitmap(10,20, Icon_not_good, 40, 40, WHITE);
          break;
        case 3:
          display.drawBitmap(10,20, Icon_Neutral, 40, 40, WHITE);
          break;
        case 2:
          display.drawBitmap(10,20, Icon_Happy, 40, 40, WHITE);
          break;
        case 1:
          display.drawBitmap(10,20, Icon_Super_Happy, 40, 40, WHITE);
          break;
      }

      if (bInternet) {
        display.drawBitmap(74,20, Icon_Super_Happy, 40, 40, WHITE);
      } else {
        display.drawBitmap(74,20, Icon_Sad, 40, 40, WHITE);
      }
       display.display();

  }
 

}