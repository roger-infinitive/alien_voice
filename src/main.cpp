#include <stdio.h>
#include <assert.h>

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

#include "file_io.h"
#include "profiler_timer.h"
#include "simple_tokenizer.h"
#include "cmu_dictionary.h"
#include "speech_audio.h"
#include "alien_speech_data.h"

// This extracts the alpha chars like AA from AA0 and skips the 'stress' number. 
// Examples of 'Phone' Tokens: B, Z, AA0, AA1, EH2, etc.
ParsedToken NextPhoneToken(Tokenizer* tokenizer) {
    ParsedToken token = ParseWhitespace(tokenizer);
    if (token.type == ParsedTokenType_EndOfStream) {
        return token;
    }
    
    ParsedToken symbol = {};
    symbol.text = tokenizer->at;
    symbol.type = ParsedTokenType_Identifier;
    
    for (;;) {
        if (tokenizer->at[0] == 0) {
            break;
        } else if (IsAlpha(tokenizer->at[0])) {
            symbol.length++;
        } else if (tokenizer->at[0] == ' ' || IsNumber(tokenizer->at[0])) {
            tokenizer->at++;
            break;
        }
        tokenizer->at++;
    }
    
    return symbol;
}

int main(int argc, char** argv) {
    bool show_phones = false;
    const char* sentence = "Space exploration turns distant points of light into places with landscapes weather and history expanding our sense of what is possible By sending probes telescopes and people beyond Earth we learn how planets form how stars live and die and how our own world fits into a much larger story The same pursuit also drives practical breakthroughs from sharper imaging and safer materials to new ways of communicating while uniting people around a shared curiosity Most of all it invites a rare kind of perspective that our home is precious our knowledge is still young and the universe is vast enough to keep surprising us";
    
    int args_parsed = 1;
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == '-') {
            const char* arg = &argv[i][2]; 
            if (strcmp(arg, "help") == 0) {
                printf("Usage: %s [--show-phones] <message>\n", argv[0]);
                return 0;
            } else if (strcmp(arg, "show-phones") == 0) {
                show_phones = true;
            }
            args_parsed++;
        }
    }
    
    if (argc > args_parsed) {
        sentence = argv[args_parsed];        
    }

    const char* dict_filepath = "data/cmudict.dict";
    CMU_Dictionary cmu_dict = {};
    if (!LoadDictionary(dict_filepath, &cmu_dict, HeapAllocator)) {
        return 1;
    }
    
    UnitClip unit_clips[Unit_Count];
    for (int i = 0; i < countOf(UnitAssetPaths); i++) {
        const char* path = UnitAssetPaths[i];
        if (!LoadClipF32(&unit_clips[i], path, 1, 48000)) {
            fprintf(stderr, "Failed to load audio file: %s\n", path);
        }
    }
    
    ma_engine engine;
    ma_result result = ma_engine_init(0, &engine);
    assert(result == MA_SUCCESS);
    
    Tokenizer tokenizer = {};
    tokenizer.at = (char*)sentence;
    
    const int MAX_STRING_BUFFER = 256;
    char search_buffer[MAX_STRING_BUFFER];
    
    const int MAX_OUTPUT_CLIPS = 512;
    int output_length = 0;
    UnitClip output[MAX_OUTPUT_CLIPS];
        
    ParsedToken token = NextToken(&tokenizer);
    while (token.type != ParsedTokenType_EndOfStream) {
        switch (token.type) {
            case ParsedTokenType_Identifier: {            
                assert(token.length < MAX_STRING_BUFFER);
                memcpy(search_buffer, token.text, token.length);
                search_buffer[token.length] = 0;
                
                ToLowerCase(search_buffer, token.length);
                
                char onset_consonant = 'X';
                
                ParsedToken phones = {};
                if (GetPhones(&cmu_dict, search_buffer, &phones)) {
                    char phones_buffer[MAX_STRING_BUFFER];
                    memcpy(phones_buffer, phones.text, phones.length);
                    phones_buffer[phones.length] = 0;
                    
                    if (show_phones) {
                        printf("%s: %.*s\n", search_buffer, phones.length, phones.text);
                    }
                
                    Tokenizer phones_tokenizer = {};
                    phones_tokenizer.at = phones_buffer;
                    
                    for (;;) {
                        ParsedToken symbol = NextPhoneToken(&phones_tokenizer);
                        if (symbol.type == ParsedTokenType_EndOfStream) {
                            break;
                        }
                        
                        int index = 0;
                        if (IsVowel(symbol, &index)) {
                            bool found = false;
                            
                            for (int i = 0; i < countOf(UnitStrings); i++) {
                                const char* unit_str = UnitStrings[i];
                                if (unit_str[0] == onset_consonant && unit_str[1] == vowel_map[index].value[0]) {
                                    assert(output_length < MAX_OUTPUT_CLIPS);
                                    output[output_length++] = unit_clips[i];
                                    found = true;
                                    break;
                                }
                            }
                            
                            #ifdef _DEBUG
                                if (!found) {
                                    fprintf(stderr, "NOT FOUND: %c%c\n", onset_consonant, vowel_map[index].value[0]);
                                }
                            #endif
                            
                        } else if (IsConsonant(symbol, &index)) {
                            onset_consonant = consonant_map[index].value[0];
                        }
                    }                    
                } else {
                    #ifdef _DEBUG
                        printf("Unable to find word in dictionary.\n");
                    #endif
                }
            } break;
        }
        
        token = NextToken(&tokenizer);
    }
        
    ma_uint32 xfadeFrames = (ma_uint32)(0.1f * 48000);
    RenderedAudio rendered_audio = RenderConcatenated(output, 0, output_length, 1, 48000, xfadeFrames);
    PlayRendered(&engine, &rendered_audio);
    
    double ms = (rendered_audio.frameCount * 1000.0) / (double)rendered_audio.sampleRate;
    Sleep(ms);

    return 0;
}