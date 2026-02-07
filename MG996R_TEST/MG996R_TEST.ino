#include <Servo.h>  

Servo Servo_MG996R;      
const int servoPin = 2;  // D2

void setup() {
  Servo_MG996R.attach(servoPin);  
  Servo_MG996R.writeMicroseconds(1500);  // Initial state: stopped
  delay(1000);  // Wait for the servo to stabilize 1000ms
}

void loop() {
  
  Servo_MG996R.writeMicroseconds(1400); //Flower Closed
  delay(20000);  // 20,000ms 
  
  Servo_MG996R.writeMicroseconds(1500); //Flower Stop
  delay(1000);

  Servo_MG996R.writeMicroseconds(1200); //Flower Open
  delay(20000);  //20,000ms 

  Servo_MG996R.writeMicroseconds(1500); //Flower Stop
  delay(1000);

}
