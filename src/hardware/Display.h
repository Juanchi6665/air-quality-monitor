#pragma once
#include <U8g2lib.h>

#include "core/Measurement.h"

// Informacion de estado que la pantalla muestra ademas de las mediciones.
struct SystemStatus {
  String room;
  String ip;
  bool sdOk = false;
  uint8_t clients = 0;
};

class Display {
 public:
  Display(uint8_t scl, uint8_t sda) : oled_(U8G2_R0, U8X8_PIN_NONE, scl, sda) {}

  void begin() {
    oled_.begin();
    oled_.clearBuffer();
    oled_.setFont(u8g2_font_ncenB08_tr);
    oled_.drawStr(10, 12, "NRF - Data logger");
    oled_.drawStr(10, 40, "Iniciando...");
    oled_.sendBuffer();
  }

  void nextPage(int steps) { page_ = ((page_ + steps) % PAGE_COUNT + PAGE_COUNT) % PAGE_COUNT; }
  void home() { page_ = 0; }

  void render(const Measurement& m, const SystemStatus& st) {
    oled_.clearBuffer();
    oled_.setFont(u8g2_font_6x12_tf);
    switch (page_) {
      case 0:
        oled_.drawStr(0, 12, "RESUMEN");
        line(30, "Temp:", m.temperature(), "%.1f C");
        line(45, "Hum: ", m.humidity(), "%.1f %%");
        line(60, "CO2: ", m.co2, "%.0f ppm");
        break;
      case 1:
        oled_.drawStr(0, 12, "AIRE (ENS160)");
        line(30, "TVOC:", m.tvoc, "%.0f ppb");
        line(45, "eCO2:", m.eco2, "%.0f ppm");
        line(60, "AQI: ", m.aqi, "%.0f / 5");
        break;
      case 2: big("TEMP C", m.tempDht, "%.1f"); break;
      case 3: big("TEMP F", isnan(m.tempDht) ? NAN : m.tempDht * 1.8f + 32, "%.1f"); break;
      case 4: big("HUMEDAD", m.humDht, "%.1f%%"); break;
      case 5: big("RUIDO dB", m.soundDb, "%.0f"); break;
      case 6:
        oled_.drawStr(0, 12, "RED / ESTADO");
        oled_.setCursor(0, 26); oled_.printf("IP: %s", st.ip.c_str());
        oled_.setCursor(0, 39); oled_.printf("Hab: %s", st.room.c_str());
        oled_.setCursor(0, 52); oled_.printf("SD: %s", st.sdOk ? "OK" : "ERROR");
        oled_.setCursor(0, 64); oled_.printf("Clientes: %u", st.clients);
        break;
    }
    oled_.sendBuffer();
  }

 private:
  static constexpr int PAGE_COUNT = 7;

  void line(int y, const char* label, float v, const char* fmt) {
    oled_.setCursor(0, y);
    oled_.print(label);
    oled_.print(' ');
    if (isnan(v)) oled_.print("--");
    else oled_.printf(fmt, v);
  }

  void big(const char* title, float v, const char* fmt) {
    oled_.drawStr(0, 12, title);
    oled_.setFont(u8g2_font_logisoso32_tf);
    oled_.setCursor(0, 64);
    if (isnan(v)) oled_.print("--");
    else oled_.printf(fmt, v);
  }

  U8G2_SH1106_128X64_NONAME_F_HW_I2C oled_;
  int page_ = 0;
};
