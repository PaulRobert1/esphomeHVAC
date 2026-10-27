#pragma once
#include <cmath>
#include <cstdint>
namespace hp {
// RAM-only lease. Only authenticated, decoded transport sensor updates call receive().
struct CirculationRequest {
  static constexpr uint32_t timeout_ms = 20000;
  uint32_t received_at = 0;
  bool seen = false;
  bool demand = false;
  void receive(float value, uint32_t now) {
    seen = std::isfinite(value) && (value == 0.0f || value == 1.0f);
    demand = seen && value == 1.0f;
    received_at = now;
  }
  bool fresh(uint32_t now) {
    if (seen && uint32_t(now - received_at) >= timeout_ms) {
      seen = false;
      demand = false;
    }
    return seen;
  }
  bool requested(uint32_t now) { return fresh(now) && demand; }
};
}
