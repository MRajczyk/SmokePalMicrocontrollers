#include <SoftwareSerial.h>
SoftwareSerial espSerial(5, 6);

int i;
void setup() {
  Serial.begin(115200);
  espSerial.begin(115200);
  delay(2000);
}

void loop() {
  Serial.println("I send \"a\"");
  espSerial.println("a");
  delay(1000);
  if (espSerial.available() > 0) {
    Serial.print("I get: \"");
    Serial.write(espSerial.read());
    Serial.println("\"");
  } else {
    Serial.println("no message back :(");
  }
  delay(1000);
}