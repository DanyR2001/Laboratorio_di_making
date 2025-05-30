# 🌀 Deumidificatore Smart DIY

Un deumidificatore intelligente basato su cella Peltier, controllabile via WiFi e Telegram, progettato per essere economico, modulare e open-source.

![Design 3D](https://via.placeholder.com/400x300?text=Case+Design+V1) ![Assembly](https://via.placeholder.com/400x300?text=Componenti+Assemblati) ![Internals](https://via.placeholder.com/400x300?text=Elettronica+Interna)

## 🚀 Panoramica
Progetto nato per risolvere problemi di umidità elevata (>80%) in ambienti domestici, offrendo un'alternativa economica e smart ai deumidificatori commerciali. Combina una cella Peltier con sensori di temperatura/umidità e controllo remoto in un case stampabile in 3D.

## 🔍 Caratteristiche Principali
- **Economico**: Costo totale ~50€ (vs 150-400€ modelli smart)
- **Smart Control**: Interfaccia web locale + Bot Telegram
- **Modulare**: Case stampabile in 3D con design modulare
- **Sostenibile**: Riutilizzo componenti da hardware dismesso
- **Monitoraggio**: Sensori temperatura/umidità in tempo reale
- **Silenzioso**: 37-52 dBA (a seconda della modalità)

## 📦 Componenti Richiesti
| Componente              | Quantità | Prezzo Amazon | Prezzo AliExpress |
|-------------------------|----------|---------------|-------------------|
| ESP32                   | 1        | €6,50         | €2,59             |
| Sensore AHT25           | 2        | €7,99         | €3,64             |
| Modulo Relè             | 1        | €1,52         | €2,89             |
| Level Shifter 3.3V-5V   | 1        | €1,40         | €0,11             |
| Multiplexer I2C         | 1        | €1,60         | €0,42             |
| Alimentatore 12V 5A     | 1        | €14,98        | €7,39             |
| DC-DC Step-down         | 1        | €2,80         | €1,27             |
| Cella Peltier TEC1-12706| 1        | €4,50         | €2,06             |
| **Totale**              |          | **€41,29**    | **€20,37**        |

### 🛠️ Hardware Aggiuntivo
- Scheda Madre AM2 (ASROCK N68-S) per dissipatori: ~€25 su eBay
- Filamento PLA per stampa 3D

## ⚙️ Specifiche Tecniche
| Caratteristica             | Valore                |
|----------------------------|-----------------------|
| Capacità deumidificazione  | 0,3-0,4 L/giorno     |
| Potenza refrigerante       | ~10W                 |
| Consumo energetico         | 24-30W (2,0-2,5A @12V)|
| Temperatura lato freddo    | 8-18°C               |
| Rumorosità                 | 37-52 dBA            |
| Controllo umidità          | 55-65% RH            |
| Connettività               | WiFi + Telegram Bot  |

## 📂 Struttura del Progetto
Laboratorio_di_making/
├── STL_Files/           # File 3D per la stampa
│   ├── bicchiere_superiore_v3.stl
│   ├── bicchiere_intermedio_v2.stl
│   └── bicchiere_inferiore_v2.stl
├── Arduino_Code/        # Codice sorgente
│   ├── main.ino
│   ├── config.h
│   └── libraries/
├── Schematics/          # Schemi elettrici
├── Documentation/       # Documentazione aggiuntiva
└── README.md
## ⚡ Setup e Installazione
### 1. Preparazione Hardware
- Stampa 3D: Stampa tutti i componenti del case usando PLA
- Assemblaggio: Monta i dissipatori dalla scheda madre AM2
- Cablaggio: Segui lo schema elettrico fornito
- Montaggio: Assembla tutti i componenti nel case stampato

### 2. Setup Software
1. Installa Arduino IDE e le librerie necessarie
2. Configura ESP32:
   ```bash
   # Aggiungi l'URL per i board ESP32
   https://dl.espressif.com/dl/package_esp32_index.json
3. Carica il firmware sull'ESP32

### 3. Configurazione Telegram BotCerca @BotFather su Telegram
1. Crea un nuovo bot con /newbot
2. Ottieni token e chat ID
3. Inserisci i dati in config.h

### 4. Accesso Web Interface 
- URL locale: http://esp32.local/
- API sensori: http://esp32.local/localSensors

## 📊 Prestazioni e Test
### Test di Collaudo
- Riduzione umidità: ~10% in 10 ore
- Rimozione vapore: ~150ml/50m³
- Temperatura operativa: Lato freddo 8-18°C, lato caldo ~50°C

### 🔈 Profilo Acustico

| Modalità    | Livello Sonoro  |
|-------------|-----------------|
| Sleep       | 37,7 dBA        |
| Min         | 40 dBA          |
| Med         | 43 dBA          |
| Max         | 52 dBA          |	
	
### ⚡ Efficienza Energetica
- Punto ottimale: 2,0-2,5A @ 12V
- COP: 0,7-0,8 nella zona ottimale
- Potenza frigorifera: 7-12W

## 🖥️ Utilizzo
### Web Interface
- Controllo ON/OFF del deumidificatore
- Monitoraggio temperature lato caldo/freddo
- Regolazione velocità ventola
- Visualizzazione dati sensori

### 🤖 Telegram Bot
- Stessi controlli dell'interfaccia web
- Controllo accesso tramite ID Telegram

## 🛠️ Personalizzazione
### Hardware
- Dissipatori maggiori per prestazioni superiori
- Doppia cella Peltier (configurazione cascata/parallela)
- Sistema di drenaggio per uso continuo

## Software
- Logging su SD per storico dati
- Integrazione cloud (dashboard online)
- App mobile dedicata
- Algoritmi ML per ottimizzazione automatica

## 🚀 Sviluppi Futuri
- Dashboard avanzata con grafici storici
- Integrazione domotica (Home Assistant, Alexa)
- AI optimization per cicli automatici
- App mobile nativa
- Cloud storage per i dati
- Notifiche push personalizzate

## 🤝 Contribuire
Contributi benvenuti! Processo suggerito:
1. Fork del repository
2. Crea un branch per la tua feature (git checkout -b feature/AmazingFeature)
3. Commit delle modifiche (git commit -m 'Add some AmazingFeature')
4. Push sul branch (git push origin feature/AmazingFeature)
5. Apri una Pull Request

### Aree di contributo
- Bug fix e ottimizzazioni
- Nuove funzionalità
- Miglioramenti documentazione
- Modifiche design case
- Test in condizioni diverse

## 📜 Licenza
Distribuito con licenza MIT. Vedi LICENSE per dettagli.

## ❓ Supporto
- Issues: Usa la sezione Issues di GitHub
- Discussioni: Partecipa alle Discussions
- Contatto diretto: [danielerusso_@hotmail.it]

## Disclaimer
Questo progetto è fornito "as-is" per scopi educativi e di prototipazione. Utilizzare con cautela rispettando le norme di sicurezza elettrica.

# ⭐ Se questo progetto ti è stato utile, lascia una stella su GitHub!


