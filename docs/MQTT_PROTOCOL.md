# MQTT Protocol

## Supported transports

The WebUI transport selector maps to URI schemes:

| WebUI | URI scheme | Security |
|---|---|---|
| WS | `ws://` | Unencrypted WebSocket |
| WSS | `wss://` | WebSocket over TLS |
| TCP / MQTT | `mqtt://` | Unencrypted TCP |
| TLS / MQTTS | `mqtts://` | MQTT over TLS |

For WS/WSS, configure the WebSocket path (commonly `/mqtt`). Host, port, path, username, password, and CA certificate are configurable. TLS/WSS will not connect unless a CA PEM is provided; certificate verification is not disabled.

Implementation uses PsychicMqttClient, which wraps ESP-MQTT and supports all four transports.

## Topics

For device ID `esp32-fan-controller`, base topic `devices`, and fan ID `fan-001`:

- Command: `devices/esp32-fan-controller/fan-001/set`
- State: `devices/esp32-fan-controller/fan-001/state`
- Device status: `devices/esp32-fan-controller/status`

## Payloads

Command (retained commands are ignored):
```json
{"speed": 3}
```

OFF:
```json
{"speed": 0}
```

State (retained):
```json
{"fan_id":"fan-001","power":true,"speed":3}
```

Device status is retained plain text: `online` or Last Will `offline`.

MQTT Control has an independent WebUI toggle. Disabled MQTT messages are ignored. Losing the broker connection does not change the fan state and does not disable local WebUI, buttons, or BLE.
