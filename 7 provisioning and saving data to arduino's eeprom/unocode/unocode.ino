#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 8
SoftwareSerial espSerial(5, 6);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

String receivedString;
boolean sendTemperatureFlag;
void setup() {
  receivedString = "";
  sendTemperatureFlag = false;
  Serial.begin(9600);
  espSerial.begin(9600);
  sensors.begin();
}

void loop() {
  int i = 0;
  while (!sendTemperatureFlag && espSerial.available() > 0) {
    ++i;
    char receivedChar = espSerial.read();
    if(receivedChar == '<') {
      receivedString = "";
    }
    else if(receivedChar == '>') {
      Serial.print("I get: \"");
      Serial.println(receivedString);
      Serial.println("\"");
      if(receivedString == "ready") {
        sendTemperatureFlag = true;
      }
      receivedString = "";
    }
    else {
      receivedString += receivedChar;
    }
  }
  if(sendTemperatureFlag) {
    sensors.requestTemperatures(); 
    double readTemperature = sensors.getTempCByIndex(0);
    Serial.print("Read temperature in Celcius: ");
    Serial.println(readTemperature); 
    Serial.println(("<" + String(readTemperature) + ">").c_str());
    espSerial.write(("<" + String(readTemperature) + ">").c_str());
  }
  delay(1000);
}