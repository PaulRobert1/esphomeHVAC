# ESPHome floor heating and solar / heat-pump controller

Current Home Assistant ESPHome configuration snapshot: **27 October 2026**. Two ESP32 controllers coordinate five floor-heating zones, a DIY water-to-water heat pump and a solar-thermal pump charging a 300 L storage tank.

| Configuration | Hardware / purpose |
|---|---|
| [floorheating.yaml](floorheating.yaml) | `househeating`, NodeMCU-32S: five zone valves, floor pump, HA room-temperature inputs, RF CSI extension (no BLE telemetry) |
| [HVACSolarHeater.yaml](HVACSolarHeater.yaml) | `hvac-solarheater`, ESP32 DevKit: compressor sequencing, two heat-pump pumps, temperatures, flow meters, solar pump PWM and fault protection |

**Web interface login for both configurations: `admin` / `admin`.** Change the `web_server.auth` fields for your installation. This repository update does not flash installed controllers or change their current credentials.

## Start here

1. Copy this repository, including the `.h`, `.js`, package and `components/` files, into your ESPHome configuration directory.
2. Copy `secrets.example.yaml` to `secrets.yaml`. Replace the Wi-Fi, OTA and API values, network addresses and circulation transport key. Documentation-network addresses and zero API keys are examples, not working credentials. Generate independent native API keys with `openssl rand -base64 32`.
3. Review [wiring](docs/wiring.md). The GPIO map is extracted from the current source; it is not a verified schematic of every terminal or power connection. Review GPIO9 on the floor board before building a replacement.
4. Adapt the floor controller's HA temperature entity IDs, each Dallas probe address and the installation-specific sensor calibration offsets.
5. Read [operation and recovery](docs/operation.md), including the distinction between a floor circulation request and a compressor heating request.
6. Run `esphome config floorheating.yaml` and `esphome config HVACSolarHeater.yaml`, then `esphome compile` for each. Use a current compatible ESPHome environment; the optional CSI extension requires Arduino >=3.3.11 (IDF >=5.5.5).
7. Commission with actuator power isolated first; check polarity, sensor placement and actual flow before enabling the compressor. Upload is a separate deliberate operation.

The web JavaScript is embedded into each firmware, so its UI does not require an external JavaScript CDN. The native API is encrypted; OTA has a separate password. Neither uses the web login.

## Included dependencies

- `heatpump_controller.h`: heat-pump state machine and protections.
- `solar_controller.h`: differential solar charging with hysteresis and speed ramping.
- `circulation_request.h`: expiring, RAM-only circulation demand.
- `*_web_bundle.js`: embedded web interfaces; bundled ESPHome webserver MIT notices are retained.
- `atlantis-receiver.yaml`, `components/atlantis_csi/`: RF CSI extension present in the current floor source. It is unrelated to heating demand. See [optional extensions](docs/operation.md#optional-floor-controller-extensions) before enabling it on another installation.

Older standalone heat-pump examples and the extensionless configuration are superseded. They remain in Git history, rather than appearing beside the current supported pair.

See [validation and provenance](docs/validation.md) for what was checked and the limits of this source snapshot. Physical installations need correctly rated relays/contactors, protection and independent thermal/pressure cut-outs; GPIOs are control signals, not load power outputs.
