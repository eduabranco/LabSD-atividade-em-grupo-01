### Como Executar

**Primeiramente, entramos na pasta do programa em Rust, como no abaixo.**

```shell
cd trabalho_sockets_rust
```

**Como configuramos como múltiplos binários, você usa a flag** **--bin** **para dizer ao Cargo qual arquivo quer rodar. Abra terminais diferentes para servidor e cliente.**

#### Para testar o servidor Multithread:

**code**Bash

```
cargo run --bin server_threaded
```

#### Para testar o servidor Async (Epoll - Alta performance):

**code**Bash

```
cargo run --bin server_async
```

#### Para rodar o Trabalho Final (Compilador Remoto):

**Terminal 1 (Servidor):**

**code**Bash

```
cargo run --bin compiler_server
```

**Terminal 2 (Cliente Gráfico):**

**code**Bash

```
cargo run --bin client_gui
```

---

### 5. Pré-requisitos do Sistema (Linux)

**Como você está usando Linux e o cliente tem interface gráfica (**eframe**), você pode precisar instalar algumas bibliotecas de sistema antes de compilar pela primeira vez. No Ubuntu/Debian/Mint, rode:**

**code**Bash

```
sudo apt-get install libxcb-render0-dev libxcb-shape0-dev libxcb-xfixes0-dev libxkbcommon-dev libssl-dev libgtk-3-dev
```

**Isso garante que o Rust consiga desenhar a janela do cliente na sua tela.**
