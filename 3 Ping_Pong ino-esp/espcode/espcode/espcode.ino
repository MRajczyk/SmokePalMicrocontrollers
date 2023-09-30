void setup() {
  Serial.begin(115200);
}

void loop() {
  if (Serial.available()) {
    char recv = Serial.read();
    if(recv == 'a') {
      Serial.write('b');
    }
  }
}