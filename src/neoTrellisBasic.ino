/* This example shows basic usage of the NeoTrellis.
  The buttons will light up various colors when pressed.
  The interrupt pin is not used in this example.
*/

#include "Adafruit_NeoTrellis.h"
#include <RotaryEncoder.h>

Adafruit_NeoTrellis trellis;

// Rotary encoder pins (adjust to available pins on Nano 33 IoT)
RotaryEncoder encoder1(2, 3, RotaryEncoder::LatchMode::TWO03);
RotaryEncoder encoder2(4, 5, RotaryEncoder::LatchMode::TWO03);

int lastPos1 = 0, lastPos2 = 0;

const int TONE_PIN = 7;

//define a callback for key presses
TrellisCallback blink(keyEvent evt){
  // Check is the pad pressed?
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    Serial.print("Pressed: ");
    Serial.println(evt.bit.NUM);
    tone(TONE_PIN, 440 + (evt.bit.NUM * 50), 100); // Play tone based on button number
    trellis.pixels.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, trellis.pixels.numPixels(), 0, 255))); //on rising
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
  // or is the pad released?
    Serial.print("Released: ");
    Serial.println(evt.bit.NUM);
    trellis.pixels.setPixelColor(evt.bit.NUM, 0); //off falling
  }

  // Turn on/off the neopixels!
  trellis.pixels.show();

  return 0;
}

void setup() {
  Serial.begin(9600);
  pinMode(TONE_PIN, OUTPUT);
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
}

void loop() {
  trellis.read();  // interrupt management does all the work! :)
  
  // Handle encoder 1
  encoder1.tick();
  int newPos1 = encoder1.getPosition();
  if (newPos1 != lastPos1) {
    if (newPos1 > lastPos1) {
      Serial.println("Encoder 1: CW");
    } else {
      Serial.println("Encoder 1: CCW");
    }
    lastPos1 = newPos1;
  }
  
  // Handle encoder 2
  encoder2.tick();
  int newPos2 = encoder2.getPosition();
  if (newPos2 != lastPos2) {
    if (newPos2 > lastPos2) {
      Serial.println("Encoder 2: CW");
    } else {
      Serial.println("Encoder 2: CCW");
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
