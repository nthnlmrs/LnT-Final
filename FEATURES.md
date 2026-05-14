# 📋 Daftar Fitur — Real SSL/TLS & TCP Network Scanner

Dokumen ini menjelaskan seluruh fitur yang diimplementasikan dalam program beserta struktur data dan detail teknisnya.

---

## 1. 🌐 DNS Resolution

**File:** `src/scanner.c`

- Menerjemahkan nama domain (contoh: `google.com`) menjadi alamat IP menggunakan `gethostbyname()`
- Jika resolusi gagal (domain tidak ditemukan / tidak ada koneksi), scan dibatalkan dan error dicatat ke activity log
- IP hasil resolusi disimpan dalam format string `INET_ADDRSTRLEN` (`inet_ntop`)

---

## 2. 📡 TCP Connectivity Scan & Latency

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

**File:** `src/scanner.c`

Menentukan kondisi perangkat berdasarkan hasil TCP scan:

| Kondisi | Status |
|---------|--------|
| `loss > 50%` ATAU `avg_latency > 500ms` | `Bad` |
| Selain itu | `Good` |

Status ini disimpan di node graph dan di-export ke CSV.

---

## 4. 🔒 SSL/TLS Certificate Scanning

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

Peringatan juga ditampilkan dengan marker `[!!!]` / `[!]` di **Menu 3** (Database Sertifikat).

---

## 6. 🌳 AVL Tree — Database Sertifikat

**File:** `src/cert_db.c`, `include/cert_db.h`

Self-balancing Binary Search Tree (AVL Tree) untuk menyimpan data sertifikat secara terurut berdasarkan `domain_name`.

**Operasi yang didukung:**

| Fungsi | Deskripsi |
|--------|-----------|
| `insert_cert()` | Insert node baru atau **update data** jika domain sudah ada (tidak duplikat) |
| `print_certs()` | Traversal **in-order** (domain tampil alfabetis) dengan marker expiry |
| `free_cert_tree()` | Bebaskan seluruh memori tree secara rekursif |

**Mekanisme balancing:**
- Rotasi **Left-Left (LL)** → `rightRotate()`
- Rotasi **Right-Right (RR)** → `leftRotate()`
- Rotasi **Left-Right (LR)** → `leftRotate()` + `rightRotate()`
- Rotasi **Right-Left (RL)** → `rightRotate()` + `leftRotate()`

**Data yang disimpan per node:**
```c
char domain_name[128];
char issuer[256];
char expire_date[128];
char protocol[32];
char cert_risk[32];
int  height;          // untuk tracking balance factor
```

---

## 7. 🗺️ Network Graph — Topologi Jaringan

**File:** `src/network_graph.c`, `include/network_graph.h`

**Directed Graph** berbasis adjacency list untuk merepresentasikan koneksi antar perangkat.

**Struktur:**
- `DeviceNode` — merepresentasikan satu host/domain (linked list)
- `EdgeNode` — merepresentasikan koneksi dari satu device ke device lain (linked list per node)

**Operasi yang didukung:**

| Fungsi | Deskripsi |
|--------|-----------|
| `add_device()` | Tambah perangkat baru; jika domain sudah ada, **update IP dan health** |
| `add_connection()` | Tambah edge antar device; jika sudah ada, **update latency dan loss** |
| `print_graph()` | Tampilkan semua node beserta koneksi (latency, loss) |
| `free_graph()` | Bebaskan semua node dan edge dari memori |

**Global state:**
- `graph_head` — pointer ke list seluruh perangkat
- `local_device` — pointer ke node "Localhost" (titik asal koneksi)

---

## 8. 📝 Activity Logger — Doubly Linked List

**File:** `src/logger.c`, `include/logger.h`

Log aktivitas menggunakan **Doubly Linked List (DLL)** agar traversal bisa dilakukan dua arah.

**Operasi yang didukung:**

| Fungsi | Deskripsi |
|--------|-----------|
| `add_log()` | Tambah entri log baru di **tail** beserta timestamp otomatis |
| `print_logs()` | Tampilkan semua log dari head ke tail |
| `free_all_logs()` | Bebaskan semua node log dari memori |

**Timestamp** dibuat otomatis menggunakan `time()` + `strftime()` dengan format `YYYY-MM-DD HH:MM:SS`.

Setiap log juga **otomatis disimpan ke CSV** (`data/activity_log.csv`) saat `add_log()` dipanggil.

---

## 9. 💾 Export ke CSV

**File:** `src/csv_helper.c`, `include/csv_helper.h`

Menyimpan hasil scan dan log aktivitas ke file CSV secara otomatis.

**File yang dihasilkan:**

| File | Kolom |
|------|-------|
| `data/scan_results.csv` | `timestamp, domain, ip, latency_ms, packet_loss_percent, health, issuer, expire_date, protocol, risk` |
| `data/activity_log.csv` | `timestamp, activity` |

**Fitur tambahan:**
- **Header otomatis** — header kolom hanya ditulis jika file kosong atau belum ada (`file_is_empty_or_not_exist()`)
- **CSV-safe escaping** — karakter `"` di dalam nilai di-escape menjadi `""` sesuai standar RFC 4180 (`csv_write_field()`)
- **Auto-create direktori** — folder `data/` dibuat otomatis jika belum ada (`ensure_data_directory()`)

---

## 10. 🖥️ Cross-Platform Support

**File:** `include/common.h`, semua `.c`

Program dapat dikompilasi dan dijalankan di:

| Platform | Mekanisme |
|----------|-----------|
| **Windows** | `#ifdef _WIN32`: menggunakan Winsock2 (`WSAStartup` / `WSACleanup`), `closesocket()`, `Sleep()`, `_mkdir()` |
| **Linux / macOS** | POSIX sockets, `close()`, `usleep()`, `mkdir()` |

Deteksi platform dilakukan via preprocessor `#ifdef _WIN32`.

---

## 11. 🖱️ CLI Interaktif

**File:** `src/main.c`

Menu interaktif berbasis terminal dengan 5 pilihan:

```
1. Scan Domain / IP Baru
2. Lihat Topologi Jaringan (Graph)
3. Lihat Database Sertifikat (AVL)
4. Lihat Activity Log (DLL)
0. Keluar
```

**Fitur input:**
- Validasi input menu dengan `scanf()` + pengecekan return value
- `clear_buffer()` — membersihkan sisa karakter di stdin setelah input angka agar `fgets()` berikutnya tidak error
- Penanganan input tidak valid dengan pesan error dan looping kembali ke menu

**Saat keluar (pilih `0`):**
- Semua log dicatat
- Seluruh memori dibebaskan: `free_all_logs()`, `free_cert_tree()`, `free_graph()`
- Winsock dibersihkan (`WSACleanup()`) di Windows
