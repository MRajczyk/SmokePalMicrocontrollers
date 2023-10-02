#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

/****** SOFTWARE SERIAL PINOUT *******/
String receivedString;

/****** WiFi Connection Details *******/
const char* ssid = "xxxx";
const char* password = "xxxx";

/******* MQTT Broker Connection Details *******/
IPAddress mqtt_server = {192, 168, 100, 3};
const char* mqtt_username = "esp8266";
const char* mqtt_password = "password";
const int mqtt_port = 1883;

/**** WiFi Connectivity Initialisation *****/
WiFiClient espClient;

/**** MQTT Client Initialisation Using WiFi Connection *****/
PubSubClient client(espClient);

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
  // Serial.print("\nConnecting to ");
  // Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
  }
  randomSeed(micros());
  // Serial.println("\nWiFi connected\nIP address: ");
  // Serial.println(WiFi.localIP());
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
      // Serial.write("<ready>");
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
