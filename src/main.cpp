#include <stdio.h>
#include "file_io.h"
#include "assert.h"

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
    
    int sub_cluster_count;
    CMU_Cluster* sub_clusters;
};

#define MAX_CLUSTERS 2048
int total_clusters;
CMU_Cluster root_cluster;
CMU_Cluster* clusters;

CMU_Cluster* InsertCluster(CMU_Entry* entry, int text_index) {
    assert(total_clusters < MAX_CLUSTERS);

    CMU_Cluster* cluster = &clusters[total_clusters];
    cluster->c = entry->key.text[text_index];
    cluster->first = entry;
    cluster->count = 1;
    cluster->sub_cluster_count = 0;
    cluster->sub_clusters = 0;
    
    total_clusters += 1;
    return cluster;
}

void BuildSubClusters(CMU_Cluster* cluster, int text_index) {
    CMU_Cluster* sub_cluster = InsertCluster(cluster->first, text_index);
    cluster->sub_clusters = sub_cluster;
    cluster->sub_cluster_count = 1;
            
    for (int j = 1; j < cluster->count; j++) {
        CMU_Entry* entry = &cluster->first[j];
        
        if (entry->key.length <= text_index) {
            sub_cluster->count += 1;
            continue;
        } 
        
        if (sub_cluster->c != entry->key.text[text_index]) {
            sub_cluster = InsertCluster(entry, text_index);
            cluster->sub_cluster_count += 1;
        } else {
            sub_cluster->count += 1;
        }
    }        
}

// Slow version (used to demonstrate speed differences), use GetPhones() instead.
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
    
    for (int i = 0; i < root_cluster.sub_cluster_count; i++) {
        CMU_Cluster* cluster = &root_cluster.sub_clusters[i];
        if (cluster->c == search[0]) {
            for (int j = 0; j < cluster->sub_cluster_count; j++) {
                CMU_Cluster* sub_cluster = &cluster->sub_clusters[j];
                
                // nocheckin: what if its a single letter?
                if (sub_cluster->c == search[1]) {
                    for (int k = 0; k < sub_cluster->count; k++) {
                        CMU_Entry* entry = &sub_cluster->first[k];
                        if (StringEquals(search, search_length, entry->key.text, entry->key.length)) {
                            *token = entry->value;
                            return true;
                        }
                    }
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
    {
        clusters = (CMU_Cluster*)HeapAlloc(MAX_CLUSTERS * sizeof(CMU_Cluster));
        
        root_cluster.c = entries[0].key.text[0];
        root_cluster.first = &entries[0];
        root_cluster.count = entry_count;
        
        BuildSubClusters(&root_cluster, 0);

        for (int i = 0; i < root_cluster.sub_cluster_count; i++) {
            CMU_Cluster* cluster = &root_cluster.sub_clusters[i];
            BuildSubClusters(cluster, 1);
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