/*
Mechanical Flower - LDR Control
- LDR Control Logic:
  * lighter → Lower analog value → flower Open.
  * Dim light → Higher analog value → flower Closes.
  * Medium light (within threshold range) → flower Stop.
- Reset button (D4) to clear the state when flower fully close.
- EEPROM support: Resumes state after power loss.
- Rotate Right, Flower Open; Rotate Left, Flower Close.
 */

#include <Servo.h>
#include <EEPROM.h>  

const int SERVO_PIN    = 2;  
const int PHOTO_SW_PIN = 3;  
const int RESET_PIN    = 4;  
const int PHOTO_PIN    = A0; 

Servo Servo_MG996R;
const int SERVO_STOP_US  = 1500;  
const int SERVO_OPEN_US  = 1900;  
const int SERVO_CLOSE_US = 1100;  

const unsigned long OPEN_TOTAL_MS  = 30000;  // Opening takes 30 seconds
const unsigned long CLOSE_TOTAL_MS = 32000;  // Closing takes 32 seconds

//Light Thresholds 
const int THRESH_OPEN  = 220; 
const int THRESH_CLOSE = 270; 

// Internal Variables 
bool isPhotoEnable = false;   
long currentPos = 0;          
unsigned long lastLoopTime = 0; 
bool resetBtnFlag = false;
unsigned long resetDebounce = 0;
bool photoSwFlag = false;
unsigned long photoSwDebounce = 0;

struct SaveData{ long savedPos; };
SaveData saveData;
const int EEPROM_ADDR = 0; 

void setup() {
  Servo_MG996R.attach(SERVO_PIN);
  Servo_MG996R.writeMicroseconds(SERVO_STOP_US);
  
  pinMode(PHOTO_SW_PIN, INPUT_PULLUP);
  pinMode(RESET_PIN, INPUT_PULLUP);
  pinMode(PHOTO_PIN, INPUT);
  Serial.begin(9600);
  delay(100); 

  // Load saved position
  EEPROM.get(EEPROM_ADDR, saveData);
  if(saveData.savedPos < 0 || saveData.savedPos > OPEN_TOTAL_MS) saveData.savedPos = 0; 
  currentPos = saveData.savedPos;
  lastLoopTime = millis();

  Serial.println("==== Mechanical Flower System ====");
  Serial.println(" >> System Ready.");
}

void loop() {
  unsigned long now = millis();
  long elapsed = now - lastLoopTime; 
  lastLoopTime = now;
  if(elapsed > 100) elapsed = 100; 

  // D4 Reset Button
  bool resetBtnState = digitalRead(RESET_PIN) == LOW;
  if(resetBtnState && millis() - resetDebounce > 50 && !resetBtnFlag){
    resetBtnFlag = true;
    resetDebounce = millis();
    
    // Reset Logic
    currentPos = 0;
    saveData.savedPos = 0;
    EEPROM.put(EEPROM_ADDR, saveData);
    Servo_MG996R.writeMicroseconds(SERVO_STOP_US);
    isPhotoEnable = false;
    Serial.println(" >> Forced Reset Triggered. Position Zeroed.");
  }else if(!resetBtnState) resetBtnFlag = false;

  // D3 Mode Switch 
  bool photoSwState = digitalRead(PHOTO_SW_PIN) == LOW;
  if(photoSwState && millis() - photoSwDebounce > 50 && !photoSwFlag){
    photoSwFlag = true;
    photoSwDebounce = millis();
    isPhotoEnable = !isPhotoEnable;  
    if(isPhotoEnable) Serial.println(" >> Touch Sensor ENABLED");
    else {
      Servo_MG996R.writeMicroseconds(SERVO_STOP_US);
      Serial.println(" >> Touch Sensor DISABLED");
    }
  }else if(!photoSwState) photoSwFlag = false;

  // Core Logic 
  if(isPhotoEnable){
    int photoVal = getFilteredPhotoValue();
    
    // Scene 1: OPENING 
    if(photoVal < THRESH_OPEN){
      if(currentPos < OPEN_TOTAL_MS){
        Servo_MG996R.writeMicroseconds(SERVO_OPEN_US);
        currentPos += elapsed; 
        
        // Display using currentPos directly 
        if(currentPos % 500 < 50) printStatus("OPENING", photoVal, currentPos, OPEN_TOTAL_MS); 
      } else {
        // Reached Top
        Servo_MG996R.writeMicroseconds(SERVO_STOP_US);
        
        if (currentPos != OPEN_TOTAL_MS) {
           Serial.println(" >> Opening Complete. Holding Position.");
        }
        currentPos = OPEN_TOTAL_MS; 
      }
    }
    
    // Scene 2: CLOSING 
    else if(photoVal > THRESH_CLOSE){
      
      if(currentPos > 0){
        Servo_MG996R.writeMicroseconds(SERVO_CLOSE_US);
        
        // Physical Calculation (Gearbox logic)
        // Even though scale is 30s, actual movement matches 32s pace
        long stepBack = (elapsed * OPEN_TOTAL_MS) / CLOSE_TOTAL_MS; 
        if(stepBack == 0 && elapsed > 0) stepBack = 1;

        //  Core Subtraction
        if(currentPos > stepBack){
          currentPos -= stepBack;
        } else {
          currentPos = 0; 
        }
        
        // Display Calculation 
        long closingDisplayTime = (OPEN_TOTAL_MS - currentPos) * CLOSE_TOTAL_MS / OPEN_TOTAL_MS;

        if(closingDisplayTime % 500 < 50) printStatus("CLOSING", photoVal, closingDisplayTime, CLOSE_TOTAL_MS);

      } else {
        // Stop Logic
        Servo_MG996R.writeMicroseconds(SERVO_STOP_US);
        
        static bool isPrinted = false;
        if(currentPos == 0 && !isPrinted){
           printStatus("CLOSING", photoVal, CLOSE_TOTAL_MS, CLOSE_TOTAL_MS);
           Serial.println(" >> Closing Complete. Switching to IDLE."); 
           isPrinted = true;
        }
        if(currentPos > 0) isPrinted = false;
      }
    }
    
    // Scene 3: IDLE 
    else {
      Servo_MG996R.writeMicroseconds(SERVO_STOP_US);
    }

    // Boundary Clamping
    if(currentPos > OPEN_TOTAL_MS) currentPos = OPEN_TOTAL_MS;
    if(currentPos < 0) currentPos = 0;

    if(abs(currentPos - saveData.savedPos) > 1000){
       saveData.savedPos = currentPos;
       EEPROM.put(EEPROM_ADDR, saveData);
    }
  }
}

// Helper Functions
int getFilteredPhotoValue(){
  long sum = 0;
  for(int i=0; i<10; i++){
    sum += analogRead(PHOTO_PIN);
    delay(2);
  }
  return (int)(sum / 10);
}

void printStatus(String action, int val, long timeDisplay, long timeTotal){
  Serial.print("Running: ");
  Serial.print(action);
  
  Serial.print(" | Light:"); 
  Serial.print(val);
  
  Serial.print(" | Time: "); 
  Serial.print(timeDisplay);
  Serial.print("/");
  Serial.print(timeTotal);
  Serial.println(" ms");
}