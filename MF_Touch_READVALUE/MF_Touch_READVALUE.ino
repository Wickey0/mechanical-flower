// Touch Sensor Read Value
int touchPin = 5; //D5

void setup() {

  Serial.begin(9600);
  pinMode(touchPin, INPUT);

}

void loop() {

  int touchState = digitalRead(touchPin); // Translate 

  if(touchState == HIGH){
     Serial.println("Touched! (1)");
  } else {
     Serial.println("No Touch (0)");
  }
  
  delay(100); 
}
