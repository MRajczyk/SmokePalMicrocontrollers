#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define EEPROM_SIZE 256
#define STR_LEN 128

/****** WEBSERVER STATIC HTML *******/
String responseHTML = ""
                      "<!DOCTYPE html><html lang='en'><head>"
                      "<meta name='viewport' content='width=device-width'>"
                      "<title>SmokePal WiFi configuration</title></head><body>"
                      "<h1>SmokePal Wi-Fi configuration</h1><p>Please enter credentials to wifi network bound to be used with SmokePal system</p>"
                      "<div>"
                      "<form action=\"/save\" method=\"POST\">"
                        "<input type=\"text\" name=\"ssid\" placeholder=\"ssid\"><br />"
                        "<input type=\"password\" name=\"password\" placeholder=\"password\"><br />"
                        "<input type=\"submit\" value=\"save\">"
                      "</form>"
                      "</div>"
                      "</body></html>";

String responseCredentialsInputHTML = ""
                                      "<!DOCTYPE html><html lang='en'><head>"
                                      "<meta name='viewport' content='width=device-width'>"
                                      "<title>SmokePal WiFi configuration</title></head><body>"
                                      "<p>Thanks for providing the data. The device will now restart and try to connect to wifi network using passed credentials. If it fails, it will enter AP mode again.</p>"
                                      "</body></html>";

String responseBadSsidInputHTML = ""
                                      "<!DOCTYPE html><html lang='en'><head>"
                                      "<meta name='viewport' content='width=device-width'>"
                                      "<title>SmokePal WiFi configuration</title></head><body>"
                                      "<p>The ssid can't be blank</p>"
                                      "<a href=\"/\">Go back</a>"
                                      "</body></html>";

/****** CONST VARIABLES *******/
const byte DNS_PORT = 53;
IPAddress apIP(172, 217, 28, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

/****** SOFTWARE SERIAL PINOUT *******/
//rx, tx pins
SoftwareSerial unoSerial(5, 4);
String receivedString;

/****** WiFi Connection Details *******/
char eeprom_ssid[STR_LEN] = {0};
char eeprom_password[STR_LEN] = {0};

/****** MQTT Broker Connection Details *******/
IPAddress mqtt_server = {192, 168, 100, 3};
const char* mqtt_username = "esp8266";
const char* mqtt_password = "password";
const int mqtt_port = 1883;

/****** WiFi Connectivity Initialisation ******/
WiFiClient espClient;

/****** MQTT Client Initialisation Using WiFi Connection ******/
PubSubClient client(espClient);

/****** Form handling method ******/
void handle_form() {
  if(!webServer.arg("ssid") || webServer.arg("ssid").length() == 0) {
    webServer.send(200, "text/html", responseBadSsidInputHTML);
  }

  String wifi_ssid = "";
  String wifi_password = "";

  if (webServer.hasArg("ssid")) {
    wifi_ssid = webServer.arg("ssid");
  }
  if (webServer.hasArg("password")) {
    wifi_password = webServer.arg("password");
  }
  webServer.send(200, "text/html", responseCredentialsInputHTML);
  
  //save wifi credentials to eeprom memory in order to preserve it on restart
  wifi_ssid.toCharArray(eeprom_ssid, STR_LEN);
  wifi_password.toCharArray(eeprom_password, STR_LEN);

  EEPROM.put(0, eeprom_ssid);
  EEPROM.put(128, eeprom_password);
  EEPROM.commit();
  EEPROM.end();
  delay(1000);
  Serial.println("Data saved to eeprom. Restarting...");
  ESP.restart();
}

/****** Function that converts array of numbers to String literal (in this use case it's a MAC Address) ******/
String macToStr(const uint8_t* mac) {
  String result;

  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5){
      result += ':';
    }
  }

  return result;
}

