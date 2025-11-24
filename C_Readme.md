## Como Compilar e Rodar

Primeiramente, defina o IP do Servidor no arquivo `client_gui.c`, na seção `#define SERVER_IP "192.168.99.23"`

A biblioteca `ncurses` geralmente já vem instalada. Caso precise dos headers (raro não ter em ambientes educacionais, mas possível), instale `libncurses5-dev` ou `ncurses-devel`.

1. **Compilar:**
   Você precisa "ligar" a biblioteca com a flag `-lncurses`.

   ```bash
   gcc client_gui.c -o client_gui -lncurses
   ```
2. **Rodar:**
   Certifique-se de que o servidor (`server_rust` criado na resposta anterior) está rodando em um terminal.

   Terminal 1 (Servidor):

   ```bash
   ./server_rust 51482
   ```

   Terminal 2 (Cliente):

   ```bash
   ./client_gui
   ```
