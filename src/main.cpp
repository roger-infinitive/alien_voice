#include <stdio.h>
#include "file_io.h"
#include "time.h"

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

struct Timer {
    LARGE_INTEGER start;
};

Timer StartTimer() {
    Timer timer = {};
    QueryPerformanceCounter(&timer.start);
    return timer;
}

double StopTimer(Timer timer) {
    LARGE_INTEGER stop;
    QueryPerformanceCounter(&stop);
    
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    
    LONGLONG elapsedParts = stop.QuadPart - timer.start.QuadPart;
    double time = (elapsedParts * 1000.0f) / frequency.QuadPart; 
    return time;
}

enum ParsedTokenType {
    TokenType_Identifier,
    TokenType_EndOfLine,
    TokenType_EndOfStream
};

struct ParsedToken {
    ParsedTokenType type;
    int length;
    char* text;
};

struct Tokenizer {
    char* at;
};

struct CMU_Entry {
    ParsedToken key;
    ParsedToken value;
};

ParsedToken NextToken(Tokenizer* tokenizer) {
    ParsedToken token = {};
    if (tokenizer->at == 0 || tokenizer->at[0] == 0) {
        token.type = TokenType_EndOfStream;
        return token;
    }
    
    while (tokenizer->at[0] == ' ' || tokenizer->at[0] == '\t' || tokenizer->at[0] == '\r') {
        tokenizer->at++;
    }
        
    if (tokenizer->at[0] == '\n') {
        token.type = TokenType_EndOfLine;
        tokenizer->at++;
        return token;
    }
    
    token.type = TokenType_Identifier;    
    token.text = tokenizer->at;
    for (;;) {
        if (tokenizer->at[0] == 0 || IsWhitespace(tokenizer->at[0])) {
            break;
        }
        token.length++;
        tokenizer->at++;
    }
    
    return token;
}

ParsedToken NextTokenLine(Tokenizer* tokenizer) {
    ParsedToken token = {};
    if (tokenizer->at == 0 || tokenizer->at[0] == 0) {
        token.type = TokenType_EndOfStream;
        return token;
    }
    
    while (tokenizer->at[0] == ' ' || tokenizer->at[0] == '\t' || tokenizer->at[0] == '\r') {
        tokenizer->at++;
    }
        
    if (tokenizer->at[0] == '\n') {
        token.type = TokenType_EndOfLine;
        tokenizer->at++;
        return token;
    }
    
    token.type = TokenType_Identifier;    
    token.text = tokenizer->at;
    for (;;) {
        if (tokenizer->at[0] == '\n') {
            tokenizer->at++;
            break;
        } else if (tokenizer->at[0] == 0) {
            break;
        }
        
        token.length++;
        tokenizer->at++;
    }
    
    return token;
}

void ExtendToken(ParsedToken* token, ParsedToken extendTo) {
    token->length = (extendTo.text - token->text) + extendTo.length - 1;
}

int entry_count;
CMU_Entry* entries;

struct CMU_Cluster {
    char c;
    CMU_Entry* first;
    int count;
};

int total_clusters;
CMU_Cluster* clusters;

bool GetPhonesLinear(const char* search, ParsedToken* token) {
    int search_length = CStringLength(search);
    
    for (int i = 0; entry_count; i++) {
        CMU_Entry* entry = &entries[i];
        if (StringEquals(search, search_length, entry->key.text, entry->key.length)) {
            *token = entry->value;
            return true;
        }
    }
    
    return false;
}

bool GetPhones(const char* search, ParsedToken* token) {
    int search_length = CStringLength(search);
    
    for (int j = 0; j < total_clusters; j++) {
        CMU_Cluster* cluster = &clusters[j];
        if (cluster->c == search[0]) {
            for (int k = 0; k < cluster->count; k++) {
                CMU_Entry* entry = &cluster->first[k];
                if (StringEquals(search, search_length, entry->key.text, entry->key.length)) {
                    *token = entry->value;
                    return true;
                }
            }
        }            
    }
    
    return false;
}

int main(int argc, char** argv) {
    MemoryBuffer mb = {};
    if (!ReadEntireFileAndNullTerminate("data/cmudict.dict", &mb, HeapAllocator)) {
        fprintf(stderr, "Failed to read cmudict.dict.\n");
        return 1;
    }
    
    Tokenizer tokenizer = {};
    tokenizer.at = mb.buffer;
    
    for (;;) {
        if (tokenizer.at[0] == 0) {
            break;
        } else if (tokenizer.at[0] == '\n') {
            entry_count += 1;
        }
        tokenizer.at++;
    }

    printf("entry_count: %d\n", entry_count);
    entries = (CMU_Entry*)HeapAlloc(entry_count * sizeof(CMU_Entry));
    
    int current_entry = 0;
    tokenizer.at = mb.buffer;
    for (;;) {
        ParsedToken token = NextToken(&tokenizer);
        if (token.type == TokenType_EndOfStream) {
            break;
        }
        
        CMU_Entry* entry = &entries[current_entry];
        entry->key = token;
        entry->value = NextTokenLine(&tokenizer);
        current_entry++;
    }
    
    // Build an acceleration structure

    int MAX_CLUSTERS = 256;
    clusters = (CMU_Cluster*)HeapAlloc(MAX_CLUSTERS * sizeof(CMU_Cluster));
    
    CMU_Cluster* cluster = &clusters[0];
    cluster->c = entries[0].key.text[0];
    cluster->first = &entries[0];
    cluster->count = 1;
    total_clusters += 1;
    
    for (int i = 1; i < entry_count; i++) {
        char c = entries[i].key.text[0];    
        if (cluster->c != c) {
            if (total_clusters == MAX_CLUSTERS) {
                fprintf(stderr, "Not enough cluster storage!");
                return 1;
            }
        
            cluster = &clusters[total_clusters];
            cluster->c = entries[i].key.text[0];
            cluster->first = &entries[i];
            cluster->count = 1;
            total_clusters += 1;
        } else {
            cluster->count += 1;
        }
    }
    
    const char* search = "zwicker";
    
    int max_iterations = 10000;
    
    Timer timer = StartTimer();
    ParsedToken phones = {};
    for (int i = 0; i < max_iterations; i++) {
        GetPhonesLinear(search, &phones);
    }
    double ms = StopTimer(timer);
    printf("%.*s\n", phones.length, phones.text);
    printf("Time: %f ms\n", ms);

    printf("\n");
    timer = StartTimer();
    phones = {};
    for (int i = 0; i < max_iterations; i++) {
        GetPhones(search, &phones);
    }
    ms = StopTimer(timer);
    printf("%.*s\n", phones.length, phones.text);
    printf("Time: %f ms\n", ms);

    return 0;
}