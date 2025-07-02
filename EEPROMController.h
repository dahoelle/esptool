#ifndef EEPROM_CTL_H
#define EEPROM_CTL_H
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>
class EEPROMController {
public:
  static EEPROMController& getInstance() {
    static EEPROMController instance;
    return instance;
  }

  void begin(size_t size = 512) {
    eepromSize = size;
    EEPROM.begin(size);
    nextFreeAddr = 0;
    entries.clear();
  }

  void registerStaticEntry(const String& key, size_t size) {
    if (entries.count(key)) return;
    if (nextFreeAddr + size >= eepromSize - jsonReserve) {
      Serial.println("EEPROM overflow: can't register " + key);
      return;
    }
    entries[key] = { nextFreeAddr, size };
    nextFreeAddr += size;
  }

  void debugPrintUsage() {
    Serial.print("Static EEPROM usage: ");
    Serial.print(nextFreeAddr);
    Serial.print(" / ");
    Serial.println(eepromSize);
  }

  //dictionary proxy
  class EEPROMEntryProxy {
  public:
    EEPROMEntryProxy(EEPROMController* ctrl, const String& key)
      : controller(ctrl), key(key) {}

    template<typename T>
    EEPROMEntryProxy& operator=(const T& value) {
      controller->set<T>(key, value);
      return *this;
    }

    template<typename T, size_t N>
    EEPROMEntryProxy& operator=(const T (&arr)[N]) {
      controller->set<T, N>(key, arr);
      return *this;
    }

    template<typename T>
    operator T() const {
      return controller->get<T>(key);
    }

    template<typename T, size_t N>
    operator T*() const {
      static T temp[N];
      controller->get<T, N>(key, temp);
      return temp;
    }

  private:
    EEPROMController* controller;
    String key;
  };

  EEPROMEntryProxy operator[](const String& key) {
    return EEPROMEntryProxy(this, key);
  }

  // GET/SET values
  template<typename T>
  void set(const String& key, const T& val) {
    checkKey(key, sizeof(T));
    EEPROM.put(entries[key].address, val);
    EEPROM.commit();
  }

  template<typename T>
  T get(const String& key) {
    checkKey(key, sizeof(T));
    T val;
    EEPROM.get(entries[key].address, val);
    return val;
  }

  // GET/SET arrays
  template<typename T, size_t N>
  void set(const String& key, const T (&arr)[N]) {
    checkKey(key, sizeof(T) * N);
    for (size_t i = 0; i < N; ++i)
      EEPROM.put(entries[key].address + i * sizeof(T), arr[i]);
    EEPROM.commit();
  }

  template<typename T, size_t N>
  void get(const String& key, T (&arr)[N]) {
    checkKey(key, sizeof(T) * N);
    for (size_t i = 0; i < N; ++i)
      EEPROM.get(entries[key].address + i * sizeof(T), arr[i]);
  }

  //dynamic JSON space accessors
  void setJson(const String& key, const JsonObject& obj) {
    String jsonStr;
    serializeJson(obj, jsonStr);

    size_t available = eepromSize - nextFreeAddr;
    size_t required = key.length() + 1 + jsonStr.length() + 2; // +1 for '|', +1 for len, +1 for '\0'

    if (required >= available) {
      Serial.println("EEPROM: Not enough space for JSON");
      return;
    }

    size_t base = nextFreeAddr;

    size_t i = 0;
    EEPROM.write(base + i++, (uint8_t)key.length());
    for (char c : key) EEPROM.write(base + i++, c);
    EEPROM.write(base + i++, '|');
    for (char c : jsonStr) EEPROM.write(base + i++, c);
    EEPROM.write(base + i, '\0');
    EEPROM.commit();

    Serial.print("EEPROM: Wrote JSON at ");
    Serial.print(base);
    Serial.print(" using ");
    Serial.print(required);
    Serial.println(" bytes");
  }
  bool getJson(const String& key, JsonObject& outObj) {
    size_t base = nextFreeAddr;
    size_t i = 0;

    uint8_t keyLen = EEPROM.read(base + i++);
    String storedKey;
    for (uint8_t j = 0; j < keyLen; ++j)
      storedKey += (char)EEPROM.read(base + i++);

    if (storedKey != key || EEPROM.read(base + i++) != '|') {
      Serial.println("EEPROM: JSON key mismatch");
      return false;
    }

    String jsonStr;
    char c;
    while ((c = EEPROM.read(base + i++)) != '\0' && c != 0xFF)
      jsonStr += c;

    //doc resize
    const size_t jsonCapacity = jsonStr.length() + 16;
    StaticJsonDocument<512> tempDoc; // temporary stack allocation
    DeserializationError err = deserializeJson(tempDoc, jsonStr);

    if (err) {
      Serial.println("EEPROM: JSON deserialization failed");
      return false;
    }

    outObj.set(tempDoc.as<JsonObject>());
    return true;
  }

private:
  EEPROMController() = default;

  struct Entry {
    size_t address;
    size_t size;
  };

  std::map<String, Entry> entries;
  size_t nextFreeAddr = 0;
  size_t eepromSize = 512;
  const size_t jsonReserve = 64;

  void checkKey(const String& key, size_t size) {
    if (!entries.count(key))
        Serial.println("EEPROM key not registered: " + key);
    else if (entries[key].size < size)
        Serial.println("EEPROM type too large for key: " + key);
  }
};

#endif
/*
=========EXAMPLE USAGE=================
EEPROMController& e = EEPROMController::getInstance();

void setup() {
    Serial.begin(115200);
    delay(1000);

    e.begin(512);  // Total EEPROM size

    // Register static entries
    e.registerStaticEntry("heat", sizeof(int));
    e.registerStaticEntry("moisture", sizeof(float));
    e.registerStaticEntry("states", sizeof(bool) * 3);

    e.debugPrintUsage();  // Print how much EEPROM is used

    // Static entry usage
    e["heat"] = 42;
    e["moisture"] = 0.75f;
    bool status[3] = {true, false, true};
    e["states"] = status;

    Serial.print("Heat: "); Serial.println((int)e["heat"]);
    Serial.print("Moisture: "); Serial.println((float)e["moisture"]);

    // Dynamic JSON example
    StaticJsonDocument<128> doc;
    doc["sensor"] = "BME280";
    doc["temp"] = 24.6;
    doc["humid"] = 58.2;

    e.setJson("env", doc.as<JsonObject>());

    // Load it back
    StaticJsonDocument<128> outDoc;
    if (e.getJson("env", outDoc.to<JsonObject>())) {
        Serial.println("Loaded JSON:");
        serializeJsonPretty(outDoc, Serial);
    } else {
        Serial.println("Failed to load JSON");
    }
}

*/
