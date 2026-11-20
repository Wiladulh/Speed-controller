# Hardware Configuration and Safety

## Current target: ESP32-C3 Super Mini

The default PlatformIO environment is `esp32-c3-super-mini`, using the ESP32-C3 DevKitM-1 board definition for the ESP32-C3 MCU. The firmware profile reserves:
- Relay-driver outputs: GPIO3, GPIO4, GPIO5, GPIO6.
- Physical button inputs: GPIO0, GPIO1, GPIO2, GPIO7, GPIO10.

This gives enough direct GPIO for one fan with four speed outputs and five buttons. GPIO2 is a strapping pin: it is permitted only for an Active LOW button input with an external pull-up that preserves HIGH at reset. Do not hold that button while resetting or powering on the board. Confirm the exact Super Mini clone's pin labels and schematic before wiring. GPIO8/9 are deliberately not offered because they are commonly used for onboard LED/BOOT functions or boot strapping.

The C3 Super Mini profile does not provide enough direct GPIO for a second complete fan. Additional fans require a suitable I/O expander or a board with more exposed GPIO. Do not bypass the allowlist by selecting arbitrary pins.

## GPIO selection

The WebUI provides separate dropdown lists for relay outputs and button inputs. Pins must be unique across all speed and button functions. Unassigned pins are allowed during initial setup, but the associated fan remains locked until every configured speed output is assigned and the profile passes validation.

Other PlatformIO profiles for classic ESP32 and ESP32-S3 remain available for future board targets, but they are not the current default.

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
