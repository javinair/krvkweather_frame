import socket
import os
from datetime import datetime

UDP_IP = "0.0.0.0"
UDP_PORT = 12345

def get_incremental_filename(base_name="udp_capture", extension=".log"):
    counter = 1
    while True:
        filename = f"{base_name}_{counter}{extension}"
        if not os.path.exists(filename):
            return filename
        counter += 1

def capture_udp_packets(host=UDP_IP, port=UDP_PORT, buffer_size=1024, timeout=1):
    filename = get_incremental_filename()
    print(f"Writing packages in: {filename}\n")
    
    with open(filename, "w", encoding="utf-8") as file:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.bind((host, port))
            sock.settimeout(timeout)  # 🔹 Añadir timeout
            print(f"Listening on {host}:{port}...\n")
            
            try:
                while True:
                    try:
                        data, addr = sock.recvfrom(buffer_size)
                        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                        log_entry = f"[{timestamp}] {data.decode()}\n"
                        print(log_entry, end="")
                        file.write(log_entry)
                        file.flush()
                    except socket.timeout:
                        pass  # 🔹 Permite salir del bucle cuando no hay datos
            except KeyboardInterrupt:
                print("\nStopped by user.")
            finally:
                sock.close()

if __name__ == "__main__":
    capture_udp_packets()
