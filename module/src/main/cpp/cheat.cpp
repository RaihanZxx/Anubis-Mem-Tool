#include <unistd.h>
#include <android/log.h>
#include <thread>
#include <chrono>
#include <cstdio>
#include <dlfcn.h> // Header untuk dlopen

#include "zygisk.hpp"
#include "dobby.h"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "AnubisGuardian", __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

// --- KONFIGURASI ---
const char* GAME_LIBRARY_NAME = "libMyGame.so";
const unsigned long PROCESS_SHOOT_OFFSET = 0x33bfb54;
// --------------------

static void (*original_ProcessShoot)(void* this_ptr);

void hooked_ProcessShoot(void* this_ptr) {
    static bool hook_triggered = false;
    if (!hook_triggered) {
        LOGD("-------------------- RECOIL HOOK TRIGGERED! FUNGSI DILUMPUHKAN! --------------------");
        hook_triggered = true;
    }
    return;
}

// Fungsi get_module_base kita sekarang lebih bisa diandalkan
static uintptr_t get_module_base(const char* module_name) {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[1024];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), fp)) {
        // Kita hanya mencari nama file, bukan path lengkap, dan pastikan executable
        if (strstr(line, module_name) && strstr(line, "r-xp")) {
            base = (uintptr_t)strtoul(line, NULL, 16);
            break;
        }
    }
    fclose(fp);
    return base;
}

void do_hooking_thread() {
    LOGD("Hooking thread started. Waiting for %s to be loaded by the system...", GAME_LIBRARY_NAME);
    
    // PENDEKATAN BARU: Gunakan dlopen untuk memeriksa apakah library sudah dimuat
    void* handle = nullptr;
    while ((handle = dlopen(GAME_LIBRARY_NAME, RTLD_NOLOAD)) == nullptr) {
        // Jika handle adalah null, library belum siap. Tunggu.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    // Jika kita sampai di sini, library PASTI sudah dimuat. Tutup handle yang tidak kita perlukan lagi.
    dlclose(handle);

    LOGD("System has loaded %s! Now finding its base address.", GAME_LIBRARY_NAME);
    
    uintptr_t base_address = get_module_base(GAME_LIBRARY_NAME);
    if (base_address == 0) {
        LOGD("FATAL: Library loaded but could not find base address in /maps. Aborting.");
        return;
    }

    LOGD("%s found at address: %p", GAME_LIBRARY_NAME, (void*)base_address);
    
    // Coba dengan atau tanpa Thumb bit. Mulai dengan yang tanpa.
    void* target_address = (void*)(base_address + PROCESS_SHOOT_OFFSET);
    // void* target_address = (void*)(base_address + (PROCESS_SHOOT_OFFSET | 1));

    LOGD("Calculated target address: %p", target_address);

    int result = DobbyHook(target_address, (void*)hooked_ProcessShoot, (void**)&original_ProcessShoot);

    if (result == 0) {
        LOGD("DobbyHook SUCCESS! No recoil should be active.");
    } else {
        LOGD("DobbyHook FAILED with code: %d", result);
    }
}

class AnubisGuardianModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        auto app_name_jstring = args->nice_name;
        if (!app_name_jstring) return;

        const char *app_name = this->env->GetStringUTFChars(app_name_jstring, nullptr);
        if (!app_name) return;
        
        const char* target_package_name = "com.gameparadiso.milkchoco"; // GANTI DENGAN PAKET ANDA

        if (strcmp(app_name, target_package_name) == 0) {
            LOGD("Target application detected. Starting professional hooking thread.");
            std::thread(do_hooking_thread).detach();
        }
        this->env->ReleaseStringUTFChars(app_name_jstring, app_name);
    }
private:
    Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(AnubisGuardianModule)