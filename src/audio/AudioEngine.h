/**
 * @file AudioEngine.h
 * @brief Audio processing engine for CV output and DSP
 * 
 * This module handles all audio-rate processing including CV generation,
 * envelope processing, and filter control.
 */

#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <stdint.h>
#include "../state/SystemState.h"

/**
 * @brief Audio processing engine
 * 
 * This class manages all audio-rate processing and CV output generation.
 * It runs on Core 0 at 8kHz for real-time performance.
 */
class AudioEngine {
public:
    AudioEngine();
    
    /**
     * @brief Initialize audio engine and CV outputs
     */
    void init();
    
    /**
     * @brief Process one audio sample (called at 8kHz)
     * This is the main audio callback that generates CV outputs
     */
    void processSample();
    
    /**
     * @brief Set the sample rate
     * @param sampleRate Sample rate in Hz (default 8000)
     */
    void setSampleRate(float sampleRate) { this->sampleRate = sampleRate; }
    
    /**
     * @brief Get current CV1 output (pitch)
     * @return CV1 value (0.0-1.0 for PWM)
     */
    float getCV1() const { return cv1Output; }
    
    /**
     * @brief Get current CV2 output (velocity)
     * @return CV2 value (0.0-1.0 for PWM)
     */
    float getCV2() const { return cv2Output; }
    
    /**
     * @brief Get current CV3 output (filter)
     * @return CV3 value (0.0-1.0 for PWM)
     */
    float getCV3() const { return cv3Output; }
    
    /**
     * @brief Get current CV4 output (envelope)
     * @return CV4 value (0.0-1.0 for PWM)
     */
    float getCV4() const { return cv4Output; }

private:
    float sampleRate = 8000.0f;
    
    // CV outputs (0.0-1.0 for PWM)
    float cv1Output = 0.0f; // Pitch CV (1V/octave)
    float cv2Output = 0.0f; // Velocity CV
    float cv3Output = 0.0f; // Filter CV
    float cv4Output = 0.0f; // Envelope CV
    
    // Envelope state
    float envelopeLevel = 0.0f;
    bool envelopeActive = false;
    bool lastTrigState = false;
    
    // Envelope parameters (in samples)
    float attackTime = 0.01f * 8000.0f;   // 10ms attack
    float decayTime = 0.1f * 8000.0f;     // 100ms decay
    float sustainLevel = 0.7f;            // 70% sustain
    float releaseTime = 0.2f * 8000.0f;   // 200ms release
    
    enum EnvelopeStage {
        ENV_IDLE,
        ENV_ATTACK,
        ENV_DECAY,
        ENV_SUSTAIN,
        ENV_RELEASE
    };
    
    EnvelopeStage envelopeStage = ENV_IDLE;
    float envelopeCounter = 0.0f;
    
    // Processing methods
    void processEnvelope();
    void updateCVOutputs();
    
    // Helper methods
    float noteToCV(int midiNote);
    float velocityToCV(float velocity);
    float filterToCV(float filterValue);
};

#endif // AUDIO_ENGINE_H
