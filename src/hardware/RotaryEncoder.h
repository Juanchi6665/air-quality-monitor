#pragma once
#include <Arduino.h>

// Encoder rotativo por interrupcion: no pierde pasos aunque el loop este ocupado.
class RotaryEncoder {
 public:
  RotaryEncoder(uint8_t clk, uint8_t dt, uint8_t sw) : clk_(clk), dt_(dt), sw_(sw) {}

  void begin() {
    pinMode(clk_, INPUT_PULLUP);
    pinMode(dt_, INPUT_PULLUP);
    pinMode(sw_, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(clk_), isr, this, RISING);
  }

  // Pasos acumulados desde la ultima llamada (+ horario, - antihorario).
  int takeSteps() {
    noInterrupts();
    int s = steps_;
    steps_ = 0;
    interrupts();
    return s;
  }

  // true una vez por pulsacion (antirrebote sin delay).
  bool wasPressed() {
    bool down = digitalRead(sw_) == LOW;
    uint32_t now = millis();
    bool pressed = false;
    if (down && !lastDown_ && now - lastPressMs_ > 200) {
      pressed = true;
      lastPressMs_ = now;
    }
    lastDown_ = down;
    return pressed;
  }

 private:
  static void isr(void* arg);  // en RotaryEncoder.cpp (debe estar en IRAM)

  uint8_t clk_, dt_, sw_;
  volatile int steps_ = 0;
  volatile uint32_t lastIsrUs_ = 0;
  bool lastDown_ = false;
  uint32_t lastPressMs_ = 0;
};
