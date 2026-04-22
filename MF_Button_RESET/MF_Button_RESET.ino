/*
Mechanical Flower - Button Reset
- Press (D3) to Rotate Right or Left.
- Double Press (D3) to Stop.
- After Stop, default start to Open the flower.
- Rotate Right, Flower Open; Rotate Left, Flower Close.
 */
#include <Servo.h>

Servo servo;

//Parameter Definitions
const int servoPin = 5;       
const int buttonPin = 3;      


const int SERVO_STOP_DEG  = 90;   // Stopped
const int SERVO_OPEN_DEG  = 170;  // Rotate Right / Move Up (Opening speed)
const int SERVO_CLOSE_DEG = 10;   // Rotate Left / Move Down (Closing speed)

enum ServoState {
  ROTATING_RIGHT,   
  ROTATING_LEFT,    
  STOPPED           
};

ServoState currentState = STOPPED; // Initial state set to STOPPED

//Button Variables
int lastButtonState = HIGH;
int buttonState;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Double-click detection variables
unsigned long lastPressTime = 0;
unsigned long doubleClickTime = 500;  
int clickCount = 0;

void setup() {
  servo.attach(servoPin);
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Initial action: Stop the servo
  servo.write(SERVO_STOP_DEG);
  currentState = STOPPED;
  
  Serial.begin(9600);
  Serial.println("System Ready - Initial State: Stopped");
}

void loop() {
  int reading = digitalRead(buttonPin);

  // Debounce Logic
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading; 
      
      //Detect Button Press (Active LOW)
      if (buttonState == LOW) { 
        unsigned long currentTime = millis();
        
        // Check for double-click vs single-click
        if (currentTime - lastPressTime < doubleClickTime) {
          // Double-click Logic 
          clickCount++;
          Serial.println("Double-click detected -> STOP");
          
          if (clickCount >= 1) { 
            currentState = STOPPED;
            servo.write(SERVO_STOP_DEG); 
            clickCount = 0;
          }
        } 
        else {
          // Single-click Logic 
          clickCount = 0; 
          
          if (currentState == ROTATING_RIGHT) {
            // Currently Rotating Right -> Switch to Left
            currentState = ROTATING_LEFT;
            servo.write(SERVO_CLOSE_DEG); // Send 1100
            Serial.println("Switched to: LEFT");
            
          } else if (currentState == ROTATING_LEFT) {
            // Currently Rotating Left -> Switch to Right
            currentState = ROTATING_RIGHT;
            servo.write(SERVO_OPEN_DEG); // Send 1900
            Serial.println("Switched to: RIGHT");
            
          } else if (currentState == STOPPED) {
            // Currently Stopped -> Default start to Right
            currentState = ROTATING_RIGHT;
            servo.write(SERVO_OPEN_DEG); // Send 1900
            Serial.println("Started: Resuming RIGHT");
          }
        }
        
        lastPressTime = currentTime;
      }
    }
  }
  
  lastButtonState = reading;
}
