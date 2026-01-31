#include <stdio.h>
#include "file_io.h"

enum TokenType {
    TokenType_Identifier,
    TokenType_EndOfLine,
    TokenType_EndOfStream
};

struct Token {
    TokenType type;
    int length;
    char* text;
};

struct Tokenizer {
    char* at;
};

struct CMU_Entry {
    Token key;
    Token value;
};

Token NextToken(Tokenizer* tokenizer) {
    Token token = {};
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

Token NextTokenLine(Tokenizer* tokenizer) {
    Token token = {};
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

void ExtendToken(Token* token, Token extendTo) {
    token->length = (extendTo.text - token->text) + extendTo.length - 1;
}

int main(int argc, char** argv) {
    MemoryBuffer mb = {};
    if (!ReadEntireFileAndNullTerminate("data/cmudict.dict", &mb, HeapAllocator)) {
        fprintf(stderr, "Failed to read cmudict.dict.\n");
        return 1;
    }
    
    Tokenizer tokenizer = {};
    tokenizer.at = mb.buffer;
    
    int entry_count = 0;
    for (;;) {
        if (tokenizer.at[0] == 0) {
            break;
        } else if (tokenizer.at[0] == '\n') {
            entry_count += 1;
        }
        tokenizer.at++;
    }

    printf("entry_count: %d\n", entry_count);
    
    CMU_Entry* entries = (CMU_Entry*)HeapAlloc(entry_count * sizeof(CMU_Entry));
    
    int current_entry = 0;
    tokenizer.at = mb.buffer;
    for (;;) {
        Token token = NextToken(&tokenizer);
        if (token.type == TokenType_EndOfStream) {
            break;
        }
        
        CMU_Entry* entry = &entries[current_entry];
        entry->key = token;
        entry->value = NextTokenLine(&tokenizer);
        current_entry++;
    }
    
    for (int i = 0; i < 4; i++) {
        CMU_Entry* entry = &entries[i];
        printf("%.*s => %.*s\n", entry->key.length, entry->key.text, entry->value.length, entry->value.text); 
    }
    
    printf("done\n");
    
    return 0;
}