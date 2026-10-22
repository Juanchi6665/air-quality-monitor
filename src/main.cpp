// ============================================================
//  Monitor de calidad del aire - ESP32-S3
//
//  main.cpp solo crea los objetos y los coordina. Cada parte vive
//  en su carpeta:
//    include/Config.h   pines, tiempos y WiFi
//    src/core/          modelo de datos (Measurement, LiveData)
//    src/sensors/       un objeto por sensor + SensorHub
//    src/hardware/      OLED, encoder y reloj RTC
//    src/storage/       microSD organizada por habitaciones
//    src/net/           punto de acceso, API y pagina web
// ============================================================
#include <Arduino.h>
#include <Wire.h>

#include "Config.h"
#include "core/LiveData.h"
#include "core/Measurement.h"
#include "hardware/Clock.h"
#include "hardware/Display.h"
#include "hardware/RotaryEncoder.h"
#include "net/WebPortal.h"
#include "sensors/ClimateSensors.h"
#include "sensors/GasSensors.h"
#include "sensors/SensorHub.h"
#include "sensors/SoundSensor.h"
#include "storage/RoomStorage.h"

// Temporizador no bloqueante (reemplaza a los delay()).
class Every {
 public:
  explicit Every(uint32_t ms) : ms_(ms) {}
  bool due(uint32_t now) {
    if (now - last_ < ms_) return false;
    last_ = now;
    return true;
  }

 private:
  uint32_t ms_;
  uint32_t last_ = 0;
};

/* ---------------- Objetos ---------------- */
DhtSensor dht(Pins::DHT);
AhtSensor aht;
Ens160Sensor ens160;
Mhz19Sensor mhz19(Serial1, Pins::MHZ19_RX, Pins::MHZ19_TX);
SoundSensor sound(Pins::SOUND);
SensorHub sensors;

Display display(Pins::I2C_SCL, Pins::I2C_SDA);
RotaryEncoder encoder(Pins::ENC_CLK, Pins::ENC_DT, Pins::ENC_SW);
Clock rtcClock;

LiveData live;
RoomStorage storage;
WebPortal web(live, storage, sensors, rtcClock);

MeasurementAverager logAverage;     // promedio de cada minuto -> SD
MeasurementAverager recentAverage;  // promedio de 10 s -> grafico en vivo

Every sampleTimer(Timing::SAMPLE_MS);
Every recentTimer(Timing::RECENT_MS);
Every displayTimer(Timing::DISPLAY_MS);
Every serialTimer(Timing::SERIAL_MS);
int lastLoggedMinute = -1;

/* ---------------- Funciones ---------------- */

void printMeasurement(const Measurement& m) {
  Serial.printf("[%s] T=%.1fC H=%.1f%% CO2=%.0f TVOC=%.0f eCO2=%.0f dB=%.0f\n",
                storage.currentRoom().c_str(), m.temperature(), m.humidity(), m.co2, m.tvoc, m.eco2,
                m.soundDb);
}

void takeSample() {
  DateTime now = rtcClock.now();
  Measurement m = sensors.snapshot(now.unixtime());
  ens160.setEnvironment(m.temperature(), m.humidity());

  live.setLatest(m);
  logAverage.add(m);
  recentAverage.add(m);

  // Guardar una linea por minuto (al cambiar el minuto, como antes).
  if (now.minute() != lastLoggedMinute) {
    if (lastLoggedMinute >= 0) {
      // Hora "redonda" (hh:mm:00) = fin del minuto promediado.
      Measurement avg = logAverage.result(now.unixtime() - now.second());
      if (storage.append(avg)) Serial.println("[SD] Guardado");
      else Serial.println("[SD] Error al guardar");
    }
    logAverage.reset();
    lastLoggedMinute = now.minute();
  }
}

/* ---------------- Setup / Loop ---------------- */

void setup() {
  Serial.begin(115200);
  Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);

  display.begin();
  encoder.begin();
  if (!rtcClock.begin()) Serial.println("[RTC] No detectado: usando reloj interno");

  sensors.add(dht);
  sensors.add(aht);
  sensors.add(ens160);
  sensors.add(mhz19);
  sensors.add(sound);
  sensors.begin();

  if (!storage.begin(Pins::SD_CS)) Serial.println("[SD] No detectada: se reintentara");
  Serial.printf("[SD] Habitacion activa: %s\n", storage.currentRoom().c_str());

  web.begin(Net::AP_SSID, Net::AP_PASSWORD, Net::AP_CHANNEL);
}

void loop() {
  uint32_t now = millis();

  web.loop();
  rtcClock.service();
  sensors.update(now);

  if (sampleTimer.due(now)) takeSample();

  if (recentTimer.due(now)) {
    live.pushRecent(recentAverage.result(rtcClock.now().unixtime()));
    recentAverage.reset();
  }

  if (int steps = encoder.takeSteps()) display.nextPage(steps);
  if (encoder.wasPressed()) display.home();

  if (displayTimer.due(now)) {
    SystemStatus st{storage.currentRoom(), web.ip(), storage.sdOk(), web.clients()};
    display.render(live.latest(), st);
  }

  if (serialTimer.due(now)) printMeasurement(live.latest());
}
