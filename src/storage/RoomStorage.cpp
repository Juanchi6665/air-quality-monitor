#include "storage/RoomStorage.h"

#include <Preferences.h>
#include <RTClib.h>
#include <SD.h>
#include <SPI.h>

#include <algorithm>

#include "Config.h"

namespace {
constexpr const char* PREFS_NS = "monitor";
constexpr const char* PREFS_ROOM = "room";

struct Lock {
  explicit Lock(SemaphoreHandle_t m) : m_(m) { xSemaphoreTake(m_, portMAX_DELAY); }
  ~Lock() { xSemaphoreGive(m_); }
  SemaphoreHandle_t m_;
};

void writeHeader(File& f) {
  f.print("fecha_hora");
  for (const FieldDef& fd : FIELDS) {
    f.print(';');
    f.print(fd.key);
  }
  f.print('\n');
}
}  // namespace

RoomStorage::RoomStorage() : mutex_(xSemaphoreCreateMutex()) {}

bool RoomStorage::begin(uint8_t csPin) {
  Lock lock(mutex_);
  sdOk_ = SD.begin(csPin);
  if (sdOk_ && !SD.exists(Storage::ROOMS_DIR)) SD.mkdir(Storage::ROOMS_DIR);

  Preferences prefs;
  prefs.begin(PREFS_NS, true);
  current_ = prefs.getString(PREFS_ROOM, Storage::DEFAULT_ROOM);
  prefs.end();

  if (sdOk_) ensureRoomDir(current_);
  return sdOk_;
}

String RoomStorage::currentRoom() const {
  Lock lock(mutex_);
  return current_;
}

std::vector<String> RoomStorage::rooms() {
  Lock lock(mutex_);
  std::vector<String> out;
  if (!sdOk_) return out;
  File dir = SD.open(Storage::ROOMS_DIR);
  for (File e = dir.openNextFile(); e; e = dir.openNextFile()) {
    if (e.isDirectory()) out.push_back(e.name());
    e.close();
  }
  dir.close();
  std::sort(out.begin(), out.end());
  return out;
}

bool RoomStorage::createRoom(const String& name, String& outId) {
  outId = sanitize(name);
  if (outId.isEmpty()) return false;
  {
    Lock lock(mutex_);
    if (!sdOk_ || !ensureRoomDir(outId)) return false;
  }
  return selectRoom(outId);
}

bool RoomStorage::selectRoom(const String& id) {
  Lock lock(mutex_);
  if (!sdOk_ || !SD.exists(roomDir(id))) return false;
  current_ = id;
  Preferences prefs;
  prefs.begin(PREFS_NS, false);
  prefs.putString(PREFS_ROOM, id);
  prefs.end();
  return true;
}

bool RoomStorage::append(const Measurement& m) {
  Lock lock(mutex_);
  if (!sdOk_) {  // reintenta por si la tarjeta se inserto despues del arranque
    SD.end();
    sdOk_ = SD.begin(Pins::SD_CS) && ensureRoomDir(current_);
    if (!sdOk_) return false;
  }

  DateTime t(m.epoch);
  char path[64];
  snprintf(path, sizeof(path), "%s/%s/%04d-%02d-%02d.csv", Storage::ROOMS_DIR, current_.c_str(),
           t.year(), t.month(), t.day());

  bool isNew = !SD.exists(path);
  File f = SD.open(path, FILE_APPEND);
  if (!f) {
    sdOk_ = false;  // se reintenta en la proxima escritura
    return false;
  }
  if (isNew) writeHeader(f);

  char buf[24];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", t.year(), t.month(), t.day(), t.hour(),
           t.minute(), t.second());
  f.print(buf);
  for (const FieldDef& fd : FIELDS) {
    f.print(';');
    float v = m.*(fd.member);
    if (!isnan(v)) f.print(v, fd.decimals);
  }
  f.print('\n');
  f.close();
  return true;
}

std::vector<String> RoomStorage::days(const String& room) {
  Lock lock(mutex_);
  std::vector<String> out;
  if (!sdOk_) return out;
  File dir = SD.open(roomDir(sanitize(room)));
  if (!dir) return out;
  for (File e = dir.openNextFile(); e; e = dir.openNextFile()) {
    String n = e.name();
    if (!e.isDirectory() && n.endsWith(".csv")) out.push_back(n.substring(0, n.length() - 4));
    e.close();
  }
  dir.close();
  std::sort(out.begin(), out.end());
  return out;
}

String RoomStorage::dayPath(const String& room, const String& day) const {
  return roomDir(sanitize(room)) + "/" + sanitize(day) + ".csv";
}

// Solo letras, numeros, '-' y '_' (FAT y URLs seguras). Espacios -> '_'.
String RoomStorage::sanitize(const String& name) {
  String out;
  for (size_t i = 0; i < name.length() && out.length() < 24; ++i) {
    char c = name[i];
    if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') out += char(tolower(c));
    else if (c == ' ') out += '_';
  }
  return out;
}

bool RoomStorage::ensureRoomDir(const String& id) {
  String dir = roomDir(id);
  return SD.exists(dir) || SD.mkdir(dir);
}

String RoomStorage::roomDir(const String& id) const { return String(Storage::ROOMS_DIR) + "/" + id; }
