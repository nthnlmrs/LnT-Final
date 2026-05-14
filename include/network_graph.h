#ifndef NETWORK_GRAPH_H
#define NETWORK_GRAPH_H

typedef struct EdgeNode {
    struct DeviceNode* dest;
    double latency;
    float package_loss;
    struct EdgeNode* next;
} EdgeNode;

typedef struct DeviceNode {
    char domain_name[128];
    char ip_address[64];
    char health_status[16];
    EdgeNode* edges;
    struct DeviceNode* next;
} DeviceNode;

DeviceNode* add_device(const char* domain, const char* ip, const char* health);
void add_connection(DeviceNode* src, DeviceNode* dest, double latency, float loss);
void print_graph();
void free_graph();

extern DeviceNode* graph_head;
extern DeviceNode* local_device;

#endif // NETWORK_GRAPH_H
