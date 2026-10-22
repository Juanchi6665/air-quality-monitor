#pragma once
#include <HardwareSerial.h>
#include <MHZ19.h>
#include <ScioSense_ENS160.h>

#include "Config.h"
#include "sensors/Sensor.h"

class Ens160Sensor : public PeriodicSensor {
 public:
  Ens160Sensor() : PeriodicSensor(Timing::ENS160_MS) {}
  const char* name() const override { return "ENS160"; }

  bool begin() override {
    present_ = ens_.begin() && ens_.setMode(ENS160_OPMODE_STD);
    return present_;
  }

  // Compensacion de temperatura/humedad: mejora la precision del ENS160.
  void setEnvironment(float tempC, float hum) {
    if (present_ && !isnan(tempC) && !isnan(hum)) ens_.set_envdata(tempC, hum);
  }

  void fill(Measurement& m) const override {
    m.tvoc = tvoc_;
    m.eco2 = eco2_;
    m.aqi = aqi_;
  }

 protected:
  bool read() override {
    if (!present_ || !ens_.measure(false)) return false;  // false = no bloquear
    tvoc_ = ens_.getTVOC();
    eco2_ = ens_.geteCO2();
    aqi_ = ens_.getAQI();
    return true;
  }

 private:
  ScioSense_ENS160 ens_;
  bool present_ = false;
  float tvoc_ = NAN, eco2_ = NAN, aqi_ = NAN;
};

// Usa un UART por hardware del ESP32 (mas fiable que SoftwareSerial)
// sobre los mismos pines 16/17.
class Mhz19Sensor : public PeriodicSensor {
 public:
  Mhz19Sensor(HardwareSerial& serial, uint8_t rxPin, uint8_t txPin)
      : PeriodicSensor(Timing::MHZ19_MS), serial_(serial), rx_(rxPin), tx_(txPin) {}
  const char* name() const override { return "MH-Z19C"; }

  bool begin() override {
    serial_.begin(9600, SERIAL_8N1, rx_, tx_);
    mhz_.begin(serial_);
    mhz_.autoCalibration();
    return true;
  }

  void fill(Measurement& m) const override {
    m.co2 = co2_;
    m.tempMhz = temp_;
  }

 protected:
  bool read() override {
    int co2 = mhz_.getCO2();
    if (mhz_.errorCode != RESULT_OK) return false;
    co2_ = co2;
    temp_ = mhz_.getTemperature();
    return true;
  }

 private:
  HardwareSerial& serial_;
  uint8_t rx_, tx_;
  MHZ19 mhz_;
  float co2_ = NAN, temp_ = NAN;
};
