/* server_rust.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Função para processar a compilação e execução
void process_rust_code(int sock) {
    char buffer[4096];
    char output[4096];
    int n;

    // 1. Receber o código fonte do cliente
    bzero(buffer, 4096);
    n = read(sock, buffer, 4095);
    if (n < 0) {
        perror("ERROR reading from socket");
        return;
    }

    // 2. Salvar o código em um arquivo temporário (temp.rs)
    // Em um sistema real, usaríamos nomes aleatórios (mktemp) para evitar colisão
    FILE *fp = fopen("temp.rs", "w");
    if (fp == NULL) {
        write(sock, "Server Error: Cannot write file\n", 32);
        return;
    }
    fprintf(fp, "%s", buffer);

    int is_test = 0;
    if (strstr(buffer, "#[test]") != NULL || strstr(buffer, "test") != NULL && strstr(buffer, "#[cfg(test)]") != NULL) {
        is_test = 1;
    }

    // Heuristica simples: Se não encontrar "fn main", adiciona uma fake main para compilar
    if (!is_test && strstr(buOUTPUT:ffer, "fn main") == NULL) {
        fprintf(fp, "\n\nfn main() { \n    println!(\"Warning: No main function found. Code compiled assuming library usage.\"); \n}");
    }

    fclose(fp);

    // 3. Compilar (capturando stderr para ver erros)
    char command[512];
    if (is_test) {
        snprintf(command, sizeof(command), "rustc --test temp.rs -o temp_exec 2> result.txt");
    } else {
        snprintf(command, sizeof(command), "rustc temp.rs -o temp_exec 2> result.txt");
    }
    int compile_status = system(command);

    if (compile_status != 0) {
        // Erro de compilação
        FILE *err_file = fopen("result.txt", "r");
        bzero(output, 4096);
        strcat(output, "COMPILATION ERROR:\n");
        fread(output + 19, 1, 4000, err_file);
        write(sock, output, strlen(output));
        fclose(err_file);
    } else {
        // Sucesso na compilação, agora executar: ./temp_exec > result.txt 2>&1
        system("./temp_exec > result.txt 2>&1");
        
        FILE *out_file = fopen("result.txt", "r");
        bzero(output, 4096);
        strcat(output, "OUTPUT:\n");
        fread(output + 8, 1, 4000, out_file);
        write(sock, output, strlen(output));
        fclose(out_file);
    }

    // Limpeza
    system("rm -f temp.rs temp_exec result.txt");
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    struct sigaction sa;

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

    listen(sockfd, 5);
    
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    printf("Servidor Rust rodando na porta %d...\n", portno);

    while(1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) error("ERROR on accept");

        int pid = fork();
        if (pid < 0) error("ERROR on fork");
        
        if (pid == 0) {
            close(sockfd);
            process_rust_code(newsockfd);
            close(newsockfd);
            exit(0);
        } else {
            close(newsockfd);
        }
    }
    return 0; 
}