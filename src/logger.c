#include "common.h"
#include "logger.h"
#include "csv_helper.h"

typedef struct LogNode {
    char activity[256];
    char timestamp[64];
    struct LogNode* prev;
    struct LogNode* next;
} LogNode;

static LogNode* log_head = NULL;
static LogNode* log_tail = NULL;

void add_log(const char* activity) {
    LogNode* new_node = (LogNode*)malloc(sizeof(LogNode));
    if (new_node == NULL) {
        printf("[-] Gagal alokasi memori untuk log.\n");
        return;
    }
    strncpy(new_node->activity, activity, 255);
    new_node->activity[255] = '\0';
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(new_node->timestamp, 64, "%Y-%m-%d %H:%M:%S", tm_info);
    new_node->next = NULL;
    new_node->prev = log_tail;

    if (log_tail == NULL) {
        log_head = new_node;
    } else {
        log_tail->next = new_node;
    }
    log_tail = new_node;

    save_log_to_csv(new_node->timestamp, new_node->activity);
}

void print_logs() {
    printf("\n--- ACTIVITY LOG ---\n");
    LogNode* curr = log_head;
    if (!curr) {
        printf("Belum ada log aktivitas.\n");
    }
    while (curr) {
        printf("[%s] %s\n", curr->timestamp, curr->activity);
        curr = curr->next;
    }
}

void free_all_logs() {
    LogNode* curr = log_head;
    while (curr) {
        LogNode* temp = curr;
        curr = curr->next;
        free(temp);
    }
    log_head = NULL;
    log_tail = NULL;
}
