
String receivedString;
void setup() {
  Serial.begin(9600);
  receivedString = "";
}

void loop() {
  while (Serial.available() > 0) {
    char receivedChar = Serial.read();
    if(receivedChar == '<') {
      receivedString = "";
    }
    else if(receivedChar == '>') {
      if(receivedString == "PING") {
        Serial.write("<PONG>");
      }
      else {
        Serial.write("Send \"PING\" to get \"PONG\" back");
      }
      receivedString = "";
    }
    else {
      receivedString += receivedChar;
    }
  }
}