#include <unistd.h>
#include <android/log.h>
#include <thread>
#include <chrono>
#include <cstdio>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "zygisk.hpp"
#include "dobby.h"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "Anubis-Mem-Tool", __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;

// ============================== PUSAT KONFIGURASI CHEAT ==============================
// Aktifkan atau nonaktifkan cheat yang Anda inginkan di sini.

// -- CHEAT DASAR / QoL --
const bool ENABLE_NO_RECOIL         = true;
const bool ENABLE_FASTER_RELOAD     = true;
const bool ENABLE_PERFECT_ACCURACY  = true;
const bool ENABLE_SLIGHT_RAPID_FIRE = true;
const bool ENABLE_NO_SCOPE_SWAY     = true; // Kamera 100% stabil saat membidik.

// -- CHEAT POWERFUL (Gunakan dengan bijak) --
const bool ENABLE_WALLHACK          = true;  // Melihat musuh menembus tembok.
const bool ENABLE_DAMAGE_REDUCTION  = true;  // Mengurangi damage yang diterima sebesar 50%.

// --- Daftar Offset Fungsi ---
// CharStatusCalculator
const unsigned long PROCESS_SHOOT_OFFSET      = 0x33bfb54; // No Recoil
const unsigned long GET_RELOAD_SPEED_OFFSET   = 0x35adc74; // Faster Reload
const unsigned long GET_AIM_SPREAD_OFFSET     = 0x35adca4; // Perfect Accuracy
const unsigned long GET_SHOOT_DELAY_OFFSET    = 0x35add94; // Slight Rapid Fire
// CEntityObject
const unsigned long PROCESS_GET_HURT_OFFSET   = 0x32439e8; // Damage Reduction
// CTarget
const unsigned long IS_VISIBLE_OFFSET         = 0x31d5b24; // Wallhack
// CameraActionFovTo
const unsigned long CAMERA_UPDATE_OFFSET      = 0x329acf8; // No Scope Sway

// Nama library game (digunakan sebagai sinyal)
const char* GAME_LIBRARY_NAME = "libMyGame.so";
// =====================================================================================

// --- Definisi Pointer & Fungsi Hook ---

// No Recoil
static void (*original_ProcessShoot)(void*);
void hooked_ProcessShoot(void*) { return; }

// Faster Reload
static float (*original_GetReloadSpeedRate)(void*);
float hooked_GetReloadSpeedRate(void* this_ptr) {
    return original_GetReloadSpeedRate(this_ptr) * 1.55f; // 35% lebih cepat
}

// Perfect Accuracy
static float (*original_GetAimSpreadShooting)(void*);
float hooked_GetAimSpreadShooting(void* this_ptr) {
    return 0.0f;
}

// Slight Rapid Fire
static float (*original_GetShootDelay)(void*);
float hooked_GetShootDelay(void* this_ptr) {
    return original_GetShootDelay(this_ptr) * 0.25f; // 25% lebih cepat
}

// Damage Reduction
static void (*original_ProcessGetHurt)(void*, short);
void hooked_ProcessGetHurt(void* this_ptr, short damage) {
    // Kurangi damage yang diterima sebesar 50%
    // Catatan: Ini mungkin juga berlaku untuk NPC/turret.
    short reduced_damage = damage * 0.5f;
    original_ProcessGetHurt(this_ptr, reduced_damage);
}

// Wallhack
static bool (*original_IsVisible)(void*);
bool hooked_IsVisible(void* this_ptr) {
    // Paksa game untuk berpikir semua target selalu terlihat.
    return true;
}

// No Scope Sway
static void (*original_CameraUpdate)(void*, float);
void hooked_CameraUpdate(void* this_ptr, float time) {
    // Abaikan fungsi update kamera, melumpuhkan goyangan (sway).
    return;
}


