# CYD Voice

ESPHome firmware that turns a **Freenove ESP32-S3 CYD** (Cheap Yellow Display, model FNK0104) into a fully functional **Home Assistant voice assistant** with touchscreen UI.

## Features

- **Voice Assistant** — Local wake word detection ("Okay Nabu") with full Home Assistant Assist pipeline (STT + intent + TTS)
- **Touch Display** — 3-page LVGL interface: clock, smart home controls, voice assistant
- **Configurable Buttons** — 4 customizable buttons with 74 MDI icons, configurable entity and action from Home Assistant
- **Timers & Alarms** — Voice-activated timers with live countdown on display and audible alarm (dismiss by touch or wake word)
- **Auto Display Management** — Configurable auto-return to clock and display sleep timeout
- **Tap-to-Talk** — Tap the voice circle to start/stop the assistant without wake word
- **Continued Conversation** — Assistant stays listening when the response contains a question
- **All Settings from HA** — Volume, timeouts, button config — everything adjustable from Home Assistant without reflashing

## Hardware Required

| Component | Model | Notes |
|---|---|---|
| Board | [Freenove ESP32-S3 Display (FNK0104)](https://github.com/Freenove/Freenove_ESP32_S3_Display) | 2.8" IPS capacitive touch, 240x320 |
| Speaker | Included in kit | PH1.25mm connector + FM8002E amplifier |
| Microphone | Integrated MEMS | LMA2718B381 via ES8311 codec |

No additional hardware required. The board includes everything needed.

## Prerequisites

- **Home Assistant** with [ESPHome integration](https://esphome.io/guides/getting_started_hassio.html)
- **Assist pipeline** configured in HA (STT + TTS providers, e.g., OpenAI, Whisper, Piper)
- **ESPHome** (container or add-on) for compilation

## Installation

### 1. Clone the repository

```bash
git clone https://github.com/ragio579/cyd-voice.git
cd cyd-voice
```

### 2. Create your secrets file

```bash
cp secrets.yaml.example secrets.yaml
```

Edit `secrets.yaml` with your values:

```yaml
wifi_ssid: "YourWiFiSSID"
wifi_password: "YourWiFiPassword"
wifi_ssid_2: "OptionalBackupSSID"       # optional second network
wifi_password_2: "OptionalBackupPassword"
api_encryption_key: "YOUR_KEY_HERE"      # generate with: openssl rand -base64 32
ota_password: "your-ota-password"
```

### 3. First flash (USB)

Connect the board via USB-C and flash using [web.esphome.io](https://web.esphome.io):

1. Open https://web.esphome.io in Chrome/Edge
2. Click **Connect** and select the serial port
3. Click **Install** and select the compiled `.factory.bin`

To compile the firmware:

```bash
esphome compile cyd-voice.yaml
```

The binary will be at `.esphome/build/cyd-voice/.pioenvs/cyd-voice/firmware.factory.bin`.

### 4. Add to Home Assistant

After flashing, the device connects to your WiFi. In Home Assistant:

1. Go to **Settings > Devices & Services > ESPHome**
2. The device should be auto-discovered. Click **Configure**
3. Enter the API encryption key from your `secrets.yaml`
4. The device appears with voice assistant, controls, and configuration entities

### 5. Subsequent updates (OTA)

After the first flash, updates can be done over WiFi:

```bash
esphome upload cyd-voice.yaml --device <DEVICE_IP>
```

## Display Pages

### Page 1 — Clock
Large clock display with date (Italian locale) and active timer countdown.

### Page 2 — Controls
4 configurable buttons in a 2x2 grid. Each button can be assigned:
- **Icon** — Select from 74 built-in MDI icons (see [icon list](documentation/icon-list.md))
- **Entity** — Any Home Assistant entity ID
- **Action** — Choose from common HA actions (toggle, turn on/off, etc.)

### Page 3 — Voice Assistant
Central circle showing assistant state:
- **Blue** — Ready, waiting for wake word
- **Red** — Listening
- **Green** — Responding

Tap the circle to start/stop the assistant manually.

## Configuration

All settings are available as Home Assistant entities under the device's configuration section:

| Entity | Type | Default | Description |
|---|---|---|---|
| Volume | Number (0-10) | 7 | Speaker volume. 0 = mute, 10 = max |
| Clock Return Timeout | Number (seconds) | 30 | Auto-return to clock page after inactivity. 0 = disabled |
| Display Timeout | Number (seconds) | 60 | Auto display off after inactivity. 0 = disabled |
| Button 1-4 Icon | Select | Various | Icon for each control button |
| Button 1-4 Entity | Text | Empty | HA entity ID for each button |
| Button 1-4 Action | Select | light.toggle | HA action for each button |

## Voice Commands

After saying "Okay Nabu" (or tapping the circle), you can:

- Control your smart home ("Turn on the bedroom light")
- Ask questions ("What's the weather?")
- Set timers ("Set a timer for 5 minutes")
- And anything else your Assist pipeline supports

### Timers

Active timers appear on the clock page with a live countdown. When a timer finishes:
- The display wakes up and shows the clock page
- An audible alarm beeps every 2 seconds
- Touch anywhere on the screen or say "Okay Nabu" to dismiss

## Technical Details

### Why Custom Components?

The Freenove FNK0104 uses an **ES8311** audio codec that requires **I2S duplex mode** (TX and RX channels created simultaneously). Standard ESPHome `i2s_audio` speaker and microphone components create channels separately, which doesn't work with this codec.

This firmware includes two custom components that solve this:

- **`duplex_mic`** — Microphone component that reads from a shared I2S RX channel
- **`duplex_speaker`** — Speaker component that writes to a shared I2S TX channel

Both channels are initialized together at boot using the raw ESP-IDF I2S API, and the ES8311 codec is configured manually via I2C registers.

### Audio Architecture

```
Boot → I2S duplex (TX+RX) → ES8311 init via I2C
  → duplex_mic feeds micro_wake_word ("Okay Nabu")
  → wake word detected → voice_assistant streams to HA
  → HA pipeline: STT → Intent → TTS
  → audio response → duplex_speaker → ES8311 DAC → FM8002E amp → speaker
```

### Pin Map

| Function | GPIO | Notes |
|---|---|---|
| I2S MCLK | 4 | Master clock |
| I2S BCK | 5 | Bit clock |
| I2S DIN | 6 | Microphone data |
| I2S WS | 7 | Word select |
| I2S DOUT | 8 | Speaker data |
| Amp Enable | 1 | FM8002E, **active LOW** |
| I2C SDA | 16 | Shared: touch + codec |
| I2C SCL | 15 | Shared: touch + codec |
| Display CS | 10 | ILI9341 SPI |
| Display DC | 46 | |
| Display BL | 45 | Backlight (LEDC) |
| Touch INT | 17 | FT6336U |
| Touch RST | 18 | |
| RGB LED | 42 | WS2812B |

### Display Settings

The ILI9341 on this board requires specific settings:
- `invert_colors: true`
- `color_order: BGR`
- `swap_xy: true` (landscape mode)
- Touch: `swap_xy: true, mirror_y: true`

## Customization

### Language

The UI labels and clock date are in Italian. To change them, edit these sections in `cyd-voice.yaml`:

**Voice assistant labels** (search for these strings):
- `"DI' \"OKAY, NABU\""` — Ready state text
- `"ASCOLTO..."` — Listening state
- `"RISPOSTA..."` — Responding state

**Clock date** (in the interval lambda):
- `giorni[]` array — Day names
- `mesi[]` array — Month names

### Adding Icons

The firmware includes 74 MDI icons. See [documentation/icon-list.md](documentation/icon-list.md) for the full list. To add more icons, add their Unicode codepoint to the `font_icons` glyphs list in the YAML and include the icon name in the `select` options and `get_glyph` map.

## License

MIT

## Credits

- [ESPHome](https://esphome.io/) — Firmware framework
- [Freenove](https://github.com/Freenove/Freenove_ESP32_S3_Display) — Hardware and documentation
- [Home Assistant](https://www.home-assistant.io/) — Smart home platform
- [Material Design Icons](https://pictogrammers.com/library/mdi/) — Button icons
