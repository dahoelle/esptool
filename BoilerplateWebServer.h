#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

class BoilerplateWebServer : public ISetup {
public:
    MyWebServer(AsyncWebServer& srv) : server(srv) {}

    void setup() override {
        //ROOT HTML
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(200, "text/html", "<html><body><h1>MyWebServer Root</h1></body></html>");
        });

        // POST /heat/
        server.on("/heat/", HTTP_POST, [](AsyncWebServerRequest *request) {
            if (!request->hasParam("seconds", true)) {
                request->send(400, "text/plain", "Missing 'seconds' parameter");
                return;
            }
            String secondsStr = request->getParam("seconds", true)->value();
            int seconds = secondsStr.toInt();
            if (seconds < 0 || seconds > 90) {
                request->send(400, "text/plain", "Invalid 'seconds' parameter");
                return;
            }
            Serial.printf("Heating for %d seconds\n", seconds);
            request->send(200, "text/plain", "Heating started");
        });

        // PUT /wifi-config/
        server.on("/wifi-config/", HTTP_PUT, [](AsyncWebServerRequest *request) {
            if (!request->hasParam("ssid", true) || !request->hasParam("passkey", true)) {
                request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
                return;
            }
            String ssid = request->getParam("ssid", true)->value();
            String passkey = request->getParam("passkey", true)->value();
            if (ssid.length() > 31 || passkey.length() > 31) {
                request->send(400, "application/json", "{\"error\":\"ssid or passkey too long\"}");
                return;
            }
          
            Serial.printf("WiFi config set: SSID=%s, Passkey=%s\n", ssid.c_str(), passkey.c_str());
            request->send(200, "application/json", "{\"status\":\"success\"}");
        });

        // 404 handler
        server.onNotFound([](AsyncWebServerRequest *request) {
            Serial.printf("404 Not Found: %s\n", request->url().c_str());
            request->send(404, "text/plain", "Not Found");
        });

        server.begin();
        Serial.println("Web server started");
    }

private:
    AsyncWebServer& server;
};
