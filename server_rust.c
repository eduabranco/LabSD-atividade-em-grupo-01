/* server_rust_fixed.c */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void sigchld_handler(int s) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// Function to reliably write all bytes to a file descriptor
ssize_t write_all(int fd, const void *buf, size_t count) {
    size_t written = 0;
    const char *ptr = buf;
    while (written < count) {
        ssize_t n = write(fd, ptr + written, count - written);
        if (n == -1) return -1;
        written += n;
    }
    return written;
}

void process_rust_code(int sock) {
    char buffer[4096];
    char output[8192];
    ssize_t n;

    // 1. Read source code
    bzero(buffer, 4096);
    n = read(sock, buffer, 4095);
    if (n < 0) {
        perror("ERROR reading from socket");
        return;
    }

    // 2. Create a unique temporary directory for this request
    char temp_dir[] = "/tmp/rust_job_XXXXXX";
    if (mkdtemp(temp_dir) == NULL) {
        perror("mkdtemp");
        write(sock, "Server Error: Could not create temp dir\n", 40);
        return;
    }

    // Construct file paths
    char src_file[256], bin_file[256], out_file[256], cmd[1024];
    snprintf(src_file, sizeof(src_file), "%s/main.rs", temp_dir);
    snprintf(bin_file, sizeof(bin_file), "%s/main", temp_dir);
    snprintf(out_file, sizeof(out_file), "%s/output.txt", temp_dir);

    // Write buffer to main.rs
    int fd = open(src_file, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        perror("open src");
        goto cleanup;
    }
    write_all(fd, buffer, n);
    close(fd);

    // 3. Compile: rustc /tmp/xxx/main.rs -o /tmp/xxx/main
    snprintf(cmd, sizeof(cmd), "rustc \"%s\" -o \"%s\" > \"%s\" 2>&1", src_file, bin_file, out_file);
    int compile_status = system(cmd);

    bzero(output, sizeof(output));

    if (compile_status != 0) {
        // Compilation Failed
        strcat(output, "--- COMPILATION ERROR ---\n");
        int out_fd = open(out_file, O_RDONLY);
        if (out_fd >= 0) {
            ssize_t r = read(out_fd, output + strlen(output), 4000);
            close(out_fd);
        }
    } else {
        // Compilation Success - Execute
        // Using 'timeout' command to prevent infinite loops (5 seconds limit)
        snprintf(cmd, sizeof(cmd), "timeout 5s \"%s\" > \"%s\" 2>&1", bin_file, out_file);
        int run_status = system(cmd);
        
        if (run_status == 124 * 256) { // timeout exit code 124
             strcat(output, "--- EXECUTION TIMED OUT ---\n");
        } else {
             strcat(output, "--- OUTPUT ---\n");
             int out_fd = open(out_file, O_RDONLY);
             if (out_fd >= 0) {
                 ssize_t r = read(out_fd, output + strlen(output), 4000);
                 close(out_fd);
             }
        }
    }

    // Send response
    write_all(sock, output, strlen(output));

cleanup:
    // Clean up temporary directory: rm -rf /tmp/rust_job_XXXXXX
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", temp_dir);
    system(cmd);
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

    listen(sockfd, 50); // Increased backlog
    
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    printf("Fixed Rust Server running on port %d...\n", portno);

    while(1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
             if (errno == EINTR) continue;
             error("ERROR on accept");
        }

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