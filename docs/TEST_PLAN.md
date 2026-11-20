# Test Plan

## Build and static validation
- PlatformIO build for the default ESP32-C3 Super Mini environment and alternate classic ESP32 / ESP32-S3 profiles.
- Native click sequence unit tests via pio test -e native.
- Verify library dependency versions and reproducible build.
- Review warnings and compile errors before claiming PASS.

## Configuration/WebUI
- First boot creates unique AP and WebUI credentials and prints them to Serial.
- Basic authentication protects all API routes.
- AP + STA operation and Wi-Fi reconnect behavior.
- GPIO output dropdown contains only GPIO3–6 and input dropdown only GPIO0, GPIO1, GPIO2, GPIO7, GPIO10 on the default C3 profile.
- GPIO2 external bias preserves HIGH boot level.
- Invalid fanCount values are rejected before configuration array access.
- Duplicate/reserved GPIO assignments rejected.
- Incomplete hardware profile persists but fan output remains locked.
- Secrets are not returned by the configuration GET endpoint.

## Fan state machine
- Boot leaves all outputs inactive.
- OFF request immediately de-energizes every speed output.
- Speed change turns all outputs OFF before dead time, then exactly one target output ON.
- Rapid target changes replace the pending target safely.
- Unavailable speed rejected.
- Simulated millis wraparound.

## Inputs and arbitration
- Five physical buttons debounce and map to OFF/Speed 1–4.
- WebUI and MQTT command can turn fan ON when iTag is absent.
- WebUI status endpoint reports MQTT connectivity and each fan's configured/state/current/target speed.
- WebUI command response is queued and the displayed fan state changes to the requested speed after the break-before-make interval.
- Presence OFF cannot override WebUI/MQTT in the same command batch.
- A manual command while iTag is absent suppresses the pending one-shot Presence OFF until the iTag is detected near again.
- MQTT disabled ignores MQTT commands.

## MQTT
- WS, WSS, TCP, TLS connect independently.
- TLS certificate verification rejects invalid CA.
- Authentication, reconnect, Last Will, retained state.
- Retained set commands ignored.
- Only configured fan command topics are subscribed.
- Changing fanCount or a fan ID in WebUI causes MQTT subscriptions to be rebuilt using the new topic set.
- WebUI and MQTT commands are accepted only for configured fans and available speed values.
- MQTT disconnect does not alter fan state or local controls.

## BLE
- Scan/register the exact iTag.
- Capture advertising payload for each click count 1–5.
- Connect model-specific packet decoder to click edge interface.
- Validate random-address behavior.
- Calibrate RSSI threshold around 2 m and test timeout/hysteresis.
- Verify Presence OFF and no automatic restart on return.

## Hardware safety
- Verify GPIO boot states with relay driver disconnected from mains.
- Verify physical interlock and fail-safe behavior.
- Validate relay/contactor ratings and complete mains schematic with qualified review before load testing.
