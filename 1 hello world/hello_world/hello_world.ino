#include <SoftwareSerial.h>

SoftwareSerial espSerial(5, 6);

String str;
void setup() {
Serial.begin(115200);
espSerial.begin(115200);
dht.begin();
delay(2000);
}
void loop()
{
float h = dht.readHumidity();
// Read temperature as Celsius (the default)
float t = dht.readTemperature();
Serial.print("H: ");
Serial.print(h);
Serial.print("% ");
Serial.print(" T: ");
Serial.print(t);
Serial.p