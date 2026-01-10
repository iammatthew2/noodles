/* This example shows basic usage of the NeoTrellis.
  The buttons will light up various colors when pressed.
  The interrupt pin is not used in this example.
*/

#include "Adafruit_NeoTrellis.h"
#include <RotaryEncoder.h>

Adafruit_NeoTrellis trellis;

// Rotary encoder pins (adjust to available pins on Nano 33 IoT)
RotaryEncoder encoder1(3, 4, RotaryEncoder::LatchMode::TWO03);
RotaryEncoder encoder2(5, 6, RotaryEncoder::LatchMode::TWO03);

int lastPos1 = 0, lastPos2 = 0;

const int TONE_PIN = 7;
const int BUTTON1_PIN = 8;
const int BUTTON2_PIN = 9;
const int KILL_SWITCH_PIN = 2; // unused for now. Wired up and ready.

int lastButton1State = HIGH;
int lastButton2State = HIGH;

//define a callback for key presses
TrellisCallback blink(keyEvent evt){
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    Serial.print("yPressed: ");
    Serial.println(evt.bit.NUM);
    tone(TONE_PIN, 440 + (evt.bit.NUM * 50), 100);
    trellis.pixels.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, trellis.pixels.numPixels(), 0, 255))); //on rising
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    Serial.print("xReleased: ");
    Serial.println(evt.bit.NUM);
    trellis.pixels.setPixelColor(evt.bit.NUM, 0);
  }

  // Turn on/off the neopixels!
  trellis.pixels.show();

  return 0;
}

void setup() {
  Serial.begin(9600);
  pinMode(TONE_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  // while(!Serial) delay(1);
  
  if (!trellis.begin()) {
    Serial.println("Could not start trellis, check wiring?");
    while(1) delay(1);
  } else {
    Serial.println("NeoPixel Trellis started");
  }

  //activate all keys and set callbacks
  for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i, blink);
  }

  //do a little animation to show we're on
  for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, Wheel(map(i, 0, trellis.pixels.numPixels(), 0, 255)));
    trellis.pixels.show();
    delay(50);
  }
  for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, 0x000000);
    trellis.pixels.show();
    delay(50);
  }
  
  Serial.println("Setup complete. Waiting for input...");
  tone(TONE_PIN, 523, 50); // C5
}

void loop() {
  trellis.read();  // interrupt management does all the work! :)
  
  // Handle button 1
  int button1State = digitalRead(BUTTON1_PIN);
  if (button1State == LOW && lastButton1State == HIGH) {
    Serial.println("Button 1 Pressed");
    tone(TONE_PIN, 440, 100); // A4
  }
  lastButton1State = button1State;
  
  // Handle button 2
  int button2State = digitalRead(BUTTON2_PIN);
  if (button2State == LOW && lastButton2State == HIGH) {
    Serial.println("Button 2 Pressed");
    tone(TONE_PIN, 550, 100); // C#5
  }
  lastButton2State = button2State;
  
  // Handle encoder 1
  encoder1.tick();
  int newPos1 = encoder1.getPosition();
  if (newPos1 != lastPos1) {
    if (newPos1 > lastPos1) {
      Serial.println("Encoder 1: CW");
      tone(TONE_PIN, 523, 50); // C5
    } else {
      Serial.println("Encoder 1: CCW");
      tone(TONE_PIN, 392, 50); // G4
    }
    lastPos1 = newPos1;
  }
  
  // Handle encoder 2
  encoder2.tick();
  int newPos2 = encoder2.getPosition();
  if (newPos2 != lastPos2) {
    if (newPos2 > lastPos2) {
      Serial.println("Encoder 2: CW");
      tone(TONE_PIN, 659, 50); // E5
    } else {
      Serial.println("Encoder 2: CCW");
      tone(TONE_PIN, 330, 50); // E4
    }
    lastPos2 = newPos2;
  }
  
  // delay(20);
}


/******************************************/

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return trellis.pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return trellis.pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return trellis.pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}
