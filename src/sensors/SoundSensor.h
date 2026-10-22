#pragma once
#include "Config.h"
#include "sensors/Sensor.h"

// Microfono analogico. En vez de bloquear 50 ms con un while (como antes),
// toma una muestra por cada pasada del loop y cierra la ventana a los 50 ms.
class SoundSensor : public Sensor {
 public:
  explicit SoundSensor(uint8_t pin) : pin_(pin) {}
  const char* name() const override { return "Microfono"; }

  bool begin() override {
    pinMode(pin_, INPUT);
    analogReadResolution(12);
    return true;
  }

  void update(uint32_t nowMs) override {
    uint16_t sample = analogRead(pin_);
    if (sample < 4095) {  // descarta saturaciones
      if (sample > max_) max_ = sample;
      if (sample < min_) min_ = sample;  // antes era "else if": se perdian minimos
    }

    if (nowMs - windowStart_ >= Timing::SOUND_WINDOW_MS) {
      if (max_ >= min_) {
        float peakToPeak = max_ - min_;
        float db = DB_MIN + (peakToPeak - P2P_MIN) * (DB_MAX - DB_MIN) / (P2P_MAX - P2P_MIN);
        db_ = constrain(db, DB_MIN, DB_MAX);
        ok_ = true;
      }
      windowStart_ = nowMs;
      max_ = 0;
      min_ = 4095;
    }
  }

  void fill(Measurement& m) const override { m.soundDb = db_; }

 private:
  // Calibracion original: 20..4095 cuentas -> 49.5..90 dB
  static constexpr float P2P_MIN = 20, P2P_MAX = 4095, DB_MIN = 49.5f, DB_MAX = 90.0f;

  uint8_t pin_;
  uint32_t windowStart_ = 0;
  uint16_t max_ = 0, min_ = 4095;
  float db_ = NAN;
};
