#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define EEPROM_SIZE 256
#define STR_LEN 128

//webserver stuff
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

const byte DNS_PORT = 53;
IPAddress apIP(172, 217, 28, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

/****** SOFTWARE SERIAL PINOUT *******/
String receivedString;

/****** WiFi Connection Details *******/
char eeprom_ssid[STR_LEN] = {0};
char eeprom_password[STR_LEN] = {0};

/******* MQTT Broker Connection Details *******/
IPAddress mqtt_server = {192, 168, 100, 3};
const char* mqtt_username = "esp8266";
const char* mqtt_password = "password";
const int mqtt_port = 1883;

/**** WiFi Connectivity Initialisation *****/
WiFiClient espClient;

/**** MQTT Client Initialisation Using WiFi Connection *****/
PubSubClient client(espClient);

void handle_form() {
  if(!webServer.arg( "ssid" ) || webServer.arg( "ssid" ).length() == 0) {
    webServer.send(200, "text/html", responseBadSsidInputHTML);
  }

  String wifi_ssid = "";
  String wifi_password = "";

  if ( webServer.hasArg( "ssid" )) {
    wifi_ssid = webServer.arg( "ssid" );
  }
  if ( webServer.hasArg( "password" )) {
    wifi_password = webServer.arg( "password" );
  }
  
  webServer.send(200, "text/html", responseCredentialsInputHTML);
  
  //writing to eeprom
  wifi_ssid.toCharArray(eeprom_ssid, STR_LEN);
  wifi_password.toCharArray(eeprom_password, STR_LEN);

  EEPROM.put(0, eeprom_ssid);
  EEPROM.put(128, eeprom_password);
  EEPROM.commit();
  EEPROM.end();
  delay(1000);
  ESP.restart();
}

String macToStr(const uint8_t* mac){

  String result;

  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5){
      result += ':';
    }
  }

  return result;
}

void setup_wifi() {
  delay(10);
  Serial.print("\nConnecting to ");

  //try to connect to wifi with credentials stored in flash
  EEPROM.get(0, eeprom_ssid);
  Serial.println(eeprom_ssid);
  EEPROM.get(128, eeprom_password);
  WiFi.mode(WIFI_STA);
  WiFi.begin(String(eeprom_ssid).c_str(), String(eeprom_password).c_str());

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter < 40) {
    delay(500);
    Serial.print(".");
    ++counter;
  }
  if(counter == 40) {
    Serial.println("Failed, starting AP server to get new credentials");
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
  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());
}

/************* Connect to MQTT Broker ***********/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Serial.print("Attempting MQTT connection...");
    String clientName = "esp8266-";
    uint8_t mac[6];
    WiFi.macAddress(mac);
    clientName += macToStr(mac);
    // Serial.print("Client ID : ");
    // Serial.println(clientName);
    // Attempt to connect
    if (client.connect(clientName.c_str(), mqtt_username, mqtt_password)) {
      // Serial.println("connected");
      Serial.write("<ready>");
    } else {
      // Serial.print("failed, rc=");
      // Serial.print(client.state());
      // Serial.println(" try again in 5 seconds");   // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage+=(char)payload[i];

  //Serial.println("Message arrived ["+String(topic)+"]"+incommingMessage);
}

/**** Method for Publishing MQTT Messages **********/
void publishMessage(const char* topic, String payload , boolean retained){
  if (client.publish(topic, payload.c_str(), true)) {
      //Serial.println("Message published ["+String(topic)+"]: "+payload);
  }
}

void setup() {
  // put your setup code here, to run once:
  receivedString = "";
  Serial.begin(9600);
  while (!Serial) delay(1);
  EEPROM.begin(EEPROM_SIZE);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect(); // check if client is connected
  client.loop();
  //read temperature
  while (Serial.available() > 0) {
    char receivedChar = Serial.read();
    if(receivedChar == '<') {
      receivedString = "";
    }
    else if(receivedChar == '>') {
      DynamicJsonDocument doc(1024);

      doc["deviceId"] = "NodeMCU";
      doc["temperature"] = receivedString;

      char mqtt_message[128];
      serializeJson(doc, mqtt_message);

      publishMessage("esp8266_data", mqtt_message, true);
      receivedString = "";
    }
    else {
      receivedString += receivedChar;
    }
  }
  delay(1000);
}
