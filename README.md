# Motion Sensor

This example creates an Occupancy Sensor (PIR motion sensor) device using the
ESP Matter data model. It's a sibling project to `light/`, following the same
structure and conventions.

See the [docs](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html)
for more information about building and flashing the firmware.

## Hardware

- **Board:** ESP32-C6 DevKit
- **Sensor:** generic PIR module with a digital output (e.g. HC-SR501),
  active-high on motion detected
- **Wiring:** connect the PIR module's digital output to `GPIO2` (default).
  To use a different pin, run `idf.py menuconfig` → `Example Configuration`
  → `PIR sensor GPIO number` (`PIR_GPIO_NUM`) before building. Avoid the
  ESP32-C6 strapping pins (4, 5, 8, 9, 15).
- **Factory reset:** reuses the onboard BOOT button, same long-press pattern
  as the `light` example.

## Behavior

- Exposes a single Matter `occupancy_sensor` endpoint. The `OccupancySensing`
  cluster's `Occupancy` attribute mirrors the PIR's digital output level
  directly — no software debounce and no firmware-side hold timer. How long
  "occupied" is reported for is controlled entirely by the PIR module itself
  (most modules, e.g. HC-SR501, have an onboard potentiometer for this).
- Detection is interrupt-driven (GPIO ISR → FreeRTOS task), not polled, so
  reporting is close to instantaneous.
- The device reports its real initial state right after commissioning starts,
  so a controller (e.g. Alexa) doesn't see a stale "no occupancy" value if
  the sensor is already triggered at boot.
- HC-SR501-class PIR modules emit spurious triggers during their warm-up
  period (~30-60 s after power-on) — this is normal sensor behavior, not a
  firmware bug.

## 1. Additional Environment Setup

No additional setup is required.

## 2. Post Commissioning Setup

No additional setup is required.
