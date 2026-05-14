#ifndef SCANNER_H
#define SCANNER_H

#include "cert_db.h"

extern CertNode* avl_root;

void scan_target(const char* domain);

#endif // SCANNER_H
