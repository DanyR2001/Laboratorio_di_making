#include <HTTPClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <AHTxx.h>
#include <Wire.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <esp32-hal-ledc.h>
#include "config.h"
#include <time.h>
#include <ESPmDNS.h>

// NTP per SSL
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;        // Europa/Rome
const int daylightOffset_sec = 3600;    // ora legale
const unsigned long ntpTimeout = 10000; // 10 secondi timeout NTP


// Pin I2C e sensore
#define I2C_SDA       21
#define I2C_SCL       22
#define SENSOR_ADDR   0x38

// Pin relè
#define RELAY_PIN     16

// PWM ventola/LED
#define FAN_PWM_PIN        17
#define FAN_PWM_FREQ       25000
#define FAN_PWM_RESOLUTION 8

// Multiplexer TCA9548A
#define MUX_ADDR 0x70

// Definizione della tastiera inline
String keyboardJson = "[[{\"text\":\"SLEEP\",\"callback_data\":\"fan_47\"},"
                      "{\"text\":\"MIN\",\"callback_data\":\"fan_100\"}],"
                      "[{\"text\":\"MED\",\"callback_data\":\"fan_170\"},"
                      "{\"text\":\"MAX\",\"callback_data\":\"fan_255\"}]]";

WebServer server(80);
WiFiClientSecure secureClient;
UniversalTelegramBot bot(botToken, secureClient);
AHTxx aht25(SENSOR_ADDR, AHT2x_SENSOR);

float temp1=0, hum1=0;
float temp2=0, hum2=0;
bool relayState=false;
int fanSpeed=0;

// Controllo accesso Telegram
bool isAuthorized(const String& chat_id) {
  for (int i = 0; i < NUM_AUTHORIZED_USERS; ++i) {
    if (chat_id == String(AUTHORIZED_USERS[i])) return true;
  }
  return false;
}

// Selettore canale MUX
void selectMUXChannel(uint8_t channel) {
  Wire.beginTransmission(MUX_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
  delay(100);
}

bool readSensor1() {
  selectMUXChannel(1);
  if (!aht25.begin(I2C_SDA, I2C_SCL, 100000)) return false;
  temp1 = aht25.readTemperature();
  hum1  = aht25.readHumidity();
  Serial.printf("Sens1: %.1f°C, %.1f%%\n", temp1, hum1);
  return true;
}
bool readSensor2() {
  selectMUXChannel(2);
  if (!aht25.begin(I2C_SDA, I2C_SCL, 100000)) return false;
  temp2 = aht25.readTemperature();
  hum2  = aht25.readHumidity();
  Serial.printf("Sens2: %.1f°C, %.1f%%\n", temp2, hum2);
  return true;
}

void setFanSpeed(int speed) {
  fanSpeed = constrain(speed, 0, 255);
  ledcWrite(FAN_PWM_PIN, fanSpeed);
}

String webpage() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>ESP32 Dashboard</title>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" rel="stylesheet">
</head>
<body class="bg-light">
  <div class="container py-4">

    <h1 class="text-center mb-4">🌬️ Smart Climate Controller</h1>

    <!-- Riga principale -->
    <div class="row gx-4 gy-4 align-items-stretch">

      <!-- Colonna sinistra: DEUMIDIFICATORE (rowspan 2) -->
      <div class="col-md-3 d-flex">
        <div class="card shadow-sm flex-fill">
          <div class="card-body d-flex flex-column justify-content-center text-center">
            <h5 class="card-title">💧 Deumidificatore</h5>
            <button class="btn btn-success m-1" onclick="toggleRelay('on')">Accendi</button>
            <button class="btn btn-danger m-1" onclick="toggleRelay('off')">Spegni</button>
            <p class="mt-3 h5">Stato: <span id="relay">%R%</span></p>
        </div>
        </div>
      </div>

      <!-- Colonna centrale: 2 card impilate -->
      <div class="col-md-6">
        <div class="row gx-4 gy-4">

          <!-- Temperatura lato caldo -->
          <div class="col-12">
            <div class="card shadow-sm h-100">
              <div class="card-body">
                <h5 class="card-title">🔥 Temperatura lato caldo</h5>
                <p id="sensor2" class="display-5 mb-1">%S2%</p>
                <span id="hum2" class="badge bg-warning">Umidità: %H2%%</span>
              </div>
            </div>
          </div>

          <!-- Temperatura lato freddo -->
          <div class="col-12">
            <div class="card shadow-sm h-100">
              <div class="card-body">
                <h5 class="card-title">🌡️ Temperatura lato freddo</h5>
                <p id="sensor1" class="display-5 mb-1">%S1%</p>
                <span id="hum1" class="badge bg-info">Umidità: %H1%%</span>
              </div>
            </div>
          </div>

        </div>
      </div>

      <!-- Colonna destra: VENTOLA (rowspan 2) -->
      <div class="col-md-3 d-flex">
        <div class="card shadow-sm flex-fill">
          <div class="card-body d-flex flex-column justify-content-center text-center">
            <h5 class="card-title">🌀 Ventola</h5>
            <button class="btn btn-secondary m-1" onclick="setFan(47)">SLEEP</button>
            <button class="btn btn-primary m-1"   onclick="setFan(100)">MIN</button>
            <button class="btn btn-warning m-1"   onclick="setFan(170)">MED</button>
            <button class="btn btn-danger m-1"    onclick="setFan(255)">MAX</button>
            <p class="mt-3 h5">Velocità: <span id="fval">%F%</span></p>
          </div>
        </div>
      </div>

    </div>
  </div>

<script>
function toggleRelay(s) {
  fetch('/relay/' + s)
    .then(r => r.text())
    .then(d => { document.getElementById('relay').innerText = d; });
}

function setFan(v) {
  fetch('/fan?speed=' + v)
    .then(r => r.text())
    .then(_ => {
      document.getElementById('fval').innerText = v;
    });
}

function update() {
  fetch('/status')
    .then(r => r.json())
    .then(d => {
      document.getElementById('relay').innerText    = d.relay    ? 'Acceso' : 'Spento';
      document.getElementById('fval').innerText     = d.fanSpeed;
      document.getElementById('sensor2').innerText  = d.temp2.toFixed(1) + ' °C';
      document.getElementById('hum2').innerText     = 'Umidità: ' + d.hum2.toFixed(1) + '%';
      document.getElementById('sensor1').innerText  = d.temp1.toFixed(1) + ' °C';
      document.getElementById('hum1').innerText     = 'Umidità: ' + d.hum1.toFixed(1) + '%';
    });
}

// Primo refresh e poi ogni secondo
update();
setInterval(update, 2000);
</script>

</body>
</html>
)rawliteral";

  page.replace("%S1%", String(temp1,1) + " °C");
  page.replace("%H1%", String(hum1,1));
  page.replace("%S2%", String(temp2,1) + " °C");
  page.replace("%H2%", String(hum2,1));
  page.replace("%R%", relayState ? "Acceso" : "Spento");
  page.replace("%F%", String(fanSpeed));

  return page;
}

