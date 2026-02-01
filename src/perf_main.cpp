#include "cmu_dictionary.h"
#include "profiler_timer.h"

int main(int argc, char** argv) {
    const char* dict_filepath = "data/cmudict.dict";

    CMU_Dictionary cmu_dict = {};
    if (!LoadDictionary(dict_filepath, &cmu_dict, HeapAllocator)) {
        return 1;
    }
    
    // Show performance between Linear search and Clustered search.
    // Use a word towards the end of the list to illustrate how slow a naive linear search is.
    
    int max_iterations = 10000;
    const char* search = "zwicker";
    
    // Linear search
    printf("\nRunning %d iterations for GetPhonesLinear...\n", max_iterations);
    Timer timer = StartTimer();
    ParsedToken phones = {};
    for (int i = 0; i < max_iterations; i++) {
        GetPhonesLinear(&cmu_dict, search, &phones);
    }
    double ms = StopTimer(timer);
    printf("    Result: %.*s\n", phones.length, phones.text);
    printf("    Time: %f ms\n", ms);

    // Clustered search
    printf("\nRunning %d iterations for GetPhones (Clustered)...\n", max_iterations);
    timer = StartTimer();
    phones = {};
    for (int i = 0; i < max_iterations; i++) {
        GetPhones(&cmu_dict, search, &phones);
    }
    ms = StopTimer(timer);
    printf("    Result: %.*s\n", phones.length, phones.text);
    printf("    Time: %f ms\n", ms);
}