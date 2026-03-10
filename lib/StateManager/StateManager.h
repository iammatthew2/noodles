/// @brief Manages application state transitions and visual feedback (HOME,
/// SELECTING, CONTROL modes).

#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>

class NeoTrellisController;

enum ControlState {
  HOME,
  SELECTING,
  CONTROL_YODEL,
  CONTROL_SKIPPY,
  CONTROL_JIBBERS,
  CONTROL_PICKLES,
  CONTROL_PUDDLES,
  CONTROL_NURBO
};

struct AppDefinition {
  const char* name;
  const char* topic;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

class StateManager {
 public:
  StateManager(NeoTrellisController* trellis, int tonePin,
               const AppDefinition* appDefs, int appCount);

  // State getters and setters
  ControlState getCurrentState() const { return controlState; }
  int getSelectedAppIndex() const { return selectedAppIndex; }
  void setSelectedAppIndex(int index);

  // State transitions
  void enterHome();
  void enterSelecting();
  void enterControl();
  void toggleSelecting();  // Convenience: HOME->SELECTING or SELECTING->HOME or
                           // CONTROL->SELECTING

  // Display updates
  void updateBlink();  // Call from loop() to handle blinking effects

  // Helper checks
  bool isInControlState() const { return controlState >= CONTROL_YODEL; }
  bool isSelecting() const { return controlState == SELECTING; }
  bool isHome() const { return controlState == HOME; }

  // Get app count
  int getAppCount() const { return appCount; }
  const AppDefinition* getApps() const { return apps; }
  const AppDefinition* getCurrentApp() const;

 private:
  ControlState controlState;
  int selectedAppIndex;
  NeoTrellisController* trellisController;
  int tonePin;
  const AppDefinition* apps;
  int appCount;
  bool selectBlinkOn;
  unsigned long lastBlinkMs;
  static const unsigned long BLINK_INTERVAL_MS = 300;

  ControlState getControlStateForApp(int appIndex);
  void updateDisplay();
};

#endif
