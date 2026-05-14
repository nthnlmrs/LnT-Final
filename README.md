# 🔍 Real SSL/TLS & TCP Network Scanner

Scanner jaringan berbasis C yang dapat melakukan scan domain/IP secara real-time untuk mengecek:
- Konektivitas TCP dan latency
- Informasi sertifikat SSL/TLS (issuer, tanggal expire, protokol)
- **Peringatan otomatis** jika sertifikat sudah expired atau akan expire dalam 30 hari
- Topologi jaringan (Graph)
- Riwayat aktivitas (Log)

---

## 📋 Prasyarat

### Linux / macOS

Install dependensi berikut sebelum build:

```bash
# Ubuntu / Debian
sudo apt update
sudo apt install gcc make libssl-dev

# Arch Linux
sudo pacman -S gcc make openssl

# macOS (Homebrew)
brew install openssl
```

> **macOS**: Jika OpenSSL tidak ditemukan saat compile, tambahkan flag berikut ke `LDFLAGS` di Makefile:
> ```
> LDFLAGS = -lssl -lcrypto -L$(brew --prefix openssl)/lib
> CFLAGS  = -Wall -Iinclude -g -I$(brew --prefix openssl)/include
> ```

### Windows

Gunakan salah satu dari berikut:

**Opsi A — MSYS2 (Direkomendasikan)**
1. Download dan install [MSYS2](https://www.msys2.org/)
2. Buka terminal **MSYS2 MINGW64** lalu jalankan:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-openssl make
   ```
3. Gunakan terminal MSYS2 untuk langkah Build & Jalankan di bawah.

**Opsi B — WSL (Windows Subsystem for Linux)**
```bash
sudo apt update && sudo apt install gcc make libssl-dev
```
Lalu ikuti langkah Linux di atas.

---

## 🔨 Build

Masuk ke direktori project, lalu jalankan:

### Linux / macOS / MSYS2 Terminal
```bash
make
```

### Windows (PowerShell)
Jika menggunakan MSYS2 tetapi ingin build dari PowerShell:
```powershell
C:\msys64\usr\bin\bash.exe -c "export PATH=/mingw64/bin:/usr/bin:$PATH && make"
```

Output binary akan tersimpan di `bin/scanner` (Linux/macOS) atau `bin/scanner.exe` (Windows).

Untuk membersihkan hasil build:
```bash
make clean
```

---

## ▶️ Cara Menjalankan

```bash
# Linux / macOS / MSYS2
./bin/scanner

# Windows (PowerShell / CMD, setelah build via MSYS2)
.\bin\scanner.exe
```

---

## 📖 Menu Program

```
===================================
 REAL SSL/TLS & TCP SCANNER
===================================
1. Scan Domain / IP Baru
2. Lihat Topologi Jaringan (Graph)
3. Lihat Database Sertifikat (AVL)
4. Lihat Activity Log (DLL)
0. Keluar
```

| Menu | Fungsi |
|------|--------|
| **1** | Scan domain — cek latency TCP, SSL cert info, dan status expire |
| **2** | Tampilkan topologi jaringan semua device yang sudah di-scan |
| **3** | Tampilkan database sertifikat (AVL Tree, in-order / terurut) |
| **4** | Tampilkan riwayat aktivitas |
| **0** | Keluar dan bebaskan semua memori |

### Contoh Penggunaan

```
Pilih menu: 1
Masukkan Domain (contoh: google.com): google.com

[+] Memulai scan pada google.com...
[+] Scan selesai! Hasil sudah disimpan di memori dan CSV.
```

### Peringatan Sertifikat

| Indikator | Arti |
|-----------|------|
| `[!!!]` | Sertifikat sudah **KEDALUWARSA** |
| `[!]`   | Sertifikat akan expire dalam **≤ 30 hari** |
| *(kosong)* | Sertifikat masih valid |

---

## 📁 Struktur Direktori

```
.
├── include/
│   ├── common.h          # Header library standar & OpenSSL
│   ├── cert_db.h         # AVL Tree untuk database sertifikat
│   ├── csv_helper.h      # Helper simpan data ke CSV
│   ├── logger.h          # Doubly Linked List untuk activity log
│   ├── network_graph.h   # Graph untuk topologi jaringan
│   └── scanner.h         # Fungsi utama scan
├── src/
│   ├── main.c            # Entry point & menu utama
│   ├── cert_db.c         # Implementasi AVL Tree
│   ├── csv_helper.c      # Implementasi CSV writer
│   ├── logger.c          # Implementasi DLL logger
│   ├── network_graph.c   # Implementasi Graph
│   └── scanner.c         # Implementasi scan TCP & SSL
├── data/                 # (auto-generated) Hasil scan CSV
│   ├── scan_results.csv
│   └── activity_log.csv
├── Makefile
└── README.md
```

---

## 📊 Output CSV

Hasil scan otomatis disimpan ke folder `data/` (dibuat otomatis):

- **`data/scan_results.csv`** — Hasil scan domain (latency, loss, cert info)
- **`data/activity_log.csv`** — Log semua aktivitas beserta timestamp

---

## ⚠️ Catatan

- Program membutuhkan **koneksi internet aktif** untuk DNS resolution dan SSL handshake.
- Scan dilakukan ke **port 443** (HTTPS). Domain yang tidak mendukung HTTPS akan menampilkan info SSL sebagai `Unknown`.
- Semua data disimpan **in-memory** selama program berjalan dan **di-free** saat keluar (pilih `0`).
