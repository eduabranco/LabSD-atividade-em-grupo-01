## Como Compilar e Rodar

1. **Instale os pré-requisitos:**

   * Compilador Rust: `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustups | sh`
   * Bibliotecas GTK3: `sudo apt-get install libgtk-3-dev`
2. **Compile o Servidor:**

   ```bash
   gcc server_rust.c -o server_rust
   ```
3. **Compile o Cliente Gráfico:**

   ```bash
   gcc client_gui.c -o client_gui `pkg-config --cflags --libs gtk+-3.0`
   ```
4. **Execute:**

   * Terminal 1: `./server_rust 51482`
   * Terminal 2: `./client_gui` (Digite o código e clique no botão).
