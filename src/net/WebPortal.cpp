#include "net/WebPortal.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <WiFi.h>

#include "net/WebPage.h"

namespace {
void sendJson(AsyncWebServerRequest* req, const JsonDocument& doc, int code = 200) {
  AsyncResponseStream* res = req->beginResponseStream("application/json");
  res->setCode(code);
  serializeJson(doc, *res);
  req->send(res);
}

void sendError(AsyncWebServerRequest* req, int code, const char* msg) {
  JsonDocument doc;
  doc["error"] = msg;
  sendJson(req, doc, code);
}

String param(AsyncWebServerRequest* req, const char* name, bool post = false) {
  const AsyncWebParameter* p = req->getParam(name, post);
  return p ? p->value() : String();
}

void putValue(JsonVariant dst, float v) {
  if (isnan(v)) dst.set(nullptr);
  else dst.set(roundf(v * 10) / 10);
}
}  // namespace

void WebPortal::begin(const char* ssid, const char* password, uint8_t channel) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password, channel);
  dns_.start(53, "*", WiFi.softAPIP());  // cualquier dominio -> la placa
  routes();
  server_.begin();
  Serial.printf("[WiFi] AP \"%s\" en http://%s\n", ssid, ip().c_str());
}

String WebPortal::ip() const { return WiFi.softAPIP().toString(); }
uint8_t WebPortal::clients() const { return WiFi.softAPgetStationNum(); }

void WebPortal::routes() {
  server_.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "text/html; charset=utf-8", WEB_PAGE);
  });

  server_.on("/api/fields", HTTP_GET, [](AsyncWebServerRequest* req) {
    JsonDocument doc;
    for (const FieldDef& f : FIELDS) {
      JsonObject o = doc.add<JsonObject>();
      o["key"] = f.key;
      o["label"] = f.label;
      o["unit"] = f.unit;
      o["dec"] = f.decimals;
    }
    sendJson(req, doc);
  });

  server_.on("/api/live", HTTP_GET, [this](AsyncWebServerRequest* req) {
    Measurement m = live_.latest();
    JsonDocument doc;
    doc["epoch"] = m.epoch;
    doc["room"] = storage_.currentRoom();
    doc["sd"] = storage_.sdOk();
    doc["rtc"] = clock_.rtcOk();
    JsonObject values = doc["values"].to<JsonObject>();
    for (const FieldDef& f : FIELDS) putValue(values[f.key], m.*(f.member));
    putValue(doc["temp"], m.temperature());
    putValue(doc["hum"], m.humidity());
    JsonObject sensors = doc["sensors"].to<JsonObject>();
    for (Sensor* s : sensors_.sensors()) sensors[s->name()] = s->ok();
    sendJson(req, doc);
  });

  server_.on("/api/recent", HTTP_GET, [this](AsyncWebServerRequest* req) {
    std::vector<Measurement> pts = live_.recent();
    JsonDocument doc;
    JsonArray t = doc["t"].to<JsonArray>();
    for (const Measurement& m : pts) t.add(m.epoch);
    for (const FieldDef& f : FIELDS) {
      JsonArray col = doc[f.key].to<JsonArray>();
      for (const Measurement& m : pts) putValue(col.add<JsonVariant>(), m.*(f.member));
    }
    sendJson(req, doc);
  });

  server_.on("/api/rooms", HTTP_GET, [this](AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["current"] = storage_.currentRoom();
    JsonArray arr = doc["rooms"].to<JsonArray>();
    for (const String& r : storage_.rooms()) arr.add(r);
    sendJson(req, doc);
  });

  server_.on("/api/rooms", HTTP_POST, [this](AsyncWebServerRequest* req) {
    String id;
    if (!storage_.createRoom(param(req, "name", true), id)) {
      return sendError(req, 400, "Nombre invalido o SD no disponible");
    }
    JsonDocument doc;
    doc["current"] = id;
    sendJson(req, doc);
  });

  server_.on("/api/rooms/select", HTTP_POST, [this](AsyncWebServerRequest* req) {
    String id = param(req, "id", true);
    if (!storage_.selectRoom(id)) return sendError(req, 404, "Habitacion inexistente");
    JsonDocument doc;
    doc["current"] = id;
    sendJson(req, doc);
  });

  server_.on("/api/days", HTTP_GET, [this](AsyncWebServerRequest* req) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (const String& d : storage_.days(param(req, "room"))) arr.add(d);
    sendJson(req, doc);
  });

  server_.on("/api/history", HTTP_GET, [this](AsyncWebServerRequest* req) {
    String path = storage_.dayPath(param(req, "room"), param(req, "day"));
    if (!storage_.sdOk() || !SD.exists(path)) return sendError(req, 404, "Sin datos");
    // Se envia en bloques directo desde la SD: no ocupa RAM aunque el archivo sea grande.
    req->send(SD, path, "text/csv", req->hasParam("download"));
  });

  server_.on("/api/time", HTTP_POST, [this](AsyncWebServerRequest* req) {
    uint32_t epoch = param(req, "epoch", true).toInt();
    if (epoch < 1700000000UL) return sendError(req, 400, "Hora invalida");
    clock_.requestSet(epoch);
    req->send(204);
  });

  // Portal cautivo: cualquier otra URL lleva a la pagina principal.
  server_.onNotFound([](AsyncWebServerRequest* req) { req->redirect("http://192.168.4.1/"); });
}
