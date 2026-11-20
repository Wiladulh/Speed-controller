# ESP32 Fan Speed Controller — Specification

## 1. Scope

A modular ESP32 firmware for independently controlled fans. Board profiles, fan count, relay topology, and GPIO allocation must follow verified hardware rather than assumptions in software.

## 2. Confirmed behavior

### Fan speed model

- Logical speed values: 0 = OFF, 1 = Speed 1, 2 = Speed 2, 3 = Speed 3, 4 = Speed 4.
- A fan exposes four logical speed outputs. The physical relay count and electrical topology depend on the actual fan circuit.
- Configured fan profiles may expose 2, 3, or 4 usable speeds. Commands for unavailable speeds must be rejected safely.
- Logical speed index must not be assumed to equal a GPIO number.

### Control sources

- WebUI control is always enabled and has no OFF toggle.
- MQTT Control has an independent WebUI toggle.
- BLE Presence has an independent WebUI toggle.
- Physical buttons remain locally available.
- All sources feed a central command arbitration layer.
- WebUI and enabled MQTT commands may turn a fan ON even when the registered iTag is absent.
- MQTT disabled means MQTT commands are ignored; it does not affect WebUI, BLE, or physical buttons.
- MQTT disconnection does not change the current fan state and does not disable local controls.

### BLE iTag

- One click: Speed 1.
- Two clicks: Speed 2.
- Three clicks: Speed 3.
- Four clicks: Speed 4.
- Five clicks: OFF.
- Long press is not used.
- Registered iTag identity and actual event/presence behavior must be verified with the user's physical iTag.
- Presence is an optional automation. When enabled, a confirmed absence may command the associated fan OFF.
- When the iTag returns, the fan must not automatically restore its previous speed. A new explicit command is required.
- Approx. 2 m is a calibrated proximity target, not a precise BLE distance measurement. RSSI smoothing, hysteresis, and an absence timeout are required before declaring absence.

### Startup and transitions

- All speed outputs OFF at startup/restart.
- Do not restore a previously running speed automatically after restart.
- Break-before-make: turn all speed outputs OFF, wait the configured dead time, then activate only the target output.
- Use a non-blocking state machine based on millis(), never delay() for fan transitions.
- Central software interlock must prevent more than one speed output being active at a time.
- Hardware interlock is recommended; software alone cannot guarantee physical relay mutual exclusion.
- Software output state is not proof of actual relay contact state without feedback hardware.

## 3. WebUI and network

- Wi-Fi mode: AP + STA.
- AP setup mode is available for first-time configuration or recovery.
- WebUI is authenticated.
- BLE Presence and MQTT Control are independent persistent toggles.
- WebUI control itself is always enabled.
- MQTT transport is selectable in WebUI: WS, WSS, TCP (MQTT), or TLS (MQTTS).
- Broker host, port, WebSocket path, credentials, and trusted CA PEM are configurable.
- WSS/TLS must verify the broker certificate; do not silently disable certificate validation.
- MQTT command topics must not be retained. State/status may be retained.
- On MQTT reconnect, publish actual device state; do not replay stale retained commands.
- MQTT set payload is JSON: {"speed":0..4}. Retained set commands are ignored.
- MQTT Last Will reports online/offline.
- BLE Presence continues to operate when MQTT is disconnected.

## 4. Initial software target

- PlatformIO.
- Arduino framework for ESP32.
- esp32dev is only a generic initial compile target, not a final board selection.
- WebUI has GPIO dropdowns for each speed output and each of the five buttons.
- Initial GPIO options are a conservative generic classic ESP32 DevKit profile only; other board families require their own verified pin profile.
- Up to eight logical fan profiles are supported in configuration; real capacity is limited by the selected board's available IO and any verified expander hardware.
- Incomplete GPIO profiles may be saved, but the corresponding fan remains locked OFF.

## 5. Unresolved pre-flight items

1. Exact ESP32 board/module and first reference board.
2. Fan wiring diagram and whether the speed inputs are mutually exclusive by electrical design.
3. Relay/contactor/driver type, contact ratings, active polarity, and safe output levels.
4. GPIO map and any IO expander decision.
5. Maximum fan count based on actual GPIO and power budget.
6. Required break-before-make dead time.
7. Hardware interlock, isolation, overcurrent and surge protection.
8. Output feedback/fault detection.
9. iTag exact model/protocol and advertising payload sequences for click decoding; stable identity, idle advertisement behavior, RSSI calibration, and timeout.
10. MQTT broker details, topic namespace, authentication, and payload schema.
11. WebUI authentication and AP credential provisioning policy.
12. Button electrical wiring, debounce, and per-fan mapping.

## 6. Safety note

Do not connect mains wiring or energize relays based only on this firmware. Validate the complete schematic, isolation, protective devices, relay/contactor ratings for the actual motor load, and hardware interlock before testing with a fan. Initial firmware must remain unable to drive outputs until a complete hardware profile is configured and reviewed.
