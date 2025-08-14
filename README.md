<div align="center">
 <h1>Anubis Mem-Tool</h1>

 <p align="center">
   <strong>Sebuah modul Zygisk minimalis untuk <i>native function hooking</i> pada Android, dirancang untuk modifikasi game.</strong>
 </p>

 <p align="center">
   <img src="https://img.shields.io/badge/Language-C%2B%2B-blue.svg" alt="Language C++">
   <img src="https://img.shields.io/badge/License-MIT-green.svg" alt="License MIT">
   <img src="https://img.shields.io/badge/Status-Active-brightgreen.svg" alt="Status Active">
 </p>
</div>

Anubis Mem-Tool adalah modul Zygisk yang ringan dan kuat yang memungkinkan *hooking* langsung pada fungsi *native* di dalam aplikasi Android. Dengan memanfaatkan kekuatan Zygisk untuk injeksi tahap awal dan **Dobby** untuk *inline hooking* yang tangguh, modul ini menyediakan alternatif tersembunyi dibandingkan *framework* besar seperti Frida untuk analisis dan modifikasi game.

## ✨ Fitur Utama

*   **Injeksi via Zygisk:** Dimuat sangat awal dalam siklus hidup aplikasi, membuatnya sulit dideteksi.
*   **Native Function Hooking:** Menggunakan [Dobby](https://github.com/jmpews/Dobby) untuk memodifikasi fungsi target secara langsung di level *assembly*, memastikan performa maksimal.
*   **Konfigurasi Statis:** Didesain untuk satu target spesifik, menghilangkan jejak yang biasanya ditinggalkan oleh *server* atau *listener* eksternal.
*   **Jejak Minimal:** Tidak memiliki ketergantungan pada *framework* eksternal yang berat, menghasilkan ukuran yang sangat kecil dan *overhead* yang rendah.
*   **Sabar & Tangguh:** Secara cerdas menunggu hingga library target dimuat oleh aplikasi sebelum menerapkan *hook*, memastikan kompatibilitas dengan game yang menggunakan *Split APK*.

## 🔧 Prasyarat

*   **Untuk Pengguna:** Magisk atau KernelSU dengan **Zygisk diaktifkan**.
*   **Untuk Pengembang:** Android NDK (diatur melalui Android Studio atau manual).

## 🚀 Instalasi (Untuk Pengguna)

1.  Unduh rilis terbaru dari [Release](https://github.com/RaihanZxx/Anubis-Mem-Tool/releases).
2.  Buka aplikasi Magisk/KernelSU.
3.  Buka bagian `Modul`.
4.  Pilih `Instal dari penyimpanan` dan pilih file `.zip` yang telah diunduh.
5.  Reboot perangkat Anda.

## 🛠️ Membangun dari Sumber (Untuk Pengembang)

1.  **Clone repositori ini:**
    ```bash
    git clone https://github.com/RaihanZxx/Anubis-Mem-Tool
    cd Anubis-Mem-Tool
    ```

2.  **Buka proyek** di Android Studio atau jalankan build dari baris perintah:
    ```bash
    ./gradlew :module:zipRelease
    ```

3.  File `.zip` modul yang siap di-flash akan tersedia di direktori `module/release/`.

## ⚙️ Konfigurasi

Semua logika cheat dikonfigurasi secara statis di dalam kode. Untuk menargetkan game atau fungsi yang berbeda, edit file `module/src/main/cpp/shield.cpp`:

> **PENTING:** Anda harus mengkonfigurasi **3 variabel** ini agar modul berfungsi.

```cpp
// module/src/main/cpp/shield.cpp

// --- KONFIGURASI ---

// 1. Nama library target yang berisi fungsi.
const char* GAME_LIBRARY_NAME = "libMyGame.so";

// 2. Offset dari fungsi yang ingin di-hook.
const unsigned long PROCESS_SHOOT_OFFSET = 0x33bfb54;

// ... di dalam fungsi postAppSpecialize ...

// 3. Nama paket lengkap dari game target Anda.
const char* target_package_name = "com.Your.Game.PackageName";
```

## ⚠️ Penafian

Modul ini dibuat untuk tujuan pendidikan dan penelitian. Memodifikasi perangkat lunak dapat melanggar Persyaratan Layanan mereka. Penggunaan modul ini dapat mengakibatkan pemblokiran akun. Gunakan dengan risiko Anda sendiri.

## 📜 Lisensi

Kode unik dan kontribusi signifikan dalam proyek **Anubis Mem-Tool** ini dirilis di bawah **MIT License**. Anda dapat melihat detailnya di file `LICENSE`.

Harap dicatat bahwa proyek ini pada awalnya di-fork dari [Zygisk-Module-Sample](https://github.com/topjohnwu/zygisk-module-sample) oleh topjohnwu. Bagian dasar dan struktur template dari proyek tersebut tidak memiliki lisensi eksplisit dan semua hak atas kode tersebut tetap menjadi milik penulis aslinya. Lisensi MIT hanya berlaku untuk modifikasi dan kode baru yang diperkenalkan dalam fork ini.
