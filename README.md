# ESP32 Fan Speed Controller

Firmware project for a modular ESP32-based multi-fan speed controller.

## Project status

Initial repository setup. Hardware-specific GPIO assignments, relay polarity, relay driver, fan wiring topology, and the maximum number of fans are intentionally not assumed. The firmware must keep outputs inactive until a verified hardware profile is supplied.

## Confirmed control design

- Four logical speed outputs per fan; actual electrical topology depends on the verified fan circuit.
- Five physical buttons per fan: OFF and Speed 1–4.
- BLE iTag: 1 click = Speed 1, 2 = Speed 2, 3 = Speed 3, 4 = Speed 4, 5 = OFF. Long press is not used.
- BLE Presence and MQTT Control are independent WebUI toggles.
- WebUI control is always enabled.
- MQTT and WebUI commands may turn a fan ON even when the iTag is absent.
- BLE Presence may turn a fan OFF after the registered iTag is judged absent. Returning iTag does not automatically restore the previous speed.
- MQTT disconnection does not stop local controls or change the current fan state.
- On boot/restart, all fan outputs must be OFF.
- Speed transitions use break-before-make and a non-blocking state machine.

## Safety boundary

This project controls hardware that may switch hazardous fan/mains circuits. Do not connect mains wiring based only on firmware assumptions. Confirm the fan circuit, relay/contactor ratings, isolation, protection, and hardware interlock with the actual schematic and qualified review before energizing the system.

## Planned firmware environment

PlatformIO with the Arduino framework for ESP32 is the initial development target. The reference board and hardware profile remain configurable and are not yet frozen.

See [docs/SPECIFICATION.md](docs/SPECIFICATION.md) for confirmed decisions and unresolved pre-flight items.
