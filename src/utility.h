#ifndef UTILITY_H
#define UTILITY_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cstdint>
#include <string.h>
#include <climits>

#define local_persist static
#define internal static
#define global

#ifdef _MSC_VER
#pragma section(".roglob", read)
#define read_only __declspec(allocate(".roglob"))
#else
#define read_only   // no-op on GCC/Linux
#endif

#define MAX_NAME_LENGTH 128
#define MAX_PATH_LENGTH 512

#define DECLARE_ENUM(name, type) typedef type name; enum : type

#define countOf(_array) (sizeof(_array) / sizeof(*_array))
#define MIN_SIGNED_INT -2147483648
#define KILOBYTES(x) ((x) * 1024)
#define MEGABYTES(x) ((x) * 1024 * 1024)

#define ZeroStruct(ptr) memset((ptr), 0, sizeof(*(ptr)))
#define ZeroArray(ptr, count) memset((ptr), 0, sizeof(*(ptr)) * (count))

#define CopyMem(dest, src) do { \
    static_assert(sizeof(*(dest)) == sizeof(*(src)), "Cannot copy mem with different sizes."); \
    memcpy((dest), (src), sizeof(*(dest))); \
} while(0)

#define CopyArray(dest, src, count) do { \
    static_assert(sizeof(*(dest)) == sizeof(*(src)), "Cannot copy arrays with different element sizes."); \
    memcpy((dest), (src), (count) * sizeof(*(dest))); \
} while(0)

#define StructEqual(a, b) (memcmp(a, b, sizeof(*(a))) == 0)

#if EDITOR_MODE || _DEBUG
    #include <cstdio>
    #define LOG_MESSAGE(...) printf(__VA_ARGS__)
#else
    #define LOG_MESSAGE(...)
#endif

//Services provided by the platform layer.
#ifndef ASSERT_MESSAGE_BOX
    void ShowDebugMessageBox(const char* message) { LOG_MESSAGE(message); }
    void ShowErrorMessageBox(const char* message) { LOG_MESSAGE(message); }
#else
    void ShowDebugMessageBox(const char* message);
    void ShowErrorMessageBox(const char* message);
#endif

char ASSERT_MESSAGE[2048];
char ASSERT_BOX_MESSAGE[2048];

#if defined(_MSC_VER)
    #define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__)
    #define DEBUG_BREAK() __builtin_trap()
#else
    #define DEBUG_BREAK() raise(SIGTRAP)
#endif

// TODO(roger): We need to disable ERROR or handle them gracefully for full release.

