#ifndef EEPROM_CTL_H
#define EEPROM_CTL_H
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>

class EEPROMController {
public:
  static EEPROMController& getInstance() {                          //singelton
    static EEPROMController instance;
    return instance;
  }
  void begin(size_t eepromSize = 512) {
    this->eepromSize = eepromSize;
    if (!EEPROM.begin(eepromSize))
      Serial.println("EEPROM Controller init failed");
  }
  void registerStaticEntry(const String& key, size_t size) {        //space for static structs
      if (entries.count(key)) {
        Serial.println("Duplicate key: " + key);
        return;
      }
      if (nextFreeAddr + size >= eepromSize - 64) {                 //space for dynamic JSON
        Serial.println("EEPROM overflow when registering: " + key);
        return;
      }

      entries[key] = { nextFreeAddr, size };
      nextFreeAddr += size;
  }
  void debugPrintUsage() {
    Serial.print("Static EEPROM usage: ");
    Serial.print(nextFreeAddr);
    Serial.print("/");
    Serial.println(eepromSize);
  }

  // DICT behavior
  template<typename T> 
  T get(const String& key) {
    checkKey(key, sizeof(T));
    T val;
    EEPROM.get(entries[key].address, val);
    return val;
  }
  template<typename T>
  void set(const String& key, const T& val) {
    checkKey(key, sizeof(T));
    EEPROM.put(entries[key].address, val);
    EEPROM.commit();
  }

  // JSON set
  void setJson(const String& key, const JsonObject& obj) {
    String jsonStr;
    serializeJson(obj, jsonStr);
    size_t base = nextFreeAddr;
    size_t len = jsonStr.length();

    if (base + len + 2 >= eepromSize) {
      Serial.println("EEPROM full: cannot write JSON");
      return;
    }

    EEPROM.write(base, key.length());
    for (size_t i = 0; i < key.length(); i++)
      EEPROM.write(base + 1 + i, key[i]);
    EEPROM.write(base + 1 + key.length(), '|'); // simple separator
    for (size_t i = 0; i < len; ++i)
      EEPROM.write(base + 2 + key.length() + i, jsonStr[i]);
    EEPROM.commit();
  }
  // JSON get
  bool getJson(const String& key, JsonObject& outObj) {
    size_t base = nextFreeAddr;
    if (EEPROM.read(base) != key.length())
      return false;
    String keyIn;
    for (size_t i = 0; i < key.length(); ++i)
      keyIn += (char)EEPROM.read(base + 1 + i);
    if (keyIn != key || EEPROM.read(base + 1 + key.length()) != '|')
      return false;
    String jsonStr;
    for (size_t i = base + 2 + key.length(); i < eepromSize; ++i) {
      char c = EEPROM.read(i);
      if (c == '\0' || c == 0xFF) break;
      jsonStr += c;
    }

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, jsonStr);
    if (err) return false;

    outObj.set(doc.as<JsonObject>());
    return true;
  }

private:
  struct Entry {
    size_t address;
    size_t size;
  };
  std::map<String, Entry> entries;
  size_t nextFreeAddr = 0;
  size_t eepromSize = 512;
  EEPROMController() = default;

  void checkKey(const String& key, size_t size) {
    if (!entries.count(key))
      Serial.println("Key not registered: " + key);
    else if (entries[key].size < size)
      Serial.println("Key size mismatch: " + key);
  }
};
#endif
/*
=========EXAMPLE USAGE=================
EEPROMController& e = EEPROMController::getInstance();

void setup() {
  Serial.begin(115200);
  e.begin(512);

  // Register static keys
  e.registerStaticEntry("heat", sizeof(int));
  e.registerStaticEntry("moisture", sizeof(float));
  e.registerStaticEntry("states", sizeof(bool) * 3);
  e.debugPrintUsage();

  // Set values
  e.set("heat", 42);
  e.set("moisture", 0.73f);

  bool s[3] = {true, false, true};
  e.set("states", s);  // You’ll need to overload this if you want arrays

  // Get values
  int heat = e.get<int>("heat");
  float moisture = e.get<float>("moisture");

  Serial.println("Heat: " + String(heat));
  Serial.println("Moisture: " + String(moisture));

  // Store JSON
  StaticJsonDocument<128> doc;
  doc["sensor"] = "DHT22";
  doc["value"] = 23.4;
  e.putJson("env", doc.as<JsonObject>());
}
*/
