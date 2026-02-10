/* server_fork.c */
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>

void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Handler para limpar processos filhos finalizados (evitar zumbis)
void sigchld_handler(int s) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    struct sigaction sa;

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Opção para reutilizar a porta imediatamente após fechar o servidor
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 100); // Aumentei o backlog

    // Configura limpeza de processos mortos
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    #ifdef SA_RESTART
        sa.sa_flags = SA_RESTART;
    #else
        sa.sa_flags = 0;
    #endif
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
        error("sigaction");

    clilen = sizeof(cli_addr);

    // LOOP INFINITO
    while(1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) error("ERROR on accept");

        int pid = fork(); // Cria novo processo
        
        if (pid < 0) {
            error("ERROR on fork");
        }
        
        if (pid == 0) { 
            // PROCESSO FILHO (trata o cliente)
            close(sockfd); // Filho não precisa do socket de escuta
            
            memset(buffer, 0, 256);
            n = read(newsockfd, buffer, 255);
            if (n < 0) error("ERROR reading from socket");
            
            // printf("Filho %d recebeu: %s\n", getpid(), buffer); // Debug opcional
            
            n = write(newsockfd, "I got your message", 18);
            if (n < 0) error("ERROR writing to socket");
            
            close(newsockfd);
            exit(0); // Filho termina aqui
        } else {
            // PROCESSO PAI (volta a escutar)
            close(newsockfd); // Pai não precisa do socket do cliente
        }
    }
    return 0; 
}