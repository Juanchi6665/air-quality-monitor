#pragma once
#include <vector>

#include "sensors/Sensor.h"

// Agrupa los sensores: los inicia, los actualiza y arma una Measurement.
class SensorHub {
 public:
  void add(Sensor& s) { sensors_.push_back(&s); }

  void begin() {
    for (Sensor* s : sensors_) {
      bool ok = s->begin();
      Serial.printf("[Sensor] %-10s %s\n", s->name(), ok ? "OK" : "NO DETECTADO");
    }
  }

  void update(uint32_t nowMs) {
    for (Sensor* s : sensors_) s->update(nowMs);
  }

  Measurement snapshot(uint32_t epoch) const {
    Measurement m;
    m.epoch = epoch;
    for (Sensor* s : sensors_) s->fill(m);
    return m;
  }

  const std::vector<Sensor*>& sensors() const { return sensors_; }

 private:
  std::vector<Sensor*> sensors_;
};
