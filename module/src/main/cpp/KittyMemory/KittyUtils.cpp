#include "KittyUtils.h"

namespace KittyUtils {

    void trim_string(std::string &str) 
    {
        str.erase(std::remove_if(str.begin(), str.end(), [](char c)
                                 { return (c == ' ' || c == '\n' || c == '\r' ||
                                           c == '\t' || c == '\v' || c == '\f'); }),
                  str.end());
    }

    bool validateHexString(std::string &hex) 
    {
        if (hex.empty()) return false;

        if (hex.compare(0, 2, "0x") == 0)
            hex.erase(0, 2);

        trim_string(hex);
        
        if (hex.length() < 2 || hex.length() % 2 != 0) return false;

        for (size_t i = 0; i < hex.length(); i++) {
            if (!std::isxdigit((unsigned char) hex[i]))
                return false;
        }
        
        return true;
    }

    // --- Versi BEBAS IOSTREAM dari toHex ---
    void toHex(
            void *const data,
            const size_t dataLength,
            std::string &dest
    ) {
        if (!data || dataLength == 0) {
            dest.clear();
            return;
        }

        unsigned char *byteData = reinterpret_cast<unsigned char *>(data);
        dest.resize(dataLength * 2);
        char* buffer = &dest[0];

        for (size_t i = 0; i < dataLength; ++i) {
            snprintf(buffer + (i * 2), 3, "%02X", byteData[i]);
        }
    }

    // --- Versi BEBAS IOSTREAM dari fromHex ---
    void fromHex(
            const std::string &in,
            void *const data
    ) {
        size_t length = in.length();
        unsigned char *byteData = reinterpret_cast<unsigned char *>(data);

        for (size_t strIndex = 0, dataIndex = 0; strIndex < length; ++dataIndex) {
            char tmpStr[3] = {in[strIndex++], in[strIndex++], 0};
            byteData[dataIndex] = static_cast<unsigned char>(strtol(tmpStr, nullptr, 16));
        }
    }

}