# ESPHome — Sveglia Smart Camera da Letto

## Finalita

Flashare una **Freenove ESP32-S3 CYD** (2.8" IPS capacitive touch, 240x320) per creare una **sveglia smart con voice satellite Assist** per Home Assistant, con:

- **Display LVGL** touch landscape 320x240, 3 pagine: orologio/sveglia, controlli stanza, voice assist
- **Voice Assistant** integrato con la pipeline Assist di HA (STT OpenAI + TTS OpenAI)
- **Wake word locale** (micro Wake Word su ESP32-S3) o su server (openWakeWord)
- **Microfono MEMS integrato** + **speaker incluso** (audio codec ES8311)

## Hardware (verificato 2026-03-25)

| Componente | Modello | Stato |
|---|---|---|
| Board | Freenove ESP32-S3 Display FNK0104 (Touch) 2.8" IPS 240x320 | Funzionante |
| Microfono | MEMS integrato (LMA2718B381) via ES8311 I2S | Funzionante (raw ESP-IDF) |
| Speaker | Incluso nel kit (connettore PH1.25mm) + amplificatore FM8002E | Funzionante (raw ESP-IDF) |
| Audio Codec | ES8311 (I2C + I2S) | Funzionante (init manuale, I2S duplex obbligatorio) |

### Specifiche confermate
- [x] **PSRAM**: 8MB OPI confermato
- [x] **USB**: USB-C (dati + alimentazione)
- [x] **Display**: ILI9341 via SPI — `invert_colors: true`, `color_order: BGR`
- [x] **Touch**: FT6336U capacitivo via I2C (componente ESPHome: `ft63x6`)
- [x] **RGB LED**: WS2812B su GPIO42 (componente ESPHome: `esp32_rmt_led_strip`)
- [x] **Microfono MEMS integrato** — NON serve INMP441 esterno
- [x] **Speaker incluso** nel kit — NON serve MAX98357A esterno

### Pinout completo verificato

| Funzione | Pin | Note |
|---|---|---|
| **Display SPI** | | |
| TFT_CS | GPIO10 | |
| TFT_MOSI | GPIO11 | |
| TFT_SCK | GPIO12 | |
| TFT_MISO | GPIO13 | |
| TFT_BL | GPIO45 | Backlight (strapping pin) |
| TFT_DC | GPIO46 | Data/Command (strapping pin) |
| **I2C** (touch + codec) | | |
| SDA | GPIO16 | Shared FT6336 + ES8311 |
| SCL | GPIO15 | Shared FT6336 + ES8311 |
| **Touch FT6336** | | |
| RST | GPIO18 | |
| INT | GPIO17 | |
| **I2S Audio** | | |
| MCK | GPIO4 | Master clock |
| BCK | GPIO5 | Bit clock |
| WS | GPIO7 | Word select |
| DINT | GPIO6 | Data in (microfono) |
| DOUT | GPIO8 | Data out (speaker) |
| AP_ENABLE | GPIO1 | Amplifier enable |
| **RGB LED** | | |
| DIN | GPIO42 | WS2812B |
| **SD Card** (SDMMC) | | |
| CLK | GPIO38 | |
| CMD | GPIO40 | |
| D0-D3 | GPIO39/41/48/47 | |
| **USB** | | |
| D- | GPIO19 | |
| D+ | GPIO20 | |
| **Serial** | | |
| TXD0 | GPIO43 | |
| RXD0 | GPIO44 | |

### Documentazione
Scaricata in `documentation/Freenove_ESP32_S3_Display-main/`:
- `Tutorial_With_Touch/Tutorial.pdf` — tutorial completo
- `Datasheet/` — schematico, datasheet componenti (ES8311, FT6336, ILI9341, FM8002E, MEMS mic)
- `Datasheet/2.8inch_ESP32-S3_Display_Schematic.pdf` — schema elettrico

## Riferimento Home Assistant

### Installazione
- **Percorso config HA**: `/home/ragio579/homeassistant_test`
- **Container**: `homeassistant` (docker compose in `/home/ragio579/homeassistant_test/docker-compose.yml`)
- **Rete**: host mode, HTTP su `http://192.168.1.10:8123`
- **HTTPS**: `https://ragio-server.tail280efd.ts.net:8444` via Caddy + Tailscale
- **Istruzioni complete**: vedi `/home/ragio579/homeassistant_test/CLAUDE.md`

### Pipeline Assist gia configurata
- **STT**: `openai_stt` (custom component, modello `gpt-4o-mini-transcribe`)
- **TTS**: `openai_tts` (custom component)
- **Lingua**: italiano
- **Intent attivi**: `TestIntent`, `CercaOggetto`, `ControllaTelecamere`, `PulisciStanza`
- **Intent da implementare**: `CercaWeb`, `MostraTelecamera` (richiedono `device_id` del satellite)
- **Custom sentences**: `/home/ragio579/homeassistant_test/custom_sentences/it/intents.yaml`
- **Intent script**: `/home/ragio579/homeassistant_test/intent_script.yaml`

### Voice PE gia esistenti
4 dispositivi Nabu Casa Home Assistant Voice PE collegati:
- `home-assistant-voice-0a3c1b` (MAC: 20:F8:3B:0A:3C:1B)
- `home-assistant-voice-0a898f` (MAC: 20:F8:3B:0A:89:8F)
- `home-assistant-voice-0acdff` (MAC: 20:F8:3B:0A:CD:FF)
- `home-assistant-voice-0a65da` (MAC: 20:F8:3B:0A:65:DA)

Questi usano firmware Nabu Casa pre-compilato. La Freenove richiede firmware ESPHome custom.

### Integrazioni rilevanti
- **ESPHome**: integrazione gia attiva in HA (gestisce i 4 Voice PE)
- **MQTT**: broker Mosquitto su host (usato da Frigate, camqtt, n8n)
- **browser_mod**: per intent `MostraTelecamera` — il satellite puo inviare `device_id` ad HA che poi comanda il tablet della stessa stanza

## Architettura del firmware ESPHome

### Componenti previsti
```yaml
esp32:
  board: esp32-s3-devkitc-1
  framework:
    type: esp-idf          # necessario per S3 + PSRAM + voice

psram:
  mode: octal              # 8MB PSRAM per LVGL + audio buffer

wifi:                       # connessione a rete locale
api:                        # comunicazione con HA
ota:                        # aggiornamenti over-the-air
logger:

# Audio
i2s_audio:                  # bus I2S per mic + speaker
microphone:                 # MEMS integrato via ES8311
speaker:                    # Speaker incluso via ES8311 + FM8002E
micro_wake_word:            # wake word locale (hey_jarvis)
voice_assistant:            # pipeline Assist di HA

# Display
spi:                        # bus SPI per display
display:                    # ILI9341 240x320
touchscreen:                # FT6336U capacitive touch
lvgl:                       # UI con pagine e widget

# Opzionale
light:                      # LED status (se presente sulla board)
```

### Pagine display (LVGL landscape 320x240, tileview swipe orizzontale)
1. **Orologio** — card centrata: HH:MM bianco grande, data italiano, prossima sveglia azzurra
2. **Controlli** — 4 pulsanti 2x2 con icone MDI:
   - ceiling-light → `light.zbminil2_camera_da_letto`
   - floor-lamp → `light.lampada_angolare_govee`
   - toilet → `light.zbminil2_bagno_camera`
   - weather-night → decorativo (nessuna azione)
   - Icone: grigie se spente, gialle `#FFC107` se accese
3. **Voice Assist** — cerchio stato (idle=blu, listening=rosso, thinking=viola, speaking=verde) + label comando/risposta

### Impostazioni display critiche
- `invert_colors: true` + `color_order: BGR` (necessari per colori corretti su questo pannello)
- Display transform: `swap_xy: true` (no mirror)
- Touch transform: `swap_xy: true, mirror_y: true`
- Tileview `bg_color: 0x000000` con `bg_opa: COVER` (sfondo nero)
- Timezone POSIX: `CET-1CEST,M3.5.0,M10.5.0/3` (Europe/Rome non funziona con ESP-IDF)
- Per azioni HA: usare `homeassistant.action` (non `.service`) e abilitare azioni nel device ESPHome in HA

### Scoperte critiche audio (2026-03-28)

1. **I2S DUPLEX obbligatorio**: l'ES8311 NON funziona con canali I2S TX-only o RX-only di ESPHome. Serve `i2s_new_channel(&cfg, &tx, &rx)` con ENTRAMBI i canali creati insieme. I componenti ESPHome standard `speaker:` e `microphone:` NON funzionano.
2. **Amplificatore FM8002E ACTIVE LOW**: GPIO1 LOW = amplificatore acceso, HIGH = spento. Pin ESPHome: `inverted: true`.
3. **ES8311 init richiede delay**: il componente ESPHome `audio_dac: es8311` fallisce al boot perche' manca un delay 20ms tra reset (0x1F) e clear reset (0x00). Init manuale via I2C necessaria.
4. **Ordine init**: I2S duplex (create + enable) PRIMA → ES8311 register init via I2C DOPO (il codec ha bisogno di MCLK attivo per configurarsi).
5. **Custom component `duplex_mic`**: microphone ESPHome custom in `custom_components/duplex_mic/` che legge dal canale RX duplex. Necessario per `micro_wake_word`.
6. **Audio nel FreeRTOS task**: operazioni audio lunghe (echo, playback) devono girare in task separati per non bloccare il main loop di ESPHome (watchdog timeout ~5s).
7. **LVGL non thread-safe**: non chiamare `lv_*` da FreeRTOS task. Usare globals + interval nel main loop.

### Flusso voce
```
Boot → I2S duplex (TX+RX) → ES8311 init via I2C → duplex_mic.start()
    → micro_wake_word (locale, modello "okay_nabu")
    → wake word rilevato → registra con VAD (silenzio 1s = stop)
    → [futuro] voice_assistant.start (stream a HA)
    → HA pipeline: STT (OpenAI) → Intent/Conversation → TTS (OpenAI)
    → audio risposta → i2s_channel_write(tx) → ES8311 DAC → FM8002E amp → speaker
```

## Comandi utili

```bash
# Avviare ESPHome Dashboard
docker compose -f /home/ragio579/esphome/docker-compose.yml up -d

# Dashboard web
# http://192.168.1.10:6052

# Primo flash via USB (dalla macchina con USB collegato)
# Alternativa: https://web.esphome.io da browser Chrome/Edge

# Compilare e flashare OTA (usare upload, NON run)
docker exec esphome esphome upload satellite-test.yaml --device 192.168.1.186

# Log di un dispositivo
docker exec esphome esphome logs satellite-test.yaml --device 192.168.1.186

# Compilare senza flashare
docker exec esphome esphome compile satellite-test.yaml

# Validare configurazione
docker exec esphome esphome config satellite-test.yaml
```

## Stato attuale (2026-03-28)

### Firmware
- **`voice-test.yaml`** (v0.4.10) — Voice assistant HA completo: wake word + STT + TTS + volume + tap-to-talk
- **`audio-test.yaml`** (v0.3.1) — Test audio standalone: tono, echo, wake word + VAD echo
- **`satellite-test.yaml`** — LVGL 3 pagine (orologio, controlli luci, voice placeholder) — da unificare con voice-test

### Completati
- [x] Board ricevuta e testata (board test via web flasher Freenove)
- [x] Container ESPHome avviato (`docker compose up -d`)
- [x] Display LVGL landscape 320x240 con colori HA dark mode corretti
- [x] Touch funzionante con coordinate corrette
- [x] Pagina orologio + controlli luci + voice (in satellite-test.yaml)
- [x] **Audio speaker funzionante** (raw ESP-IDF I2S duplex + init manuale ES8311)
- [x] **Audio microfono funzionante** (I2S RX duplex, MEMS via ES8311 ADC)
- [x] **Amplificatore FM8002E** funzionante (GPIO1 active LOW, inverted)
- [x] **Wake word "okay nabu"** funzionante (micro_wake_word + custom duplex_mic component)
- [x] **Echo test con VAD** funzionante (registra finche' c'e' voce, riproduce)
- [x] **Livello mic RMS** mostrato in tempo reale sul display
- [x] WiFi dual (casa + hotspot GalaxyRagio) per test fuori casa
- [x] Share Samba `\\ragio-server\esphome` per accesso firmware da Windows

### Completati (2026-03-28)
- [x] Custom speaker component (`duplex_speaker`) wrappa I2S TX duplex
- [x] Voice assistant HA funzionante (wake word + STT OpenAI + TTS OpenAI)
- [x] Tap-to-talk (pulsante display avvia Assist)
- [x] Continued conversation (HA riattiva ascolto su domande)
- [x] Volume controllabile da HA (number entity → ES8311 registro 0x32)
- [x] UI stati: blu (pronto) → rosso (ascolto) → verde (risposta) → blu
- [x] Auto-restart wake word dopo ogni ciclo

### Prossimi passi
1. [ ] Unificare voice-test.yaml e satellite-test.yaml (LVGL 3 pagine + voice)
2. [ ] Rinominare firmware a nome stanza definitivo
3. [ ] Configurare pipeline Assist dedicata in HA
4. [ ] Aggiungere `device_id` del nuovo satellite al mapping degli intent
5. [ ] Implementare intent `MostraTelecamera` e `CercaWeb` con il `device_id`
6. [ ] Valutare media_player per TTS (richiede rete locale, non funziona via hotspot)
