#pragma once
#include <Arduino.h>
#include <math.h>

// Una muestra completa de todos los sensores. NAN = dato no disponible.
struct Measurement {
  uint32_t epoch = 0;  // segundos Unix (hora local del RTC)

  float tempDht   = NAN;
  float heatIndex = NAN;
  float humDht    = NAN;
  float tempAht   = NAN;
  float humAht    = NAN;
  float tvoc      = NAN;
  float eco2      = NAN;
  float aqi       = NAN;
  float co2       = NAN;
  float tempMhz   = NAN;
  float soundDb   = NAN;

  // Valores combinados que se muestran en el resumen.
  float temperature() const { return mean2(tempDht, tempAht); }
  float humidity() const { return mean2(humDht, humAht); }

 private:
  static float mean2(float a, float b) {
    if (isnan(a)) return b;
    if (isnan(b)) return a;
    return (a + b) / 2.0f;
  }
};

// Descripcion de cada columna. Agregar un dato nuevo = agregar una linea aqui;
// CSV, JSON y la web se generan a partir de esta tabla.
struct FieldDef {
  const char* key;
  const char* label;
  const char* unit;
  uint8_t decimals;
  float Measurement::*member;
};

inline constexpr FieldDef FIELDS[] = {
    {"temp_dht", "Temperatura DHT22", "°C", 1, &Measurement::tempDht},
    {"st_dht", "Sensacion termica", "°C", 1, &Measurement::heatIndex},
    {"hum_dht", "Humedad DHT22", "%", 1, &Measurement::humDht},
    {"temp_aht", "Temperatura AHT", "°C", 1, &Measurement::tempAht},
    {"hum_aht", "Humedad AHT", "%", 1, &Measurement::humAht},
    {"co2", "CO2 (MH-Z19C)", "ppm", 0, &Measurement::co2},
    {"temp_mhz", "Temperatura MH-Z19C", "°C", 1, &Measurement::tempMhz},
    {"tvoc", "TVOC", "ppb", 0, &Measurement::tvoc},
    {"eco2", "eCO2", "ppm", 0, &Measurement::eco2},
    {"aqi", "Indice AQI (1-5)", "", 0, &Measurement::aqi},
    {"db", "Ruido", "dB", 1, &Measurement::soundDb},
};
inline constexpr size_t FIELD_COUNT = sizeof(FIELDS) / sizeof(FIELDS[0]);

// Acumula muestras y entrega el promedio de cada campo (ignora NAN).
class MeasurementAverager {
 public:
  void add(const Measurement& m) {
    for (size_t i = 0; i < FIELD_COUNT; ++i) {
      float v = m.*(FIELDS[i].member);
      if (!isnan(v)) {
        sum_[i] += v;
        count_[i]++;
      }
    }
  }

  Measurement result(uint32_t epoch) const {
    Measurement m;
    m.epoch = epoch;
    for (size_t i = 0; i < FIELD_COUNT; ++i) {
      m.*(FIELDS[i].member) = count_[i] ? float(sum_[i] / count_[i]) : NAN;
    }
    return m;
  }

  void reset() {
    for (size_t i = 0; i < FIELD_COUNT; ++i) {
      sum_[i] = 0;
      count_[i] = 0;
    }
  }

 private:
  double sum_[FIELD_COUNT] = {};
  uint16_t count_[FIELD_COUNT] = {};
};
