#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <vector>

#include "core/Measurement.h"

// Guarda las mediciones en la microSD organizadas por habitacion:
//
//   /rooms/<habitacion>/2026-10-22.csv   <- un archivo por dia
//
// La habitacion activa se recuerda en la memoria NVS (sobrevive reinicios).
// Es segura para usar desde el loop y desde el servidor web a la vez.
class RoomStorage {
 public:
  RoomStorage();

  bool begin(uint8_t csPin);
  bool sdOk() const { return sdOk_; }

  // Habitaciones
  String currentRoom() const;
  std::vector<String> rooms();
  bool createRoom(const String& name, String& outId);  // crea y selecciona
  bool selectRoom(const String& id);

  // Datos
  bool append(const Measurement& m);
  std::vector<String> days(const String& room);
  String dayPath(const String& room, const String& day) const;

  static String sanitize(const String& name);

 private:
  bool ensureRoomDir(const String& id);
  String roomDir(const String& id) const;

  SemaphoreHandle_t mutex_;
  bool sdOk_ = false;
  String current_;
};
