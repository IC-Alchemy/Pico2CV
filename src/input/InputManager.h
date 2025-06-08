/**
 * @file InputManager.h
 * @brief Manages all input sources (touch matrix, distance sensor, buttons)
 * 
 * This module centralizes input handling and provides a clean interface
 * for the main application to access input state.
 */

#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <stdint.h>
#include "../state/SystemState.h"

/**
 * @brief Input manager for centralized input handling
 * 
 * This class manages all input sources and updates the system state
 * with current input values.
 */
class InputManager {
public:
    InputManager();
    
    /**
     * @brief Initialize input systems
     */
    void init();
    
    /**
     * @brief Update all input sources
     * Should be called regularly from the main loop
     */
    void update();
    
    /**
     * @brief Get the currently selected step for editing
     * @return Step index (0-15) or -1 if none selected
     */
    int getSelectedStep() const { return selectedStep; }
    
    /**
     * @brief Check if a specific step is being touched
     * @param stepIndex Step to check (0-15)
     * @return true if step is being touched
     */
    bool isStepTouched(int stepIndex) const;
    
    /**
     * @brief Get distance sensor reading in millimeters
     * @return Distance in mm
     */
    int getDistanceMM() const { return distanceMM; }
    
    /**
     * @brief Check if record button 16 is held
     */
    bool isButton16Held() const { return button16Held; }
    
    /**
     * @brief Check if record button 17 is held
     */
    bool isButton17Held() const { return button17Held; }
    
    /**
     * @brief Check if record button 18 is held
     */
    bool isButton18Held() const { return button18Held; }

private:
    // Input state
    int selectedStep = -1;
    int distanceMM = 0;
    bool button16Held = false;
    bool button17Held = false;
    bool button18Held = false;
    bool stepTouched[16] = {false};
    
    // Update methods for different input sources
    void updateTouchMatrix();
    void updateDistanceSensor();
    void updateButtons();
    
    // Helper methods
    void updateSystemState();
};

#endif // INPUT_MANAGER_H
