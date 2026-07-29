#pragma once

#include <cstdio>
#include <cstdlib>

class Filesystem {
public:
    // Reads entire file into a newly allocated char buffer.
    // The buffer is null-terminated but may contain null chars inside.
    // On success, returns pointer to buffer and sets outSize to file size.
    // On failure, returns nullptr and outSize is 0.
    static char* readFile(const char* filename, size_t* outSize) {
        outSize = 0;
#ifdef _WIN32
        FILE* file = NULL;
        ::fopen_s(&file, filename, "rb");
#else
        FILE* file = ::fopen(filename, "rb");
    #ifdef __EMSCRIPTEN__
        printf("[Filesystem::readFile]: Reading file '%s'\n", filename);
    #endif // __EMSCRIPTEN
#endif // _WIN32
        if (!file) return nullptr;

        // Seek to end to get size
        if (std::fseek(file, 0, SEEK_END) != 0) {
            std::fclose(file);
            return nullptr;
        }
        long fileSize = std::ftell(file);
        if (fileSize < 0) {
            std::fclose(file);
            return nullptr;
        }
        std::rewind(file);

        // Allocate buffer (+1 for null terminator)
        char* buffer = (char*)std::malloc(fileSize + 1);
        if (!buffer) {
            std::fclose(file);
            return nullptr;
        }

        size_t readBytes = std::fread(buffer, 1, fileSize, file);
        std::fclose(file);

        if (readBytes != (size_t)fileSize) {
            std::free(buffer);
            return nullptr;
        }

        buffer[fileSize] = '\0';  // Null terminate for safety
        if (outSize != NULL) { *outSize = fileSize; }
        return buffer;
    }

    // Writes the buffer to the specified file.
    // Returns true on success, false on failure.
    static bool writeFile(const char* filename, const char* data, size_t size) {
#ifdef _WIN32
        FILE* file = NULL;
        ::fopen_s(&file, filename, "wb");
#else
        FILE* file = ::fopen(filename, "wb");
#endif // _WIN32
        if (!file) return false;

        size_t written = std::fwrite(data, 1, size, file);
        std::fclose(file);
        return (written == size);
    }
};
