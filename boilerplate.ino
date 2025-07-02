#include "boilerplate.h"

std::vector<ISetup*> setupables;

void setup() {
    Serial.begin(115200);
    delay(5);

    // Create and collect ISetup instances
    setupables.push_back(new DummySetup("GPIO"));
    setupables.push_back(new DummySetup("Sensors"));
    setupables.push_back(new MyWebServer());
    setupables.push_back(new DummySetup("Scheduler"));

    Serial.println("Beginning setup sequence...");

    for (size_t i = 0; i < setupables.size(); ++i) {
        Serial.printf("Setup step %zu/%zu starting\n", i + 1, setupables.size());
        setupables[i]->setup();
        Serial.printf("Setup step %zu/%zu completed\n", i + 1, setupables.size());
    }

    Serial.println("All setup steps complete");
}

void loop() {
    delay(1000);
}