void handleRoot() { server.send(200, "text/html", webpage()); }
void handleRelayOn() { digitalWrite(RELAY_PIN, HIGH); relayState=true; server.send(200,"text/plain","Acceso"); }
void handleRelayOff(){ digitalWrite(RELAY_PIN, LOW); relayState=false; server.send(200,"text/plain","Spento"); }
void handleFan() { if(server.hasArg("speed")){ setFanSpeed(server.arg("speed").toInt()); server.send(200,"text/plain","OK"); } else server.send(400,"text/plain","Missing"); }
void handleLocalSensors() {
  // Leggi i due sensori locali
  if (!readSensor1() || !readSensor2()) {
    server.send(500, "text/plain", "Errore lettura sensori locali");
    return;
  }
  // Costruisci il JSON di risposta
  String resp = "{";
  resp += "\"temp1\":" + String(temp1,1) + ",";
  resp += "\"hum1\":"  + String(hum1,1)  + ",";
  resp += "\"temp2\":" + String(temp2,1) + ",";
  resp += "\"hum2\":"  + String(hum2,1);
  resp += "}";
  server.send(200, "application/json", resp);
}


void handleStatus(){
  String j = "{";
  j += "\"temp1\":" + String(temp1,1) + ",";
  j += "\"hum1\":" + String(hum1,1) + ",";
  j += "\"temp2\":" + String(temp2,1) + ",";
  j += "\"hum2\":" + String(hum2,1) + ",";
  j += "\"relay\":";
  j += relayState ? "true" : "false";
  j += ",";
  j += "\"fanSpeed\":" + String(fanSpeed);
  j += "}";
  server.send(200, "application/json", j);
}

void reconnectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.println("Riconnessione WiFi...");
  WiFi.disconnect();
  WiFi.reconnect();
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi riconnesso");
    // Riconfigura client sicuro dopo la riconnessione
    secureClient.stop(); // Chiude la connessione precedente
    secureClient.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Imposta il certificato
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // Ri-sincronizza NTP
  } else {
    Serial.println("\nFallimento riconnessione WiFi!");
  }
}

