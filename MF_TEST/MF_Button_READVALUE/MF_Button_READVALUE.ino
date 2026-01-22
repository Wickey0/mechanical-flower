// Read Button State 

int buttonPin = 3; // D3 

void setup() {

  Serial.begin(9600);
// INPUT_PULLUP，the pin is HIGH by default.
  pinMode(buttonPin, INPUT_PULLUP);

}

void loop() {
  // Read the state of the digital pin
  // 2 Results：1 (HIGH) or  0 (LOW)
  int buttonState = digitalRead(buttonPin);
  
  Serial.println(buttonState); 
  delay(200);
}