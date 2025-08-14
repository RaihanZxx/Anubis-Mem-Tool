#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <fstream>
#include <android/log.h>

#include "zygisk.hpp"
#include "dobby.h"

// Macro untuk mempermudah logging ke logcat
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "AnubisGuardian", __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

// Pointer untuk menyimpan fungsi fopen yang asli
static FILE* (*original_fopen)(const char* pathname, const char* mode);

// Fungsi fopen hasil hook kita
static FILE* hook_fopen(const char* pathname, const char* mode) {
    if (pathname == nullptr) {
        return original_fopen(pathname, mode);
    }

    // Cek jika game mencoba membaca /proc/self/maps
    if (strcmp(pathname, "/proc/self/maps") == 0) {
        LOGD("INTERCEPTED: XignCode is trying to read /proc/self/maps. Serving clean version.");

        // Membuat nama file sementara yang unik untuk proses ini
        std::string clean_maps_path = "/data/local/tmp/clean_map_" + std::to_string(getpid()) + ".txt";
        
        std::ifstream original_maps("/proc/self/maps");
        std::ofstream clean_maps(clean_maps_path);

        std::string line;
        while (std::getline(original_maps, line)) {
            // Jika baris TIDAK mengandung string mencurigakan, tulis ke file bersih kita
            if (line.find("frida") == std::string::npos &&
                line.find("gadget") == std::string::npos &&
                line.find("re.zyg.fri") == std::string::npos &&
                line.find("Dobby") == std::string::npos &&
                line.find("AnubisGuardian") == std::string::npos) { // Sembunyikan modul kita juga!
                clean_maps << line << std::endl;
            }
        }
        
        original_maps.close();
        clean_maps.close();

        // Kembalikan file handle dari file yang sudah kita bersihkan
        // Setelah fopen, sistem akan menghapus file sementara ini, jadi tidak akan menumpuk
        FILE* result = original_fopen(clean_maps_path.c_str(), mode);
        remove(clean_maps_path.c_str()); // Hapus file sementara setelah dibuka
        return result;
    }
    
    // Jika bukan file yang kita targetkan, panggil fungsi asli
    return original_fopen(pathname, mode);
}


class AnubisGuardianModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        // Ambil nama proses aplikasi dari argumen
        auto app_name_jstring = args->nice_name;
        if (app_name_jstring == nullptr) return;

        const char *app_name = this->env->GetStringUTFChars(app_name_jstring, nullptr);
        if (app_name == nullptr) return;

        // ===================================================================
        // PENTING: Ganti "com.gameparadiso.milkchoco" dengan paket game Anda!
        // ===================================================================
        const char* target_app_name = "com.gameparadiso.milkchoco"; 

        bool is_target_app = strcmp(app_name, target_app_name) == 0;

        // Jangan lupa bebaskan memori setelah selesai membandingkan
        this->env->ReleaseStringUTFChars(app_name_jstring, app_name);

        // Jika bukan aplikasi target, hentikan eksekusi
        if (!is_target_app) {
            return;
        }

        LOGD("Target application [%s] detected. Applying hooks.", target_app_name);

        int dobby_result = DobbyHook(
            (void *)fopen,
            (void *)hook_fopen,
            (void **)&original_fopen
        );

        if (dobby_result == 0) {
            LOGD("fopen hook successful!");
        } else {
            LOGD("fopen hook FAILED with code: %d", dobby_result);
        }
    }

private:
    Api *api;
    JNIEnv *env;
};

// Daftarkan modul kita agar bisa dikenali oleh Zygisk
REGISTER_ZYGISK_MODULE(AnubisGuardianModule)