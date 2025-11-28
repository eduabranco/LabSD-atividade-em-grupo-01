import socket
import threading
import time

HOST = '127.0.0.1'
PORT = 51482
NUM_REQUESTS = 500  # Quantidade de conexões para tentar

def client_task(i):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(2) # Timeout curto
        s.connect((HOST, PORT))
        s.sendall(b"Teste de stress")
        data = s.recv(1024)
        s.close()
        # print(f"Cliente {i}: OK")
    except Exception as e:
        print(f"Cliente {i}: ERRO - {e}")

threads = []
print(f"Iniciando {NUM_REQUESTS} requisicoes...")
start = time.time()

for i in range(NUM_REQUESTS):
    t = threading.Thread(target=client_task, args=(i,))
    threads.append(t)
    t.start()

for t in threads:
    t.join()

end = time.time()
print(f"Tempo total: {end - start:.2f} segundos")