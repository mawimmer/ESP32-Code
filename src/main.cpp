#include <Arduino.h>

// WICHTIG: Die Dummy-Instanz für den LED Strip anlegen, die in wled.h deklariert wurde
#include "wled.h"
Strip strip;

// Hier holst du deine Usermod-Datei rein
#include "Multiple_Rotary_Encoder.h"

void setup() {
    Serial.begin(115200);
    delay(1000); // Kurz warten, damit der Serial Monitor sich verbinden kann
    Serial.println("\n--- Starte WLED Usermod Mock ---");

    // Usermod Setup aufrufen
    multiple_rotary_encoder.setup();

    // WICHTIG: Dein Usermod ist standardmäßig deaktiviert (enabled = false).
    // Da wir hier keine WLED-Config laden, erzwingen wir die Aktivierung für den Test.
    multiple_rotary_encoder.enable(true);
    Serial.println("[Mock] Usermod wurde manuell aktiviert.");
}

void loop() {
    // Usermod endlos laufen lassen
    multiple_rotary_encoder.loop();
    
    // Kurze Pause, damit der ESP32 Watchdog Timer (WDT) nicht triggert
    vTaskDelay(pdMS_TO_TICKS(10));
}