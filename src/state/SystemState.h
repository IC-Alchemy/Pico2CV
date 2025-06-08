/**
 * @file SystemState.h
 * @brief Thread-safe system state management
 * 
 * This module provides thread-safe access to shared system state,
 * replacing the scattered volatile global variables with a structured approach.
 */

#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdint.h>
#include <atomic>

/**
 * @brief Thread-safe system state container
 * 
 * This class encapsulates all shared state variables and provides
 * atomic access methods to ensure thread safety between cores.
 */
class SystemState {
public:
    // Audio/DSP state
    std::atomic<int> note1{0};
    std::atomic<float> freq1{440.0f};
    std::atomic<float> vel1{0.5f};
    
    // Envelope state
    std::atomic<bool> trigenv1{false};
    std::atomic<bool> trigenv2{false};
    
    // UI state
    std::atomic<int> selectedStepForEdit{-1};
    std::atomic<int> mm{0};
    
    // Button states
    std::atomic<bool> button16Held{false};
    std::atomic<bool> button17Held{false};
    std::atomic<bool> button18Held{false};
    
    // Scale data (not atomic as it's read-only after initialization)
    int scale[5][48];
    
    // Singleton access
    static SystemState& getInstance() {
        static SystemState instance;
        return instance;
    }
    
    // Audio state setters/getters
    void setNote1(int note) { note1.store(note); }
    int getNote1() const { return note1.load(); }
    
    void setFreq1(float freq) { freq1.store(freq); }
    float getFreq1() const { return freq1.load(); }
    
    void setVel1(float vel) { vel1.store(vel); }
    float getVel1() const { return vel1.load(); }
    
    // Envelope state setters/getters
    void setTrigEnv1(bool trig) { trigenv1.store(trig); }
    bool getTrigEnv1() const { return trigenv1.load(); }
    
    void setTrigEnv2(bool trig) { trigenv2.store(trig); }
    bool getTrigEnv2() const { return trigenv2.load(); }
    
    // UI state setters/getters
    void setSelectedStepForEdit(int step) { selectedStepForEdit.store(step); }
    int getSelectedStepForEdit() const { return selectedStepForEdit.load(); }
    
    void setMM(int distance) { mm.store(distance); }
    int getMM() const { return mm.load(); }
    
    // Button state setters/getters
    void setButton16Held(bool held) { button16Held.store(held); }
    bool getButton16Held() const { return button16Held.load(); }
    
    void setButton17Held(bool held) { button17Held.store(held); }
    bool getButton17Held() const { return button17Held.load(); }
    
    void setButton18Held(bool held) { button18Held.store(held); }
    bool getButton18Held() const { return button18Held.load(); }
    
    // Scale access
    int getScaleNote(int scaleIndex, int noteIndex) const {
        if (scaleIndex >= 0 && scaleIndex < 5 && noteIndex >= 0 && noteIndex < 48) {
            return scale[scaleIndex][noteIndex];
        }
        return 0;
    }
    
    void setScaleNote(int scaleIndex, int noteIndex, int value) {
        if (scaleIndex >= 0 && scaleIndex < 5 && noteIndex >= 0 && noteIndex < 48) {
            scale[scaleIndex][noteIndex] = value;
        }
    }

private:
    SystemState() {
        // Initialize scale array to default values
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 48; j++) {
                scale[i][j] = j; // Default chromatic scale
            }
        }
    }
    
    // Prevent copying
    SystemState(const SystemState&) = delete;
    SystemState& operator=(const SystemState&) = delete;
};

#endif // SYSTEM_STATE_H
