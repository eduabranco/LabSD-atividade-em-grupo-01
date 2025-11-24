
# Projeto de Sockets em Rust

Este projeto demonstra diferentes implementações de servidores TCP em Rust, juntamente com um cliente com interface gráfica (GUI) para interagir com um servidor de compilação remota.

## Estrutura do Projeto

O projeto está organizado em múltiplos binários dentro de um único crate do Cargo. Cada arquivo em `src/bin/` corresponde a um executável que pode ser compilado e executado separadamente.

- **`compiler_server.rs`**: Um servidor assíncrono que recebe código C, compila, executa e retorna o resultado.
- **`client_gui.rs`**: Um cliente com interface gráfica para enviar código C ao `compiler_server`.
- **`server_threaded.rs`**: Um servidor TCP multi-thread básico (modelo "uma thread por cliente").
- **`server_async.rs`**: Um servidor TCP assíncrono básico usando `tokio` (modelo "event-driven").

## Dependências (`Cargo.toml`)

O projeto utiliza as seguintes bibliotecas (crates):

- `tokio`: Framework para escrita de aplicações de rede assíncronas. Usado em `compiler_server.rs` e `server_async.rs`.
- `uuid`: Para gerar identificadores únicos, garantindo que os arquivos de código de diferentes clientes não colidam no servidor.
- `eframe` e `egui`: Para construir a interface gráfica do cliente de forma rápida e simples.

## 1. Servidor de Compilação (`compiler_server.rs`)

Este é o componente principal do projeto. É um servidor TCP assíncrono que funciona como um serviço de compilação remota para código C.

**Funcionalidades:**

1.  **Escuta de Conexões**: O servidor utiliza `tokio` para escutar de forma assíncrona na porta `51482`.
2.  **Tratamento de Clientes**: Para cada nova conexão, uma nova tarefa (`task`) assíncrona é criada com `tokio::spawn`. Isso permite que o servidor trate múltiplos clientes concorrentemente sem bloquear o loop principal.
3.  **Recepção do Código**: Ele lê o código-fonte C enviado pelo cliente através do socket.
4.  **Criação de Arquivos Temporários**: Para evitar conflitos entre execuções simultâneas de diferentes clientes, o servidor gera um `UUID` para criar nomes de arquivo únicos para o código-fonte (`.c`) e para o executável compilado. Os arquivos são salvos no diretório `/tmp/`.
5.  **Compilação**: O servidor invoca o compilador `gcc` como um processo externo para compilar o arquivo `.c` recebido.
6.  **Execução e Resposta**:
    -   **Se a compilação for bem-sucedida**: O servidor executa o binário gerado e captura sua saída (tanto `stdout` quanto `stderr`). A resposta enviada ao cliente contém uma mensagem de sucesso e o resultado da execução.
    -   **Se a compilação falhar**: O servidor captura o erro de compilação (`stderr` do `gcc`) e o envia como resposta ao cliente.
7.  **Limpeza**: Ao final do processo, os arquivos temporários (`.c` e o executável) são removidos para não poluir o sistema de arquivos.

**Como executar:**
```bash
cargo run --release --bin compiler_server
```

## 2. Cliente GUI (`client_gui.rs`)

Este é um cliente com interface gráfica que permite a um usuário escrever código C e enviá-lo para o `compiler_server` para execução.

**Funcionalidades:**

1.  **Interface Gráfica**: Construída com `eframe` e `egui`, a interface é simples e funcional.
2.  **Editor de Código**: Uma área de texto permite que o usuário escreva ou cole código C. A área vem pré-preenchida com um exemplo "Olá Mundo".
3.  **Configuração do Servidor**: Um campo de texto permite configurar o endereço IP e a porta do servidor de compilação. O valor padrão é `127.0.0.1:51482`.
4.  **Comunicação via Socket**:
    -   Ao clicar em "EXECUTAR", o cliente estabelece uma conexão TCP **bloqueante** (`std::net::TcpStream`) com o servidor.
    -   O código do editor é enviado para o servidor.
    -   `stream.shutdown(Shutdown::Write)` é chamado para sinalizar ao servidor que o envio de dados terminou (equivalente a um `EOF`).
    -   O cliente então aguarda e lê a resposta do servidor.
5.  **Exibição da Saída**: A resposta recebida do servidor (seja o resultado da execução ou um erro de compilação) é exibida em uma área de texto "somente leitura".

**Como executar:**
```bash
cargo run --release --bin client_gui
```

## 3. Servidores de Exemplo

O projeto inclui dois servidores mais simples que servem como exemplos de diferentes modelos de concorrência em Rust.

### `server_threaded.rs`

-   **Modelo**: Multi-Threaded (uma thread por cliente).
-   **Implementação**: Usa apenas a biblioteca padrão do Rust (`std`).
-   **Funcionamento**: Para cada conexão aceita, ele cria uma nova **thread de sistema operacional** (`std::thread::spawn`) para tratar a comunicação com aquele cliente.
-   **Comportamento**: Lê uma mensagem do cliente, envia uma resposta fixa (`"I got your message"`) e fecha a conexão.
-   **Prós e Contras**: Simples de implementar, mas pode consumir muitos recursos (memória e tempo de chaveamento de contexto) sob alta carga, pois cada cliente consome uma thread do SO.

**Como executar:**
```bash
cargo run --release --bin server_threaded
```

### `server_async.rs`

-   **Modelo**: Assíncrono / Orientado a Eventos (usando `epoll` ou similar, abstraído pelo `tokio`).
-   **Implementação**: Usa o runtime `tokio`.
-   **Funcionamento**: Utiliza um loop de eventos para gerenciar múltiplas conexões em uma única thread (ou um pequeno pool de threads). `tokio::spawn` cria uma "green thread" (tarefa leve), que é muito mais barata que uma thread de SO.
-   **Comportamento**: Similar ao `server_threaded`, ele aceita uma conexão, lê uma mensagem, envia uma resposta e fecha a conexão, mas faz isso de forma não-bloqueante.
-   **Prós e Contras**: Altamente escalável e eficiente para um grande número de conexões simultâneas, especialmente em tarefas de I/O (rede, disco). A complexidade do código é um pouco maior devido ao uso de `async/await`.

**Como executar:**
```bash
cargo run --release --bin server_async
```
