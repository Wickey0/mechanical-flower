int sensorPin = A0;
void setup() {
  Serial.begin(9600);
}
void loop() {
  int sensorVal = analogRead(sensorPin);
  Serial.println(sensorVal); 
  delay(200);
}
