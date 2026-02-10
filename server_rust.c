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
    (void)s; // Silence unused parameter warning
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

    // Gerar arquivos únicos usando PID para evitar condições de corrida
    int pid = getpid();
    char src_file[64];
    char exec_file[64];
    char out_file[64];
    
    snprintf(src_file, sizeof(src_file), "temp_%d.rs", pid);
    snprintf(exec_file, sizeof(exec_file), "temp_%d_exec", pid);
    snprintf(out_file, sizeof(out_file), "result_%d.txt", pid);

    // 1. Receber o código fonte do cliente
    memset(buffer, 0, 4096);
    n = read(sock, buffer, 4095);
    if (n < 0) {
        perror("ERROR reading from socket");
        return;
    }

    // 2. Salvar o código em um arquivo temporário
    FILE *fp = fopen(src_file, "w");
    if (fp == NULL) {
        write(sock, "Server Error: Cannot write file\n", 32);
        return;
    }
    fprintf(fp, "%s", buffer);

    int is_test = 0;
    if (strstr(buffer, "#[test]") != NULL || (strstr(buffer, "test") != NULL && strstr(buffer, "#[cfg(test)]") != NULL)) {
        is_test = 1;
    }

    // Heuristica simples: Se não encontrar "fn main", adiciona uma fake main para compilar
    if (!is_test && strstr(buffer, "fn main") == NULL) {
        fprintf(fp, "\n\nfn main() { \n    println!(\"Warning: No main function found. Code compiled assuming library usage.\"); \n}");
    }

    fclose(fp);

    // 3. Compilar (capturando stderr para ver erros)
    char command[512];
    if (is_test) {
        snprintf(command, sizeof(command), "rustc --test %s -o %s 2> %s", src_file, exec_file, out_file);
    } else {
        snprintf(command, sizeof(command), "rustc %s -o %s 2> %s", src_file, exec_file, out_file);
    }
    int compile_status = system(command);

    if (compile_status != 0) {
        // Erro de compilação
        FILE *err_file = fopen(out_file, "r");
        if (err_file) {
            memset(output, 0, 4096);
            strcat(output, "COMPILATION ERROR:\n");
            fread(output + 19, 1, 4000, err_file);
            write(sock, output, strlen(output));
            fclose(err_file);
        } else {
             write(sock, "Server Error: Cannot read compilation error log\n", 48);
        }
    } else {
        // Sucesso na compilação, agora executar
        snprintf(command, sizeof(command), "./%s > %s 2>&1", exec_file, out_file);
        system(command);
        
        FILE *out_file_fp = fopen(out_file, "r");
        if (out_file_fp) {
            memset(output, 0, 4096);
            strcat(output, "OUTPUT:\n");
            fread(output + 8, 1, 4000, out_file_fp);
            write(sock, output, strlen(output));
            fclose(out_file_fp);
        } else {
             write(sock, "Server Error: Cannot read output log\n", 37);
        }
    }

    // Limpeza
    snprintf(command, sizeof(command), "rm -f %s %s %s", src_file, exec_file, out_file);
    system(command);
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

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
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