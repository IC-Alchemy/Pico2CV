/**
 * @file Pico2CV_Refactored.ino
 * @brief Refactored main application demonstrating modular architecture
 * 
 * This version shows how the sequencer has been decoupled from global variables
 * and split into modular components with clear separation of concerns.
 */

// -----------------------------------------------------------------------------
// 1. INCLUDES & DEFINES
// -----------------------------------------------------------------------------

// --- CV PWM Pin Definitions ---
#define CV1_PWM_PIN 2   // Pitch
#define CV2_PWM_PIN 3   // Velocity
#define CV3_PWM_PIN 4   // Filter
#define CV4_PWM_PIN 5   // Envelope

// --- Core Includes ---
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <uClock.h>
#include <Wire.h>

// --- Modular Components ---
#include "src/sequencer/Sequencer.h"
#include "src/interfaces/HardwareSequencerIO.h"
#include "src/state/SystemState.h"
#include "src/input/InputManager.h"
#include "src/audio/AudioEngine.h"
#include "src/clock/ClockManager.h"

// --- Hardware Interfaces ---
#include "src/matrix/Matrix.h"
#include <Adafruit_MPR121.h>
#include <Melopero_VL53L1X.h>

// --- DSP ---
#include "src/dsp/adsr.h"
#include "src/dsp/ladder.h"
#include "src/dsp/phasor.h"

// -----------------------------------------------------------------------------
// 2. GLOBAL OBJECTS & STATE
// -----------------------------------------------------------------------------

// --- MIDI & USB ---
Adafruit_USBD_MIDI raw_usb_midi;
midi::SerialMIDI<Adafruit_USBD_MIDI> serial_usb_midi(raw_usb_midi);
midi::MidiInterface<midi::SerialMIDI<Adafruit_USBD_MIDI>> usb_midi(serial_usb_midi);

// --- Modular Components ---
HardwareSequencerIO sequencerIO;
Sequencer sequencer(&sequencerIO);
InputManager inputManager;
AudioEngine audioEngine;
ClockManager clockManager;

// --- Hardware Interfaces ---
Adafruit_MPR121 touchSensor;
Melopero_VL53L1X distanceSensor;

// --- DSP Components ---
daisysp::LadderFilter filter;
daisysp::Adsr env1;
daisysp::Adsr env2;

// --- Audio Buffer Pool ---
audio_buffer_pool_t *producer_pool = nullptr;

// -----------------------------------------------------------------------------
// 3. CORE 0 AUDIO PROCESSING
// -----------------------------------------------------------------------------

/**
 * @brief Core 0 audio processing loop
 * Handles real-time audio processing at 8kHz
 */
void core0_audio_loop() {
    while (true) {
        // Process one audio sample
        audioEngine.processSample();
        
        // Update CV outputs via PWM
        analogWrite(CV1_PWM_PIN, audioEngine.getCV1() * 255);
        analogWrite(CV2_PWM_PIN, audioEngine.getCV2() * 255);
        analogWrite(CV3_PWM_PIN, audioEngine.getCV3() * 255);
        analogWrite(CV4_PWM_PIN, audioEngine.getCV4() * 255);
        
        // Wait for next sample (125μs for 8kHz)
        delayMicroseconds(125);
    }
}

// -----------------------------------------------------------------------------
// 4. CLOCK CALLBACKS
// -----------------------------------------------------------------------------

/**
 * @brief Clock callback for step advancement
 * Called by uClock on each 16th note
 */
void onClockStep() {
    uint8_t currentStep = clockManager.getCurrentStep();
    
    // Advance sequencer step (pure step logic)
    sequencer.advanceStep(currentStep);
    
    // Handle live parameter recording separately
    SystemState& state = SystemState::getInstance();
    sequencer.recordLiveParameters(
        state.getMM(),
        state.getButton16Held(),
        state.getButton17Held(),
        state.getButton18Held(),
        state.getSelectedStepForEdit()
    );
}

/**
 * @brief Clock callback for general timing
 * Called by uClock on each clock tick (96 PPQN)
 */
void onClockTick() {
    // Handle note duration tracking
    sequencer.tickNoteDuration();
}

/**
 * @brief uClock start callback
 */
void onClockStart() {
    sequencer.start();
}

/**
 * @brief uClock stop callback
 */
void onClockStop() {
    sequencer.stop();
}

