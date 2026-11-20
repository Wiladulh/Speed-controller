# iTag BLE Integration

## Implemented

- Continuous BLE scanning.
- Scan results include address, name, RSSI, and manufacturer data encoded as hexadecimal.
- WebUI can register the observed address as the remote identity.
- Registered remote RSSI is smoothed.
- Presence threshold and absence timeout are configurable.
- Confirmed transition from near to absent sends OFF once to all configured fans.
- Returning remote restores Presence state only; it never restores the previous fan speed automatically.

## Click sequence engine and remaining adapter

The generic click timing engine is implemented. It accepts press/release edges, groups clicks within a 500 ms inter-click window, ignores presses longer than 800 ms, and maps counts 1–5 to speed 1–4/OFF. The model-specific BLE packet decoder is not yet connected to this edge interface.

The required user mapping is:
- 1 click = Speed 1
- 2 clicks = Speed 2
- 3 clicks = Speed 3
- 4 clicks = Speed 4
- 5 clicks = OFF
- Long press ignored

Cheap iTag products use differing BLE advertising/GATT formats. The exact model and actual advertising payload sequence are needed to implement a reliable decoder. The WebUI scan endpoint exposes manufacturer data in hex to support this capture. Do not treat an arbitrary advertising packet count as a button click count.

Some remotes use resolvable/random BLE addresses. If the address rotates, address-only registration is insufficient and a stable identifier or bonding-based identity must be implemented for that model.

## Presence distance

BLE RSSI is a rough proximity indicator, not a precise distance measurement. Calibrate the RSSI threshold around the intended 2 m boundary in the actual room. Body blocking, orientation, battery level, walls, and RF interference affect results.
