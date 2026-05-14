#include "common.h"
#include "scanner.h"
#include "logger.h"
#include "csv_helper.h"
#include "network_graph.h"
#include "cert_db.h"

CertNode* avl_root = NULL;

static void get_cert_date(const ASN1_TIME *time, char* output) {
    BIO *b = BIO_new(BIO_s_mem());
    if (b == NULL) {
        strcpy(output, "Unknown");
        return;
    }
    ASN1_TIME_print(b, time);
    int len = BIO_read(b, output, 127);
    if (len > 0) {
        output[len] = '\0';
    } else {
        strcpy(output, "Unknown");
    }
    BIO_free(b);
}

void scan_target(const char* domain) {
    printf("\n[+] Memulai scan pada %s...\n", domain);
    char log_msg[512];

    char ip_str[INET_ADDRSTRLEN] = "Unknown";
    struct hostent *host = gethostbyname(domain);
    if (host) {
        inet_ntop(AF_INET, host->h_addr_list[0], ip_str, INET_ADDRSTRLEN);
    } else {
        printf("[-] Gagal menemukan IP untuk %s. Pastikan koneksi internet aktif.\n", domain);
        snprintf(log_msg, sizeof(log_msg), "Failed scan %s. DNS resolution failed.", domain);
        add_log(log_msg);
        return;
    }

    int pings = 4;
    int success = 0;
    double total_latency = 0.0;

    for (int i = 0; i < pings; i++) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            continue;
        }

        struct sockaddr_in server;
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port = htons(443);
        inet_pton(AF_INET, ip_str, &server.sin_addr);

        clock_t start = clock();
        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) == 0) {
            clock_t end = clock();
            total_latency += ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
            success++;
        }

#ifdef _WIN32
        closesocket(sock);
        Sleep(200);
#else
        close(sock);
        usleep(200000);
#endif
    }

    float loss = ((pings - success) / (float)pings) * 100.0;
    double avg_latency = success > 0 ? total_latency / success : 0;
    char health[16];
    strcpy(health, (loss > 50.0 || avg_latency > 500) ? "Bad" : "Good");

    DeviceNode* target_node = add_device(domain, ip_str, health);
    if (target_node == NULL) {
        printf("[-] Gagal menambahkan device ke graph. Scan dibatalkan.\n");
        return;
    }
    add_connection(local_device, target_node, avg_latency, loss);

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    char issuer_str[256] = "Unknown";
    char exp_date[128] = "Unknown";
    char protocol[32] = "Unknown";
    char risk[32] = "High Risk";

    if (ctx == NULL) {
        printf("[-] Gagal membuat SSL context.\n");
    } else {
        SSL *ssl = SSL_new(ctx);
        if (ssl == NULL) {
            printf("[-] Gagal membuat SSL object.\n");
        } else {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock >= 0) {
                struct sockaddr_in server;
                memset(&server, 0, sizeof(server));
                server.sin_family = AF_INET;
                server.sin_port = htons(443);
                inet_pton(AF_INET, ip_str, &server.sin_addr);

                if (connect(sock, (struct sockaddr *)&server, sizeof(server)) == 0) {
                    SSL_set_fd(ssl, sock);
                    SSL_set_tlsext_host_name(ssl, domain);

                    if (SSL_connect(ssl) > 0) {
                        X509 *cert = SSL_get_peer_certificate(ssl);
                        if (cert) {
                            X509_NAME_oneline(X509_get_issuer_name(cert), issuer_str, sizeof(issuer_str));
                            const ASN1_TIME* not_after = X509_get0_notAfter(cert);
                            get_cert_date(not_after, exp_date);
                            strncpy(protocol, SSL_get_version(ssl), 31);
                            protocol[31] = '\0';

                            // Cek apakah sertifikat sudah expired
                            int is_expired = (X509_cmp_current_time(not_after) <= 0);
                            int days_left = 0, secs_left = 0;

                            if (!is_expired) {
                                ASN1_TIME* now_asn1 = ASN1_TIME_set(NULL, time(NULL));
                                if (now_asn1) {
                                    ASN1_TIME_diff(&days_left, &secs_left, now_asn1, not_after);
                                    ASN1_TIME_free(now_asn1);
                                }
                            }

                            if (is_expired) {
                                strcpy(risk, "EXPIRED");
                                printf("[!!!] PERINGATAN: Sertifikat %s sudah KEDALUWARSA!\n", domain);
                            } else if (days_left <= 30) {
                                strcpy(risk, "Expiring Soon");
                                printf("[!]  PERINGATAN: Sertifikat %s akan kedaluwarsa dalam %d hari!\n", domain, days_left);
                            } else if (strcmp(protocol, "TLSv1.2") == 0 || strcmp(protocol, "TLSv1.3") == 0) {
                                strcpy(risk, "Low Risk");
                            }
                            // Jika protokol lama dan tidak expired, tetap "High Risk"

                            X509_free(cert);
                        }
                    } else {
                        printf("[-] SSL handshake gagal untuk %s.\n", domain);
                    }
                } else {
                    printf("[-] Gagal connect ke port 443 untuk SSL scan.\n");
                }
#ifdef _WIN32
                closesocket(sock);
#else
                close(sock);
#endif
            }
            SSL_free(ssl);
        }
        SSL_CTX_free(ctx);
    }

    avl_root = insert_cert(avl_root, domain, issuer_str, exp_date, protocol, risk);

    save_scan_to_csv(domain, ip_str, avg_latency, loss, health, issuer_str, exp_date, protocol, risk);

    snprintf(log_msg, sizeof(log_msg), "Scanned %s (%s). SSL: %s, Health: %s", domain, ip_str, risk, health);
    add_log(log_msg);

    printf("[+] Scan selesai! Hasil sudah disimpan di memori dan CSV.\n");
}
