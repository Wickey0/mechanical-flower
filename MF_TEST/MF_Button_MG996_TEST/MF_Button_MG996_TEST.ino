#include <Servo.h>

Servo servo;

const int servoPin = 2;       // Servo pin
const int buttonPin = 3;      // Button pin

// Speed control parameters (Modify values here to adjust speed)
const int SPEED_STOP  = 1500; // Stop
const int SPEED_LEFT  = 1100; // Rotate Left (Full speed)
const int SPEED_RIGHT = 1900; // Rotate Right (Full speed)

// State definition
enum ServoState {
  ROTATING_RIGHT,   // Rotating right state
  ROTATING_LEFT,    // Rotating left state
  STOPPED           // Stopped state
};

ServoState currentState = STOPPED; // It is recommended to set initial state to stopped for safety

// Button variables
int lastButtonState = HIGH;
int buttonState;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

unsigned long lastPressTime = 0;
unsigned long doubleClickTime = 500;  
int clickCount = 0;

void setup() {
  servo.attach(servoPin);
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Initial action: Stop
  servo.writeMicroseconds(SPEED_STOP);
  currentState = STOPPED;
  
  Serial.begin(9600);
  Serial.println("System Ready - Initial State: Stopped");
}

void loop() {
  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading; 
      
      // --- Detect button press (LOW) ---
      if (buttonState == LOW) { 
        unsigned long currentTime = millis();
        
        // Determine single click or double click
        if (currentTime - lastPressTime < doubleClickTime) {
          // Double click logic
          clickCount++;
          Serial.println("Double Click -> Stop");
          
          if (clickCount >= 1) { 
            currentState = STOPPED;
            servo.writeMicroseconds(SPEED_STOP); 
            clickCount = 0;
          }
        } 
        else {
         
          clickCount = 0; 
          
          if (currentState == ROTATING_RIGHT) {
            // Currently rotating right -> Switch to left rotation
            currentState = ROTATING_LEFT;
            servo.writeMicroseconds(SPEED_LEFT); 
            Serial.println("Switch: Rotate Left (1100)");
            
          } else if (currentState == ROTATING_LEFT) {
            // Currently rotating left -> Switch to right rotation
            currentState = ROTATING_RIGHT;
            servo.writeMicroseconds(SPEED_RIGHT); 
            Serial.println("Switch: Rotate Right (1900)");
            
          } else if (currentState == STOPPED) {
            // Currently stopped -> Default start with right rotation
            currentState = ROTATING_RIGHT;
            servo.writeMicroseconds(SPEED_RIGHT); 
            Serial.println("Start: Resume Rotate Right (1900)");
          }
        }
        
        lastPressTime = currentTime;
      }
    }
  }
  
  lastButtonState = reading;
}