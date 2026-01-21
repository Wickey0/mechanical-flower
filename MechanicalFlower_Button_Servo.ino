/*
 * Project: Mechanical Flower - Button
 * Features:
 * - Long press (D3) to Start/Stop.
 * - Reset button (D4) to clear state (Zero logic).
 * - EEPROM support: Resumes state after power loss.
 */

#include <Servo.h>
#include <EEPROM.h>


const int SERVO_PIN    = 2;  
const int BUTTON_PIN   = 3;  
const int RESET_PIN    = 4;  

Servo flowerServo;
const int SERVO_STOP_US  = 1500;
const int SERVO_OPEN_US  = 1900;  // Open position signal
const int SERVO_CLOSE_US = 1100;  // Close position signal

const unsigned long OPEN_DURATION  = 27000;  // Time to fully open (ms)
const unsigned long CLOSE_DURATION = 29000;  // Time to fully close (ms)

// State Definitions 
// Phase Enum: 0 = Idle/Stop, 1 = Opening, 2 = Closing
int workPhase = 0;

// Control Flags
bool isRunning      = false;
bool isHolding      = false;
unsigned long pressStartTime = 0;
const long HOLD_TIME = 500;      

// Timing Variables
unsigned long currentPhaseTime = 0; // Elapsed time in current phase
unsigned long phaseTimeBase    = 0; // Timestamp for calculation

// Reset Button Variables
bool resetBtnFlag = false;
unsigned long resetDebounce = 0;
const long RESET_DELAY = 50;

// EEPROM Variables
struct SaveData {
    int phase;
    unsigned long time;
};
SaveData saveData;
const int EEPROM_ADDR = 0;

// Safety: Prevent EEPROM burnout by limiting write frequency
unsigned long lastSaveTime = 0;
const long SAVE_INTERVAL = 200; // Save state every 1s while running


void setup() {
    // Hardware Init
    flowerServo.attach(SERVO_PIN);
    flowerServo.writeMicroseconds(SERVO_STOP_US); // Default: Stop
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(RESET_PIN, INPUT_PULLUP);
    
    Serial.begin(9600);
    delay(100);

    // Load State from EEPROM
    EEPROM.get(EEPROM_ADDR, saveData);
    
    if (saveData.phase == 1 && saveData.time >= OPEN_DURATION) {
        saveData.phase = 2; // Switch to closing if opened too long
        saveData.time = 0;
    } else if (saveData.phase == 2 && saveData.time >= CLOSE_DURATION) {
        saveData.phase = 1; // Switch to opening if closed too long
        saveData.time = 0;
    }

    // Restore State
    workPhase = saveData.phase;
    currentPhaseTime = saveData.time;

    // Boot Log
    Serial.println("===== Mechanical Flower System Ready =====");
    Serial.print("Restored State: ");
    if (workPhase == 0) Serial.println("IDLE");
    else Serial.println((workPhase == 1 ? "Flower OPENING" : " Flower CLOSING") + String(" | Time: ") + currentPhaseTime + "ms");
}


void loop() {
    unsigned long now = millis();

    // Reset Logic (D4) - Highest Priority
    bool resetPressed = (digitalRead(RESET_PIN) == LOW);
    
    if (resetPressed && (now - resetDebounce > RESET_DELAY) && !resetBtnFlag) {
        resetBtnFlag = true;
        resetDebounce = now;
        
        // Action: Reset all logic, stop servo
        workPhase = 0;
        currentPhaseTime = 0;
        isRunning = false;
        isHolding = false;
        flowerServo.writeMicroseconds(SERVO_STOP_US);
        
        // Clear EEPROM immediately
        saveData.phase = 0;
        saveData.time = 0;
        EEPROM.put(EEPROM_ADDR, saveData);
        
        Serial.println("\n[SYSTEM RESET] State cleared. Servo Stopped.");
    } else if (!resetPressed) {
        resetBtnFlag = false;
    }

    // Control Logic (D3) - Long Press to Run
    bool btnDown = (digitalRead(BUTTON_PIN) == LOW);
    
    if (btnDown) {
        if (!isHolding) {
            pressStartTime = now;
            isHolding = true;
        }
        // Trigger run after Hold Time
        if (now - pressStartTime >= HOLD_TIME) {
            isRunning = true;
            if (phaseTimeBase == 0) {
                phaseTimeBase = now; // Initialize timer
            }
        }
    } else {
        // Button Released: Stop Everything
        if (isRunning || isHolding) {
            flowerServo.writeMicroseconds(SERVO_STOP_US);
            isRunning = false;
            isHolding = false;
            phaseTimeBase = 0;
            
            // Save state on stop
            saveData.phase = workPhase;
            saveData.time = currentPhaseTime;
            EEPROM.put(EEPROM_ADDR, saveData);
            Serial.println(" [STOP] State Saved.");
        }
    }

    // 3. Motion Logic - Time Based Switching
    if (isRunning) {
        // Calculate elapsed time for this tick
        unsigned long deltaTime = now - phaseTimeBase;
        phaseTimeBase = now; // Update base for next tick
        
        // --- Phase 1: Opening ---
        if (workPhase == 1) {
            flowerServo.writeMicroseconds(SERVO_OPEN_US);
            currentPhaseTime += deltaTime;
            
            if (currentPhaseTime >= OPEN_DURATION) {
                workPhase = 2; // Switch to Closing
                currentPhaseTime = 0;
                Serial.println(" >> Opening Complete. Switching to flower CLOSE.");
            }
        }
        // --- Phase 2: Closing ---
        else if (workPhase == 2) {
            flowerServo.writeMicroseconds(SERVO_CLOSE_US);
            currentPhaseTime += deltaTime;
            
            if (currentPhaseTime >= CLOSE_DURATION) {
                workPhase = 1; // Switch to Opening
                currentPhaseTime = 0;
                Serial.println(" >> Closing Complete. Switching to flower OPEN.");
            }
        }
        // --- Phase 0: Initial Start ---
        else if (workPhase == 0) {
            workPhase = 1;
            Serial.println(" >> Starting Sequence: OPENING");
        }

        // Clamp values to prevent overflow bugs
        currentPhaseTime = constrain(currentPhaseTime, 0, max(OPEN_DURATION, CLOSE_DURATION));
        
        // Update SaveData struct
        saveData.phase = workPhase;
        saveData.time = currentPhaseTime;

        // Periodic EEPROM Save (Power-loss protection)
        // Optimized to save every 1s instead of every loop cycle
        if (now - lastSaveTime > SAVE_INTERVAL) {
            EEPROM.put(EEPROM_ADDR, saveData);
            lastSaveTime = now;
            
            // Debug Output
            Serial.print("Running: ");
            Serial.print(workPhase == 1 ? "Flower OPEN " : "Flower CLOSE ");
            Serial.print(currentPhaseTime);
            Serial.println("ms");
        }
    }
}