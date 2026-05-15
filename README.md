# 🔍 Real SSL/TLS & TCP Network Scanner

Scanner jaringan berbasis C yang dapat melakukan scan domain/IP secara real-time untuk mengecek:
- Konektivitas TCP dan latency
- Informasi sertifikat SSL/TLS (issuer, tanggal expire, protokol)
- **Peringatan otomatis** jika sertifikat sudah expired atau akan expire dalam 30 hari
- Topologi jaringan (Graph)
- Riwayat aktivitas (Log)

---

## 📋 Prasyarat

**Pendahuluan (Penjelasan Risk):** Tidak memenuhi prasyarat instalasi dapat menyebabkan kegagalan build. Risiko tidak terinstalnya OpenSSL adalah program tidak akan bisa memverifikasi sertifikat SSL, yang dapat membuat jaringan rentan jika sertifikat sebenarnya sudah invalid atau berbahaya.

### Linux / macOS

Install dependensi berikut sebelum build:

```bash
# Ubuntu / Debian
sudo apt update
sudo apt install gcc make libssl-dev libgtk-3-dev

# Arch Linux
sudo pacman -S gcc make openssl gtk3

# macOS (Homebrew)
brew install openssl gtk+3
```

### Windows

Gunakan salah satu dari berikut:

**Opsi A — MSYS2 (Direkomendasikan)**
1. Download dan install [MSYS2](https://www.msys2.org/)
2. Buka terminal **MSYS2 MINGW64** lalu jalankan:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-openssl mingw-w64-x86_64-gtk3 make
   ```
3. Gunakan terminal MSYS2 untuk langkah Build & Jalankan di bawah.

**Opsi B — WSL (Windows Subsystem for Linux)**
```bash
sudo apt update && sudo apt install gcc make libssl-dev libgtk-3-dev
```
Lalu ikuti langkah Linux di atas.

---

## 🔨 Build

**Pendahuluan (Penjelasan Risk):** Gagal dalam tahap build berarti aplikasi tidak dapat berjalan. Risiko menggunakan binary yang di-build dari source yang tidak diverifikasi dapat memicu celah keamanan.

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

**Pendahuluan (Penjelasan Risk):** Menjalankan scanner di lingkungan jaringan tanpa izin (unauthorized scanning) dapat dianggap sebagai tindakan intrusif atau scanning ilegal oleh IDS/IPS jaringan. Gunakan hanya pada jaringan/domain yang Anda miliki atau memiliki izin.

```bash
# Linux / macOS / MSYS2
./bin/scanner

# Windows (PowerShell / CMD, setelah build via MSYS2)
.\bin\scanner.exe
```

---

## 📖 Antarmuka GUI Program

**Pendahuluan (Penjelasan Risk):** Program sekarang menggunakan antarmuka grafis GTK3. Risiko jika environment tidak mendukung GTK (seperti server tanpa GUI) adalah program tidak dapat dijalankan. Penggunaan memori juga lebih tinggi dibanding CLI.

Program akan membuka jendela GTK dengan fitur:
- Input Domain / IP
- Tombol Scan
- Tombol Topologi Jaringan (Graph)
- Tombol Database Sertifikat (AVL)
- Tombol Activity Log (DLL)
- Tampilan teks hasil operasi

### Peringatan Sertifikat

| Indikator | Arti |
|-----------|------|
| `[!!!]` | Sertifikat sudah **KEDALUWARSA** |
| `[!]`   | Sertifikat akan expire dalam **≤ 30 hari** |
| *(kosong)* | Sertifikat masih valid |

---

## 📁 Struktur Direktori

**Pendahuluan (Penjelasan Risk):** Mengubah struktur direktori tanpa menyesuaikan `Makefile` dapat merusak proses kompilasi. Menghapus file header akan menghilangkan fungsi inti dari aplikasi.

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
│   ├── main.c            # Entry point & GUI GTK
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

**Pendahuluan (Penjelasan Risk):** Data dalam CSV disimpan dalam bentuk plaintext. Jika ada informasi sensitif dari log atau scan, ada risiko data bocor jika direktori `data/` tidak diamankan.

Hasil scan otomatis disimpan ke folder `data/` (dibuat otomatis):

- **`data/scan_results.csv`** — Hasil scan domain (latency, loss, cert info)
- **`data/activity_log.csv`** — Log semua aktivitas beserta timestamp

---

## ⚠️ Catatan

**Pendahuluan (Penjelasan Risk):** Mengabaikan catatan ini dapat mengakibatkan kegagalan fungsi aplikasi. Tidak adanya koneksi internet mengakibatkan DNS resolution gagal dan menghentikan proses scan.

- Program membutuhkan **koneksi internet aktif** untuk DNS resolution dan SSL handshake.
- Scan dilakukan ke **port 443** (HTTPS). Domain yang tidak mendukung HTTPS akan menampilkan info SSL sebagai `Unknown`.
- Semua data disimpan **in-memory** selama program berjalan dan **di-free** saat keluar (pilih `0`).
