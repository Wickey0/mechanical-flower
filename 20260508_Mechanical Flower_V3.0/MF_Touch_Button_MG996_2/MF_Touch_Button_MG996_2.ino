/*
Mechanical Flower - Touch Sensor Control
- Touch Sensor (D6) to Start/Stop Rotate after a short hold.
- Reset button (D4) to clear the state when flower is fully closed.
- Manual close button (D7): hold to force the flower closing direction for mechanical recovery.
- EEPROM support: Resumes state after power loss.
- Rotate Right, Flower Open; Rotate Left, Flower Close.
 */

#include <Servo.h>
#include <EEPROM.h>  

const int SERVO_PIN         = 7;  
const int TOUCH_PIN         = 5;  
const int MANUAL_CLOSE_PIN  = 4; 
const int RESET_PIN         = 3;   

Servo flowerServo; 
const int SERVO_STOP_DEG  = 90;   // Stopped
const int SERVO_OPEN_DEG  = 145;  // Rotate Right / Move Up (Opening speed)
const int SERVO_CLOSE_DEG = 35;   // Rotate Left / Move Down (Closing speed)

const unsigned long OPEN_DURATION  = 6500;  // Time to fully open (ms)
const unsigned long CLOSE_DURATION = 6500;  // Time to fully close (ms)

bool isRunning = false;    
bool isTouching = false;   
unsigned long touchStartTime = 0;
const long HOLD_TIME = 50; 

// Work phase: 0=Stop, 1=Opening, 2=Closing
int workPhase = 0;
unsigned long currentPhaseTime = 0;
unsigned long phaseTimeBase = 0;

bool resetBtnFlag = false;
unsigned long resetDebounce = 0;
const long RESET_DELAY = 50;

bool manualCloseBtnFlag = false;
unsigned long manualCloseDebounce = 0;
const long MANUAL_CLOSE_DELAY = 50;

// EEPROM Data Structure 
struct SaveData{
  int phase;
  unsigned long time;
};
SaveData saveData;
const int EEPROM_ADDR = 0;

// Prevent EEPROM burnout by limiting write frequency
unsigned long lastSaveTime = 0;
const long SAVE_INTERVAL = 200; //EEPROM save interval

void setup() {
  flowerServo.attach(SERVO_PIN);
  flowerServo.write(SERVO_STOP_DEG); 
  pinMode(TOUCH_PIN, INPUT); 
  pinMode(RESET_PIN, INPUT_PULLUP);
  pinMode(MANUAL_CLOSE_PIN, INPUT_PULLUP);
  
  Serial.begin(9600);
  delay(100); 

  // Read memory state on power up
  EEPROM.get(EEPROM_ADDR, saveData);
  
  if(saveData.phase == 1 && saveData.time >= OPEN_DURATION){
    saveData.phase = 2; saveData.time = 0;
  }else if(saveData.phase == 2 && saveData.time >= CLOSE_DURATION){
    saveData.phase = 1; saveData.time = 0;
  }
  
  workPhase = saveData.phase;
  currentPhaseTime = saveData.time;

  // Unified startup log format
  Serial.println("===== Mechanical Flower System Ready =====");
  Serial.print("Restored State: ");
  if (workPhase == 0) Serial.println("IDLE");
  else Serial.println((workPhase == 1 ? "Flower OPENING" : " Flower CLOSING") + String(" | Time: ") + currentPhaseTime + "ms");
}

