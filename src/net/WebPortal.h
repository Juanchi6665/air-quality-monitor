#pragma once
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

#include "core/LiveData.h"
#include "hardware/Clock.h"
#include "sensors/SensorHub.h"
#include "storage/RoomStorage.h"

// Punto de acceso WiFi + servidor web con la API REST y la pagina.
//
//   GET  /                 pagina web
//   GET  /api/live         ultima medicion + estado
//   GET  /api/recent       ultima hora en RAM (grafico en vivo)
//   GET  /api/fields       descripcion de las columnas
//   GET  /api/rooms        habitaciones y la activa
//   POST /api/rooms        name=...  crea y selecciona
//   POST /api/rooms/select id=...    cambia la habitacion activa
//   GET  /api/days?room=   dias guardados de una habitacion
//   GET  /api/history?room=&day=     CSV del dia (tambien sirve para descargar)
//   POST /api/time         epoch=... ajusta el reloj con la hora del telefono
class WebPortal {
 public:
  WebPortal(LiveData& live, RoomStorage& storage, SensorHub& sensors, Clock& clock)
      : server_(80), live_(live), storage_(storage), sensors_(sensors), clock_(clock) {}

  void begin(const char* ssid, const char* password, uint8_t channel);
  void loop() { dns_.processNextRequest(); }  // portal cautivo

  String ip() const;
  uint8_t clients() const;

 private:
  void routes();

  AsyncWebServer server_;
  DNSServer dns_;
  LiveData& live_;
  RoomStorage& storage_;
  SensorHub& sensors_;
  Clock& clock_;
};
