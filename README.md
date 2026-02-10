## Como Rodar a Q1 (Comparação de Arquiteturas de Servidor)

### Compilação Manual
Caso queira compilar cada servidor individualmente sem usar o `make`:

**Server Fork:**
```bash
gcc q1/server_fork.c -o q1/server_fork
```

**Server Thread:**
```bash
gcc q1/server_thread.c -o q1/server_thread -pthread
```

**Server Epoll:**
```bash
gcc q1/server_epoll.c -o q1/server_epoll
```

### Execução

**Servidor com Fork (Processos)**

```bash
./q1/server_fork 8080
```

**Servidor com Threads**

```bash
./q1/server_thread 8080
```

**Servidor com Epoll (Event-Loop)**

```bash
./q1/server_epoll 8080
```

---



## Como Rodar a Q2 (Teste de Estresse)

O script de teste de carga está em `q2/stress_test.py`. Ele conecta simultaneamente ao servidor para testar sua capacidade.

1. Inicie um dos servidores da Q1 ou Q3 em um terminal.
2. Em outro terminal, execute o script:

```bash
python3 q2/stress_test.py
```

3. Siga as instruções interativas para inserir IP, Porta e Número de conexões.Como Compilar e Rodar a Q3
4. **Instale os pré-requisitos:**

   * Compilador Rust: `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`
   * Bibliotecas GTK3: `sudo apt-get install libgtk-3-dev`
5. **Compile o Servidor:**

   ```bash
   gcc server_rust.c -o server_rust
   ```
   3.**Compile o Cliente Gráfico:**

   ```bash
   gcc client_gui.c -o client_gui `pkg-config--cflags--libs gtk+-3.0`
   ```
   4.**Execute:**

   * Terminal 1: `./server_rust 51482`
   * Terminal 2: `./client_gui` (Digite o código e clique no botão).
