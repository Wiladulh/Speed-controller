# Architecture

## Runtime flow

All input sources enqueue a `FanCommand` into the FreeRTOS command queue. The main loop arbitrates commands per fan and passes the selected target to that fan's `FanController`.

Command priority:
1. Physical button
2. WebUI and MQTT (same priority; most recently dequeued wins)
3. BLE button event
4. BLE Presence OFF automation

This ensures a Presence timeout cannot override a WebUI or MQTT command received in the same processing batch. A Presence OFF event is emitted once per absence transition, not continuously. A manual command while the remote is absent suppresses the pending one-shot absence action until the remote is detected near again.

## Modules

- `ConfigManager`: versioned JSON configuration in Preferences; validation and persistence.
- `BoardProfile`: initial conservative GPIO allowlist for classic ESP32 DevKit.
- `FanController`: per-fan break-before-make relay state machine.
- `ButtonManager`: five debounced physical inputs per fan.
- `BleRemoteManager`: BLE scanning, remote registration identity, RSSI smoothing, Presence timeout, and configurable fan mapping.
- `ItagClickSequence`: generic 1–5 click timing engine; a model-specific BLE packet adapter must feed press/release edges.
- `MqttManager`: MQTT 3.1.1 over WS/WSS/TCP/TLS using PsychicMqttClient.
- `WifiManager`: simultaneous setup AP and STA connection/retry.
- `WebServerManager`: authenticated local WebUI and JSON API.

## Hardware profiles

The current generic profile exposes a curated set of output-capable classic ESP32 GPIOs. GPIO options are selected in WebUI for each speed and each physical button. Pin uniqueness is validated globally across configured fans. The firmware does not assume speed number equals GPIO number.

The logical configuration supports up to eight fan profiles. The actual number that can be activated depends on available pins, board variant, relay drivers, and any future IO expander support. An unconfigured or incomplete fan profile remains locked OFF.

## Concurrency

MQTT and BLE callbacks execute outside the Arduino main loop. They only enqueue commands or update scan observations; fan GPIO switching is done in the main loop. Hardware transitions never wait with `delay()`.

## Current implementation boundary

The exact iTag advertising/button packet format is not yet identified. BLE scanning and Presence are implemented, but translating the registered remote's proprietary advertising packets into 1–5 click events requires a captured packet sequence from the user's exact iTag. The firmware must not guess this packet format.