bool syncNTP() {
  Serial.print("Sincronizzazione NTP");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  unsigned long start = millis();
  while (millis() - start < ntpTimeout) {
    if (getLocalTime(&timeinfo, 5000)) { // Timeout 5s
      Serial.println(" OK");
      return true;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println(" Fallito!");
  return false;
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status()!=WL_CONNECTED) delay(500);
  Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());

  if (MDNS.begin("esp32")) {
    Serial.println("mDNS avviato: esp32.local");
    MDNS.addService("http", "tcp", 80); // Registra il servizio HTTP
  } else {
    Serial.println("Errore nell'avvio di mDNS");
  }


  if (!syncNTP()) {
    Serial.println("Avvio senza tempo sincronizzato. SSL potrebbe non funzionare!");
  }

  pinMode(RELAY_PIN, OUTPUT); digitalWrite(RELAY_PIN, LOW);
  Wire.begin(I2C_SDA, I2C_SCL, 100000);
  pinMode(FAN_PWM_PIN, OUTPUT);
  ledcAttach(FAN_PWM_PIN, FAN_PWM_FREQ, FAN_PWM_RESOLUTION);
  setFanSpeed(0);

  server.on("/", handleRoot);
  server.on("/relay/on", handleRelayOn);
  server.on("/relay/off", handleRelayOff);
  server.on("/fan", handleFan);
  server.on("/status", handleStatus);
  server.on("/localSensors", HTTP_GET, handleLocalSensors);
  server.begin();
  secureClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);
}

void loop() {
  reconnectWiFi();

  server.handleClient();
  static unsigned long lastRun = 0;
  static unsigned long lastSensorRead = 0;

  if(millis() - lastSensorRead > 1500) {
    readSensor1();
    readSensor2();
    lastSensorRead = millis();
  }

  if (millis() - lastRun > 1000) {
    int n = bot.getUpdates(bot.last_message_received + 1);
    while (n) {
      for (int i = 0; i < n; ++i) {
        String cid = String(bot.messages[i].chat_id);
        String txt = bot.messages[i].text;
        String uname = bot.messages[i].from_name;

        if (!isAuthorized(cid)) {
          bot.sendMessage(cid, "🚫 *Accesso negato.*\n"
                               "Il tuo ID non è autorizzato a utilizzare questo bot.\n"
                               "Contatta l'amministratore per richiedere l'accesso.",
                               "Markdown");
          continue;
        }

        if (bot.messages[i].type == "callback_query") {
        String callbackData = bot.messages[i].text;
        if (callbackData.startsWith("fan_")) {
          int speed = callbackData.substring(4).toInt();
          setFanSpeed(speed);
          bot.sendMessage(cid, "✅ Velocità ventola impostata a " + String(speed), "");
        }
      } else if (txt == "/fan") {
        bot.sendMessageWithInlineKeyboard(cid,
          "🌀 Seleziona la velocità della ventola:",
          "", keyboardJson);
      }else if (txt == "/start") {
          String msg = "👋 *Benvenuto " + uname + "!*\n"
                       "Questo bot controlla un deumidificatore DIY basato su ESP32 con sensori AHT25, relè e ventola.\n\n"
                       "💡 Usa /help per vedere tutti i comandi disponibili.";
          bot.sendMessage(cid, msg, "Markdown");
        } else if (txt == "/help") {
          String msg = "📋 *Comandi disponibili:*\n"
                       "/sensors – Mostra valori dei sensori\n"
                       "/turnon – Accende il deumidificatore\n"
                       "/turnoff – Spegne il deumidificatore\n"
                       "/fan – Imposta la velocità della ventola\n"
                       "/status – Stato generale (sensori, stato, ventola)\n"
                       "/help – Mostra questo messaggio\n"
                       "/start – Messaggio di benvenuto";
          bot.sendMessage(cid, msg, "Markdown");
        } else if (txt == "/sensors") {
          String msg = String("🌡️ *Sensori:*\n") +
                       "Sensore lato freddo ❄️: " + String(temp1,1) + "°C, " + String(hum1,1) + "%\n" +
                       "Sensore lato caldo 🔥: " + String(temp2,1) + "°C, " + String(hum2,1) + "%";
          bot.sendMessage(cid, msg, "Markdown");
        } else if (txt == "/turnon") {
          digitalWrite(RELAY_PIN, HIGH); relayState = true;
          bot.sendMessage(cid, "🔌 Deumidificatore acceso", "");
        } else if (txt == "/turnoff") {
          digitalWrite(RELAY_PIN, LOW); relayState = false;
          bot.sendMessage(cid, "🔌 Deumidificatore spento", "");
        } else if (txt.startsWith("/fan_speed")) {
          int sp = txt.substring(11).toInt(); setFanSpeed(sp);
          bot.sendMessage(cid, "🌀 Velocità ventola impostata a " + String(fanSpeed), "");
        } else if (txt == "/status") {
          String msg = String("📊 *Stato sistema:*\n") +
                       "Sensore lato freddo ❄️: " + String(temp1,1) + "°C, " + String(hum1,1) + "%\n" +
                       "Sensore lato caldo 🔥: " + String(temp2,1) + "°C, " + String(hum2,1) + "%\n" +
                       "Stato Deumificatore: " + (relayState ? "On" : "Off") + "\n" +
                       "Ventola: " + String(relayState ? String(fanSpeed) : "Off");
          bot.sendMessage(cid, msg, "Markdown");
        } else {
          bot.sendMessage(cid, "❓ Comando non riconosciuto. Usa /help per la lista dei comandi.");
        }
      }
      lastRun = millis();
      n = bot.getUpdates(bot.last_message_received + 1);
    }
  }
}
