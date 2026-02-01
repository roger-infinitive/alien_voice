#ifndef _ALIEN_SPEECH_DATA_H_
#define _ALIEN_SPEECH_DATA_H_

struct SymbolPair {
    const char* key;
    const char* value; 
};

SymbolPair vowel_map[] = {
    #define VOWEL(symbol, alien_vowel) { #symbol, #alien_vowel },
    #include "symbols.xmacro"
};

SymbolPair consonant_map[] = {
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

#endif //_ALIEN_SPEECH_DATA_H_