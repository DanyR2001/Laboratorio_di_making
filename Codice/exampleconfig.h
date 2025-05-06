// File: config.h
#ifndef CONFIG_H
#define CONFIG_H

#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

// WiFi credentials
const char* ssid = "yourSIDD";
const char* password = "yourPassword";

// Telegram bot token
const String botToken = "yourBotToken";

// Lista utenti Telegram autorizzati
#define NUM_AUTHORIZED_USERS 1
const long AUTHORIZED_USERS[NUM_AUTHORIZED_USERS] = {
    000000000  // <- sostituisci con il tuo chat_id
  };

#endif  // CONFIG_H