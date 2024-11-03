import socket
from datetime import datetime

UDP_IP = "0.0.0.0"
UDP_PORT = 12345

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Escuchando en {UDP_IP}:{UDP_PORT}...")

while True:
    data, _ = sock.recvfrom(1024)
    timestamp = datetime.now().strftime("%d/%m/%y %H:%M:%S")
    print(f"[{timestamp}] {data.decode()}")
