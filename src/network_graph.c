#include "common.h"
#include "network_graph.h"

DeviceNode* graph_head = NULL;
DeviceNode* local_device = NULL;

DeviceNode* add_device(const char* domain, const char* ip, const char* health) {
    DeviceNode* curr = graph_head;
    while (curr) {
        if (strcmp(curr->domain_name, domain) == 0) {
            strncpy(curr->ip_address, ip, 63);
            curr->ip_address[63] = '\0';
            strncpy(curr->health_status, health, 15);
            curr->health_status[15] = '\0';
            return curr;
        }
        curr = curr->next;
    }

    DeviceNode* new_node = (DeviceNode*)malloc(sizeof(DeviceNode));
    if (new_node == NULL) {
        printf("[-] Gagal alokasi memori untuk device.\n");
        return NULL;
    }

    strncpy(new_node->domain_name, domain, 127);
    new_node->domain_name[127] = '\0';
    strncpy(new_node->ip_address, ip, 63);
    new_node->ip_address[63] = '\0';
    strncpy(new_node->health_status, health, 15);
    new_node->health_status[15] = '\0';
    new_node->edges = NULL;
    new_node->next = graph_head;
    graph_head = new_node;

    return new_node;
}

void add_connection(DeviceNode* src, DeviceNode* dest, double latency, float loss) {
    if (src == NULL || dest == NULL) {
        return;
    }

    EdgeNode* curr_edge = src->edges;
    while (curr_edge) {
        if (curr_edge->dest == dest) {
            curr_edge->latency = latency;
            curr_edge->package_loss = loss;
            return;
        }
        curr_edge = curr_edge->next;
    }

    EdgeNode* new_edge = (EdgeNode*)malloc(sizeof(EdgeNode));
    if (new_edge == NULL) {
        printf("[-] Gagal alokasi memori untuk koneksi graph.\n");
        return;
    }

    new_edge->dest = dest;
    new_edge->latency = latency;
    new_edge->package_loss = loss;
    new_edge->next = src->edges;
    src->edges = new_edge;
}

void print_graph() {
    printf("\n--- NETWORK GRAPH ---\n");
    DeviceNode* curr = graph_head;
    if (!curr) {
        printf("Belum ada perangkat di jaringan.\n");
    }
    while (curr) {
        printf("Node: %s (%s) [%s]\n", curr->domain_name, curr->ip_address, curr->health_status);
        EdgeNode* edge = curr->edges;
        while (edge) {
            printf("  -> terhubung ke %s (Latency: %.2f ms, Loss: %.1f%%)\n",
                edge->dest->domain_name, edge->latency, edge->package_loss);
            edge = edge->next;
        }
        curr = curr->next;
    }
}

void free_graph() {
    DeviceNode* curr = graph_head;
    while (curr) {
        EdgeNode* edge = curr->edges;
        while (edge) {
            EdgeNode* temp_edge = edge;
            edge = edge->next;
            free(temp_edge);
        }
        DeviceNode* temp_node = curr;
        curr = curr->next;
        free(temp_node);
    }
    graph_head = NULL;
    local_device = NULL;
}
