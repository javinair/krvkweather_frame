import socket
from datetime import datetime

UDP_IP = "0.0.0.0"
UDP_PORT = 12345

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
sock.settimeout(1.0)  # Timeout de 1 segundo

print(f"Escuchando en {UDP_IP}:{UDP_PORT}...")

try:
    while True:
        try:
            data, _ = sock.recvfrom(1024)
            timestamp = datetime.now().strftime("%d/%m/%y %H:%M:%S")
            print(f"[{timestamp}] {data.decode()}")
        except socket.timeout:
            # Si no hay datos, simplemente pasa
            pass
except KeyboardInterrupt:
    print("\nApagando monitor UDP...")
finally:
    sock.close()
