# Hardware Configuration and Safety

## GPIO selection

The WebUI offers GPIO dropdowns for Speed 1–4 and the five buttons (OFF, Speed 1–4). PlatformIO includes initial compile-time profiles for generic classic ESP32 DevKit, ESP32-S3 DevKitC-1, and ESP32-C3 DevKitM-1. Each build environment supplies its own conservative GPIO allowlist to the WebUI. These are starting profiles only; verify the exact module, board revision, onboard peripherals, flash/PSRAM configuration, and schematic. ESP32-S2/C6 and other boards are not included yet.

Pins must be unique across speed and button functions. Unassigned pins are allowed during initial setup, but the associated fan remains locked until every configured speed output is assigned and the profile passes validation.

## Relay polarity

Each fan profile has Active LOW / Active HIGH selection. This only describes the logic level at the ESP32 driver input; it does not define mains-side wiring or relay contact behavior.

## Required hardware review

Before connecting a real fan:
- Verify the fan's exact wiring and speed-selection topology.
- Verify motor voltage/current, inrush, inductive switching rating, and contact derating for the selected relay/contactor.
- Provide galvanic isolation and suitable overcurrent/surge protection.
- Provide hardware mutual exclusion where required by the fan circuit.
- Confirm safe boot/reset pin levels and relay-driver default state.
- Consider external IO expanders only after selecting a board and calculating the power/IO budget.
- Test low-voltage logic and relay driver without mains first.

The firmware's software interlock cannot guarantee relay contact mutual exclusion if a relay welds or driver hardware fails.