// --- Logika Pencarian Heuristik & Thread Utama ---
static uintptr_t find_best_guess_base_address() {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[1024];
    uintptr_t best_guess_base = 0;
    size_t max_size = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (!strstr(line, "r-xp") || strstr(line, "/system/") || strstr(line, "/apex/") || strstr(line, "zygisk")) continue;
        uintptr_t start, end;
        if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR, &start, &end) != 2) continue;
        size_t current_size = end - start;
        if (current_size > max_size) {
            max_size = current_size;
            best_guess_base = start;
        }
    }
    fclose(fp);
    if (best_guess_base != 0) LOGD("HEURISTIC_SUCCESS: Found base address %p", (void*)best_guess_base);
    return best_guess_base;
}

void do_hooking_thread() {
    LOGD("Hooking thread started. Waiting for %s...", GAME_LIBRARY_NAME);
    void* handle = nullptr;
    while ((handle = dlopen(GAME_LIBRARY_NAME, RTLD_NOLOAD)) == nullptr) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    dlclose(handle);
    LOGD("System confirmed %s is loaded. Starting heuristic search...", GAME_LIBRARY_NAME);
    
    uintptr_t base_address = find_best_guess_base_address();
    if (base_address == 0) {
        LOGD("FATAL: Heuristic search failed. Aborting.");
        return;
    }
    LOGD("Base address found: %p. Applying hooks...", (void*)base_address);
    
    // -- Pasang semua hook yang diaktifkan --
    if (ENABLE_NO_RECOIL) {
        void* addr = (void*)(base_address + PROCESS_SHOOT_OFFSET);
        DobbyHook(addr, (void*)hooked_ProcessShoot, (void**)&original_ProcessShoot);
        LOGD("Hooked [No Recoil] at %p", addr);
    }
    if (ENABLE_FASTER_RELOAD) {
        void* addr = (void*)(base_address + GET_RELOAD_SPEED_OFFSET);
        DobbyHook(addr, (void*)hooked_GetReloadSpeedRate, (void**)&original_GetReloadSpeedRate);
        LOGD("Hooked [Faster Reload] at %p", addr);
    }
    if (ENABLE_PERFECT_ACCURACY) {
        void* addr = (void*)(base_address + GET_AIM_SPREAD_OFFSET);
        DobbyHook(addr, (void*)hooked_GetAimSpreadShooting, (void**)&original_GetAimSpreadShooting);
        LOGD("Hooked [Perfect Accuracy] at %p", addr);
    }
    if (ENABLE_SLIGHT_RAPID_FIRE) {
        void* addr = (void*)(base_address + GET_SHOOT_DELAY_OFFSET);
        DobbyHook(addr, (void*)hooked_GetShootDelay, (void**)&original_GetShootDelay);
        LOGD("Hooked [Slight Rapid Fire] at %p", addr);
    }
    if (ENABLE_DAMAGE_REDUCTION) {
        void* addr = (void*)(base_address + PROCESS_GET_HURT_OFFSET);
        DobbyHook(addr, (void*)hooked_ProcessGetHurt, (void**)&original_ProcessGetHurt);
        LOGD("Hooked [Damage Reduction] at %p", addr);
    }
    if (ENABLE_WALLHACK) {
        void* addr = (void*)(base_address + IS_VISIBLE_OFFSET);
        DobbyHook(addr, (void*)hooked_IsVisible, (void**)&original_IsVisible);
        LOGD("Hooked [Wallhack] at %p", addr);
    }
    if (ENABLE_NO_SCOPE_SWAY) {
        void* addr = (void*)(base_address + CAMERA_UPDATE_OFFSET);
        DobbyHook(addr, (void*)hooked_CameraUpdate, (void**)&original_CameraUpdate);
        LOGD("Hooked [No Scope Sway] at %p", addr);
    }
}

// --- ZYGISK MODULE BOILERPLATE ---
class AnubisMemToolModule : public zygisk::ModuleBase {
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
        const char* target_package_name = "com.gameparadiso.milkchoco";
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

REGISTER_ZYGISK_MODULE(AnubisMemToolModule)