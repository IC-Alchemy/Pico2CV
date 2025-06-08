/**
 * @file Sequencer.h
 * @brief Modular 16-step sequencer class interface.
 *
 * Provides a step sequencer with step toggle, note assignment, playhead
 * advance, and state query. Designed for integration with matrix scanning and
 * output modules (MIDI, gate).
 *
 * Example:
 *   #include "Sequencer.h"
 *   SequencerIO* io = new HardwareSequencerIO();
 *   Sequencer seq(io);
 *   // Initialize and start
 *   seq.init();
 *   seq.start();
 *
 *   // Configure steps
 *   // step 0: gate on, slide off, note index 8, velocity 0.75, filter 0.3
 *   seq.setStep(0, true, false, 8, 0.75f, 0.3f);
 *   // step 1 using Step object
 *   seq.setStep(1, Step(true, true, 12, 1.0f, 0.5f));
 *
 *   // In clock callback
 *   void onClockTick(uint8_t beat) {
 *       seq.advanceStep(beat);
 *       const Step& stepData = seq.getStep(beat);
 *       // Use stepData.gate, stepData.note, stepData.velocity, stepData.filter
 *   }
 */

#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "SequencerDefs.h"
#include "../interfaces/SequencerIO.h"

#define SEQUENCER_NUM_STEPS 16

class Sequencer {
public:
  Sequencer();
  Sequencer(SequencerIO* io);
  
  // Set the I/O interface (for dependency injection)
  void setIO(SequencerIO* io) { this->io = io; }

  // Step length (number of steps in the sequence, user-adjustable, max 16)
  uint8_t getStepLength() const { return stepLength; }
  void setStepLength(uint8_t len) { stepLength = (len > 0 && len <= SEQUENCER_NUM_STEPS) ? len : SEQUENCER_NUM_STEPS; }

  // Instantly play a step for real-time feedback (does not advance playhead)
  void playStepNow(uint8_t stepIdx);

  void init();

  // Start/stop sequencer
  void start();
  void stop();
  void reset();

  /**
   * @brief Processes the sequencer logic for the given step.
   * @param current_uclock_step The current step number (0-15) provided by uClock.
   * 
   * This method has been refactored to separate concerns:
   * - Pure step advancement logic
   * - Parameter recording is handled separately
   */
  void advanceStep(uint8_t current_uclock_step);
  
  /**
   * @brief Records live parameters for the currently selected step.
   * @param mm_distance Distance sensor reading
   * @param is_button16_held Button 16 state
   * @param is_button17_held Button 17 state  
   * @param is_button18_held Button 18 state
   * @param current_selected_step_for_edit Currently selected step for editing
   */
  void recordLiveParameters(int mm_distance, bool is_button16_held, 
                           bool is_button17_held, bool is_button18_held,
                           int current_selected_step_for_edit);

  // Toggle step ON/OFF
  void toggleStep(uint8_t stepIdx);

  // Set note for a step
  void setStepNote(uint8_t stepIdx, uint8_t note);
  void setStepVelocity(uint8_t stepIdx, uint8_t velocity);
  void setStepFiltFreq(uint8_t stepIdx, float filter);
  
  // Set full step data (overloads)
  void setStep(int index, bool gate, bool slide, int note, float velocity, float filter);
  void setStep(int index, const Step& stepData);

  // Query step and playhead state
  const Step &getStep(uint8_t stepIdx) const;
  uint8_t getPlayhead() const;
  bool isRunning() const;
  
  int8_t getLastNote() const;
  void setLastNote(int8_t note);

  const SequencerState& getState() const;
  void triggerEnvelope();
  void releaseEnvelope();

  // Monophonic note duration tracking (Step 2 integration plan)
  /**
   * @brief Start a monophonic note with a specified duration (in ticks).
   * @param note MIDI note number to play.
   * @param duration Number of ticks the note should last.
   */
  void startNote(uint8_t note, float velocity, uint16_t duration);

  /**
   * @brief Decrement the note duration counter. If zero, sends NoteOff and clears state.
   */
  void tickNoteDuration();

  /**
   * @brief Sends NoteOff for the current note and clears the active note state.
   */
  void handleNoteOff();

private:
  // I/O interface for hardware abstraction
  SequencerIO* io = nullptr;
  
  // Sequencer state now stored in SequencerState from SequencerDefs.h
  void resetState();
  void initializeSteps();
  bool validateState() const;
  SequencerState state;
  bool errorFlag = false;

  /**
   * @brief Tracks the last played MIDI note for proper noteOff handling.
   * Stores the actual MIDI note value sent. -1 means no note is currently playing.
   */
  int8_t lastNote = -1;
  
  uint8_t stepLength = SEQUENCER_NUM_STEPS; // Default 16, user-adjustable

  // Monophonic note duration tracking variables
  int8_t currentNote = -1;           // -1 means no note is currently active
  uint16_t noteDurationCounter = 0;  // Remaining duration in ticks
};

#endif // SEQUENCER_H
