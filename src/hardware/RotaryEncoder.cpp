#include "hardware/RotaryEncoder.h"

void IRAM_ATTR RotaryEncoder::isr(void* arg) {
  auto* e = static_cast<RotaryEncoder*>(arg);
  uint32_t now = micros();
  if (now - e->lastIsrUs_ < 1500) return;  // antirrebote
  e->lastIsrUs_ = now;
  e->steps_ += digitalRead(e->dt_) ? -1 : 1;  // misma convencion que el codigo original
}
