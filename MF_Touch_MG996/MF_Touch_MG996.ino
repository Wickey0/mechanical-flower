/*
Mechanical Flower - Touch Sensor Control
- Touch Senosr (D5) to Start/Stop Rotate within 15ms (Long Press) .
- Reset button (D4) to clear the state when flower fully close.
- EEPROM support: Resumes state after power loss.
- Rotate Right, Flower Open; Rotate Left, Flower Close.
 */

#include <Servo.h>
#include <EEPROM.h>  

const int SERVO_PIN    = 2;  
const int TOUCH_PIN    = 5;  
const int RESET_PIN    = 4;  

Servo flowerServo; 
const int SERVO_STOP_US  = 1500;  
const int SERVO_OPEN_US  = 1900;  
const int SERVO_CLOSE_US = 1100;  

const unsigned long OPEN_DURATION  = 27000;  // Time to fully open (ms)
const unsigned long CLOSE_DURATION = 29000;  // Time to fully close (ms)

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
  flowerServo.writeMicroseconds(SERVO_STOP_US); 
  
  pinMode(TOUCH_PIN, INPUT); 
  pinMode(RESET_PIN, INPUT_PULLUP);
  
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
    
    flowerServo.writeMicroseconds(SERVO_STOP_US);
    Serial.println("[SYSTEM RESET] State cleared. Servo Stopped."); 
  }else if(!resetPressed){
    resetBtnFlag = false;
  }

  // D5 Touch Sensor Control Logic 
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
       flowerServo.writeMicroseconds(SERVO_STOP_US);
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
      flowerServo.writeMicroseconds(SERVO_OPEN_US);
      currentPhaseTime += deltaTime;
      
      if(currentPhaseTime >= OPEN_DURATION){
        workPhase = 2; // Switch to closing
        currentPhaseTime = 0;
        Serial.println(" >> Opening Complete. Switching to flower CLOSE."); 
      }
    }
    // Closing Phase
    else if(workPhase == 2){
      flowerServo.writeMicroseconds(SERVO_CLOSE_US);
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