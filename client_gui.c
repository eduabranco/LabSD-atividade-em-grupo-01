/* client_gui.c */
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// Estrutura para passar widgets para a função de callback
typedef struct {
    GtkWidget *text_view_input;
    GtkWidget *text_view_output;
    GtkWidget *entry_ip;
    GtkWidget *entry_port;
} AppWidgets;

void on_run_button_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; // Silenciar aviso de parâmetro não usado
    AppWidgets *app = (AppWidgets *)data;
    GtkTextBuffer *buffer_in, *buffer_out;
    GtkTextIter start, end;
    char *code_text;
    const char *ip_str, *port_str;
    
    // Obter texto do editor
    buffer_in = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->text_view_input));
    gtk_text_buffer_get_bounds(buffer_in, &start, &end);
    code_text = gtk_text_buffer_get_text(buffer_in, &start, &end, FALSE);

    // Obter IP e Porta
    ip_str = gtk_entry_get_text(GTK_ENTRY(app->entry_ip));
    port_str = gtk_entry_get_text(GTK_ENTRY(app->entry_port));
    int portno = atoi(port_str);

    // --- Lógica de Socket ---
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char response[4096];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        code_text = "Erro: Não foi possível abrir o socket.";
        goto display_error;
    }

    server = gethostbyname(ip_str);
    if (server == NULL) {
        code_text = "Erro: Host não encontrado.";
        goto display_error;
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        code_text = "Erro: Falha na conexão com o servidor.";
        goto display_error;
    }

    // Enviar código
    n = write(sockfd, code_text, strlen(code_text));
    if (n < 0) {
        code_text = "Erro: Falha ao enviar dados.";
        goto display_error;
    }

    // Ler resposta
    memset(response, 0, 4096);
    n = read(sockfd, response, 4095);
    if (n < 0) {
        strcpy(response, "Erro ao ler resposta.");
    }
    
    close(sockfd);

    // Exibir no Output
    buffer_out = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->text_view_output));
    gtk_text_buffer_set_text(buffer_out, response, -1);
    return;

display_error:
    buffer_out = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->text_view_output));
    gtk_text_buffer_set_text(buffer_out, code_text, -1);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *run_btn;
    GtkWidget *scroll_in, *scroll_out;
    GtkWidget *label_in, *label_out;
    AppWidgets *widgets = g_slice_new(AppWidgets);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Cliente Rust Compiler");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    // Configuração IP/Porta
    widgets->entry_ip = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(widgets->entry_ip), "127.0.0.1");
    widgets->entry_port = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(widgets->entry_port), "51482");
    
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("IP:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->entry_ip, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Porta:"), 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->entry_port, 3, 0, 1, 1);

    // Área de Código (Input)
    label_in = gtk_label_new("Código Rust:");
    gtk_widget_set_halign(label_in, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label_in, 0, 1, 4, 1);

    widgets->text_view_input = gtk_text_view_new();
    scroll_in = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scroll_in, 580, 200);
    gtk_container_add(GTK_CONTAINER(scroll_in), widgets->text_view_input);
    gtk_grid_attach(GTK_GRID(grid), scroll_in, 0, 2, 4, 1);
    
    // Código inicial de exemplo
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->text_view_input));
    gtk_text_buffer_set_text(buf, "fn main() {\n    println!(\"Ola do Servidor Linux!\");\n}", -1);

    // Botão
    run_btn = gtk_button_new_with_label("Compilar e Executar Remotamente");
    g_signal_connect(run_btn, "clicked", G_CALLBACK(on_run_button_clicked), widgets);
    gtk_grid_attach(GTK_GRID(grid), run_btn, 0, 3, 4, 1);

    // Área de Saída (Output)
    label_out = gtk_label_new("Saída / Erros:");
    gtk_widget_set_halign(label_out, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label_out, 0, 4, 4, 1);

    widgets->text_view_output = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(widgets->text_view_output), FALSE); // Read only
    scroll_out = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scroll_out, 580, 150);
    gtk_container_add(GTK_CONTAINER(scroll_out), widgets->text_view_output);
    gtk_grid_attach(GTK_GRID(grid), scroll_out, 0, 5, 4, 1);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}