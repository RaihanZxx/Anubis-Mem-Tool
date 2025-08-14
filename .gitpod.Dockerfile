FROM gitpod/workspace-full

# Instal prasyarat dasar
USER root
RUN sudo apt-get update && sudo apt-get install -y openjdk-17-jdk wget unzip

# Beralih kembali ke pengguna gitpod untuk keamanan
USER gitpod
WORKDIR /home/gitpod

# Atur variabel lingkungan yang akan digunakan di seluruh image
# Menggunakan NDK v26.1 yang sangat stabil untuk proyek Android
ENV ANDROID_HOME="/home/gitpod/android-sdk"
ENV ANDROID_NDK_HOME="${ANDROID_HOME}/ndk/26.1.10909125"
ENV PATH="${PATH}:${ANDROID_HOME}/cmdline-tools/latest/bin:${ANDROID_HOME}/platform-tools"

# Unduh, instal, dan bersihkan Android SDK
RUN mkdir -p ${ANDROID_HOME} && \
    wget -q https://dl.google.com/android/repository/commandlinetools-linux-10406996_latest.zip -O cmdline-tools.zip && \
    unzip -q cmdline-tools.zip -d ${ANDROID_HOME} && \
    mv ${ANDROID_HOME}/cmdline-tools ${ANDROID_HOME}/cmdline-tools-temp && \
    mkdir -p ${ANDROID_HOME}/cmdline-tools && \
    mv ${ANDROID_HOME}/cmdline-tools-temp ${ANDROID_HOME}/cmdline-tools/latest && \
    rm cmdline-tools.zip

# Terima lisensi dan instal paket NDK yang benar
RUN yes | sdkmanager --licenses > /dev/null && \
    sdkmanager "platform-tools" "ndk;26.1.10909125" > /dev/null