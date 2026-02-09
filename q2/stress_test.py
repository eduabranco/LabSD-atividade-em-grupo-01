import socket
import threading
import time

HOST = input("Digite o endereco IP do servidor: ")
PORT = int(input("Digite a porta do servidor: "))
NUM_REQUESTS = int(input("Digite a quantidade de conexões para tentar: "))  # Quantidade de conexões para tentar

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