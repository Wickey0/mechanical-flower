#include <Servo.h>  // 引入舵机库

Servo Servo_MG996R;      // 创建舵机对象
const int servoPin = 2;  // 舵机信号线接D9

void setup() {
  Servo_MG996R.attach(servoPin);  // 将舵机绑定到指定引脚
  Servo_MG996R.writeMicroseconds(1500);  // 初始状态：停止
  delay(1000);  // 等待舵机稳定
}

void loop() {
  // 正传（顺时针）：1000us脉宽（转速最快）
  //Servo_MG996R.writeMicroseconds(1400);
  //delay(30000);  // 正传20秒
  
  // 停止1秒
  //Servo_MG996R.writeMicroseconds(1500);
  //delay(1000);

  Servo_MG996R.writeMicroseconds(1200);
  delay(30000);  // 正传20秒

  Servo_MG996R.writeMicroseconds(1500);
  delay(1000);

}