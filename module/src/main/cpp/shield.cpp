#include <unistd.h>
#include <string>
#include <cstdio>
#include <android/log.h>

#include "zygisk.hpp"
#include "dobby.h"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "AnubisGuardian", __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

// Pointer untuk menyimpan fungsi fopen yang asli
static FILE* (*original_fopen)(const char* pathname, const char* mode);

// Fungsi fopen hasil hook kita (versi C-style I/O)
static FILE* hook_fopen(const char* pathname, const char* mode) {
    if (pathname == nullptr) {
        return original_fopen(pathname, mode);
    }

    if (strcmp(pathname, "/proc/self/maps") == 0) {
        LOGD("INTERCEPTED: XignCode is trying to read /proc/self/maps.");

        // Buat path file sementara menggunakan C-style snprintf
        char clean_maps_path[256];
        snprintf(clean_maps_path, sizeof(clean_maps_path), "/data/local/tmp/clean_map_%d.txt", getpid());

        // Buka file asli untuk dibaca
        FILE* original_fp = original_fopen("/proc/self/maps", "r");
        if (original_fp == nullptr) {
            LOGD("Failed to open original /proc/self/maps. Aborting hook.");
            return original_fopen(pathname, mode); // Kembalikan panggilan asli jika gagal
        }

        // Buka file sementara untuk ditulis
        FILE* clean_fp = original_fopen(clean_maps_path, "w");
        if (clean_fp == nullptr) {
            LOGD("Failed to open clean map file for writing. Aborting hook.");
            fclose(original_fp);
            return original_fopen(pathname, mode);
        }

        char line_buffer[1024];
        // Baca file asli baris per baris
        while (fgets(line_buffer, sizeof(line_buffer), original_fp) != nullptr) {
            std::string line_str(line_buffer);
            // Cek jika baris mengandung string mencurigakan
            if (line_str.find("frida") == std::string::npos &&
                line_str.find("gadget") == std::string::npos &&
                line_str.find("re.zyg.fri") == std::string::npos &&
                line_str.find("Dobby") == std::string::npos &&
                line_str.find("AnubisGuardian") == std::string::npos) {
                // Tulis baris yang bersih ke file sementara
                fputs(line_buffer, clean_fp);
            }
        }
        
        // Tutup kedua file pointer
        fclose(original_fp);
        fclose(clean_fp);

        LOGD("Clean map generated at %s. Serving it to XignCode.", clean_maps_path);

        // Kembalikan file pointer dari file yang sudah bersih
        FILE* result = original_fopen(clean_maps_path, mode);
        // Hapus file sementara setelah dibuka oleh proses target
        remove(clean_maps_path);
        return result;
    }
    
    return original_fopen(pathname, mode);
}


class AnubisGuardianModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        auto app_name_jstring = args->nice_name;
        if (app_name_jstring == nullptr) return;

        const char *app_name = this->env->GetStringUTFChars(app_name_jstring, nullptr);
        if (app_name == nullptr) return;
        
        // PENTING: Ganti dengan paket game Anda!
        const char* target_app_name = "com.gameparadiso.milkchoco"; 

        bool is_target_app = strcmp(app_name, target_app_name) == 0;
        this->env->ReleaseStringUTFChars(app_name_jstring, app_name);

        if (!is_target_app) {
            return;
        }

        LOGD("Target application [%s] detected. Applying fopen hook.", target_app_name);

        int dobby_result = DobbyHook(
            (void *)fopen, (void *)hook_fopen, (void **)&original_fopen);

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

REGISTER_ZYGISK_MODULE(AnubisGuardianModule)