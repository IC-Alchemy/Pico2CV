# Sequencer Refactoring Guide

## Overview

This document describes the comprehensive refactoring of the Pico2CV sequencer firmware to improve modularity, testability, and maintainability. The refactoring addresses several architectural issues in the original codebase and introduces a clean, modular design.

## Problems Addressed

### 1. Mixed Responsibilities
**Original Issue**: The `Sequencer::advanceStep()` method handled both step advancement logic and live parameter recording, violating the Single Responsibility Principle.

**Solution**: Split into two separate methods:
- `advanceStep(uint8_t current_uclock_step)` - Pure step advancement logic
- `recordLiveParameters(...)` - Live parameter recording logic

### 2. Global Variable Dependencies
**Original Issue**: The sequencer was tightly coupled to global variables, making it difficult to test and maintain.

**Solution**: Introduced dependency injection through the `SequencerIO` interface:
```cpp
class SequencerIO {
public:
    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) = 0;
    virtual void setNote1(int note) = 0;
    virtual int getScaleNote(int scaleIndex, int noteIndex) = 0;
    // ... other interface methods
};
```

### 3. Thread Safety Issues
**Original Issue**: Volatile global variables scattered throughout the codebase without proper synchronization.

**Solution**: Centralized thread-safe state management:
```cpp
class SystemState {
    std::atomic<int> note1{0};
    std::atomic<float> freq1{440.0f};
    std::atomic<bool> trigenv1{false};
    // ... other atomic variables
};
```

## New Architecture

### Core Components

#### 1. SequencerIO Interface (`src/interfaces/SequencerIO.h`)
- **Purpose**: Abstracts all external dependencies of the sequencer
- **Benefits**: Enables dependency injection, improves testability
- **Methods**: MIDI operations, envelope control, system state access

#### 2. HardwareSequencerIO (`src/interfaces/HardwareSequencerIO.h`)
- **Purpose**: Concrete implementation for hardware integration
- **Benefits**: Bridges abstraction with real hardware components
- **Dependencies**: SystemState, MIDI interface

#### 3. SystemState (`src/state/SystemState.h`)
- **Purpose**: Thread-safe centralized state management
- **Benefits**: Eliminates scattered global variables, ensures thread safety
- **Features**: Atomic operations, singleton pattern, type safety

#### 4. Modular Components
- **InputManager** (`src/input/InputManager.h`): Centralized input handling
- **AudioEngine** (`src/audio/AudioEngine.h`): Audio processing and CV generation
- **ClockManager** (`src/clock/ClockManager.h`): Timing and clock management

### Dependency Flow

```
Main Application
├── InputManager ──→ SystemState
├── AudioEngine ──→ SystemState
├── ClockManager ──→ Sequencer
└── Sequencer ──→ SequencerIO ──→ SystemState
```

## Key Improvements

### 1. Separation of Concerns
Each component has a single, well-defined responsibility:
- **Sequencer**: Pure step sequencing logic
- **InputManager**: Input handling and state updates
- **AudioEngine**: Real-time audio processing
- **ClockManager**: Timing and synchronization

### 2. Dependency Injection
The sequencer no longer depends on global variables:
```cpp
// Before
void Sequencer::advanceStep(...) {
    note1 = midiNote;  // Direct global access
    trigenv1 = true;   // Direct global access
}

// After
void Sequencer::advanceStep(uint8_t step) {
    if (io) {
        io->setNote1(midiNote);     // Through interface
        io->triggerEnvelope();      // Through interface
    }
}
```

### 3. Thread-Safe State Management
Replaced volatile globals with atomic operations:
```cpp
// Before
volatile int note1;
volatile bool trigenv1;

// After
class SystemState {
    std::atomic<int> note1{0};
    std::atomic<bool> trigenv1{false};
    
    void setNote1(int note) { note1.store(note); }
    int getNote1() const { return note1.load(); }
};
```

### 4. Improved Testability
Components can now be tested in isolation:
```cpp
// Mock implementation for testing
class MockSequencerIO : public SequencerIO {
    void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // Record call for verification
    }
};

// Test setup
MockSequencerIO mockIO;
Sequencer sequencer(&mockIO);
```

## Migration Guide

### For Existing Code

1. **Replace Global Variable Access**:
   ```cpp
   // Old
   note1 = 60;
   
   // New
   SystemState::getInstance().setNote1(60);
   ```

2. **Update Sequencer Usage**:
   ```cpp
   // Old
   sequencer.advanceStep(step, mm, btn16, btn17, btn18, selectedStep);
   
   // New
   sequencer.advanceStep(step);
   sequencer.recordLiveParameters(mm, btn16, btn17, btn18, selectedStep);
   ```

3. **Use Dependency Injection**:
   ```cpp
   // Old
   Sequencer seq;
   
   // New
   HardwareSequencerIO io;
   Sequencer seq(&io);
   ```

### For New Features

1. **Extend SequencerIO Interface**: Add new methods for additional hardware
2. **Use SystemState**: Store shared state in the centralized manager
3. **Follow Component Pattern**: Create focused, single-responsibility modules

## Benefits Achieved

### 1. Maintainability
- Clear separation of concerns
- Reduced coupling between components
- Easier to understand and modify individual parts

### 2. Testability
- Components can be tested in isolation
- Mock implementations enable unit testing
- Dependency injection facilitates test setup

### 3. Scalability
- Easy to add new features without affecting existing code
- Modular design supports incremental development
- Clear interfaces enable parallel development

### 4. Reliability
- Thread-safe state management prevents race conditions
- Type-safe interfaces reduce runtime errors
- Centralized state management improves consistency

## File Structure

```
src/
├── interfaces/
│   ├── SequencerIO.h              # Abstract interface
│   └── HardwareSequencerIO.h     # Hardware implementation
├── state/
│   └── SystemState.h             # Thread-safe state management
├── input/
│   └── InputManager.h            # Input handling
├── audio/
│   └── AudioEngine.h             # Audio processing
├── clock/
│   └── ClockManager.h            # Timing management
└── sequencer/
    ├── Sequencer.h               # Refactored sequencer
    ├── Sequencer.cpp             # Implementation
    └── SequencerDefs.h           # Data structures
```

## Next Steps

### Immediate Improvements
1. **Implement Missing Components**: Complete InputManager, AudioEngine, and ClockManager implementations
2. **Add Unit Tests**: Create test suites for each component
3. **Performance Optimization**: Profile and optimize critical paths

### Future Enhancements
1. **Configuration Management**: Add persistent settings storage
2. **Plugin Architecture**: Enable runtime feature loading
3. **Remote Control**: Add network-based control interface
4. **Advanced Sequencing**: Implement polyrhythms, probability, etc.

## Conclusion

This refactoring transforms the Pico2CV sequencer from a monolithic, tightly-coupled system into a modular, maintainable, and testable architecture. The new design follows software engineering best practices while maintaining the real-time performance requirements of the embedded system.

The separation of concerns, dependency injection, and thread-safe state management provide a solid foundation for future development and ensure the codebase remains manageable as it grows in complexity.
