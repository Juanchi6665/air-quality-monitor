#pragma once
#include <Arduino.h>

#include "core/Measurement.h"

// Interfaz comun para todos los sensores. Para sumar un sensor nuevo:
// heredar de Sensor (o PeriodicSensor), implementar sus metodos y
// registrarlo en SensorHub desde main.cpp.
class Sensor {
 public:
  virtual ~Sensor() = default;
  virtual const char* name() const = 0;
  virtual bool begin() = 0;
  virtual void update(uint32_t nowMs) = 0;     // llamado en cada loop, no bloqueante
  virtual void fill(Measurement& m) const = 0;  // copia su ultimo valor
  bool ok() const { return ok_; }

 protected:
  bool ok_ = false;
};

// Sensor que se lee cada cierto intervalo.
class PeriodicSensor : public Sensor {
 public:
  explicit PeriodicSensor(uint32_t intervalMs) : intervalMs_(intervalMs) {}

  void update(uint32_t nowMs) override {
    if (!started_ || nowMs - lastMs_ >= intervalMs_) {
      started_ = true;
      lastMs_ = nowMs;
      ok_ = read();
    }
  }

 protected:
  virtual bool read() = 0;  // devuelve true si la lectura fue valida

 private:
  uint32_t intervalMs_;
  uint32_t lastMs_ = 0;
  bool started_ = false;
};
