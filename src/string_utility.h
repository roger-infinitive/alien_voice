#ifndef STRING_UTILITY_H
#define STRING_UTILITY_H

#include "utility.h"
#include <string.h>
#include <ctype.h>

u32 CStringLength(const char* str) {
    if (str == 0) { return 0; }
    const char* current = str;
    while (current[0]) {
        ++current;
    }
    return current - str;
}

struct String {
    char* buffer;
    u32 length;
    
    inline char operator[](u32 index) {
        return buffer[index];
    }
};

String InitString(char* cString) {
    String str = {};
    str.buffer = cString;
    str.length = CStringLength(cString);
    return str;
}

struct DynamicString {
    char* buffer;
    u32 length;
    u32 capacity;
    Allocator allocator;
    
    inline char operator[](u32 index) {
        return buffer[index];
    }
};

String ToString(DynamicString* dynamicString) {
    String string;
    string.buffer = dynamicString->buffer;
    string.length = dynamicString->length;
    return string;
}

void InitDynamicString(DynamicString* string, Allocator allocator, u32 capacity) {
    string->buffer = (char*)allocator.alloc(capacity);
    memset(string->buffer, 0, capacity);
    string->length = 0;
    string->capacity = capacity;
    string->allocator = allocator;
}

void Resize(DynamicString* string, size_t size) {
    char* buffer = (char*)string->allocator.alloc(size);
    size_t copySize = (string->length < size - 1) ? string->length : size - 1;
    
    memcpy(buffer, string->buffer, copySize);
    buffer[copySize] = '\0';
    
    string->allocator.free(string->buffer);
    string->buffer = buffer;
    string->capacity = size; 
    string->length = copySize;
}

void SetDynamicString(DynamicString* string, const char* cStr) {
    u32 length = CStringLength(cStr);
    
    if (length >= string->capacity) {
        string->buffer = (char*)string->allocator.alloc(length + 1);
        string->capacity = length;
    }
    
    if (string->buffer == 0) {
        return;
    }

    memcpy(string->buffer, cStr, length);
    string->buffer[length] = 0;
    string->length = length;
}

// NOTE(roger): This will truncate if the cString isn't big enough.
void CopyDynamicStringToCString(DynamicString* string, char* cString, u32 cStringCapacity) {
    u32 maxLength = (cStringCapacity - 1) <  string->length ? (cStringCapacity - 1) : string->length;
    memcpy(cString, string->buffer, maxLength);
    cString[maxLength] = 0;
}

int ContainsInvalidFileCharacters(const char *filename);
int inf_sscanf(char* str, const char* format, ...);

bool ExtractFilename(const char* path, char* buffer, size_t bufSize);
bool ExtractDirectory(const char* filePath, char* buffer, size_t bufSize);

char* EatWhitespace(char* start) {
    while ((start[0] == ' ' || start[0] == '\t' || start[0] == '\n' || start[0] == '\r') && start[1] != '\0') {
        start++;
    }
    return start;
}

void ToLowerCase(const char* src, char* dest, u32 destSize) {
    u32 i;
    for (i = 0; src[i] && i < destSize - 1; ++i) {
        dest[i] = tolower((u8)src[i]);
    }
    dest[i] = '\0';
}

void ToLowerCase(char* str, int length) {
    for (int i = 0; i < length; i++) {
        str[i] = tolower(str[i]);
    }
}

bool IsEndOfLine(char c) {
    return c == '\n' || c == '\r';
}

bool IsWhitespace(char c) {
    return (c == ' '   || 
            c == '\t'  || 
            IsEndOfLine(c));
}

bool IsAlpha(char c) {
    return (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')));
}

bool IsNumber(char c) {
    return ((c >= '0') && (c <= '9'));
}

bool ContainsString(const char* string, const char* check) {
    int stringLength = CStringLength(string);
    int checkLength  = CStringLength(check);
    
    if (checkLength > stringLength) {
        return false;
    }
    
    bool contains = false;
    int checkIndex = 0;
    int stringIndex = 0;
    for (;;) {
        if (check[checkIndex] != string[stringIndex]) {
            checkIndex = 0;
        } else {
            checkIndex++;
            if (checkIndex >= checkLength) {
                contains = true;
            }
        }
        
        stringIndex++;
        if (stringIndex >= stringLength) {
            break;
        }
    }
    
    return contains;
}

void OverwritePathSeparators(char* path) {
    u32 index = 0;
    for (;;) {
        if (path[index] == '\\') {
            path[index] = '/';
        }
        
        index++;
        if (path[index] == '\0') {
            break;
        }
    }
}

void TruncateLastDirectory(char* directory, u32 length) {
    for (u32 i = length; i > 0; --i) {
        if (directory[i-1] == '\\' || directory[i-1] == '/') {
            directory[i-1] = 0;
            break;
        }
    }
}

char* GetSubstringBeforeHash(const char* str, AllocFunc alloc) {
    const char* hash = strchr(str, '#');
    size_t length = hash ? (size_t)(hash - str) : strlen(str);
    
    char* result = (char*)alloc(length + 1);
    if (!result) return 0;

    memcpy(result, str, length);
    result[length] = '\0';
    return result;
}

bool StringEqualsIgnoreCase(const char* a, const char* b);
char* ToSnakeCase(const char* src);

void PrintBytes(void* data, size_t size) {
    u8* bytes = (u8*)data;
    for (size_t i = 0; i < size; ++i) {
        printf("%02X ", bytes[i]);
    }
}

bool StringEquals(const char* a, int a_length, const char* b, int b_length) {
    if (a_length != b_length) {
        return false;
    }
    
    for (int j = 0; j < b_length; j++) {
        if (b[j] == a[j]) {
            continue;
        }
        return false;
    }
    
    return true;
}

#endif //STRING_UTILITY_H