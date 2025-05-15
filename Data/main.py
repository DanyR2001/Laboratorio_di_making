import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.dates import DateFormatter, AutoDateLocator

# Percorso del file CSV (modifica se necessario)
file_path = 'rawData.csv'

# Lettura del CSV saltando la riga di header contenente i nomi delle colonne
cols = ['timestamp', 'temp_cold', 'hum_cold', 'temp_hot', 'hum_hot', 'temp_amb', 'hum_amb']
df = pd.read_csv(file_path, skiprows=1, header=None, names=cols)

# Conversione del timestamp epoch in datetime leggibile
df['timestamp'] = pd.to_datetime(df['timestamp'], unit='s')

# Ordinamento crescente per timestamp
df.sort_values('timestamp', inplace=True)

# Impostiamo il timestamp come indice
df.set_index('timestamp', inplace=True)

# Creiamo il grafico
fig, ax = plt.subplots(figsize=(10, 6))

# Linee meno importanti (lato freddo e lato caldo)
ax.plot(df.index, df['hum_cold'], linewidth=1, linestyle='--', color='gray', label='Umidità Cold Side')
ax.plot(df.index, df['hum_hot'], linewidth=1, linestyle='-.', color='lightgray', label='Umidità Hot Side')

# Linea principale (umidità ambientale)
ax.plot(df.index, df['hum_amb'], linewidth=2, color='blue', label='Umidità Ambiente')

# Formattazione asse X: mostra solo ore:minuti e riduce i tick
locator = AutoDateLocator()
formatter = DateFormatter('%H:%M')
ax.xaxis.set_major_locator(locator)
ax.xaxis.set_major_formatter(formatter)
fig.autofmt_xdate(rotation=45, ha='right')

# Altre formattazioni
ax.set_xlabel('Ora (HH:MM)')
ax.set_ylabel('Umidità (%)')
ax.set_title('Andamento Umidità nel Tempo')
ax.legend()
ax.grid(alpha=0.3)

plt.tight_layout()
plt.show()
