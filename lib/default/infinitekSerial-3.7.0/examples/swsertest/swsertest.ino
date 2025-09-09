
#include <InfinitekSerial.h>

InfinitekSerial swSer(14, 12);

void setup() {
  Serial.begin(115200);
  swSer.begin();

  Serial.println("\nInfinitek serial test started");

  for (char ch = ' '; ch <= 'z'; ch++) {
    swSer.write(ch);
  }
  swSer.println("");

}

void loop() {
  while (swSer.available() > 0) {
    Serial.write(swSer.read());
  }
  while (Serial.available() > 0) {
    swSer.write(Serial.read());
  }

}
