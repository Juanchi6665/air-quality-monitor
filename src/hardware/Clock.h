#pragma once
#include <RTClib.h>

#include <atomic>

// Envoltura del RTC DS1307. Si el RTC falla, sigue contando con millis()
// a partir de la ultima hora conocida (por ejemplo, la que envia el telefono).
class Clock {
 public:
  // Seguro desde otra tarea (servidor web): el ajuste real ocurre en service(),
  // dentro del loop, para no usar el bus I2C desde dos tareas a la vez.
  void requestSet(uint32_t epoch) { pending_ = epoch; }

  void service() {
    uint32_t epoch = pending_.exchange(0);
    if (epoch) set(epoch);
  }

  bool begin() {
    rtcOk_ = rtc_.begin();
    if (rtcOk_ && !rtc_.isrunning()) rtc_.adjust(DateTime(F(__DATE__), F(__TIME__)));
    if (!rtcOk_) set(DateTime(F(__DATE__), F(__TIME__)).unixtime());
    return rtcOk_;
  }

  DateTime now() const {
    if (rtcOk_) return rtc_.now();
    return DateTime(baseEpoch_ + (millis() - baseMs_) / 1000);
  }

  void set(uint32_t epoch) {
    baseEpoch_ = epoch;
    baseMs_ = millis();
    if (rtcOk_) rtc_.adjust(DateTime(epoch));
  }

  bool rtcOk() const { return rtcOk_; }

 private:
  mutable RTC_DS1307 rtc_;
  bool rtcOk_ = false;
  std::atomic<uint32_t> pending_{0};
  uint32_t baseEpoch_ = 0;
  uint32_t baseMs_ = 0;
};
