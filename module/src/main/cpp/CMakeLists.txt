cmake_minimum_required(VERSION 3.22.1)
# Ganti 'sample' dengan nama proyek Anda, misalnya 'AnubisGuardian'
# Pastikan ini cocok dengan moduleId di build.gradle.kts
project(AnubisGuardian)

# --- BLOK ASLI DARI TEMPLATE (JANGAN DIUBAH) ---
set(LINKER_FLAGS "-ffixed-x18 -Wl,--hash-style=both")
set(CXX_FLAGS "${CXX_FLAGS} -fno-exceptions -fno-rtti -fvisibility=hidden -fvisibility-inlines-hidden")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")
# --- AKHIR BLOK ASLI ---

# 1. Ganti 'example.cpp' dengan nama file kode sumber Anda, yaitu 'shield.cpp'
#    Ganti '${MODULE_NAME}' dengan nama modul Anda 'AnubisGuardian'
add_library(AnubisGuardian SHARED shield.cpp)

# 2. Tambahkan baris-baris ini untuk menautkan Dobby
#    Target link libraries yang sudah ada ('cxx' dan 'log') kita pertahankan.
target_link_libraries(AnubisGuardian
    # Library dari template
    cxx
    log

    # --- TAMBAHAN KITA ---
    # Path ke library dobby statis (.a)
    ${CMAKE_CURRENT_SOURCE_DIR}/dobby/${CMAKE_ANDROID_ARCH_ABI}/libdobby.a
)

# 3. Tambahkan baris ini untuk memberitahu compiler di mana menemukan 'dobby.h'
target_include_directories(AnubisGuardian PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dobby
)

# Baris ini dari template asli, biarkan saja.
add_subdirectory(external)