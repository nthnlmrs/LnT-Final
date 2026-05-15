# 📋 Daftar Fitur — Real SSL/TLS & TCP Network Scanner

Dokumen ini menjelaskan seluruh fitur yang diimplementasikan dalam program beserta struktur data dan detail teknisnya.

---

## 1. 🌐 DNS Resolution

**Pendahuluan (Penjelasan Risk):** Risiko utama pada DNS Resolution adalah DNS Spoofing atau keracunan cache DNS (DNS Cache Poisoning). Jika DNS yang digunakan tidak aman, aplikasi dapat diarahkan ke IP palsu yang berbahaya, sehingga hasil scan SSL akan mengarah ke server penipu.

**File:** `src/scanner.c`

- Menerjemahkan nama domain (contoh: `google.com`) menjadi alamat IP menggunakan `gethostbyname()`
- Jika resolusi gagal (domain tidak ditemukan / tidak ada koneksi), scan dibatalkan dan error dicatat ke activity log
- IP hasil resolusi disimpan dalam format string `INET_ADDRSTRLEN` (`inet_ntop`)

---

## 2. 📡 TCP Connectivity Scan & Latency

**Pendahuluan (Penjelasan Risk):** Risiko melakukan TCP scan adalah bisa memicu sistem deteksi intrusi (IDS) atau firewall yang menganggap ping berulang ini sebagai serangan DoS ringan atau port scanning. Selain itu, latency yang diukur bisa tidak akurat karena throttling jaringan.

**File:** `src/scanner.c`

- Melakukan **4 kali percobaan koneksi TCP** ke port `443` (HTTPS) untuk mengukur stabilitas koneksi
- Mengukur **latency per koneksi** menggunakan `clock()` (dalam milidetik)
- Menghitung **rata-rata latency** dari koneksi yang berhasil
- Menghitung **packet loss** berdasarkan rasio koneksi yang gagal terhadap total percobaan:
  ```
  loss = ((total_ping - success) / total_ping) * 100%
  ```
- Delay antar ping: `200ms` (`Sleep` / `usleep`)

---

## 3. 🏥 Penilaian Health Status

**Pendahuluan (Penjelasan Risk):** Menilai health status secara naif berisiko menghasilkan false positive atau false negative. Jaringan yang sedang sibuk (congested) bisa terdeteksi "Bad" padahal server sebenarnya dalam keadaan normal.

**File:** `src/scanner.c`

Menentukan kondisi perangkat berdasarkan hasil TCP scan:

| Kondisi | Status |
|---------|--------|
| `loss > 50%` ATAU `avg_latency > 500ms` | `Bad` |
| Selain itu | `Good` |

Status ini disimpan di node graph dan di-export ke CSV.

---

## 4. 🔒 SSL/TLS Certificate Scanning

**Pendahuluan (Penjelasan Risk):** Memindai sertifikat berisiko terekspos pada kerentanan library OpenSSL jika tidak menggunakan versi terbaru. Selain itu, sertifikat yang menggunakan algoritma hashing lemah (seperti MD5/SHA1) merupakan risiko keamanan yang bisa dimanfaatkan oleh penyerang.

**File:** `src/scanner.c`

Menggunakan library **OpenSSL** untuk membuka koneksi SSL ke port 443 dan mengekstrak:

| Info | Detail |
|------|--------|
| **Issuer** | Nama CA penerbit sertifikat (`X509_NAME_oneline`) |
| **Expiry Date** | Tanggal kedaluwarsa sertifikat (`X509_get0_notAfter`) |
| **Protokol** | Versi TLS yang digunakan (`SSL_get_version`) |
| **Risk Level** | Penilaian risiko berdasarkan protokol dan expiry |

**SNI (Server Name Indication)** diaktifkan via `SSL_set_tlsext_host_name()` agar handshake ke virtual host berjalan dengan benar.

---

## 5. ⚠️ Peringatan Expiry Sertifikat

**Pendahuluan (Penjelasan Risk):** Sertifikat yang kedaluwarsa mengakibatkan koneksi tidak lagi dipercaya oleh browser/klien (Insecure Connection Risk). Ini memungkinkan serangan Man-in-the-Middle (MitM) jika pengguna tetap memaksa mengakses layanan tersebut.

**File:** `src/scanner.c`, `src/cert_db.c`

Fitur utama yang dicek secara otomatis setiap kali scan:

| Status | Kondisi | Indikator Output |
|--------|---------|-----------------|
| `EXPIRED` | Tanggal expire sudah lewat | `[!!!] PERINGATAN: Sertifikat ... sudah KEDALUWARSA!` |
| `Expiring Soon` | Sisa ≤ 30 hari | `[!]  PERINGATAN: Sertifikat ... akan kedaluwarsa dalam N hari!` |
| `Low Risk` | TLSv1.2 / TLSv1.3, masih valid > 30 hari | *(tidak ada peringatan)* |
| `High Risk` | Protokol lama (< TLS 1.2), masih valid | *(tidak ada peringatan, namun risk tercatat)* |

