#ifndef _CMU_DICTIONARY_H_
#define _CMU_DICTIONARY_H_

#include "assert.h"
#include "simple_tokenizer.h"
#include "file_io.h"

#define MAX_CMU_CLUSTERS 2048

struct CMU_Entry {
    ParsedToken key;
    ParsedToken value;
};

struct CMU_Cluster {
    // Cluster prefix: Phonemes like aa, ab, ac, etc. would be in a cluster prefixed with 'a'.  
    char c;
    
    // CMU_Entry Array Slice (or View)
    CMU_Entry* first;
    int count;
    
    // Clusters can be subdvided into smaller clusters for quicker look-up.
    // If this is 0, then its a leaf node. In that case, use the Array Slice (first, count) above. 
    int sub_cluster_count;
    CMU_Cluster* sub_clusters;
};

struct CMU_Dictionary {
    int entry_count = 0;
    CMU_Entry* entries = 0;
    
    int total_clusters;
    CMU_Cluster root_cluster;
    CMU_Cluster* clusters;
};

CMU_Cluster* InsertCluster(CMU_Dictionary* dict, CMU_Entry* first, int text_index) {
    assert(dict->total_clusters < MAX_CMU_CLUSTERS);

    CMU_Cluster* cluster = &dict->clusters[dict->total_clusters];
    cluster->c = first->key.text[text_index];
    cluster->first = first;
    cluster->count = 1;
    cluster->sub_cluster_count = 0;
    cluster->sub_clusters = 0;
    
    dict->total_clusters += 1;
    return cluster;
}

void BuildSubClusters(CMU_Dictionary* dict, CMU_Cluster* cluster, int text_index) {
    CMU_Cluster* sub_cluster = InsertCluster(dict, cluster->first, text_index);
    cluster->sub_clusters = sub_cluster;
    cluster->sub_cluster_count = 1;
            
    for (int j = 1; j < cluster->count; j++) {
        CMU_Entry* entry = &cluster->first[j];
        
        if (entry->key.length <= text_index) {
            sub_cluster->count += 1;
            continue;
        } 
        
        if (sub_cluster->c != entry->key.text[text_index]) {
            sub_cluster = InsertCluster(dict, entry, text_index);
            cluster->sub_cluster_count += 1;
        } else {
            sub_cluster->count += 1;
        }
    }        
}

// In order to generate clusters correctly for search, the input file (cmudict.dict) must be 
// alphabetically sorted otherwise the search functions are extremely likely to fail.

bool LoadDictionary(const char* filepath, CMU_Dictionary* dict, Allocator allocator) {
    MemoryBuffer mb = {};
    if (!ReadEntireFileAndNullTerminate(filepath, &mb, allocator)) {
        fprintf(stderr, "Failed to read %s.\n", filepath);
        return false;
    }
    
    char* current = mb.buffer;
    for (;;) {
        if (current[0] == 0) {
            break;
        } else if (current[0] == '\n') {
            dict->entry_count += 1;
        }
        current++;
    }

    printf("Found %d entries in %s\n", dict->entry_count, filepath);
    dict->entries = (CMU_Entry*)allocator.alloc(dict->entry_count * sizeof(CMU_Entry));
    
    int current_entry = 0;

    Tokenizer tokenizer = {};
    tokenizer.at = mb.buffer;
    for (;;) {
        ParsedToken token = NextToken(&tokenizer);
        if (token.type == ParsedTokenType_EndOfStream) {
            break;
        }
        
        CMU_Entry* entry = &dict->entries[current_entry];
        entry->key = token;
        entry->value = NextTokenLine(&tokenizer);
        current_entry++;
    }
    
    // Build the acceleration structure for look-up
    {
        dict->clusters = (CMU_Cluster*)HeapAlloc(MAX_CMU_CLUSTERS * sizeof(CMU_Cluster));
        
        dict->root_cluster.first = &dict->entries[0];
        dict->root_cluster.c = dict->entries[0].key.text[0];
        dict->root_cluster.count = dict->entry_count;
        
        BuildSubClusters(dict, &dict->root_cluster, 0);

        for (int i = 0; i < dict->root_cluster.sub_cluster_count; i++) {
            CMU_Cluster* cluster = &dict->root_cluster.sub_clusters[i];
            BuildSubClusters(dict, cluster, 1);
        }
    }
    
    return true;
}

// SLOW version! Use GetPhones() instead.
// This is used to demonstrate speed differences between linear search and the Cluster data structure.
bool GetPhonesLinear(CMU_Dictionary* dict, const char* search, ParsedToken* token) {
    int search_length = CStringLength(search);
    
    for (int i = 0; dict->entry_count; i++) {
        CMU_Entry* entry = &dict->entries[i];
        if (StringEquals(search, search_length, entry->key.text, entry->key.length)) {
            *token = entry->value;
            return true;
        }
    }
    
    return false;
}

bool GetPhones(CMU_Dictionary* dict, const char* search, ParsedToken* token) {
    int search_length = CStringLength(search);
    
    CMU_Cluster* root_cluster = &dict->root_cluster;
    
    for (int i = 0; i < root_cluster->sub_cluster_count; i++) {
        CMU_Cluster* cluster = &root_cluster->sub_clusters[i];
        if (cluster->c == search[0]) {
            for (int j = 0; j < cluster->sub_cluster_count; j++) {
                CMU_Cluster* sub_cluster = &cluster->sub_clusters[j];

                char c = ' ';
                int text_index = 1;
                if (search_length > text_index) {
                    c = search[text_index];
                }
                
                if (sub_cluster->c != c) {
                    continue;
                }
                
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
    
    return false;
}

#endif // _CMU_DICTIONARY_H_