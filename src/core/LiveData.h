#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <vector>

#include "Config.h"
#include "core/Measurement.h"

// Ultima muestra + historial corto en RAM, compartidos entre el loop()
// y el servidor web (que corre en otra tarea), por eso usa un mutex.
class LiveData {
 public:
  LiveData() : mutex_(xSemaphoreCreateMutex()) { recent_.resize(Storage::RECENT_POINTS); }

  void setLatest(const Measurement& m) {
    Lock lock(mutex_);
    latest_ = m;
  }

  Measurement latest() const {
    Lock lock(mutex_);
    return latest_;
  }

  void pushRecent(const Measurement& m) {
    Lock lock(mutex_);
    recent_[head_] = m;
    head_ = (head_ + 1) % recent_.size();
    if (count_ < recent_.size()) count_++;
  }

  // Copia ordenada de la mas antigua a la mas nueva.
  std::vector<Measurement> recent() const {
    Lock lock(mutex_);
    std::vector<Measurement> out;
    out.reserve(count_);
    size_t start = (head_ + recent_.size() - count_) % recent_.size();
    for (size_t i = 0; i < count_; ++i) out.push_back(recent_[(start + i) % recent_.size()]);
    return out;
  }

 private:
  struct Lock {
    explicit Lock(SemaphoreHandle_t m) : m_(m) { xSemaphoreTake(m_, portMAX_DELAY); }
    ~Lock() { xSemaphoreGive(m_); }
    SemaphoreHandle_t m_;
  };

  SemaphoreHandle_t mutex_;
  Measurement latest_;
  std::vector<Measurement> recent_;
  size_t head_ = 0;
  size_t count_ = 0;
};
