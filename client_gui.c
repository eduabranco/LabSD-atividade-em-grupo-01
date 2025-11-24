/* client_ncurses.c */
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 51482
#define MAX_CODE_SIZE 4096

// Função para enviar o código ao servidor e receber a resposta
void send_to_server(const char *code, char *response) {
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        strcpy(response, "Erro: Falha ao abrir socket.");
        return;
    }

    server = gethostbyname(SERVER_IP);
    if (server == NULL) {
        strcpy(response, "Erro: Host não encontrado.");
        close(sockfd);
        return;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(SERVER_PORT);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        strcpy(response, "Erro: Não foi possível conectar ao servidor.\nVerifique se ele está rodando.");
        close(sockfd);
        return;
    }

    // Enviar código
    n = write(sockfd, code, strlen(code));
    if (n < 0) {
        strcpy(response, "Erro ao enviar dados.");
        close(sockfd);
        return;
    }

    // Receber resposta
    bzero(response, MAX_CODE_SIZE);
    // Loop simples para garantir leitura (embora o servidor mande tudo de uma vez no exemplo)
    n = read(sockfd, response, MAX_CODE_SIZE - 1);
    
    if (n < 0) strcpy(response, "Erro ao ler resposta.");
    
    close(sockfd);
}

int main() {
    int ch;
    char code_buffer[MAX_CODE_SIZE] = "fn main() {\n    println!(\"Ola do Linux via NCurses!\");\n}";
    char output_buffer[MAX_CODE_SIZE] = "Pressione F2 para Compilar e Rodar. F10 para Sair.";
    int cursor_pos = strlen(code_buffer);

    // Inicialização do NCurses
    initscr();            // Inicia o modo curses
    cbreak();             // Desabilita buffer de linha (pega caractere a caractere)
    noecho();             // Não imprime automaticamente o que digita
    keypad(stdscr, TRUE); // Habilita teclas especiais (F1, F2, setas)
    start_color();        // Habilita cores

    // Definição de pares de cores
    init_pair(1, COLOR_WHITE, COLOR_BLUE);  // Janela de Código
    init_pair(2, COLOR_GREEN, COLOR_BLACK); // Janela de Saída
    init_pair(3, COLOR_BLACK, COLOR_CYAN);  // Rodapé/Menu

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Loop da Interface
    while (1) {
        clear();

        // 1. Desenhar Cabeçalho e Rodapé
        attron(COLOR_PAIR(3));
        mvprintw(0, 0, " RUST COMPILER CLIENT (Linux Socket) ");
        mvprintw(max_y - 1, 0, " F2: Executar | Backspace: Apagar | F10: Sair ");
        for(int i=strlen(" RUST COMPILER CLIENT (Linux Socket) "); i<max_x; i++) mvaddch(0, i, ' ');
        for(int i=strlen(" F2: Executar | Backspace: Apagar | F10: Sair "); i<max_x; i++) mvaddch(max_y - 1, i, ' ');
        attroff(COLOR_PAIR(3));

        // 2. Janela de Edição (Cima)
        attron(COLOR_PAIR(1));
        mvprintw(1, 0, "--- EDITOR DE CODIGO ---");
        // Preencher fundo
        for(int y=2; y < max_y/2; y++) {
            for(int x=0; x < max_x; x++) mvaddch(y, x, ' ');
        }
        mvprintw(2, 1, "%s", code_buffer);
        attroff(COLOR_PAIR(1));

        // 3. Janela de Saída (Baixo)
        attron(COLOR_PAIR(2));
        mvprintw(max_y/2, 0, "--- SAIDA DO COMPILADOR ---");
        mvprintw((max_y/2) + 1, 0, "%s", output_buffer);
        attroff(COLOR_PAIR(2));

        refresh();

        // Captura de entrada
        ch = getch();

        if (ch == KEY_F(10)) {
            break; // Sair
        }
        else if (ch == KEY_F(2)) {
            strcpy(output_buffer, "Enviando para o servidor... aguarde...");
            // Força refresh para mostrar mensagem de carregamento
            attron(COLOR_PAIR(2));
            mvprintw((max_y/2) + 1, 0, "%s", output_buffer);
            attroff(COLOR_PAIR(2));
            refresh();

            // Comunicação com o servidor
            send_to_server(code_buffer, output_buffer);
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (cursor_pos > 0) {
                code_buffer[--cursor_pos] = '\0';
            }
        }
        else if (ch == '\n') {
            if (cursor_pos < MAX_CODE_SIZE - 2) {
                code_buffer[cursor_pos++] = '\n';
                code_buffer[cursor_pos] = '\0';
            }
        }
        else if (ch >= 32 && ch <= 126) { // Caracteres imprimíveis
            if (cursor_pos < MAX_CODE_SIZE - 2) {
                code_buffer[cursor_pos++] = ch;
                code_buffer[cursor_pos] = '\0';
            }
        }
    }

    endwin(); // Encerra o modo curses
    return 0;
}