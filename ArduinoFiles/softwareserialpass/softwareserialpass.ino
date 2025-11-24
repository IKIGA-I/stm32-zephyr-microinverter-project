#include <SoftwareSerial.h>

SoftwareSerial stmSerial(10, 11); // RX, TX

void setup() {
  Serial.begin(9600);
  stmSerial.begin(9600);
  Serial.println("--- Bridge Active ---");
}

void loop() {
  // Transfer STM32 -> PC
  if (stmSerial.available()) {
    Serial.write(stmSerial.read());
  }
  
  // Transfer PC -> STM32
  if (Serial.available()) {
    // Add a tiny delay to let the STM32 catch up
    stmSerial.write(Serial.read());
    delay(2); 
  }
}