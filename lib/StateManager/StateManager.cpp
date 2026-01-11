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
    trellisController->clearPixels();
    const AppDefinition* app = getCurrentApp();
    if (app) {
      trellisController->setPixelColor(
          0, trellisController->color(app->r, app->g, app->b));
    }
    // Blink will be handled by updateBlink()
    if (selectBlinkOn) {
      trellisController->setPixelColor(15, trellisController->color(255, 0, 0));
    }
    trellisController->showPixels();
  } else if (controlState == HOME) {
    trellisController->clearPixels();
    trellisController->showPixels();
  } else if (isInControlState()) {
    trellisController->clearPixels();
    const AppDefinition* app = getCurrentApp();
    if (app) {
      trellisController->setPixelColor(
          0, trellisController->color(app->r, app->g, app->b));
      trellisController->setPixelColor(15, trellisController->color(0, 255, 0));
    }
    trellisController->showPixels();
  }
}

void StateManager::enterHome() {
  controlState = HOME;
  selectedAppIndex = 0;
  trellisController->clearPixels();
  trellisController->showPixels();
  Serial.println("State: HOME");
}

void StateManager::enterSelecting() {
  controlState = SELECTING;
  selectBlinkOn = false;
  lastBlinkMs = millis();
  Serial.println("State: SELECTING");
  updateDisplay();
}

void StateManager::enterControl() {
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
  unsigned long now = millis();
  if (now - lastBlinkMs >= BLINK_INTERVAL_MS) {
    lastBlinkMs = now;
    selectBlinkOn = !selectBlinkOn;
    updateDisplay();
  }
}
