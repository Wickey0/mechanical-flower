/*
Mechanical Flower - Button Control (Angle Command Version)
- Long press (D3) to Start/Stop rotation.
- Reset button (D4) to clear the state (use when the flower is fully closed).
- EEPROM support: Resumes state after power loss.
- Servo control: Uses servo.write(angle) method.
 */

#include <Servo.h>
#include <EEPROM.h>

const int SERVO_PIN    = 5;  
const int BUTTON_PIN   = 3;  
const int RESET_PIN    = 4;  

Servo flowerServo;

// --- Angle and Speed Definitions ---
const int SERVO_STOP_DEG  = 90;   // Stopped
const int SERVO_OPEN_DEG  = 170;  // Rotate Right / Move Up (Opening speed)
const int SERVO_CLOSE_DEG = 10;   // Rotate Left / Move Down (Closing speed)

const unsigned long OPEN_DURATION  = 5000;  // Total time required for the opening process (ms)
const unsigned long CLOSE_DURATION = 5000;  // Total time required for the closing process (ms)

// State Definitions 
// Phase Enum: 0 = Idle/Stop, 1 = Opening, 2 = Closing
int workPhase = 0;

// Control Flags
bool isRunning = false;
bool isHolding = false;
unsigned long pressStartTime = 0;
const long HOLD_TIME = 500;      

// Timing Variables
unsigned long currentPhaseTime = 0; // Time elapsed in the current phase
unsigned long phaseTimeBase    = 0; // Reference timestamp for calculating increments

// Reset Button Variables
bool resetBtnFlag = false;
unsigned long resetDebounce = 0;
const long RESET_DELAY = 50;

// EEPROM Structure
struct SaveData {
    int phase;
    unsigned long time;
};
SaveData saveData;
const int EEPROM_ADDR = 0;

// Safety: Limit EEPROM write frequency to prevent wear
unsigned long lastSaveTime = 0;
const long SAVE_INTERVAL = 1000; // Save state every 1 second while running


void setup() {
    // Hardware Initialization
    flowerServo.attach(SERVO_PIN);
    flowerServo.write(SERVO_STOP_DEG); // Initial state: Stop
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(RESET_PIN, INPUT_PULLUP);
    
    Serial.begin(9600);
    delay(100);

    // Read state from EEPROM
    EEPROM.get(EEPROM_ADDR, saveData);
    
    // Basic Boundary Checks
    if (saveData.phase == 1 && saveData.time >= OPEN_DURATION) {
        saveData.phase = 2; 
        saveData.time = 0;
    } else if (saveData.phase == 2 && saveData.time >= CLOSE_DURATION) {
        saveData.phase = 1; 
        saveData.time = 0;
    }

    // Restore State
    workPhase = saveData.phase;
    currentPhaseTime = saveData.time;

    Serial.println("===== Mechanical Flower System Ready =====");
    Serial.print("Restored State: ");
    if (workPhase == 0) Serial.println("IDLE");
    else Serial.println((workPhase == 1 ? "Flower OPENING" : " Flower CLOSING") + String(" | Time: ") + currentPhaseTime + "ms");
}


void loop() {
    unsigned long now = millis();

    // 1. Reset Logic (D4) - Highest Priority
    bool resetPressed = (digitalRead(RESET_PIN) == LOW);
    
    if (resetPressed && (now - resetDebounce > RESET_DELAY) && !resetBtnFlag) {
        resetBtnFlag = true;
        resetDebounce = now;
        
        // Action: Reset all logic and stop the servo
        workPhase = 0;
        currentPhaseTime = 0;
        isRunning = false;
        isHolding = false;
        flowerServo.write(SERVO_STOP_DEG);
        
        // Immediately clear EEPROM
        saveData.phase = 0;
        saveData.time = 0;
        EEPROM.put(EEPROM_ADDR, saveData);
        
        Serial.println("\n[SYSTEM RESET] State cleared. Servo Stopped.");
    } else if (!resetPressed) {
        resetBtnFlag = false;
    }

    // 2. Control Logic (D3) - Long press to run, release to stop
    bool btnDown = (digitalRead(BUTTON_PIN) == LOW);
    
    if (btnDown) {
        if (!isHolding) {
            pressStartTime = now;
            isHolding = true;
        }
        // Start running after the button is held for HOLD_TIME
        if (now - pressStartTime >= HOLD_TIME) {
            isRunning = true;
            if (phaseTimeBase == 0) {
                phaseTimeBase = now; // Initialize timing reference
            }
        }
    } else {
        // Button Released: Stop all actions
        if (isRunning || isHolding) {
            flowerServo.write(SERVO_STOP_DEG);
            isRunning = false;
            isHolding = false;
            phaseTimeBase = 0;
            
            // Save state when stopping
            saveData.phase = workPhase;
            saveData.time = currentPhaseTime;
            EEPROM.put(EEPROM_ADDR, saveData);
            Serial.println(" [STOP] State Saved.");
        }
    }

    // 3. Motion Logic - Time-based phase switching
    if (isRunning) {
        // Calculate time elapsed during this loop cycle
        unsigned long deltaTime = now - phaseTimeBase;
        phaseTimeBase = now; // Update reference for the next cycle
        
        // --- Phase 1: Opening (Rotate Right) ---
        if (workPhase == 1) {
            flowerServo.write(SERVO_OPEN_DEG); // Send 170-degree signal
            currentPhaseTime += deltaTime;
            
            if (currentPhaseTime >= OPEN_DURATION) {
                workPhase = 2; // Switch to Closing
                currentPhaseTime = 0;
                Serial.println(" >> Opening Complete. Switching to flower CLOSE.");
            }
        }
        // --- Phase 2: Closing (Rotate Left) ---
        else if (workPhase == 2) {
            flowerServo.write(SERVO_CLOSE_DEG); // Send 60-degree signal
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

        // Clamp value to prevent overflow
        currentPhaseTime = constrain(currentPhaseTime, 0, max(OPEN_DURATION, CLOSE_DURATION));
        
        // Update SaveData structure
        saveData.phase = workPhase;
        saveData.time = currentPhaseTime;

        // Periodic EEPROM Save (Protection against unexpected power loss)
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
