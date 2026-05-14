#include "common.h"
#include "csv_helper.h"

#define DATA_DIR "data"
#define SCAN_CSV_FILE "data/scan_results.csv"
#define LOG_CSV_FILE "data/activity_log.csv"

void ensure_data_directory() {
#ifdef _WIN32
    struct _stat st = {0};
    if (_stat(DATA_DIR, &st) == -1) {
        _mkdir(DATA_DIR);
    }
#else
    struct stat st = {0};
    if (stat(DATA_DIR, &st) == -1) {
        mkdir(DATA_DIR, 0755);
    }
#endif
}

int file_is_empty_or_not_exist(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return 1;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    return size == 0;
}

static void csv_write_field(FILE* file, const char* text) {
    fputc('"', file);
    if (text != NULL) {
        for (int i = 0; text[i] != '\0'; i++) {
            if (text[i] == '"') {
                fputc('"', file);
            }
            fputc(text[i], file);
        }
    }
    fputc('"', file);
}

void save_log_to_csv(const char* timestamp, const char* activity) {
    ensure_data_directory();
    int need_header = file_is_empty_or_not_exist(LOG_CSV_FILE);
    FILE* file = fopen(LOG_CSV_FILE, "a");
    if (file == NULL) {
        printf("[-] Gagal membuka file log CSV: %s\n", LOG_CSV_FILE);
        return;
    }
    if (need_header) {
        fprintf(file, "timestamp,activity\n");
    }
    csv_write_field(file, timestamp);
    fprintf(file, ",");
    csv_write_field(file, activity);
    fprintf(file, "\n");
    fclose(file);
}

void save_scan_to_csv(
    const char* domain,
    const char* ip,
    double latency,
    float loss,
    const char* health,
    const char* issuer,
    const char* exp_date,
    const char* protocol,
    const char* risk
) {
    ensure_data_directory();
    int need_header = file_is_empty_or_not_exist(SCAN_CSV_FILE);
    FILE* file = fopen(SCAN_CSV_FILE, "a");
    if (file == NULL) {
        printf("[-] Gagal membuka file CSV: %s\n", SCAN_CSV_FILE);
        return;
    }
    if (need_header) {
        fprintf(file, "timestamp,domain,ip,latency_ms,packet_loss_percent,health,issuer,expire_date,protocol,risk\n");
    }
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    csv_write_field(file, timestamp);
    fprintf(file, ",");
    csv_write_field(file, domain);
    fprintf(file, ",");
    csv_write_field(file, ip);
    fprintf(file, ",");
    fprintf(file, "%.2f,", latency);
    fprintf(file, "%.1f,", loss);
    csv_write_field(file, health);
    fprintf(file, ",");
    csv_write_field(file, issuer);
    fprintf(file, ",");
    csv_write_field(file, exp_date);
    fprintf(file, ",");
    csv_write_field(file, protocol);
    fprintf(file, ",");
    csv_write_field(file, risk);
    fprintf(file, "\n");
    fclose(file);
}
