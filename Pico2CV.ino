
// -----------------------------------------------------------------------------
 // 1. INCLUDES & DEFINES
// -----------------------------------------------------------------------------
// --- CV PWM Pin Definitions ---
#define CV1_PWM_PIN 2   // Pitch
#define CV2_PWM_PIN 3   // Velocity
#define CV3_PWM_PIN 4   // Filter
#define CV4_PWM_PIN 5   // Envelope
// Adjust pins as needed for your hardware
// --- Audio & DSP ---
#include <Adafruit_NeoPixel.h>


#include "src/dsp/adsr.h"
#include "src/dsp/ladder.h"
#include "src/dsp/phasor.h"
#include <cmath>
#include <cstdint>
//  #define DEBUG

// --- Sequencer ---
#include "src/sequencer/Sequencer.h"

#include <Melopero_VL53L1X.h>


#include <Wire.h>

// --- MIDI & USB ---
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <uClock.h>


// --- Touch Matrix ---
#include "src/matrix/Matrix.h"
#include <Adafruit_MPR121.h> // https://github.com/adafruit/Adafruit_MPR121_Library
 // -----------------------------------------------------------------------------
// 2. CONSTANTS & GLOBALS
// -----------------------------------------------------------------------------

// --- Step Selection & Pad Timing ---
volatile int selectedStepForEdit = -1; // -1 means no step selected
 //  Distance Sensor
volatile int raw_mm = 0;
volatile int mm = 0;
Melopero_VL53L1X sensor;


#define IRQ_PIN 1

#define NOTE_LENGTH    12 // min: 1 max: 23 DO NOT EDIT BEYOND!!! 12 = 50% on 96ppqn, same as original \
     // tb303. 62.5% for triplets time signature

// --- Sequencer ---
Sequencer seq;

// --- MIDI & Clock ---
Adafruit_USBD_MIDI raw_usb_midi;
midi::SerialMIDI<Adafruit_USBD_MIDI> serial_usb_midi(raw_usb_midi);
midi::MidiInterface<midi::SerialMIDI<Adafruit_USBD_MIDI>>
    usb_midi(serial_usb_midi);
uint8_t bpm_blink_timer = 1;

// --- Touch Matrix ---
const int PIN_TOUCH_IRQ = 6;
Adafruit_MPR121 touchSensor = Adafruit_MPR121();

// --- Multicore Counication ---
volatile int note1 = 48, note2 = 48;
volatile bool  trigenv1, trigenv2, dualEnvFlag;
volatile bool buttonEventFlag = false;
volatile uint8_t buttonEventIndex = 0;
volatile uint8_t buttonEventType = 0;
volatile uint8_t Note = 0;
volatile float vel1 = 0;
volatile float freq1 = 0.0f;
// Add button state tracking variables
volatile bool button16Held = false;
volatile bool button17Held = false;
volatile bool button18Held = false;
volatile bool recordButtonHeld = false;



// --- Oscillators & Envelopes ---
daisysp::LadderFilter filter;
daisysp::Adsr env1;
daisysp::Adsr env2;

// --- Audio Buffer Pool ---
audio_buffer_pool_t *producer_pool = nullptr;

// --- Timing ---
unsigned long previousMillis = 0;
const long interval = 1; // ms

// -----------------------------------------------------------------------------
// 3. UTILITY FUNCTIONS
// -----------------------------------------------------------------------------



float applyFilterFrequency(float targetFreq) {
  static float currentFreq = 0.0f;
  const float smoothingAlpha = 0.1f;

  currentFreq =
      smoothingAlpha * targetFreq + (1.0f - smoothingAlpha) * currentFreq;
  return currentFreq;
}


// -----------------------------------------------------------------------------
// 6. AUDIO: OSCILLATOR & ENVELOPE INITIALIZATION
// -----------------------------------------------------------------------------

void initOscillators() {
  
  env1.Init(SAMPLE_RATE);
  env2.Init(SAMPLE_RATE);
  env1.SetReleaseTime(.07f);
  env1.SetAttackTime(0.0016f);
  env2.SetAttackTime(0.001f);
  env1.SetDecayTime(0.05f);
  env2.SetDecayTime(0.121f);
  env1.SetSustainLevel(0.3f);
  env2.SetSustainLevel(0.3f);
  env2.SetReleaseTime(0.03f);

}

