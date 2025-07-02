Collection of cpp Classes to help develop ESP32

=================EEPROMController====================
seperates EEPROM into static and dynamic space.
  static space: holds fixed size structs and arrays of sructs
  dyamic space: holds json serializable objects

getInstance() : singelton access
  EEPROMController& e = EEPROMController::getInstance();

begin(eepromSize) : default eeprom startup with optional size (default 512)
  e.begin();

registerStaticEntry(key,size) to register static space variables
    e.registerStaticEntry("moisture", sizeof(float));
    e.registerStaticEntry("states", sizeof(bool) * 3);
    
implicit get/set to set structs or arrays of structs with fixed 
  e["moisture"] = 0.75f;
  bool status[3] = {true, false, true};
  e["states"] = status;

getJson()/setJson() for dynamic space access
  StaticJsonDocument<128> doc;
  doc["msg"] = "Hello World";
  e.setJson("env", doc.as<JsonObject>());
  StaticJsonDocument<128> outDoc;
  if (e.getJson("env", outDoc.to<JsonObject>())) {
      Serial.println("Loaded JSON:");
      serializeJsonPretty(outDoc, Serial);
  }
