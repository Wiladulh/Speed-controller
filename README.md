# ESP32 Fan Speed Controller

Modular PlatformIO firmware for a configurable multi-fan speed controller.

## Firmware status

The repository contains the initial firmware foundation, local WebUI/configuration, GPIO selection, physical button input, BLE scanning/Presence, and MQTT transport integration. Hardware-specific validation and the exact iTag click protocol remain dependent on the selected ESP32 board, relay/fan circuit, and captured iTag advertising packets.

## Confirmed control design

- Logical speed 0 = OFF and 1–4 = Speed 1–4.
- Each fan may be configured for 2, 3, or 4 available speeds.
- Five physical buttons per fan: OFF and Speed 1–4.
- iTag mapping target: 1 click = Speed 1, 2 = Speed 2, 3 = Speed 3, 4 = Speed 4, 5 = OFF. Long press is not used.
- BLE Presence and MQTT Control are independent WebUI toggles.
- WebUI control is always enabled.
- WebUI and enabled MQTT commands can turn a fan ON even when iTag is absent.
- Presence OFF is a one-shot automation when a previously detected registered iTag becomes absent for the configured timeout. It never automatically restores the previous speed when the remote returns.
- MQTT disconnection does not stop local controls or change the current fan state.
- On boot/restart, fan outputs remain OFF.
- Speed transitions use break-before-make and a non-blocking state machine.

## MQTT transports

WebUI supports four selectable MQTT schemes:
- WS (ws://)
- WSS (wss://)
- TCP / MQTT (mqtt://)
- TLS / MQTTS (mqtts://)

Host, port, WebSocket path, credentials, and CA PEM are configurable. TLS/WSS requires a trusted CA PEM; certificate verification is not disabled. MQTT set commands are non-retained and retained commands are ignored.

See docs/MQTT_PROTOCOL.md.

## GPIO configuration

WebUI provides separate GPIO dropdowns for Speed 1–4 and the five buttons. The initial dropdown list is a conservative generic classic ESP32 DevKit profile, not a universal list for every ESP32 family. GPIO assignments must be unique. Incomplete hardware profiles can be saved during setup, but the affected fan remains locked OFF until its output profile is complete.

The software supports up to eight logical fan profiles. Actual usable fan count depends on board GPIO availability, drivers, buttons, and any future IO expander hardware.

## Build

Install PlatformIO and build one of the supported initial board profiles:

    pio run -e esp32dev
    pio run -e esp32-s3-devkitc-1
    pio run -e esp32-c3-devkitm-1

The default environment is the generic classic ESP32 DevKit profile. Actual board/module pinouts must still be checked before assigning GPIOs. On first boot, the setup AP and WebUI credentials are generated uniquely and printed to the Serial monitor at 115200 baud.

## Important remaining hardware/model-specific work

- Confirm the actual board and its GPIO capabilities.
- Confirm the fan wiring, relay/contactor ratings, active polarity, hardware interlock, isolation, and protective devices.
- Capture BLE advertising packets for each iTag click action and connect the model-specific decoder to the implemented 1–5 click sequence engine. Presence scanning and generic click timing are implemented; the proprietary packet decoder is not claimed complete until the packet format is identified.
- Validate BLE address stability; some iTags use rotating private addresses.
- Run the test plan before hardware use.

## Security

The local WebUI uses HTTP Basic Authentication over plain HTTP. Keep it on a trusted LAN or the password-protected setup AP; do not expose port 80 to the public internet. Device configuration secrets are stored in ESP32 Preferences/NVS; enable flash/NVS encryption in a production security design.

## Safety

Do not connect mains wiring based only on firmware assumptions. Confirm the complete electrical schematic and motor-load switching ratings before energizing a fan. Software interlock cannot protect against welded relay contacts or hardware faults.