// -----------------------------------------------------------------------------
// MATRIX EVENT HANDLER
// -----------------------------------------------------------------------------

void matrixEventHandler(const MatrixButtonEvent &evt) {
#ifndef DEBUG
  // Debug: Print all matrix events
  Serial.print("[MATRIX] Event! Index: ");
  Serial.print(evt.buttonIndex);
  Serial.print(", Type: ");
  Serial.println(evt.type == MATRIX_BUTTON_PRESSED ? "PRESSED" : "RELEASED");
#endif

  // Use a static array for timing, only for pads 0-15, scoped to this function
  static unsigned long padPressTimestamps[16] = {0};

  if (evt.buttonIndex < 16) { // Sequencer steps 0-15 (Voice 1)
    if (evt.type == MATRIX_BUTTON_PRESSED) {
      padPressTimestamps[evt.buttonIndex] = millis();
    } else if (evt.type == MATRIX_BUTTON_RELEASED) {
      unsigned long pressDuration = millis() - padPressTimestamps[evt.buttonIndex];
      if (pressDuration < 400) {
        // Single tap: toggle gate state
        seq.toggleStep(evt.buttonIndex);
        bool gateState = seq.getStep(evt.buttonIndex).gate;
        Serial.print("[MATRIX] Step ");
        Serial.print(evt.buttonIndex);
        Serial.print(" gate toggled (single tap). New gate value: ");
        Serial.println(gateState ? "ON" : "OFF");
        // Optionally, update OLED or UI here to reflect new gate state
      } else {
        // Long press: select step for parameter editing
        if (selectedStepForEdit != evt.buttonIndex) {
          selectedStepForEdit = evt.buttonIndex;
#ifndef DEBUG
          Serial.print("[MATRIX] Step ");
          Serial.print(evt.buttonIndex);
          Serial.println(" selected for editing (long press).");
#endif
        } else {
          // Already selected, deselect
          selectedStepForEdit = -1;
#ifndef DEBUG
          Serial.print("[MATRIX] Step ");
          Serial.print(evt.buttonIndex);
          Serial.println(" deselected (long press on already selected step).");
#endif
        }
      }
    }
  } else {
    if (evt.type == MATRIX_BUTTON_PRESSED) {
      switch (evt.buttonIndex) {
      case 16: // Button 16 (Note)
        button16Held = true;
#ifndef DEBUG
        Serial.println("[MATRIX] Button 16 (Note) held.");
#endif
        break;
      case 17: // Button 17 (Velocity)
        button17Held = true;   
#ifndef DEBUG
        Serial.println("[MATRIX] Button 17 (Velocity) held.");
#endif
        break;
      case 18: // Button 18 (Filter)
        button18Held = true;
#ifndef DEBUG
        Serial.println("[MATRIX] Button 18 (Filter) held.");
#endif
        break;
      case 19: // Record button
#ifndef DEBUG
        Serial.println("[MATRIX] Record button held.");
#endif
        break;
      // Add param4 or other parameter buttons here if needed
      default:
#ifndef DEBUG
        Serial.print("[MATRIX] Unhandled Button Pressed: ");
        Serial.println(evt.buttonIndex);
#endif
        break;
      }
    } else if (evt.type == MATRIX_BUTTON_RELEASED) {
      switch (evt.buttonIndex) {
      case 16:
        button16Held = false;
#ifndef DEBUG
        Serial.println("[MATRIX] Button 16 (Note) released.");
#endif
        break;
      case 17:
        button17Held = false;
#ifndef DEBUG
        Serial.println("[MATRIX] Button 17 (Velocity) released.");
#endif
        break;
      case 18:
        button18Held = false;
#ifndef DEBUG
        Serial.println("[MATRIX] Button 18 (Filter) released.");
#endif
        break;
      default:
        break;
      }
    }
  
}}

// -----------------------------------------------------------------------------
// 7. MIDI & CLOCK HANDLERS
// -----------------------------------------------------------------------------

void onSync24Callback(uint32_t tick) {
  usb_midi.sendRealTime(midi::Clock);
  seq.tickNoteDuration();
}

void onClockStart() {
  Serial.println("[uCLOCK] onClockStart() called.");
  usb_midi.sendRealTime(midi::Start); // MIDI Start message
  seq.start();
}

void onClockStop() {
  Serial.println("[uCLOCK] onClockStop() called.");
  usb_midi.sendRealTime(midi::Stop); // MIDI Stop message
  seq.stop();
}

