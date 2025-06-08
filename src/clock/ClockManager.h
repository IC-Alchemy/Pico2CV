/**
 * @file ClockManager.h
 * @brief Clock and timing management for the sequencer
 * 
 * This module handles all timing-related functionality including
 * uClock integration and step timing.
 */

#ifndef CLOCK_MANAGER_H
#define CLOCK_MANAGER_H

#include <stdint.h>
#include <functional>

/**
 * @brief Clock manager for sequencer timing
 * 
 * This class manages clock sources and provides timing callbacks
 * for the sequencer engine.
 */
class ClockManager {
public:
    ClockManager();
    
    /**
     * @brief Initialize clock system
     */
    void init();
    
    /**
     * @brief Start the clock
     */
    void start();
    
    /**
     * @brief Stop the clock
     */
    void stop();
    
    /**
     * @brief Set the BPM (beats per minute)
     * @param bpm Tempo in BPM
     */
    void setBPM(float bpm);
    
    /**
     * @brief Get current BPM
     * @return Current BPM
     */
    float getBPM() const { return currentBPM; }
    
    /**
     * @brief Set step callback function
     * @param callback Function to call on each step (receives step number 0-15)
     */
    void setStepCallback(std::function<void(uint8_t)> callback) {
        stepCallback = callback;
    }
    
    /**
     * @brief Set clock callback function
     * @param callback Function to call on each clock tick (96 PPQN)
     */
    void setClockCallback(std::function<void()> callback) {
        clockCallback = callback;
    }
    
    /**
     * @brief Update clock (call from main loop)
     */
    void update();
    
    /**
     * @brief Check if clock is running
     * @return true if clock is running
     */
    bool isRunning() const { return running; }
    
    /**
     * @brief Get current step position (0-15)
     * @return Current step
     */
    uint8_t getCurrentStep() const { return currentStep; }
    
    /**
     * @brief Get current tick position within step
     * @return Tick position (0-5 for 16th notes at 96 PPQN)
     */
    uint8_t getCurrentTick() const { return currentTick; }

private:
    float currentBPM = 120.0f;
    bool running = false;
    
    // Step tracking
    uint8_t currentStep = 0;
    uint8_t currentTick = 0;
    uint32_t lastStepTime = 0;
    uint32_t stepInterval = 0; // microseconds between steps
    
    // Callbacks
    std::function<void(uint8_t)> stepCallback;
    std::function<void()> clockCallback;
    
    // Internal methods
    void calculateStepInterval();
    void handleStepAdvance();
    void handleClockTick();
};

#endif // CLOCK_MANAGER_H
