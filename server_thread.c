/* server_thread.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Função que será executada por cada thread
void *handle_client(void *arg) {
    int newsockfd = *((int *)arg);
    free(arg); // Libera a memória alocada para o ponteiro
    char buffer[256];
    int n;

    // "Detach" permite que a thread libere recursos automaticamente ao terminar
    pthread_detach(pthread_self());

    bzero(buffer, 256);
    n = read(newsockfd, buffer, 255);
    if (n < 0) {
        perror("ERROR reading from socket");
        close(newsockfd);
        return NULL;
    }

    n = write(newsockfd, "I got your message", 18);
    if (n < 0) perror("ERROR writing to socket");

    close(newsockfd);
    return NULL;
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");
    
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 500);
    clilen = sizeof(cli_addr);

    while(1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            continue;
        }

        // Alocar memória para o socket fd para passar para a thread de forma segura
        int *client_sock = malloc(sizeof(int));
        *client_sock = newsockfd;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)client_sock) < 0) {
            perror("could not create thread");
            free(client_sock);
            close(newsockfd);
        }
    }
    return 0;
}