void seqStoppedMode() {
if (!seq.isRunning()) {


}
}


/**
 * Monophonic step callback: handles rest, note length, and MIDI for a single
 * note. Preserves rests, glide (if implemented), note length, and MIDI
 * handling.
 */
void onStepCallback(uint32_t step) { // uClock provides the current step number
  // Ensure the step value wraps to the sequencer's number of steps (0-15)
  uint8_t wrapped_step = static_cast<uint8_t>(step % SEQUENCER_NUM_STEPS);

  // Advance the sequencer, passing all necessary external states
  seq.advanceStep(wrapped_step, mm,
                   button16Held, button17Held, button18Held,
                  selectedStepForEdit);

  // --- CV1–CV3 PWM Output Update (per step) ---
  // Access current step data
  const Step& currentStep = seq.getStep(wrapped_step);

  // Helper lambdas for mapping to PWM (0–65535 for 0–5V)
  auto mapToPWM = [](float norm) -> uint16_t {
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    return static_cast<uint16_t>(norm * 65535.0f);
  };
  // Pitch mapping: map note (0–24) to 0–5V (full range)
  auto noteToPWM = [](int note) -> uint16_t {
    float v = (note < 0) ? 0.0f : (note > 24 ? 1.0f : note / 24.0f);
    return static_cast<uint16_t>(v * 65535.0f);
  };

  // CV1: Pitch
  analogWrite(CV1_PWM_PIN, noteToPWM(currentStep.note));
  // CV2: Velocity
  analogWrite(CV2_PWM_PIN, mapToPWM(currentStep.velocity));
  // CV3: Filter
  analogWrite(CV3_PWM_PIN, mapToPWM(currentStep.filter));

  // --- One-shot parameter record at the beginning of each step ---
  static int lastStepIndex = -1;
  

}

// -----------------------------------------------------------------------------
// 8. ARDUINO SETUP FUNCTIONS
// -----------------------------------------------------------------------------
// Helper function to initialize envelope triggers
void initEnvelopeTriggers() {
  trigenv1 = false;
  trigenv2 = false;
}


void setup() {
analogWriteFreq(120000);
analogWriteResolution(16);

initEnvelopeTriggers();
}


void setup1() {
 
 
#if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
  // Initialize TinyUSB stack. This should be done once, early, on the core
  // handling USB.
  TinyUSB_Device_Init(0);
#endif
  usb_midi.begin(MIDI_CHANNEL_OMNI);

  delay(random(333));
  randomSeed(
      analogRead(A0) +
      millis()); // Use an unconnected analog pin and millis for better seed

  // initOLED();
#ifndef DEBUG
Serial.print(" CORE1 SETUP1 ... ");
   delay(500);
#endif
  VL53L1_Error status = 0;
  Wire.begin();               // use Wire1.begin() to use I2C-1
  sensor.initI2C(0x29, Wire); // use sensor.initI2C(0x29, Wire1); to use I2C-1

  status = sensor.initSensor();
  status = sensor.setDistanceMode(VL53L1_DISTANCEMODE_MEDIUM);
  status = sensor.setMeasurementTimingBudgetMicroSeconds(25000);
  status = sensor.setInterMeasurementPeriodMilliSeconds(30);
  status = sensor.clearInterruptAndStartMeasurement();

  seq.init();
#ifndef DEBUG
  Serial.print(" ...Distance Sensor Initialized... ");
#endif


  // Setup clock system
  uClock.init();
  uClock.setOnSync24(onSync24Callback);
  uClock.setOnClockStart(onClockStart);
  uClock.setOnClockStop(onClockStop);
  uClock.setOnStep(onStepCallback);
  uClock.setTempo(90);
  uClock.start();
  delay(45);


  if (!touchSensor.begin()) {
#ifndef DEBUG
    Serial.print(" ... ERROR - MPR121 not found... ");
#endif
    while (1) {
      delay(55);
    }
  } else {
#ifndef DEBUG
    Serial.print("... MPR121 is Rockin!....");
#endif
  }

  Matrix_init(&touchSensor);
  Matrix_setEventHandler(matrixEventHandler); // Register the event handler

  pinMode(PIN_TOUCH_IRQ, INPUT);
#ifndef DEBUG
  Serial.println("Core 1: Setup1 complete.");
#endif
  delay(100);
        seq.setStepFiltFreq(0, 1222.f);
        seq.setStepFiltFreq(8, 888.f); 
            seq.setStepFiltFreq(12, 500.f);
        seq.setStepFiltFreq(2, 1000.f);       seq.setStepFiltFreq(1, 1222.f);
        seq.setStepFiltFreq(3, 888.f); 
            seq.setStepFiltFreq(12, 500.f);
        seq.setStepFiltFreq(6, 888.f);
        seq.setStepNote(1, 0);
        seq.setStepNote(2, 1);
        seq.setStepNote(3, 2);
        seq.setStepNote(4, 0);
        seq.setStepNote(5, 1);
        seq.setStepNote(6, 2);
        seq.setStepNote(7, 3);
        seq.setStepNote(8, 4);
        seq.setStepNote(9, 5);
        seq.setStepNote(10, 6);
        seq.setStepNote(11, 7);
        seq.setStepNote(12, 8);
        seq.setStepNote(13, 9);
        seq.setStepNote(14, 10);
        seq.setStepNote(15, 11);
        seq.setStepNote(16, 12);
    seq.setStepVelocity(0, 0.5f);
    seq.setStepVelocity(1, 0.5f);
    seq.setStepVelocity(2, 0.5f);
    seq.setStepVelocity(3, 0.5f);
    seq.setStepVelocity(4, 0.5f);
    seq.setStepVelocity(5, 0.5f);
    seq.setStepVelocity(6, 0.5f);
    seq.setStepVelocity(7, 0.5f);
    seq.setStepVelocity(8, 0.5f);
    seq.setStepVelocity(9, 0.5f);
    seq.setStepVelocity(10, 0.5f);
    seq.setStepVelocity(11, 0.5f);
    seq.setStepVelocity(12, 0.5f);
    seq.setStepVelocity(13, 0.5f);
    seq.setStepVelocity(14, 0.5f);
    seq.setStepVelocity(15, 0.5f);

}

