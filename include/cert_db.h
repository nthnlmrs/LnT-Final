#ifndef CERT_DB_H
#define CERT_DB_H

typedef struct CertNode {
    char domain_name[128];
    char issuer[256];
    char expire_date[128];
    char protocol[32];
    char cert_risk[32];
    int height;
    struct CertNode* left;
    struct CertNode* right;
} CertNode;

CertNode* insert_cert(
    CertNode* node,
    const char* domain,
    const char* issuer,
    const char* exp,
    const char* proto,
    const char* risk
);

void print_certs(CertNode* root);
void free_cert_tree(CertNode* root);

#endif // CERT_DB_H
