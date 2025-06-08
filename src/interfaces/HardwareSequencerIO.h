/**
 * @file HardwareSequencerIO.h
 * @brief Concrete implementation of SequencerIO for hardware integration
 * 
 * This implementation provides the actual hardware interface for the sequencer,
 * bridging the abstraction with the real system components.
 */

#ifndef HARDWARE_SEQUENCER_IO_H
#define HARDWARE_SEQUENCER_IO_H

#include "SequencerIO.h"
#include "../state/SystemState.h"
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

// Forward declarations for external dependencies
extern midi::MidiInterface<midi::SerialMIDI<Adafruit_USBD_MIDI>> usb_midi;

/**
 * @brief Hardware implementation of SequencerIO interface
 * 
 * This class provides the concrete implementation that interfaces with
 * the actual hardware components and global state variables.
 */
class HardwareSequencerIO : public SequencerIO {
public:
    // MIDI Operations
    void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        usb_midi.sendNoteOn(note, velocity, channel);
    }
    
    void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        usb_midi.sendNoteOff(note, velocity, channel);
    }
    
    // Envelope Control
    void triggerEnvelope() override {
        SystemState::getInstance().setTrigEnv1(true);
    }
    
    void releaseEnvelope() override {
        SystemState::getInstance().setTrigEnv1(false);
    }
    
    // System State Access
    void setNote1(int note) override {
        SystemState::getInstance().setNote1(note);
    }
    
    void setFreq1(float freq) override {
        SystemState::getInstance().setFreq1(freq);
    }
    
    void setVel1(float velocity) override {
        SystemState::getInstance().setVel1(velocity);
    }
    
    // Scale Access
    int getScaleNote(int scaleIndex, int noteIndex) override {
        return SystemState::getInstance().getScaleNote(scaleIndex, noteIndex);
    }
    
    // Sensor Data
    int getDistanceMM() override {
        return SystemState::getInstance().getMM();
    }
    
    // UI State
    int getSelectedStepForEdit() override {
        return SystemState::getInstance().getSelectedStepForEdit();
    }
    
    bool isButton16Held() override {
        return SystemState::getInstance().getButton16Held();
    }
    
    bool isButton17Held() override {
        return SystemState::getInstance().getButton17Held();
    }
    
    bool isButton18Held() override {
        return SystemState::getInstance().getButton18Held();
    }
};

#endif // HARDWARE_SEQUENCER_IO_H
