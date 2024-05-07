#include <Servo.h>

Servo Testfinger;  

void setup() {
  Testfinger.attach(5) // Test auf Pin 5
}

void loop() {
  // langsames beugen und strecken eines Fingers - Testcode, wo man ggf schnell eingreifen kann

  Testfinger.write(0); //0°
  delay(200);

  Testfinger.write(20); //20°
  delay(200);

  Testfinger.write(45); //45°
  delay(200);

  Testfinger.write(60); //60°
  delay(200);

  Testfinger.write(90); //90°
  delay(200);

  Testfinger.write(60); //60°
  delay(200);

  Testfinger.write(45); //45°
  delay(200);

  Testfinger.write(20); //20°
  delay(200);

  Testfinger.write(0); //0°
  delay(200);
}
