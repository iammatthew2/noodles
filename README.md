# Noodles

A general-purpose MQTT controller built on the Arduino Nano 33 IoT. Noodles uses a NeoTrellis 4x4 keypad, two rotary encoders, and a button pair to select and control other devices over MQTT.

## Hardware

- **Board:** Arduino Nano 33 IoT
- **NeoTrellis 4x4** — illuminated keypad for app selection and control (I2C)
- **Rotary Encoder 1:** D3 / D4
- **Rotary Encoder 2:** D5 / D6
- **Button Pair:** D9 / D8
- **Kill Switch:** D2
- **Piezo Buzzer:** D7

## How It Works

Noodles has three modes managed by `StateManager`:

- **HOME** — idle display on the NeoTrellis, navigable with buttons
- **SELECTING** — browse available app targets on the keypad
- **CONTROL** — encoder turns and key presses publish MQTT messages to the selected app's control topic

Configured app targets (defined in `main.cpp`):

| App     | MQTT Topic              | Color         |
|---------|-------------------------|---------------|
| Yodel   | `apps/yodel/control`    | Red           |
| Skippy  | `apps/skippy/control`   | Green         |
| Jibbers | `apps/jibbers/control`  | Blue          |
| Pickles | `apps/pickles/control`  | Yellow        |
| Puddles | `apps/puddles/control`  | Purple        |

# Building

Built with [PlatformIO](https://platformio.org/).

## Libraries

- Adafruit NeoPixel
- Adafruit seesaw (NeoTrellis driver)
- Adafruit BusIO
- RotaryEncoder (mathertel)
- WiFiNINA
- PubSubClient (MQTT)

## Project Libraries (in `lib/`)

- **InputHandler** — routes encoder, button, and trellis events to the right handler
- **NeoTrellis/** — wraps the Adafruit NeoTrellis for pixel control and key callbacks
- **RotaryEncoder/** — `EncoderChannel` abstraction with direction callbacks
- **SimpleButtonPairController/** — debounced two-button input
- **StateManager/** — manages HOME / SELECTING / CONTROL state transitions and display
- **WiFiMQTTManager/** — handles Wi-Fi connection and MQTT publish/subscribe