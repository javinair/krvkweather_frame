import re
import argparse
import matplotlib.pyplot as plt

# Configurar el analizador de argumentos
parser = argparse.ArgumentParser(description="Procesar y graficar datos de un archivo de log.")
parser.add_argument("file_path", type=str, help="Ruta del archivo de entrada (log o CSV)")
args = parser.parse_args()

# Archivo de entrada
file_path = args.file_path

# Listas para almacenar los datos extraídos
timestamps = []
vbus = []
vshunt = []
current = []
power = []

# Expresión regular para capturar las líneas válidas
pattern = re.compile(r'\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\] (-?\d+\.\d+);(-?\d+\.\d+);(-?\d+\.\d+);(-?\d+\.\d+)')

# Leer el archivo y extraer los datos
with open(file_path, 'r') as file:
    for line in file:
        match = pattern.search(line)
        if match:
            # Extraer los valores
            timestamps.append(line.split(']')[0][1:])  # Extraer la fecha y hora
            vbus.append(float(match.group(1)))
            vshunt.append(float(match.group(2)))
            current.append(float(match.group(3)))
            power.append(float(match.group(4)))

# Crear la figura y el primer eje
fig, ax1 = plt.subplots(figsize=(10, 6))

# Primer eje (VBus)
ax1.set_xlabel("Tiempo")
ax1.set_ylabel("VBus (V)", color='tab:blue')
ax1.plot(range(len(vbus)), vbus, color='tab:blue', label='VBus', linewidth=2)
ax1.tick_params(axis='y', labelcolor='tab:blue')
ax1.set_ylim(2.0, 6.0)  # Ajuste manual del eje VBus

# Segundo eje (VShunt)
ax2 = ax1.twinx()
ax2.set_ylabel("VShunt (mV)", color='tab:orange')
ax2.plot(range(len(vshunt)), vshunt, color='tab:orange', label='VShunt', linewidth=1)
ax2.tick_params(axis='y', labelcolor='tab:orange')
ax2.set_ylim(-30, 100)  # Ajuste manual del eje VShunt

# Tercer eje (Corriente)
ax3 = ax1.twinx()
ax3.spines['right'].set_position(('outward', 60))  
ax3.set_ylabel("Current (mA)", color='tab:green')
ax3.plot(range(len(current)), current, color='tab:green', label='Current', linewidth=1.5)
ax3.tick_params(axis='y', labelcolor='tab:green')
ax3.set_ylim(-300, 300)  # Ajuste manual del eje Corriente

# Cuarto eje (Potencia)
ax4 = ax1.twinx()
ax4.spines['right'].set_position(('outward', 120))  
ax4.set_ylabel("Power (mW)", color='tab:red')
ax4.plot(range(len(power)), power, color='tab:red', label='Power', linewidth=1.5)
ax4.tick_params(axis='y', labelcolor='tab:red')
ax4.set_ylim(0, 3000)  # Ajuste manual del eje Potencia

# Mostrar el gráfico
fig.tight_layout()
plt.title("VBus, VShunt, Current, and Power")
plt.show()
