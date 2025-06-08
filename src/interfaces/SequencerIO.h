/**
 * @file SequencerIO.h
 * @brief Interface for abstracting sequencer I/O operations
 * 
 * This interface decouples the sequencer from global variables and hardware dependencies,
 * making it more modular and testable.
 */

#ifndef SEQUENCER_IO_H
#define SEQUENCER_IO_H

#include <stdint.h>

/**
 * @brief Abstract interface for sequencer I/O operations
 * 
 * This interface abstracts all external dependencies that the sequencer needs,
 * including MIDI output, envelope control, and system state access.
 */
class SequencerIO {
public:
    virtual ~SequencerIO() = default;
    
    // MIDI Operations
    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) = 0;
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) = 0;
    
    // Envelope Control
    virtual void triggerEnvelope() = 0;
    virtual void releaseEnvelope() = 0;
    
    // System State Access
    virtual void setNote1(int note) = 0;
    virtual void setFreq1(float freq) = 0;
    virtual void setVel1(float velocity) = 0;
    
    // Scale Access
    virtual int getScaleNote(int scaleIndex, int noteIndex) = 0;
    
    // Sensor Data
    virtual int getDistanceMM() = 0;
    
    // UI State
    virtual int getSelectedStepForEdit() = 0;
    virtual bool isButton16Held() = 0;
    virtual bool isButton17Held() = 0;
    virtual bool isButton18Held() = 0;
};

#endif // SEQUENCER_IO_H