#if defined(_MSC_VER)

    #define ASSERT_WARNING(condition, message, ...) \
        do { \
            if (!(condition)) { \
                sprintf(ASSERT_MESSAGE, message, __VA_ARGS__); \
                LOG_MESSAGE("WARNING: %s\n\tAssertion failed: %s\n\tFile: %s, Line: %d\n", \
                    ASSERT_MESSAGE, #condition, __FILE__, __LINE__); \
            } \
        } while (0)

    #if EDITOR_MODE || _DEBUG
        #define ASSERT_DEBUG(condition, message, ...) \
            do { \
                if (!(condition)) { \
                    sprintf(ASSERT_MESSAGE, message, __VA_ARGS__); \
                    sprintf(ASSERT_BOX_MESSAGE, "DEBUG: %s\n\tAssertion failed: %s\n\tFile: %s, Line: %d\n", \
                        ASSERT_MESSAGE, #condition, __FILE__, __LINE__); \
                    ShowDebugMessageBox(ASSERT_BOX_MESSAGE); \
                } \
            } while (0)
    #else
        #define ASSERT_DEBUG(condition, message, ...) //do nothing
    #endif

    #define ASSERT_ERROR(condition, message, ...) \
        do { \
            if (!(condition)) { \
                sprintf(ASSERT_MESSAGE, message, __VA_ARGS__); \
                sprintf(ASSERT_BOX_MESSAGE, "ERROR: %s\n\tAssertion failed: %s\n\tFile: %s, Line: %d\n", \
                    ASSERT_MESSAGE, #condition, __FILE__, __LINE__); \
                ShowErrorMessageBox(ASSERT_BOX_MESSAGE); \
            } \
        } while (0)

#elif defined(__GNUC__)

    #define ASSERT_WARNING(condition, message, ...) \
        do { \
            if (!(condition)) { \
                sprintf(ASSERT_MESSAGE, message, ##__VA_ARGS__); \
                LOG_MESSAGE("WARNING: %s\n\tAssertion failed: %s\n\tFile: %s, Line: %d\n", \
                    ASSERT_MESSAGE, #condition, __FILE__, __LINE__); \
            } \
        } while (0)

    #if EDITOR_MODE || _DEBUG
        #define ASSERT_DEBUG(condition, message, ...) \
            do { \
                if (!(condition)) { \
                    sprintf(ASSERT_MESSAGE, message, ##__VA_ARGS__); \
                    sprintf(ASSERT_BOX_MESSAGE, "DEBUG: %s\n\tAssertion failed: %s\n\tFile: %s, Line: %d\n", \
                        ASSERT_MESSAGE, #condition, __FILE__, __LINE__); \
                    ShowDebugMessageBox(ASSERT_BOX_MESSAGE); \
                } \
            } while (0)
    #else
        #define ASSERT_DEBUG(condition, message, ...) //do nothing
    #endif

    #define ASSERT_ERROR(condition, message, ...) \
        do { \
            if (!(condition)) { \
                sprintf(ASSERT_MESSAGE, message, ##__VA_ARGS__); \
                sprintf(ASSERT_BOX_MESSAGE, "ERROR: %s\n\tAssertion failed: %s\n\tFile: %s, Line: %d\n", \
                    ASSERT_MESSAGE, #condition, __FILE__, __LINE__); \
                ShowErrorMessageBox(ASSERT_BOX_MESSAGE); \
            } \
        } while (0)

#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define NOT_IMPLEMENTED() ASSERT_DEBUG(false, "Not Implemented!")

#include <stdint.h>
typedef int8_t   s8;
typedef uint8_t  u8;
typedef int16_t  s16;
typedef uint16_t u16;
typedef int32_t  s32;
typedef uint32_t u32;
typedef int64_t  s64;
typedef uint64_t u64;

#ifdef TRACE
    struct Trace {
        char* file;
        int lineNumber;
    };
    #define LOG_TRACE(message, ...) \
        printf("%s:%d: ", __FILE__, __LINE__); \
        printf(message, __VA_ARGS__);
#else
    #define LOG_TRACE(message, ...) //do nothing
#endif

unsigned int Align(unsigned int value, unsigned int alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

typedef void* (*AllocFunc)(size_t size);
typedef void* (*ReallocFunc)(void*, size_t size);
typedef void (*FreeFunc)(void*);

struct Allocator {
    AllocFunc alloc;
    ReallocFunc realloc;
    FreeFunc free;
};

void* HeapAlloc(size_t size) {
    return malloc(size);
}

void HeapFree(void* mem) {
    free(mem);
}

void* HeapRealloc(void* mem, size_t size) {
    return realloc(mem, size);
}

Allocator HeapAllocator = {
    HeapAlloc,
    HeapRealloc,
    HeapFree
};

void FreeStub(void* mem) {
    // Do Nothing
}

#define ALLOC_ARRAY(allocator, type, count) \
    ((type*)((allocator).alloc((size_t)(count) * sizeof(type))))

#if EDITOR_MODE || _DEBUG
    #define MEM_ALLOC_SIGNATURE(name) void* name(size_t size, const char* file, int line)
    #define MEM_ALLOC_PARAM void* (*mem_alloc)(size_t, const char*, int)
    #define MEM_ALLOC_ARRAY(count, type) (type*)mem_alloc(count * sizeof(type), __FILE__, __LINE__);
#else
    #define MEM_ALLOC_SIGNATURE(name) void* name(size_t size)
    #define MEM_ALLOC_PARAM void* (*mem_alloc)(size_t)
    #define MEM_ALLOC_ARRAY(count, type) (type*)mem_alloc(count * sizeof(type));
#endif

void OpenUrl(const char* url);
double SecondsPerCount();
double CurrentTimeInSeconds();
s64 CurrentTimeInMilliseconds();
s64 CurrentTimeCount();

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

// Return 64-bit FNV-1a hash for key. See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
u64 FNV1a(u64 key) {
    u64 hash = FNV_OFFSET;
    hash ^= key;
    hash *= FNV_PRIME;
    return hash;
}

#define FNV32_OFFSET 2166136261u
#define FNV32_PRIME  16777619u

constexpr u32 FNV1a_32(const char* str, size_t length, u32 hash = FNV32_OFFSET) {
    return length == 0 ? hash : FNV1a_32(str + 1, length - 1, (hash ^ (u8)(*str)) * FNV32_PRIME);
}

constexpr u32 ConstHash(const char* str) {
    size_t len = 0;
    while (str[len] != '\0') ++len;
    return FNV1a_32(str, len);
}

//NOTE(roger): This is only considered unique if the caller stores
//  and checks if the unique id is claimed. For assets, we store the ids and check
//  if there is a collision. If there is, then we continue to generate an id until there
//  isn't any collisions.

u32 GenerateUniqueId() {
    // Get the current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    // Generate a random number
    int randomNumber = rand() % 10000;

    // Construct the ID
    u32 id = ((t->tm_year % 100) * 100000000) + // Last two digits of the year
             ((t->tm_mon + 1) * 1000000) +      // Two-digit month
             (t->tm_mday * 10000) +             // Two-digit day
             (t->tm_hour * 100) +               // Two-digit hour
             randomNumber;                      // Random number

    return id;
}

#endif