// ------------------------------------------------------------------------
 // 9. MAIN LOOPS
// --------------------------------------------------------------------------

void update() {
  sensor.waitMeasurementDataReady();  // wait for the data
  sensor.getRangingMeasurementData(); // get the data
  // the measurement data is stored in an instance variable:
// Forward declaration for audio loop CPU usage reporting
  // sensor.measurementData.RangeMilliMeter

  sensor.clearInterruptAndStartMeasurement();
  mm = sensor.measurementData.RangeMilliMeter; // Starts a new measurement cycle
}

void loop() {
//  Use this loop to write to the PWM Outputs
}
void doLEDStuff() {

  // --- LED Pulsing Cyan Feedback for Selected Step ---
  static int lastSelectedStep = -1;
  static bool ledWasActive = false;

  if (selectedStepForEdit != -1) {
    // Check if selected step is currently playing and gate is ON
    bool isPlayhead = (seq.getState().playhead == selectedStepForEdit);
    bool gateOn = seq.getStep(selectedStepForEdit).gate;

    if (isPlayhead && gateOn) {
      // Gate state indication should override cyan pulse (example: white)
      setStepLedColor((uint8_t)selectedStepForEdit, 255, 255, 255);
      // Debug
      // Serial.println("[LED] Selected step is playhead and gate is ON: white.");
    } else {
      // Smooth pulse: sine wave, 1.5 Hz
      float t = millis() / 1000.0f;
      float pulse = 0.5f * (1.0f + sinf(2.0f * 3.1415926f * 1.5f * t)); // 0..1
      uint8_t brightness = (uint8_t)(pulse * 255.0f);

      // Cyan: (0, brightness, brightness)
      setStepLedColor((uint8_t)selectedStepForEdit, 0, brightness, brightness);
      // Debug
      // Serial.println("[LED] Selected step LED pulsing cyan.");
    }

    lastSelectedStep = selectedStepForEdit;
    ledWasActive = true;
  } else if (ledWasActive && lastSelectedStep != -1) {
    // Turn off or restore LED when deselected
    setStepLedColor((uint8_t)lastSelectedStep, 0, 0, 0);
    ledWasActive = false;
    lastSelectedStep = -1;
  }
}
void loop1() {
  usb_midi.read();

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 1) {

        previousMillis = currentMillis;

    update();
doLEDStuff();
    Matrix_scan(); // Add this line to process touch matrix events
  
}
}