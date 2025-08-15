#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>  // Untuk snprintf
#include <cctype>  // Untuk isxdigit, isprint

namespace KittyUtils {

    void trim_string(std::string &str);
    bool validateHexString(std::string &xstr);
    void toHex(void *const data, const size_t dataLength, std::string &dest);
    void fromHex(const std::string &in, void *const data);

    template <size_t rowSize=8, bool showASCII=true>
    std::string HexDump(const void *address, size_t len)
    {
        if (!address || len == 0 || rowSize == 0)
            return ""; 

        const unsigned char *data = static_cast<const unsigned char *>(address);
        std::string result;
        // Perkirakan ukuran untuk menghindari realokasi berulang
        result.reserve(len * 4); 

        char line_buffer[rowSize * 4 + 20];
        char ascii_buffer[rowSize + 1];

        for (size_t i = 0; i < len; i += rowSize)
        {
            char* line_ptr = line_buffer;

            // Offset
            line_ptr += snprintf(line_ptr, sizeof(line_buffer) - (line_ptr - line_buffer), "%08zX: ", i);

            // Baris byte heksadesimal
            size_t j = 0;
            for (j = 0; (j < rowSize) && ((i + j) < len); j++) {
                line_ptr += snprintf(line_ptr, sizeof(line_buffer) - (line_ptr - line_buffer), "%02X ", data[i + j]);
                ascii_buffer[j] = std::isprint(data[i + j]) ? data[i + j] : '.';
            }
            ascii_buffer[j] = '\0';

            // Isi sisa spasi kosong
            for (; j < rowSize; j++) {
                line_ptr += snprintf(line_ptr, sizeof(line_buffer) - (line_ptr - line_buffer), "   ");
            }

            // Gabungkan dengan ASCII jika diaktifkan
            if (showASCII) {
                result.append(line_buffer);
                result.append(" ");
                result.append(ascii_buffer);
                result.append("\n");
            } else {
                result.append(line_buffer);
                result.append("\n");
            }
        }

        return result;
    }

}