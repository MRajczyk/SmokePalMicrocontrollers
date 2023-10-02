#include <SoftwareSerial.h>
SoftwareSerial espSerial(5, 6);

String receivedString;
void setup() {
  receivedString = "";
  Serial.begin(9600);
  espSerial.begin(9600);
  delay(2000);
}

void loop() {
  Serial.println("I send \"PING\"");
  espSerial.write("<PING>");
  delay(2000);
  int i = 0;
  while (espSerial.available() > 0) {
    ++i;
    char receivedChar = espSerial.read();
    if(receivedChar == '<') {
      receivedString = "";
    }
    else if(receivedChar == '>') {
      Serial.print("I get: \"");
      Serial.write(receivedString.c_str());
      Serial.println("\"");
      receivedString = "";
    }
    else {
      receivedString += receivedChar;
    }
  }
  if(i == 0) {
    Serial.println("No valid message returned");
  }
  delay(1000);
}