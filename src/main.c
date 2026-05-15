#include "common.h"
#include "csv_helper.h"
#include "logger.h"
#include "cert_db.h"
#include "network_graph.h"
#include "scanner.h"
#include <gtk/gtk.h>
#ifndef _WIN32
#include <fcntl.h>
#endif

static GtkWidget *text_view;
static GtkWidget *entry_domain;
static GtkWidget *window;

static void update_text_view(const char *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, text ? text : "", -1);
}

// Redirect using a temporary file instead of a pipe to avoid deadlocks
static int stdout_bk;
static FILE* temp_out;

static void start_redirect() {
    fflush(stdout);
    stdout_bk = dup(STDOUT_FILENO);
    temp_out = tmpfile();
    if (temp_out) {
        dup2(fileno(temp_out), STDOUT_FILENO);
    }
}

static char* end_redirect() {
    fflush(stdout);
    dup2(stdout_bk, STDOUT_FILENO);
    close(stdout_bk);

    if (!temp_out) {
        return strdup("Error: failed to capture output.\n");
    }

    fseek(temp_out, 0, SEEK_END);
    long size = ftell(temp_out);
    fseek(temp_out, 0, SEEK_SET);

    char *result = malloc(size + 1);
    if (!result) {
        fclose(temp_out);
        return NULL;
    }

    size_t read_bytes = fread(result, 1, size, temp_out);
    result[read_bytes] = '\0';
    fclose(temp_out);
    return result;
}

static void on_btn_scan_clicked(GtkWidget *widget, gpointer data) {
    const char *domain = gtk_entry_get_text(GTK_ENTRY(entry_domain));

    if (strlen(domain) == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Input cannot be empty!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    start_redirect();
    scan_target(domain);
    char *output = end_redirect();
    update_text_view(output);
    free(output);
}

static void on_btn_graph_clicked(GtkWidget *widget, gpointer data) {
    start_redirect();
    print_graph();
    char *output = end_redirect();
    update_text_view(output);
    free(output);
}

static void on_btn_cert_clicked(GtkWidget *widget, gpointer data) {
    start_redirect();
    printf("\n--- DATABASE SERTIFIKAT (AVL TREE) ---\n");
    if (avl_root == NULL) {
        printf("Belum ada sertifikat tersimpan.\n");
    } else {
        print_certs(avl_root);
    }
    char *output = end_redirect();
    update_text_view(output);
    free(output);
}

static void on_btn_log_clicked(GtkWidget *widget, gpointer data) {
    start_redirect();
    print_logs();
    char *output = end_redirect();
    update_text_view(output);
    free(output);
}

static void on_window_destroy(GtkWidget *widget, gpointer data) {
    add_log("System Closed. Scanner stopped.");
    free_all_logs();
    free_cert_tree(avl_root);
    free_graph();
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    ensure_data_directory();
    add_log("System Started. Initializing Scanner.");
    local_device = add_device("Localhost", "127.0.0.1", "Good");

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Real SSL/TLS & TCP Scanner");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    GtkWidget *hbox_input = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_input, FALSE, FALSE, 0);

    GtkWidget *label_domain = gtk_label_new("Domain / IP:");
    gtk_box_pack_start(GTK_BOX(hbox_input), label_domain, FALSE, FALSE, 0);

    entry_domain = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox_input), entry_domain, TRUE, TRUE, 0);

    GtkWidget *btn_scan = gtk_button_new_with_label("Scan");
    g_signal_connect(btn_scan, "clicked", G_CALLBACK(on_btn_scan_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_input), btn_scan, FALSE, FALSE, 0);

    GtkWidget *hbox_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_buttons, FALSE, FALSE, 0);

    GtkWidget *btn_graph = gtk_button_new_with_label("Topologi Jaringan (Graph)");
    g_signal_connect(btn_graph, "clicked", G_CALLBACK(on_btn_graph_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), btn_graph, TRUE, TRUE, 0);

    GtkWidget *btn_cert = gtk_button_new_with_label("Database Sertifikat (AVL)");
    g_signal_connect(btn_cert, "clicked", G_CALLBACK(on_btn_cert_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), btn_cert, TRUE, TRUE, 0);

    GtkWidget *btn_log = gtk_button_new_with_label("Activity Log (DLL)");
    g_signal_connect(btn_log, "clicked", G_CALLBACK(on_btn_log_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), btn_log, TRUE, TRUE, 0);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);

    // Instead of override_font, we can use CSS providers but to keep it simple and warning free:
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider, "textview { font-family: monospace; font-size: 10pt; }", -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(text_view);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);

    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    gtk_widget_show_all(window);

    gtk_main();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