**Fungsi OpenSSL yang digunakan:**
- `X509_cmp_current_time()` — cek apakah cert sudah expired
- `ASN1_TIME_diff()` — hitung sisa hari sebelum expire
- `ASN1_TIME_set()` / `ASN1_TIME_free()` — buat objek waktu sekarang

Peringatan juga ditampilkan dengan marker `[!!!]` / `[!]` di **Database Sertifikat**.

---

## 6. 🌳 AVL Tree — Database Sertifikat

**Pendahuluan (Penjelasan Risk):** Menyimpan data pada memori secara tidak terbatas memiliki risiko Out-Of-Memory (OOM) jika terjadi scanning ke jutaan domain. Kegagalan alokasi memori dapat menyebabkan program crash.

**File:** `src/cert_db.c`, `include/cert_db.h`

Self-balancing Binary Search Tree (AVL Tree) untuk menyimpan data sertifikat secara terurut berdasarkan `domain_name`.

**Operasi yang didukung:**

| Fungsi | Deskripsi |
|--------|-----------|
| `insert_cert()` | Insert node baru atau **update data** jika domain sudah ada (tidak duplikat) |
| `print_certs()` | Traversal **in-order** (domain tampil alfabetis) dengan marker expiry |
| `free_cert_tree()` | Bebaskan seluruh memori tree secara rekursif |

---

## 7. 🗺️ Network Graph — Topologi Jaringan

**Pendahuluan (Penjelasan Risk):** Jika terjadi loop jaringan (circular dependency) dan algoritma traversal tidak menanganinya, program bisa masuk ke infinite loop. Risiko lain adalah kebocoran memori (memory leak) jika node edge tidak dibebaskan dengan benar.

**File:** `src/network_graph.c`, `include/network_graph.h`

**Directed Graph** berbasis adjacency list untuk merepresentasikan koneksi antar perangkat.

**Struktur:**
- `DeviceNode` — merepresentasikan satu host/domain (linked list)
- `EdgeNode` — merepresentasikan koneksi dari satu device ke device lain (linked list per node)

---

## 8. 📝 Activity Logger — Doubly Linked List

**Pendahuluan (Penjelasan Risk):** Log yang terlalu besar bisa menghabiskan RAM karena disimpan sebagai Doubly Linked List di memori. Risiko lain adalah log spoofing jika input log tidak disanitasi.

**File:** `src/logger.c`, `include/logger.h`

Log aktivitas menggunakan **Doubly Linked List (DLL)** agar traversal bisa dilakukan dua arah.

**Operasi yang didukung:**

| Fungsi | Deskripsi |
|--------|-----------|
| `add_log()` | Tambah entri log baru di **tail** beserta timestamp otomatis |
| `print_logs()` | Tampilkan semua log dari head ke tail |
| `free_all_logs()` | Bebaskan semua node log dari memori |

---

## 9. 💾 Export ke CSV

**Pendahuluan (Penjelasan Risk):** Risiko file system seperti permission denied atau disk penuh dapat menyebabkan gagalnya penulisan file CSV. Terdapat juga risiko CSV Injection jika data mengandung karakter eksekusi macro yang dibuka oleh program spreadsheet eksternal.

**File:** `src/csv_helper.c`, `include/csv_helper.h`

Menyimpan hasil scan dan log aktivitas ke file CSV secara otomatis.

**File yang dihasilkan:**

| File | Kolom |
|------|-------|
| `data/scan_results.csv` | `timestamp, domain, ip, latency_ms, packet_loss_percent, health, issuer, expire_date, protocol, risk` |
| `data/activity_log.csv` | `timestamp, activity` |

---

## 10. 🖥️ Cross-Platform Support

**Pendahuluan (Penjelasan Risk):** Perbedaan perilaku antara Winsock di Windows dan POSIX Sockets di Linux berisiko memunculkan bug platform-specific (seperti socket descriptor limits atau masalah threading).

**File:** `include/common.h`, semua `.c`

Program dapat dikompilasi dan dijalankan di Windows dan Linux / macOS. Deteksi platform dilakukan via preprocessor `#ifdef _WIN32`.

---

## 11. 🖱️ Antarmuka GTK (GIMP Toolkit)

**Pendahuluan (Penjelasan Risk):** Menjalankan program berbasis GTK dari lingkungan non-grafis dapat menyebabkan error pada main loop dan membuat program crash dengan error 'cannot open display'. Terdapat juga risiko GUI freezing jika proses scan jaringan berjalan secara synchronous (memblokir thread GUI).

**File:** `src/main.c`

Antarmuka berbasis GTK3 yang menyediakan tampilan visual yang lebih baik.

**Fitur input:**
- Validasi input domain: jika input kosong, aplikasi akan menampilkan alert/popup error dan mencegah scanning.
- Tampilan scrollable text view untuk menampilkan hasil secara real-time.
- Pembersihan resource dijamin melalui sinyal `destroy` saat jendela ditutup.
