#pragma once
#include <Arduino.h>

// ============================================================
//  Configuracion central del monitor de calidad del aire.
//  Todo lo "ajustable" vive aqui: pines, tiempos y red.
// ============================================================

namespace Pins {
// --- Pines soldados en la PCB: NO MODIFICAR ---
constexpr uint8_t MHZ19_RX   = 16;  // ESP32 recibe por aqui (TX del sensor)
constexpr uint8_t MHZ19_TX   = 17;  // ESP32 transmite por aqui (RX del sensor)
constexpr uint8_t DHT        = 7;
constexpr uint8_t SOUND      = 18;  // Microfono analogico (ADC2, ver README)
constexpr uint8_t ENC_CLK    = 5;
constexpr uint8_t ENC_DT     = 4;
constexpr uint8_t ENC_SW     = 6;
constexpr uint8_t I2C_SDA    = 8;
constexpr uint8_t I2C_SCL    = 9;
constexpr uint8_t SD_CS      = 10;  // SPI por defecto del ESP32-S3
}  // namespace Pins

namespace Timing {
constexpr uint32_t DHT_MS          = 2500;   // DHT22 necesita >= 2 s entre lecturas
constexpr uint32_t AHT_MS          = 2000;
constexpr uint32_t ENS160_MS       = 2000;
constexpr uint32_t MHZ19_MS        = 5000;   // el MH-Z19C actualiza cada ~5 s
constexpr uint32_t SOUND_WINDOW_MS = 50;     // ventana pico a pico (20 Hz)
constexpr uint32_t SAMPLE_MS       = 2000;   // toma de una muestra completa
constexpr uint32_t LOG_MS          = 60000;  // escritura en SD (promedio del intervalo)
constexpr uint32_t RECENT_MS       = 10000;  // punto del grafico "en vivo" en RAM
constexpr uint32_t DISPLAY_MS      = 200;    // refresco OLED (5 fps)
constexpr uint32_t SERIAL_MS       = 5000;
}  // namespace Timing

namespace Net {
constexpr const char* AP_SSID     = "MonitorAire";
constexpr const char* AP_PASSWORD = "monitor1234";  // minimo 8 caracteres
constexpr uint8_t     AP_CHANNEL  = 6;
}  // namespace Net

namespace Storage {
constexpr const char* ROOMS_DIR    = "/rooms";
constexpr const char* DEFAULT_ROOM = "general";
constexpr size_t      RECENT_POINTS = 360;  // 360 x 10 s = 1 hora en RAM
}  // namespace Storage
