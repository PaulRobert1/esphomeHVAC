# Source provenance and validation

The two configurations and their local dependencies were read from the Home Assistant ESPHome source directory on 2026-10-27. This establishes the current stored source, not a binary-level proof that every device is running a build from those exact bytes.

Publication changes: network credentials replaced with `!secret` references; public network/MAC examples substituted; web login set explicitly to `admin` / `admin`; legacy duplicate configurations removed from the current tree. At the owner’s request, JK BMS BLE telemetry and its scanning/client dependency are removed from the floor configuration. Device names, GPIO assignments, probe addresses, calibration and heating control algorithms are retained. No controller is flashed by this repository update.

`secrets.yaml` is ignored and never committed. The all-zero API keys in `secrets.example.yaml` are deliberately rejected by ESPHome: generate real keys before validation. Validation uses separate temporary random keys.

Checks:

- ESPHome 2026.9.1 configuration validation for both entry points.
- Full firmware compilation (Arduino framework / ESP-IDF 5.5.5), using temporary local test credentials.
- Native C++ regression smoke tests: circulation expiry, malformed demand rejection, solar qualification/overtemperature stop and compressor fail-closed on missing sensors.
- JavaScript syntax validation for both embedded web bundles.
- Standard-library publication tests verify both default web logins and absence of BLE components.
- Publication credential scan against private source credentials and a Git diff review.

Run the native checks with `g++ -std=c++17 -Wall -Wextra -pedantic tests/controllers.cpp -o /tmp/hvac-controllers-test && /tmp/hvac-controllers-test`.

These checks do not verify physical wiring, electrical protection, hydraulic flow, refrigerant charge or calibration. No live relay or compressor actuation is part of publication validation.
