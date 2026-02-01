#include <stdio.h>
#include <assert.h>

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

#include "file_io.h"
#include "profiler_timer.h"
#include "simple_tokenizer.h"
#include "cmu_dictionary.h"
#include "speech_audio.h"

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

struct StringPair {
    const char* key;
    const char* value; 
};

StringPair vowel_map[] = {
    #define VOWEL(symbol, alien_vowel) { #symbol, #alien_vowel },
    #include "symbols.xmacro"
};

StringPair consonant_map[] = {
    #define CONSONANT(symbol, alien_consonant) { #symbol, #alien_consonant },
    #include "symbols.xmacro"
};

enum Unit {
    #define ALIEN_SPEECH_UNIT(symbol) Unit_##symbol,
    #include "symbols.xmacro"
    Unit_Count
};

const char* UnitStrings[] = {
    #define ALIEN_SPEECH_UNIT(symbol) #symbol,
    #include "symbols.xmacro"
};

const char* UnitAssetPaths[] {
    #define ALIEN_SPEECH_UNIT(symbol) "data/audio/" #symbol ".mp3",
    #include "symbols.xmacro"
};

UnitClip unit_clips[Unit_Count];

bool IsVowel(ParsedToken token, int* index) {
    if (token.length == 2) {
        for (int i = 0; i < countOf(vowel_map); i++) {
            if (vowel_map[i].key[0] == token.text[0] && vowel_map[i].key[1] == token.text[1]) {
                *index = i;
                return true;
            }  
        }
    }
    return false;
}

bool IsConsonant(ParsedToken token, int* index) {
    for (int i = 0; i < countOf(consonant_map); i++) {
        if (StringEquals(consonant_map[i].key, CStringLength(consonant_map[i].key), token.text, token.length)) {
            *index = i;
            return true;
        }
    }
    return false;
}

int main(int argc, char** argv) {
    const char* dict_filepath = "data/cmudict.dict";

    CMU_Dictionary cmu_dict = {};
    if (!LoadDictionary(dict_filepath, &cmu_dict, HeapAllocator)) {
        return 1;
    }
    
    // Load Unit Clips
    for (int i = 0; i < countOf(UnitAssetPaths); i++) {
        const char* path = UnitAssetPaths[i];
        LoadClipF32(&unit_clips[i], path, 1, 48000);
    }
    
    ma_engine engine;
    ma_engine_init(0, &engine);
    
    const char* sentence = "Space exploration turns distant points of light into places with landscapes weather and history expanding our sense of what is possible By sending probes telescopes and people beyond Earth we learn how planets form how stars live and die and how our own world fits into a much larger story The same pursuit also drives practical breakthroughs from sharper imaging and safer materials to new ways of communicating while uniting people around a shared curiosity Most of all it invites a rare kind of perspective that our home is precious our knowledge is still young and the universe is vast enough to keep surprising us";
    
    bool use_example_text = true;
    if (argc > 1) {
        sentence = argv[1];        
    }
    
    Tokenizer tokenizer = {};
    tokenizer.at = (char*)sentence;
    
    const int MAX_STRING_BUFFER = 256;
    char search_buffer[MAX_STRING_BUFFER];
    
    int output_length = 0;
    UnitClip output[512];
    
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
                                    output[output_length++] = unit_clips[i];
                                    found = true;
                                    break;
                                }
                            }
                            
                            if (!found) {
                                fprintf(stderr, "NOT FOUND: %c%c\n", onset_consonant, vowel_map[index].value[0]);
                            }
                        } else if (IsConsonant(symbol, &index)) {
                            onset_consonant = consonant_map[index].value[0];
                        }
                    }
                    
                    printf("%s: %.*s\n", search_buffer, phones.length, phones.text);
                } else {
                    printf("Unable to find word in dictionary.\n");
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