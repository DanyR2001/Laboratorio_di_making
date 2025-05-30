#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <Ticker.h>
#include "DHT.h"
#include <time.h>

// ————— CONFIGURAZIONE —————
const char* ssid       = "YourSSID";
const char* password   = "YourPassword";
const char* filePath   = "/data.csv";

// Debug: se true esegue solo la prima misurazione
const bool debugMode   = false;

// Intervallo e numero max di campionamenti
const uint32_t intervalMs = 15UL * 60UL * 1000UL;  // 30 minuti
const uint8_t  maxSamples = 9 * 4;                 // 9h / 0.5h = 18 campionamenti

Ticker    sensorTicker;
uint8_t   sampleCount = 0;

WebServer server(80);

// ————— DHT11 LOCALE —————
#define LOCAL_DHT_PIN  4
#define LOCAL_DHT_TYPE DHT11
DHT dht(LOCAL_DHT_PIN, LOCAL_DHT_TYPE);

// Variabili sensori remoti e locali
float temp1 = 0.0, hum1 = 0.0, temp2 = 0.0, hum2 = 0.0;
float localTemp = 0.0, localHum = 0.0;

// ————— SETUP SPIFFS —————
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Errore montaggio SPIFFS, provo a formattare...");
    if (SPIFFS.format()) {
      Serial.println("Formattazione OK, rimonto SPIFFS...");
      if (!SPIFFS.begin(true)) {
        Serial.println("Errore dopo format");
        return;
      }
    } else {
      Serial.println("Formattazione fallita");
      return;
    }
  }
  Serial.println("SPIFFS montato");

  if (!SPIFFS.exists(filePath)) {
    File f = SPIFFS.open(filePath, FILE_WRITE);
    if (f) {
      f.println("timestamp,temp1,hum1,temp2,hum2,localTemp,localHum");
      f.close();
      Serial.println("CSV inizializzato");
    } else {
      Serial.println("Errore creazione CSV");
    }
  } else {
    Serial.println("CSV esistente trovato");
  }
}

void appendCSV(const String &line) {
  File f = SPIFFS.open(filePath, FILE_APPEND);
  if (!f) {
    Serial.println("Errore apertura CSV per append");
    return;
  }
  f.println(line);
  f.close();
}

// ————— GET REMOTE SENSORS —————
// Chiamata HTTP a ESP B per leggere /localSensors
bool getRemoteSensors(float &outT1, float &outH1, float &outT2, float &outH2) {
  HTTPClient http;
  http.begin("http://esp32.local/localSensors");
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Errore GET remota: %d\n", code);
    http.end();
    return false;
  }
  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(256);
  auto err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("JSON parse error: ");
    Serial.println(err.c_str());
    return false;
  }

  outT1 = doc["temp1"].as<float>();
  outH1 = doc["hum1"].as<float>();
  outT2 = doc["temp2"].as<float>();
  outH2 = doc["hum2"].as<float>();
  return true;
}

// ————— SAMPLING FUNCTION —————
void startSampling() {
  sampleCount++;
  Serial.printf("Campione %d di %d\n", sampleCount, maxSamples);

  // 1) Lettura DHT locale
  localHum  = dht.readHumidity();
  localTemp = dht.readTemperature();
  if (isnan(localHum) || isnan(localTemp)) {
    Serial.println("Errore lettura DHT locale");
    localHum = localTemp = 0.0;
  } else {
    Serial.printf("Locale: T=%.1f°C H=%.1f%%\n", localTemp, localHum);
  }

  // 2) Lettura sensori remoti da ESP B
  if (!getRemoteSensors(temp1, hum1, temp2, hum2)) {
    Serial.println("Errore lettura remota, zero valori");
    temp1 = hum1 = temp2 = hum2 = 0.0;
  } else {
    Serial.printf("Remoto: T1=%.1f H1=%.1f | T2=%.1f H2=%.1f\n",
                  temp1, hum1, temp2, hum2);
  }

  // 3) Costruisci linea CSV e salva
  time_t now = time(nullptr);
  String line = String(now) + "," +
                String(temp1,1) + "," + String(hum1,1) + "," +
                String(temp2,1) + "," + String(hum2,1) + "," +
                String(localTemp,1) + "," + String(localHum,1);
  appendCSV(line);
  Serial.println("Salvato: " + line);

  // 4) Ferma ticker se terminato
  if (debugMode || sampleCount >= maxSamples) {
    sensorTicker.detach();
    Serial.println("Campionamento completato");
  }
}

// ————— SETUP & LOOP —————
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n--- Avvio sistema sensori ---");

  dht.begin();
  initSPIFFS();

  // Connetti WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connettendo a WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nImpossibile connettersi al WiFi");
    return;
  }
  Serial.println("\nWiFi connesso, IP: " + WiFi.localIP().toString());

  server.on("/download", HTTP_GET, [](){
  File file = SPIFFS.open(filePath, "r");
  if (!file) {
    server.send(404, "text/plain", "File non trovato");
    return;
  }
  server.streamFile(file, "text/csv");
  file.close();
  });
  server.begin();
  Serial.println("Server HTTP avviato");

  // Sincronizza orologio via NTP
  configTime(1*3600, 1*3600, "pool.ntp.org");
  Serial.println("Sincronizzazione NTP in corso...");
  start = millis();
  while (time(nullptr) < 24*3600 && millis() - start < 10000) {
    delay(100);
  }
  Serial.println("Orologio pronto: " + String(time(nullptr)));

  // Primo campionamento e avvio ticker
  startSampling();
  if (!debugMode) {
    sensorTicker.attach_ms(intervalMs, startSampling);
    Serial.printf("Ticker avviato (%lums)\n", intervalMs);
  }
}

void loop() {
  server.handleClient();
  // Comandi via seriale
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "read") {
      // Stampa contenuto CSV
      if (SPIFFS.exists(filePath)) {
        File f = SPIFFS.open(filePath, FILE_READ);
        while (f.available()) {
          Serial.println(f.readStringUntil('\n'));
        }
        f.close();
      } else {
        Serial.println("Nessun CSV trovato");
      }
    }
    else if (cmd == "sample") {
      startSampling();
    }
    else if (cmd == "format") {
      SPIFFS.format();
      initSPIFFS();
    }
    else if (cmd == "reset") {
      ESP.restart();
    }
    else if (cmd == "help") {
      Serial.println("Comandi: read, sample, format, reset, help");
    }
  }
  delay(50);
}