void loop() {
  unsigned long now = millis(); 

  // D4 Reset Button 
  bool resetPressed = (digitalRead(RESET_PIN) == LOW); 
  
  if(resetPressed && (now - resetDebounce > RESET_DELAY) && !resetBtnFlag){
    resetBtnFlag = true;
    resetDebounce = now;
    
    // Reset action
    workPhase = 0;
    currentPhaseTime = 0;
    isRunning = false;
    isTouching = false;
    
    saveData.phase = 0;
    saveData.time = 0;
    EEPROM.put(EEPROM_ADDR, saveData);
    
    flowerServo.write(SERVO_STOP_DEG);
    Serial.println("[SYSTEM RESET] State cleared. Servo Stopped."); 
  }else if(!resetPressed){
    resetBtnFlag = false;
  }

  // D7 Manual Close Button
  // Hold this button to force close the flower after a mechanical error.
  // Release it when the flower reaches the fully closed position.
  bool manualClosePressed = (digitalRead(MANUAL_CLOSE_PIN) == LOW);

  if(manualClosePressed && (now - manualCloseDebounce > MANUAL_CLOSE_DELAY)){
    if(!manualCloseBtnFlag){
      Serial.println("[MANUAL CLOSE] Hold button: forcing flower CLOSE.");
      manualCloseBtnFlag = true;
    }

    manualCloseDebounce = now;
    isRunning = false;
    isTouching = false;
    phaseTimeBase = 0;
    flowerServo.write(SERVO_CLOSE_DEG);
    return;
  }else if(!manualClosePressed && manualCloseBtnFlag){
    manualCloseBtnFlag = false;
    flowerServo.write(SERVO_STOP_DEG);

    // After manual recovery, assume the user released the button at fully closed position.
    // Save phase=1 and time=0 so the next touch starts opening from closed.
    workPhase = 1;
    currentPhaseTime = 0;
    saveData.phase = workPhase;
    saveData.time = currentPhaseTime;
    EEPROM.put(EEPROM_ADDR, saveData);

    Serial.println("[MANUAL CLOSE] Released. Position saved as fully closed; next touch opens.");
  }

  // D6 Touch Sensor Control Logic 
  // Note: TTP223 output HIGH on touch, LOW on no touch
  bool touchDetected = (digitalRead(TOUCH_PIN) == HIGH); 

  if(touchDetected){
    // If touch just detected
    if(!isTouching){
      touchStartTime = now;
      isTouching = true;
    }
    // If touch lasts over 50ms , then start
    if(now - touchStartTime >= HOLD_TIME){
      isRunning = true;
      if(phaseTimeBase == 0) phaseTimeBase = now;
    }
  }else{
    // Release: Stop Immediately 
    if(isRunning || isTouching){
       // Servo stops immediately upon release
       flowerServo.write(SERVO_STOP_DEG);
       isRunning = false;
       isTouching = false;
       phaseTimeBase = 0;
       
       // Save progress when released
       saveData.phase = workPhase;
       saveData.time = currentPhaseTime;
       EEPROM.put(EEPROM_ADDR, saveData);
       Serial.println(" [STOP] State Saved."); // Unified stop log
    }
  }
 
  //  Running Logic 
  if(isRunning){
    // Calculate elapsed time for this tick
    unsigned long deltaTime = now - phaseTimeBase;
    phaseTimeBase = now; // Update base for next tick
    
    // Opening Phase 
    if(workPhase == 1){
      flowerServo.write(SERVO_OPEN_DEG);
      currentPhaseTime += deltaTime;
      
      if(currentPhaseTime >= OPEN_DURATION){
        workPhase = 2; // Switch to closing
        currentPhaseTime = 0;
        Serial.println(" >> Opening Complete. Switching to flower CLOSE."); 
      }
    }
    // Closing Phase
    else if(workPhase == 2){
      flowerServo.write(SERVO_CLOSE_DEG);
      currentPhaseTime += deltaTime;
      
      if(currentPhaseTime >= CLOSE_DURATION){
        workPhase = 1; // Switch to opening
        currentPhaseTime = 0;
        Serial.println(" >> Closing Complete. Switching to flower OPEN.");
      }
    }
    // Initial Start 
    else if(workPhase == 0){
      workPhase = 1;
      Serial.println(" >> Starting Sequence: OPENING"); 
    }

    // Prevent time value overflow
    currentPhaseTime = constrain(currentPhaseTime, 0, max(OPEN_DURATION, CLOSE_DURATION));
    
    // Update SaveData struct
    saveData.phase = workPhase;
    saveData.time = currentPhaseTime;

    // Periodic EEPROM Save (Power-loss protection)
    if (now - lastSaveTime > SAVE_INTERVAL) {
        EEPROM.put(EEPROM_ADDR, saveData);
        lastSaveTime = now;
        
        // Debug Output - Unified running log format
        Serial.print("Running: ");
        Serial.print(workPhase == 1 ? "Flower OPEN " : "Flower CLOSE ");
        Serial.print(currentPhaseTime);
        Serial.println("ms");
    }
  }
}