/****** WiFi setup function ******/
void setup_wifi() {
  Serial.println("Entering wifi setup...");
  delay(10);
  //try to connect to wifi with credentials stored in flash
  EEPROM.get(0, eeprom_ssid);
  EEPROM.get(128, eeprom_password);
  WiFi.mode(WIFI_STA);
  WiFi.begin(String(eeprom_ssid).c_str(), String(eeprom_password).c_str());

  // wait 20 seconds to connect to WiFi
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter < 40) {
    delay(500);
    Serial.print(".");
    ++counter;
  }
  if(counter == 40) {
    Serial.println("20 seconds have passed. Entering AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("SmokePal_wifi_setup");

    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
    dnsServer.start(DNS_PORT, "*", apIP);

    webServer.on("/", []() {
      webServer.send(200, "text/html", responseHTML);
    });

    webServer.on("/save", HTTP_POST, handle_form);

    // reply to all requests with same HTML
    webServer.onNotFound([]() {
      webServer.send(200, "text/html", responseHTML);
    });
    webServer.begin();

    while(true) {
      dnsServer.processNextRequest();
      webServer.handleClient();
    }
  }
  randomSeed(micros());
}

/****** Function handling connection to MQTT broker ******/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    String clientName = "esp8266-";
    uint8_t mac[6];
    WiFi.macAddress(mac);
    clientName += macToStr(mac);
    if (client.connect(clientName.c_str(), mqtt_username, mqtt_password)) {
      unoSerial.write("<ready>");
    } else {
      delay(5000);
    }
  }
}

/****** Debug function to read incoming MQTT data ******/
void callback(char* topic, byte* payload, unsigned int length) {
  // String incomingMessage = "";
  // for (int i = 0; i < length; i++) {
  //   incomingMessage += (char)payload[i];
  // }
  // Serial.println("Message arrived ["+String(topic)+"]"+incomingMessage);
}

/****** Method for Publishing MQTT Messages ******/
void publishMessage(const char* topic, String payload , boolean retained){
  if (client.publish(topic, payload.c_str(), true)) {
      //Serial.println("Message published ["+String(topic)+"]: "+payload);
  }
}

void setup() {
  // put your setup code here, to run once:
  receivedString = "";
  unoSerial.begin(9600);
  // Serial for debugging
  Serial.begin(9600);
  while (!unoSerial && !Serial) {
    delay(1);
  }
  delay(1000);
  Serial.println("Startup init");
  EEPROM.begin(EEPROM_SIZE);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  // check if client is connected
  if (!client.connected()) {
    reconnect(); 
  }
  client.loop();
  //read data from arduino and pack it as json
  while (unoSerial.available() > 0) {
    char receivedChar = unoSerial.read();
    if(receivedChar == '<') {
      receivedString = "";
    }
    else if(receivedChar == '>') {
      int ind1 = receivedString.indexOf(';');
      String temp1 = receivedString.substring(0, ind1);
      int ind2 = receivedString.indexOf(';', ind1 + 1 ); 
      String temp2 = receivedString.substring(ind1 + 1, ind2 + 1);
      int ind3 = receivedString.indexOf(';', ind2 + 1 );
      String temp3 = receivedString.substring(ind2 + 1, ind3 + 1);
      int ind4 = receivedString.indexOf(';', ind3 + 1 );
      String hum1 = receivedString.substring(ind3 + 1);

      DynamicJsonDocument jsonRoot(1024);

      DynamicJsonDocument readingTemp1(128);
      readingTemp1.to<JsonObject>();
      readingTemp1["sensorName"] = "Temp1";
      readingTemp1["reading"] = temp1;
      readingTemp1["type"] = "TEMP";
      jsonRoot.add(readingTemp1);

      DynamicJsonDocument readingTemp2(128);
      readingTemp2.to<JsonObject>();
      readingTemp2["sensorName"] = "Temp2";
      readingTemp2["reading"] = temp2;
      readingTemp2["type"] = "TEMP";
      jsonRoot.add(readingTemp2);
      
      DynamicJsonDocument readingTemp3(128);
      readingTemp3.to<JsonObject>();
      readingTemp3["sensorName"] = "Temp3";
      readingTemp3["reading"] = temp3;
      readingTemp3["type"] = "TEMP";
      jsonRoot.add(readingTemp3);
      
      DynamicJsonDocument readingHum1(128);
      readingHum1.to<JsonObject>();
      readingHum1["sensorName"] = "Hum1";
      readingHum1["reading"] = hum1;
      readingHum1["type"] = "HUM";
      jsonRoot.add(readingHum1);

      char mqtt_message[1024];
      serializeJson(jsonRoot, mqtt_message);

      publishMessage("esp8266_data", mqtt_message, true);
      receivedString = "";
    }
    else {
      receivedString += receivedChar;
    }
  }
}
