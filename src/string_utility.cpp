#include "string_utility.h"
#include <stdio.h>
#include <stdarg.h>
#include "tokenizer.h"

int ContainsInvalidFileCharacters(const char *filename) {
    // List of illegal characters for filenames in Windows and POSIX systems
    const char *illegal_chars = "<>:\"/\\|?*";

    while (*filename) {
        if (strchr(illegal_chars, *filename) != NULL) {
            return 1; // Found an illegal character
        }
        filename++;
    }
    return 0; // No illegal characters found
}

int inf_sscanf(char* str, const char* format, ...) {
    if (str == NULL) {
        return 0;
    }

    va_list args;
    va_start(args, format);

    str = EatWhitespace(str);

    int numConversions = 0;
    for (; *format; format++) {
        if (*format == '%') {
            format++;
            switch(*format) {
                case 'i':
                case 'd': {
                    int* iVal = va_arg(args, int*);
                    *iVal = strtol(str, (char**)&str, 10);
                    numConversions++;
                    break;
                }
                case 'f': {
                    float* fVal = va_arg(args, float*);
                    *fVal = strtod(str, (char**)&str);
                    numConversions++;
                    break;
                }
                case 'u': {
                    u32* u32Val = va_arg(args, u32*);
                    *u32Val = strtoul(str, (char**)&str, 10);
                    numConversions++;
                    break;
                }
                case 'h': {
                    format++;
                    if (*format == 'h' && format[1] == 'u') {
                        format++;

                        u8* u8Val = va_arg(args, u8*);
                        *u8Val = strtoul(str, (char**)&str, 10);
                        numConversions++;
                        break;
                    } else if (*format == 'u') {
                        u16* u16Val = va_arg(args, u16*);
                        *u16Val = strtoul(str, (char**)&str, 10);
                        numConversions++;
                        break;
                    }
                }
                case 'l': {
                    format++;
                    if (*format == 'l' && format[1] == 'u') {
                        format++;

                        u64* u64Val = va_arg(args, u64*);
                        *u64Val = strtoull(str, (char**)&str, 10);
                        numConversions++;
                    }
                    break;
                }
                case '*': {
                    format++;
                    if (*format == 's') {
                        int i = 0;
                        for (; str[i] != ' ' && str[i] != '\n' && str[i] != '\r' && str[i] != '\0'; i++);
                        str += i;
                        break;
                    }
                }
                default: {
                    if (isdigit(*format)) {
                        int number = strtol(format, (char**)&format, 10);
                        if (*format == 's') {
                            char* strVal = va_arg(args, char*);
                            int i = 0;
                            for (; i < number && str[i] != ' ' && str[i] != '\n' && str[i] != '\r' && str[i] != '\0'; i++) {
                                strVal[i] = str[i];
                            }
                            strVal[i] = '\0';
                            str += i;
                            numConversions++;
                            break;
                        }
                    }
                    ASSERT_DEBUG(false, "Format not supported: %s", format);
                    break;
                }
            }
        } else {
            str++;
        }
    }

    va_end(args);
    return numConversions;
}

bool ExtractFilename(const char* path, char* buffer, size_t bufSize) {
    const char* lastSlash = strrchr(path, '/');
    const char* lastBackSlash = strrchr(path, '\\');

    // Determine which separator is the last one
    const char* lastSeparator = (lastSlash > lastBackSlash) ? lastSlash : lastBackSlash;
    if (lastSeparator == NULL) {
        lastSeparator = path; // No separator found, assume path is the filename
    } else {
        lastSeparator++; // Move past the last separator
    }

    const char* lastDot = strrchr(lastSeparator, '.');
    if (lastDot == NULL) {
        lastDot = path + strlen(path); // No dot found, take the whole string
    }

    size_t filenameLength = lastDot - lastSeparator;
    if (filenameLength < bufSize) {
        strncpy(buffer, lastSeparator, filenameLength);
        buffer[filenameLength] = '\0'; // Null-terminate the string
    } else {
       return false;
    }

    return true;
}

bool ExtractDirectory(const char* filePath, char* buffer, size_t bufSize) {
    const char* lastSlash = strrchr(filePath, '/');
    const char* lastBackSlash = strrchr(filePath, '\\');

    // Determine which separator is the last one
    const char* lastSeparator = lastSlash > lastBackSlash ? lastSlash : lastBackSlash;
    if (lastSeparator != NULL) {
        size_t directoryLength = lastSeparator - filePath;
        if (directoryLength < bufSize) {
            strncpy(buffer, filePath, directoryLength);
            buffer[directoryLength] = '\0'; // Null-terminate the string
        } else {
            return false; //provided buffer is not large enough
        }
    } else {
        return false; //no directory (just file)
    }

    return true;
}

bool ExtractDirectory(const char* filePath, String* output) {
    Tokenizer tokenizer = {};
    tokenizer.at = (char*)filePath;

    //Find last slash
    Token lastSlash = {};
    bool parsing = true;
    while (parsing) {
        Token token = GetToken(&tokenizer);
        switch (token.type) {
            case Token_ForwardSlash:
            case Token_BackSlash: {
                lastSlash = token;
            } break;
            case Token_EndOfStream: {
                parsing = false;
            } break;
        }
    }

    if (lastSlash.type != Token_ForwardSlash && lastSlash.type != Token_BackSlash) {
        ASSERT_DEBUG("Unable to find the last slash in string: %s", filePath);
        return false;
    }

    u32 directoryLength = lastSlash.text - filePath;
    if (directoryLength < MAX_PATH_LENGTH) {
        memcpy(output->buffer, filePath, directoryLength);
        output->length = directoryLength;
        output->buffer[output->length] = 0;
    } else {
        ASSERT_DEBUG(false, "Directory length exceed MAX_PATH_LENGTH!");
        return false;
    }
    return true;
}

//Will ignore case and treat `_` == ` `
bool StringEqualsIgnoreCase(const char* a, const char* b) {
    if (!a || !b) {
        return (a == b);
    }
    
    while (*a && *b) {
        char ca = *a++;
        char cb = *b++;
        
        // Normalize underscores to spaces
        if (ca == '_') ca = ' ';
        if (cb == '_') cb = ' ';
        
        if (toupper((u8)ca) != toupper((u8)cb)) {
            return false;
        }
    }
    
    return (*a == '\0' && *b == '\0');
}

void ToSnakeCase(const char* src, char* dest, size_t destSize) {
    if (!src || destSize == 0) {
        return;
    }
    
    size_t destIndex = 0;
    int lastWasUnderscore = 0;
    
    u32 i;
    for (i = 0; src[i] && i < destSize - 1; ++i) {
        char c = src[i];
    
        if (IsWhitespace(c)) {
            if (!lastWasUnderscore && destIndex > 0) {
                dest[destIndex++] = '_';
                lastWasUnderscore = 1;
            }
        } else {
            dest[destIndex++] = tolower((u8)c);
            lastWasUnderscore = 0;
        }
    }
    
    dest[i] = '\0';
}
