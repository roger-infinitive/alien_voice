#ifndef FILE_IO_H
#define FILE_IO_H

#include "utility.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "string_utility.h"

/* PLATFORM SPECIFIC */
#ifdef _WIN64
    typedef void* FileHandle;
#elif defined(_WIN32)
    typedef void* FileHandle;
#elif defined(__linux__)
    typedef int FileHandle;
#else
    #error "Unsupported platform"
#endif

#if defined(_WIN32)
  #define fseek64 _fseeki64
  #define ftell64 _ftelli64
  typedef __int64 off64_t;
#else
  #define fseek64 fseeko
  #define ftell64 ftello
  typedef off_t off64_t;
#endif

#define WriteBufferSize 16*1024*1024 //16 MB

enum FileMode {
    FileMode_Read,
    FileMode_Write,
    FileMode_WriteRead
};

struct File {
    FileHandle handle;
    FileMode mode;
    u32 bufferPosition;
    u8* writeBuffer;
};

void MyCreateDirectory(const char* directory);
bool DirectoryExists(const char* directory);

bool GetAppDataFolder(char* directory);
void GetAssetDirectory(String* directory);

bool RemoveFile(const char* file);
File FileOpen(const char* filePath, FileMode mode);
void FileClose(File& file);
u64 FileWrite(File& file, void* buffer, u64 size);
//TODO(roger): Add FileRead API and remove the rest of fopen in codebase.

struct MemoryBuffer {
    char* buffer;
    size_t size;
    size_t position;
};

void ReadBytes(MemoryBuffer* mem, void* dest, size_t size) {
    memcpy(dest, mem->buffer + mem->position, size);
    mem->position += size;
}

char* PeekBytes(MemoryBuffer* mem) {
    ASSERT_ERROR(mem->position < mem->size, "Out of range for PeekBytes!");
    return mem->buffer + mem->position;
}

bool ReadEntireFileAndNullTerminate(const char* filePath, MemoryBuffer* outFile, Allocator allocator) {
    if (filePath == 0) {
        ASSERT_DEBUG(false, "Invalid filePath param!\n");
        return false;
    }

    FILE* file = fopen(filePath, "rb");
    if (file == 0) {
        ASSERT_DEBUG(false, "File %s failed to open: %s\n", filePath, strerror(errno));
        return false;
    }

    fseek64(file, 0, SEEK_END);
    long fileSize = ftell64(file);
    rewind(file);

    char* buffer = (char*)allocator.alloc(fileSize + 1);
    outFile->size = fileSize;
    if (buffer == 0) {
        fclose(file);
        return false;
    }

    size_t bytesRead = fread(buffer, 1, fileSize, file);
    if (bytesRead < fileSize) {
        if (allocator.free != 0) {
            allocator.free(buffer);
        }
        fclose(file);
        return false;
    }

    buffer[fileSize] = '\0';
    outFile->buffer = buffer;
    fclose(file);
    return true;
}

bool FileExists(const char* filePath) {
    FILE* f = fopen(filePath, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

#endif //FILE_IO_H