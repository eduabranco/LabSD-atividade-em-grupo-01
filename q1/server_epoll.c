#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>      // Para manipulação de arquivos (non-blocking)
#include <sys/epoll.h>  // Para epoll
#include <errno.h>

#define MAX_EVENTS 10000 // Quantos eventos processar por vez
#define BUFFER_SIZE 256

void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Função auxiliar para colocar um socket em modo Não-Bloqueante
// CRUCIAL para arquiteturas baseadas em eventos.
int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    char buffer[BUFFER_SIZE];
    
    // Variáveis do Epoll
    int epoll_fd;
    struct epoll_event event, events[MAX_EVENTS];

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    // 1. Configuração padrão do Socket (igual ao original)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, SOMAXCONN); // SOMAXCONN permite o máximo que o SO suportar

    // 2. Configuração do EPOLL

    // Cria a instância do epoll
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) error("ERROR epoll_create1");

    // Configura o evento para o socket do servidor (o que escuta conexões)
    event.events = EPOLLIN; // Queremos ser avisados quando houver dados (nova conexão)
    event.data.fd = sockfd; // O file descriptor que queremos monitorar

    // Adiciona o socket do servidor à lista de monitoramento do epoll
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &event) == -1)
        error("ERROR epoll_ctl: server socket");

    printf("Servidor Epoll rodando na porta %d...\n", portno);

    // 3. O Loop de Eventos (Event Loop)

    while (1) {
        // epoll_wait bloqueia o processo até que ALGO aconteça em QUALQUER socket monitorado
        // n_ready é o número de sockets que têm atividade agora
        int n_ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        if (n_ready == -1) error("ERROR epoll_wait");

        // Iteramos apenas sobre os sockets que tiveram atividade
        for (int i = 0; i < n_ready; i++) {
            
            // CASO 1: Atividade no Socket Principal = Nova Conexão chegando
            if (events[i].data.fd == sockfd) {
                clilen = sizeof(cli_addr);
                newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                if (newsockfd == -1) {
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                        // Processamos todas as conexões pendentes
                        break;
                    } else {
                        perror("ERROR accept");
                        break;
                    }
                }

                // Torna o novo socket do cliente não-bloqueante
                set_nonblocking(newsockfd);

                // Adiciona o novo cliente à lista de monitoramento do epoll
                event.events = EPOLLIN | EPOLLET; // EPOLLET = Edge Triggered (alta performance)
                event.data.fd = newsockfd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newsockfd, &event) == -1) {
                    perror("ERROR epoll_ctl: client socket");
                    close(newsockfd);
                }
                // printf("Novo cliente conectado: FD %d\n", newsockfd);
            } 
            
            // CASO 2: Atividade em um Socket de Cliente = Dados recebidos
            else {
                int client_fd = events[i].data.fd;
                memset(buffer, 0, BUFFER_SIZE);
                
                // Tenta ler dados
                ssize_t count = read(client_fd, buffer, sizeof(buffer));

                if (count == -1) {
                    // Erro na leitura (ou nada para ler)
                    if (errno != EAGAIN) {
                        perror("read");
                        close(client_fd); // Fecha a conexão
                    }
                } else if (count == 0) {
                    // Fim de arquivo (O cliente fechou a conexão) -> EOF
                    // O epoll remove automaticamente sockets fechados, mas é bom ser explícito
                    close(client_fd);
                    // printf("Cliente desconectado: FD %d\n", client_fd);
                } else {
                    // Temos dados! Processar aqui.
                    // printf("Recebido do FD %d: %s\n", client_fd, buffer);
                    
                    // Responder ao cliente
                    write(client_fd, "I got your message", 18);
                    
                    // IMPORTANTE: Em servidores reais de alta performance (como Nginx),
                    // você não fecharia aqui necessariamente (Keep-Alive). 
                    // Mas para seguir a lógica do seu exercício, fechamos após responder.
                    close(client_fd); 
                }
            }
        }
    }

    close(sockfd);
    return 0;
}