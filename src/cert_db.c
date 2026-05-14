#include "common.h"
#include "cert_db.h"

static int height(CertNode *N) {
    return (N == NULL) ? 0 : N->height;
}

static int max_val(int a, int b) {
    return (a > b) ? a : b;
}

static CertNode* rightRotate(CertNode *y) {
    CertNode *x = y->left;
    CertNode *T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = max_val(height(y->left), height(y->right)) + 1;
    x->height = max_val(height(x->left), height(x->right)) + 1;

    return x;
}

static CertNode* leftRotate(CertNode *x) {
    CertNode *y = x->right;
    CertNode *T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = max_val(height(x->left), height(x->right)) + 1;
    y->height = max_val(height(y->left), height(y->right)) + 1;

    return y;
}

static int getBalance(CertNode *N) {
    return (N == NULL) ? 0 : height(N->left) - height(N->right);
}

CertNode* insert_cert(
    CertNode* node,
    const char* domain,
    const char* issuer,
    const char* exp,
    const char* proto,
    const char* risk
) {
    if (node == NULL) {
        CertNode* new_node = (CertNode*)malloc(sizeof(CertNode));
        if (new_node == NULL) {
            printf("[-] Gagal alokasi memori untuk sertifikat.\n");
            return NULL;
        }

        strncpy(new_node->domain_name, domain, 127);
        new_node->domain_name[127] = '\0';
        strncpy(new_node->issuer, issuer, 255);
        new_node->issuer[255] = '\0';
        strncpy(new_node->expire_date, exp, 127);
        new_node->expire_date[127] = '\0';
        strncpy(new_node->protocol, proto, 31);
        new_node->protocol[31] = '\0';
        strncpy(new_node->cert_risk, risk, 31);
        new_node->cert_risk[31] = '\0';

        new_node->height = 1;
        new_node->left = NULL;
        new_node->right = NULL;

        return new_node;
    }

    int cmp = strcmp(domain, node->domain_name);

    if (cmp < 0) {
        node->left = insert_cert(node->left, domain, issuer, exp, proto, risk);
    } else if (cmp > 0) {
        node->right = insert_cert(node->right, domain, issuer, exp, proto, risk);
    } else {
        // Domain sudah ada, update datanya alih-alih diabaikan
        strncpy(node->issuer, issuer, 255);
        node->issuer[255] = '\0';
        strncpy(node->expire_date, exp, 127);
        node->expire_date[127] = '\0';
        strncpy(node->protocol, proto, 31);
        node->protocol[31] = '\0';
        strncpy(node->cert_risk, risk, 31);
        node->cert_risk[31] = '\0';
        return node;
    }

    node->height = 1 + max_val(height(node->left), height(node->right));

    int balance = getBalance(node);

    if (balance > 1 && strcmp(domain, node->left->domain_name) < 0) {
        return rightRotate(node);
    }
    if (balance < -1 && strcmp(domain, node->right->domain_name) > 0) {
        return leftRotate(node);
    }
    if (balance > 1 && strcmp(domain, node->left->domain_name) > 0) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    if (balance < -1 && strcmp(domain, node->right->domain_name) < 0) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

void print_certs(CertNode* root) {
    if (root != NULL) {
        print_certs(root->left);
        int is_expired      = (strcmp(root->cert_risk, "EXPIRED") == 0);
        int is_expiring     = (strcmp(root->cert_risk, "Expiring Soon") == 0);
        const char* prefix  = is_expired ? "[!!!] " : (is_expiring ? "[!]   " : "      ");
        printf("%s%s | Protocol: %s | Risk: %s | Exp: %s\n",
            prefix, root->domain_name, root->protocol, root->cert_risk, root->expire_date);
        print_certs(root->right);
    }
}

void free_cert_tree(CertNode* root) {
    if (root != NULL) {
        free_cert_tree(root->left);
        free_cert_tree(root->right);
        free(root);
    }
}
