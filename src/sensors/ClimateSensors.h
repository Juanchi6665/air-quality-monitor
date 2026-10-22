#pragma once
#include <Adafruit_AHTX0.h>
#include <DHT.h>

#include "Config.h"
#include "sensors/Sensor.h"

class DhtSensor : public PeriodicSensor {
 public:
  explicit DhtSensor(uint8_t pin) : PeriodicSensor(Timing::DHT_MS), dht_(pin, DHT22) {}
  const char* name() const override { return "DHT22"; }

  bool begin() override {
    dht_.begin();
    return true;  // el DHT no confirma presencia; se valida en cada lectura
  }

  void fill(Measurement& m) const override {
    m.tempDht = temp_;
    m.humDht = hum_;
    m.heatIndex = heatIndex_;
  }

 protected:
  bool read() override {
    float h = dht_.readHumidity();
    float t = dht_.readTemperature();
    if (isnan(h) || isnan(t)) return false;  // conserva el ultimo valor valido
    hum_ = h;
    temp_ = t;
    heatIndex_ = dht_.computeHeatIndex(t, h, false);
    return true;
  }

 private:
  DHT dht_;
  float temp_ = NAN, hum_ = NAN, heatIndex_ = NAN;
};

class AhtSensor : public PeriodicSensor {
 public:
  AhtSensor() : PeriodicSensor(Timing::AHT_MS) {}
  const char* name() const override { return "AHT2x"; }

  bool begin() override {
    present_ = aht_.begin();
    return present_;
  }

  void fill(Measurement& m) const override {
    m.tempAht = temp_;
    m.humAht = hum_;
  }

 protected:
  bool read() override {
    if (!present_) return false;
    sensors_event_t hum, temp;
    if (!aht_.getEvent(&hum, &temp)) return false;
    temp_ = temp.temperature;
    hum_ = hum.relative_humidity;
    return true;
  }

 private:
  Adafruit_AHTX0 aht_;
  bool present_ = false;
  float temp_ = NAN, hum_ = NAN;
};
