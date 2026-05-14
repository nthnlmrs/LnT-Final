#ifndef CSV_HELPER_H
#define CSV_HELPER_H

void ensure_data_directory();
int file_is_empty_or_not_exist(const char* filename);
void save_log_to_csv(const char* timestamp, const char* activity);
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
);

#endif // CSV_HELPER_H
