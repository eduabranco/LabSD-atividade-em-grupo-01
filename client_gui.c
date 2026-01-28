/* client_gui_fixed.c */
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

// Struct to pass data to the thread
typedef struct {
    char *code_text;
    char *ip_str;
    int port;
    GtkWidget *text_view_output;
    GtkWidget *run_btn; // To re-enable the button
} ThreadData;

// Function to update UI from the main loop (Thread Safe)
gboolean update_output_ui(gpointer data) {
    char *response = (char *)data;
    // We need to retrieve the widgets somehow. 
    // For simplicity, let's assume 'data' contains the struct OR pass struct as data.
    // Here we will pass a custom struct containing the message and widgets.
    return FALSE; // Remove this source
}

typedef struct {
    ThreadData *original_data;
    char *message;
} UiUpdateData;

gboolean on_ui_update(gpointer user_data) {
    UiUpdateData *update = (UiUpdateData *)user_data;
    GtkTextBuffer *buffer_out = gtk_text_view_get_buffer(GTK_TEXT_VIEW(update->original_data->text_view_output));
    gtk_text_buffer_set_text(buffer_out, update->message, -1);
    
    // Re-enable button
    gtk_widget_set_sensitive(update->original_data->run_btn, TRUE);

    // Cleanup
    free(update->original_data->code_text);
    free(update->original_data->ip_str);
    free(update->original_data); // Free the thread data structure
    free(update->message);
    free(update);
    return FALSE; // Don't call again
}

void *network_thread(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[4096];
    char *result_msg = malloc(8192); // Allocate memory for result
    
    if (!result_msg) pthread_exit(NULL);
    result_msg[0] = '\0';

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        strcpy(result_msg, "Erro: Não foi possível abrir o socket.");
        goto finish;
    }

    server = gethostbyname(data->ip_str);
    if (server == NULL) {
        strcpy(result_msg, "Erro: Host não encontrado.");
        goto finish;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(data->port);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        strcpy(result_msg, "Erro: Falha na conexão com o servidor.");
        goto finish;
    }

    n = write(sockfd, data->code_text, strlen(data->code_text));
    if (n < 0) {
        strcpy(result_msg, "Erro: Falha ao enviar dados.");
        goto finish;
    }

    // Read response
    bzero(buffer, 4096);
    n = read(sockfd, buffer, 4095);
    if (n < 0) {
        strcpy(result_msg, "Erro ao ler resposta.");
    } else {
        strncpy(result_msg, buffer, 8191);
    }
    close(sockfd);

finish:
    // Schedule UI update on main thread
    UiUpdateData *up = malloc(sizeof(UiUpdateData));
    up->original_data = data;
    up->message = result_msg;
    g_idle_add(on_ui_update, up);
    return NULL;
}

typedef struct {
    GtkWidget *text_view_input;
    GtkWidget *text_view_output;
    GtkWidget *entry_ip;
    GtkWidget *entry_port;
    GtkWidget *run_btn;
} AppWidgets;

void on_run_button_clicked(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    GtkTextBuffer *buffer_in;
    GtkTextIter start, end;

    // Prepare data for thread
    ThreadData *tdata = malloc(sizeof(ThreadData));
    
    // Get Code
    buffer_in = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->text_view_input));
    gtk_text_buffer_get_bounds(buffer_in, &start, &end);
    tdata->code_text = gtk_text_buffer_get_text(buffer_in, &start, &end, FALSE);

    // Get IP/Port
    tdata->ip_str = strdup(gtk_entry_get_text(GTK_ENTRY(app->entry_ip)));
    const char *port_str = gtk_entry_get_text(GTK_ENTRY(app->entry_port));
    tdata->port = atoi(port_str);
    
    tdata->text_view_output = app->text_view_output;
    tdata->run_btn = app->run_btn;

    // Disable button to prevent double click
    gtk_widget_set_sensitive(app->run_btn, FALSE);
    
    // Set status
    GtkTextBuffer *buf_out = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->text_view_output));
    gtk_text_buffer_set_text(buf_out, "Conectando e compilando...", -1);

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, network_thread, tdata);
    pthread_detach(thread_id);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window, *grid, *scroll_in, *scroll_out, *label_in, *label_out;
    AppWidgets *widgets = g_slice_new(AppWidgets);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Cliente Rust Compiler (Non-Blocking)");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    // IP/Port
    widgets->entry_ip = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(widgets->entry_ip), "127.0.0.1");
    widgets->entry_port = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(widgets->entry_port), "51482");
    
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("IP:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->entry_ip, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Porta:"), 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->entry_port, 3, 0, 1, 1);

    // Input Area
    label_in = gtk_label_new("Código Rust:");
    gtk_widget_set_halign(label_in, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label_in, 0, 1, 4, 1);

    widgets->text_view_input = gtk_text_view_new();
    scroll_in = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scroll_in, 580, 200);
    gtk_container_add(GTK_CONTAINER(scroll_in), widgets->text_view_input);
    gtk_grid_attach(GTK_GRID(grid), scroll_in, 0, 2, 4, 1);
    
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->text_view_input));
    gtk_text_buffer_set_text(buf, "fn main() {\n    println!(\"Ola do Servidor Linux!\");\n}", -1);

    // Button
    widgets->run_btn = gtk_button_new_with_label("Compilar e Executar Remotamente");
    g_signal_connect(widgets->run_btn, "clicked", G_CALLBACK(on_run_button_clicked), widgets);
    gtk_grid_attach(GTK_GRID(grid), widgets->run_btn, 0, 3, 4, 1);

    // Output Area
    label_out = gtk_label_new("Saída / Erros:");
    gtk_widget_set_halign(label_out, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label_out, 0, 4, 4, 1);

    widgets->text_view_output = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(widgets->text_view_output), FALSE);
    scroll_out = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scroll_out, 580, 150);
    gtk_container_add(GTK_CONTAINER(scroll_out), widgets->text_view_output);
    gtk_grid_attach(GTK_GRID(grid), scroll_out, 0, 5, 4, 1);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}