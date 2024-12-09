#include "ESP8266WiFi.h"
#include <PubSubClient.h>
#include "arduino_secrets.h"

unsigned long lTime_Strength = 0;
unsigned long lTime_scan = 0;
unsigned long lTime = 0;
const long lInterval_Strength = 1000; // 1 second
const long lInterval_scan = 10000; // 10 seconds

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
    MQTTclient.publish("WiFi/Scanner/Signal", String(WiFi.RSSI()).c_str());
    Serial.print("WiFi Scanner Signal Strangth: "); Serial.println(WiFi.RSSI());

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
    delay(100);
    Serial.println("WiFi disconnected");
    Serial.println("scan start");

    int iChannels[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
    int iSignals[13] = {-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100};
    
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
          if(WiFi.RSSI(i) > iSignals[WiFi.channel(i)]){ iSignals[WiFi.channel(i)] = WiFi.RSSI(i); }

          
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
  

}