// -----------------------------------------------------------------------------
// 5. SETUP & INITIALIZATION
// -----------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    
    // Initialize CV PWM outputs
    pinMode(CV1_PWM_PIN, OUTPUT);
    pinMode(CV2_PWM_PIN, OUTPUT);
    pinMode(CV3_PWM_PIN, OUTPUT);
    pinMode(CV4_PWM_PIN, OUTPUT);
    
    // Initialize modular components
    inputManager.init();
    audioEngine.init();
    clockManager.init();
    sequencer.init();
    
    // Initialize MIDI
    usb_midi.begin(MIDI_CHANNEL_OMNI);
    
    // Initialize uClock
    uClock.init();
    uClock.setTempo(120);
    
    // Set up clock callbacks
    clockManager.setStepCallback(onClockStep);
    clockManager.setClockCallback(onClockTick);
    
    // Set up uClock callbacks
    uClock.setOnClockStartOutput(onClockStart);
    uClock.setOnClockStopOutput(onClockStop);
    
    // Initialize hardware interfaces
    Wire.begin();
    
    if (!touchSensor.begin(0x5A)) {
        Serial.println("MPR121 not found, check wiring?");
    }
    
    if (!distanceSensor.begin()) {
        Serial.println("VL53L1X not found, check wiring?");
    }
    
    // Start Core 0 audio processing
    multicore_launch_core1(core0_audio_loop);
    
    // Start clock
    uClock.start();
    clockManager.start();
    
    Serial.println("Pico2CV Refactored - Ready!");
}

// -----------------------------------------------------------------------------
// 6. MAIN LOOP (CORE 1)
// -----------------------------------------------------------------------------

void loop() {
    // Update input manager (handles all input sources)
    inputManager.update();
    
    // Update clock manager
    clockManager.update();
    
    // Handle MIDI input
    usb_midi.read();
    
    // Handle touch matrix events
    handleTouchEvents();
    
    // Small delay to prevent overwhelming the system
    delay(1);
}

// -----------------------------------------------------------------------------
// 7. INPUT HANDLING
// -----------------------------------------------------------------------------

/**
 * @brief Handle touch matrix events
 */
void handleTouchEvents() {
    static uint16_t lastTouched = 0;
    uint16_t currentTouched = touchSensor.touched();
    
    // Check for new touches
    for (uint8_t i = 0; i < 16; i++) {
        if ((currentTouched & (1 << i)) && !(lastTouched & (1 << i))) {
            // Step touched
            onStepTouch(i);
        }
        if (!(currentTouched & (1 << i)) && (lastTouched & (1 << i))) {
            // Step released
            onStepRelease(i);
        }
    }
    
    lastTouched = currentTouched;
}

/**
 * @brief Handle step touch events
 * @param stepIndex Step that was touched (0-15)
 */
void onStepTouch(uint8_t stepIndex) {
    // Toggle step gate
    sequencer.toggleStep(stepIndex);
    
    // Play step for immediate feedback
    sequencer.playStepNow(stepIndex);
    
    // Set as selected step for editing
    SystemState::getInstance().setSelectedStepForEdit(stepIndex);
    
    Serial.print("Step ");
    Serial.print(stepIndex);
    Serial.println(" touched");
}

/**
 * @brief Handle step release events
 * @param stepIndex Step that was released (0-15)
 */
void onStepRelease(uint8_t stepIndex) {
    // Clear step selection if this was the selected step
    if (SystemState::getInstance().getSelectedStepForEdit() == stepIndex) {
        SystemState::getInstance().setSelectedStepForEdit(-1);
    }
}

// -----------------------------------------------------------------------------
// 8. UTILITY FUNCTIONS
// -----------------------------------------------------------------------------

/**
 * @brief Print system status for debugging
 */
void printSystemStatus() {
    SystemState& state = SystemState::getInstance();
    
    Serial.println("=== System Status ===");
    Serial.print("Sequencer Running: ");
    Serial.println(sequencer.isRunning() ? "Yes" : "No");
    Serial.print("Current Step: ");
    Serial.println(sequencer.getPlayhead());
    Serial.print("Selected Step: ");
    Serial.println(state.getSelectedStepForEdit());
    Serial.print("Distance: ");
    Serial.print(state.getMM());
    Serial.println("mm");
    Serial.print("Note1: ");
    Serial.println(state.getNote1());
    Serial.print("Velocity: ");
    Serial.println(state.getVel1());
    Serial.println("====================");
}
