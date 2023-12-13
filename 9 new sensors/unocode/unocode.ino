#include <dht.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS_TEMP1 8
#define ONE_WIRE_BUS_TEMP2 9
#define DHT_PIN 10
//rx, tx pins
SoftwareSerial espSerial(5, 6);

OneWire oneWireTemp1(ONE_WIRE_BUS_TEMP1);
DallasTemperature temp1Sensor(&oneWireTemp1);

OneWire oneWireTemp2(ONE_WIRE_BUS_TEMP2);
DallasTemperature temp2Sensor(&oneWireTemp2);

dht DHT; // Creats a DHT object

String receivedString;
boolean sendDataFlag;

void setup() {
  receivedString = "";
  sendDataFlag = false;
  Serial.begin(9600);
  espSerial.begin(9600);
  temp1Sensor.begin();
  temp2Sensor.begin();
  delay(3000);
}

void loop() {
  int i = 0;
  while (!sendDataFlag && espSerial.available() > 0) {
    ++i;
    char receivedChar = espSerial.read();
    if(receivedChar == '<') {
      receivedString = "";
    }
    else if(receivedChar == '>') {
      Serial.print("I get: \"");
      Serial.print(receivedString);
      Serial.println("\"");
      if(receivedString == "ready") {
        sendDataFlag = true;
      }
      receivedString = "";
    }
    else {
      receivedString += receivedChar;
    }
  }
  if(sendDataFlag) {
    temp1Sensor.requestTemperatures(); 
    temp2Sensor.requestTemperatures(); 
    double readTemperature1 = temp1Sensor.getTempCByIndex(0);
    double readTemperature2 = temp2Sensor.getTempCByIndex(0);
    int readData = DHT.read22(DHT_PIN); 
    float readTemperature3 = DHT.temperature;
    float readHumidity1 = DHT.humidity;

    Serial.println(("<" + String(readTemperature1) + ";" + String(readTemperature2) + ";" + String(readTemperature3) + ";" + String(readHumidity1) + ">").c_str());
    espSerial.write(("<" + String(readTemperature1) + ";" + String(readTemperature2) + ";" + String(readTemperature3) + ";" + String(readHumidity1) + ">").c_str());
  }
  delay(5000);
}