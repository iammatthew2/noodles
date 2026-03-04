/// @brief Manages application state transitions and visual feedback (HOME,
/// SELECTING, CONTROL modes).

#include "StateManager.h"

#include "NeoTrellisController.h"

StateManager::StateManager(NeoTrellisController* trellis, int tonePin,
                           const AppDefinition* appDefs, int appCount)
    : controlState(HOME),
      selectedAppIndex(0),
      trellisController(trellis),
      tonePin(tonePin),
      apps(appDefs),
      appCount(appCount),
      selectBlinkOn(false),
      lastBlinkMs(0) {}

void StateManager::setSelectedAppIndex(int index) {
  if (index >= 0 && index < appCount) {
    selectedAppIndex = index;
    updateDisplay();
  }
}

ControlState StateManager::getControlStateForApp(int appIndex) {
  switch (appIndex) {
    case 0:
      return CONTROL_YODEL;
    case 1:
      return CONTROL_SKIPPY;
    case 2:
      return CONTROL_JIBBERS;
    case 3:
      return CONTROL_PICKLES;
    case 4:
      return CONTROL_PUDDLES;
    default:
      return HOME;
  }
}

const AppDefinition* StateManager::getCurrentApp() const {
  if (selectedAppIndex >= 0 && selectedAppIndex < appCount) {
    return &apps[selectedAppIndex];
  }
  return nullptr;
}

void StateManager::updateDisplay() {
  if (controlState == SELECTING) {
    // Business rule: SELECTING is a 4-choice app menu on keys 0-3 only.
    trellisController->clearPixels();
    // Selection menu options:
    // key 0 -> Skippy (app index 1)
    // key 1 -> Pickles (app index 3)
    // key 2 -> Puddles (app index 4)
    // key 3 -> Yodel (app index 0)
    const int optionAppIndices[4] = {1, 3, 4, 0};

    for (uint8_t key = 0; key < 4; key++) {
      int appIndex = optionAppIndices[key];
      if (appIndex >= 0 && appIndex < appCount) {
        const AppDefinition* optionApp = &apps[appIndex];
        trellisController->setPixelColor(
            key,
            trellisController->color(optionApp->r, optionApp->g, optionApp->b));
      }
    }
    trellisController->showPixels();
  } else if (controlState == HOME) {
    // Business rule: HOME has no persistent app menu UI from StateManager.
    trellisController->clearPixels();
    trellisController->showPixels();
  } else if (isInControlState()) {
    // Business rule: CONTROL clears all keys, then shows status on keys 14/15.
    // - Key 14: selected app identity color
    // - Key 15: control-active indicator
    trellisController->clearPixels();
    const AppDefinition* app = getCurrentApp();
    if (app) {
      trellisController->setPixelColor(
          14, trellisController->color(app->r, app->g, app->b));
      trellisController->setPixelColor(15, trellisController->color(0, 255, 0));
    }
    trellisController->showPixels();
  }
}

void StateManager::enterHome() {
  // Business rule: returning HOME resets app selection to default index 0.
  controlState = HOME;
  selectedAppIndex = 0;
  trellisController->clearPixels();
  trellisController->showPixels();
  Serial.println("State: HOME");
}

void StateManager::enterSelecting() {
  // Business rule: SELECTING is entered explicitly (button flow), then
  // rendered.
  controlState = SELECTING;
  selectBlinkOn = false;
  lastBlinkMs = millis();
  Serial.println("State: SELECTING");
  updateDisplay();
}

void StateManager::enterControl() {
  // Business rule: CONTROL state is derived from selected app index.
  ControlState newState = getControlStateForApp(selectedAppIndex);
  controlState = newState;
  Serial.print("State: CONTROL -> ");
  Serial.println(getCurrentApp()->name);
  tone(tonePin, 900, 120);
  updateDisplay();
}

void StateManager::toggleSelecting() {
  if (controlState == SELECTING) {
    enterHome();
  } else if (controlState == HOME) {
    enterSelecting();
  } else if (isInControlState()) {
    enterSelecting();
  }
}

void StateManager::updateBlink() {
  if (controlState != SELECTING) return;
  // No blink behavior in selecting mode; keep display steady.
}
