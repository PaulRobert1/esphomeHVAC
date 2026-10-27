# How the controllers work

## Floor heating

Five native ESPHome thermostats consume Home Assistant room-temperature sensors. Each thermostat operates its corresponding active-low zone-valve output. The active-valve count is sampled every second; a ten-second task enables the floor pump when at least one valve is on and disables it otherwise. Consequently the pump itself may lag a valve change by approximately ten seconds. Temperature inputs depend on HA; this is not a fully standalone room-temperature system.

Thermostat settings restore from memory; the valve switches use `RESTORE_DEFAULT_OFF` (restore previous state when available, otherwise off). User control changes schedule a preferences sync after 250 ms. Firmware defaults and restored settings are different: flashing the source does not necessarily overwrite stored setpoints.

## Direct circulation link

The floor controller sends its pump state as numeric 0/1 every five seconds on encrypted UDP port **18512**. Both nodes must use the same private `circulation_transport_key`, reachable broadcast subnet and matching provider name `househeating`. The sender enables rolling codes; the receiver uses ping/pong transport support.

The receiving `CirculationRequest` lease expires after **20 seconds** without valid refresh, rejects values other than finite 0/1 and is not restored across reboots. Accepted requests run the **hot-side pump only**. They never start the compressor or cold pump. HVAC flow proof, heating and overrun retain control of their required pumps even if a floor request disappears. A physical fault blocks the external circulation request.

## Heat-pump sequence

The HVAC thermostat compares the main tank probe with its target and sets demand. Its configured deadband is 7°C and overrun is 0.7°C; it does not necessarily restart at one degree below target. Presets are in the YAML. Its mode restores from memory, so a restored OFF remains OFF unless HA or a user changes it.

The controller executes independently of HA:

1. Boot with compressor and both pump outputs off. Begin the five-minute compressor restart lockout.
2. Validate temperatures and flow telemetry before progressing.
3. On demand after the lockout, run both pumps and prove flow. Require two fresh flow samples from **each** loop, plus fresh loop temperatures for baseline measurements. Proof must complete within 45 seconds.
4. Energize the compressor only after proof. Confirm heat transfer against the pre-start baseline within the configurable window (default 60 seconds).
5. Stop on loss of demand or fault. Normal pump overrun is 30 seconds; the next compressor start remains subject to a five-minute delay.

Default controls include minimum flow 1 L/min, response delta 0.5°C, cold-outlet stop/recovery −20/−17°C, tank safety limit 75°C and hot-outlet limit 80°C. Compressor temperature trips at 99°C and recovers below 90°C. These values reflect the existing installation, including its cold-loop fluid; they are not suitable defaults for every hydraulic/refrigerant system.

Loop and compressor temperature telemetry expires after 25 seconds, main tank after 70 seconds, and flow after 7 seconds. Flow loss while running must persist three seconds to trip. Subsequent loss of confirmed transfer for 120 seconds trips. Flow/no-response failures consume a three-attempt budget before lockout. A valid sensor recovery requires 30 seconds of qualification. Reset requires thermostat demand OFF, compressor OFF and cleared physical faults. See `heatpump_controller.h` for the complete ordering and conditions.

The web interface and HA expose faults, phase, restart seconds, failure count and pump-source diagnostics. Internal GPIO switches are intentionally not exposed as ordinary compressor controls.

## Solar thermal charging

The solar controller runs every second using the main tank probe and collector thermocouple. The default solar target is 55°C, independently adjustable from the heat-pump target.

- Require a collector-to-tank difference of at least 8°C for 20 seconds before starting.
- Start at 30% PWM. Adjust once per ten seconds toward a 6°C differential, at most +2 or −3 percentage points per adjustment, within 30–100%.
- Stop when the differential falls to 2°C, the target is reached, collector temperature reaches 120°C, or required temperature input becomes invalid/stale.
- After reaching target, tank temperature must drop 2°C before that hold clears. A stopped pump has a 60-second restart pause.

This is thermal-collector control, not photovoltaic inverter control. PWM interpretation depends on the actual pump/interface.

## Home Assistant energy automation

Battery reserve, room night setback and automatic heating recovery live in the separate HA configuration, not these ESPHome files. The firmware enforces its own equipment protections, but does not infer that heating should be enabled from battery SoC alone. If the tank is cold while the thermostat is OFF, inspect HA heating enable/allow flags and initialization, then the controller's phase/fault/restart diagnostics. A circulation request is not evidence that compressor demand is enabled.

## Optional floor-controller extensions

BLE/BMS telemetry, BLE scanning and the JK component dependency have been removed from this publication. Wi-Fi runs without power saving.

The Atlantis CSI receiver is an experimental RF sensing extension, unrelated to the heating control path. Public transmitter MAC/BSSID and collector IP values are placeholders. Its radio-owner audit flag records a deployment assumption, not proof that a new board has been audited. Review radio coexistence before enabling the `RF CSI Sensing` switch. The extension starts inactive and stops for OTA. To omit it, remove the `atlantis` package, the local `atlantis_csi` external component entry and its four substitutions.

## Troubleshooting order

1. Confirm temperatures correspond to the correct physical probes and are updating.
2. Check thermostat mode/target and HA policy status.
3. Read heat-pump phase, fault and restart countdown; do not bypass the relay owner to clear a delay.
4. For missing floor circulation, check both nodes' subnet/key/provider name and `Floor Circulation Link`, `Requested`, `Accepted` and `Hot Pump Source`.
5. For failed flow proof, inspect pump operation and calibrated flow inputs. A request alone does not prove actual water movement.
6. For solar non-start, inspect collector/tank differential, qualification timer, target hold and restart pause.
