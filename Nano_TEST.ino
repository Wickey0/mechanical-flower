//Validation Communication

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  
  // Set the built-in LED pin as output mode
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // Turn on the built-in LED
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("LED is ON");
  delay(1000);
  
  // Turn off the built-in LED
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("LED is OFF");
  delay(1000);
}