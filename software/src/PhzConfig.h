#pragma once

#include <stdint.h>

#ifdef __IMXRT1062__
#include <LittleFS.h>
#include <SD.h>
#include <unordered_map>

extern bool SDcard_Ready;

namespace PhzConfig {
  using KEY = uint16_t;
  using VALUE = uint64_t;
  using ConfigMap = std::unordered_map<KEY, VALUE>;

  const char * const CONFIG_FILENAME = "GLOBALS.CFG";

  extern LittleFS_Program myfs;

  // Forward Decl
  void Init();
  void listFiles(FS &fs = myfs);
  bool load_config(const char* filename = CONFIG_FILENAME, FS &fs = myfs);
  bool save_config(const char* filename = CONFIG_FILENAME, FS &fs = myfs);
  void clear_config();

  void setValue(KEY key, VALUE value);
  bool getValue(KEY key, VALUE &value);
  void deleteKey(KEY key);

  void setData(KEY key, VALUE value);
  bool getData(KEY key, VALUE &value);
  void deleteData(KEY key);

  void printDirectory(FS &fs = myfs);
  void printDirectory(File dir, int numSpaces);
  void printSpaces(int num);
  void eraseFiles(FS &fs = myfs);

}
#else
// No filesystem-backed config storage on Teensy 3.x — provide inert stubs so
// callers compile everywhere and degrade gracefully (reads fail, writes vanish).
namespace PhzConfig {
  using KEY = uint16_t;
  using VALUE = uint64_t;

  const char * const CONFIG_FILENAME = "";

  inline void Init() {}
  inline bool load_config(const char* = CONFIG_FILENAME) { return false; }
  inline bool save_config(const char* = CONFIG_FILENAME) { return false; }
  inline void clear_config() {}

  inline void setValue(KEY, VALUE) {}
  inline bool getValue(KEY, VALUE&) { return false; }
  inline void deleteKey(KEY) {}

  inline void setData(KEY, VALUE) {}
  inline bool getData(KEY, VALUE&) { return false; }
  inline void deleteData(KEY) {}
}
#endif
