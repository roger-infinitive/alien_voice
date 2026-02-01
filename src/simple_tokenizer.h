#ifndef _SIMPLE_TOKENIZER_H_
#define _SIMPLE_TOKENIZER_H_

struct Tokenizer {
    char* at;
};

enum ParsedTokenType {
    ParsedTokenType_None = 0,
    ParsedTokenType_Identifier,
    ParsedTokenType_Series,
    ParsedTokenType_EndOfLine,
    ParsedTokenType_EndOfStream
};

struct ParsedToken {
    ParsedTokenType type;
    int length;
    char* text;
};

ParsedToken ParseWhitespace(Tokenizer* tokenizer) {
    ParsedToken token = {};
    if (tokenizer->at == 0 || tokenizer->at[0] == 0) {
        token.type = ParsedTokenType_EndOfStream;
        return token;
    }
    
    // skip most whitespace types
    while (tokenizer->at[0] == ' ' || tokenizer->at[0] == '\t' || tokenizer->at[0] == '\r') {
        tokenizer->at++;
    }
    
    // newlines are treated as a tokens for the simple text format.
    if (tokenizer->at[0] == '\n') {
        token.type = ParsedTokenType_EndOfLine;
        tokenizer->at++;
    }
    
    return token;
} 

ParsedToken NextToken(Tokenizer* tokenizer) {
    ParsedToken token = ParseWhitespace(tokenizer);
    if (token.type != ParsedTokenType_None) {
        return token;
    }
    
    token.type = ParsedTokenType_Identifier;    
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

// Unlike NextToken, this reads multiple tokens until the end of the line.
// Easier to extract a series of identifiers without having to concatenate Tokens with NextToken until EndOfLine.
// This eats (or skips) EndOfLine.
ParsedToken NextTokenLine(Tokenizer* tokenizer) {
    ParsedToken token = ParseWhitespace(tokenizer);
    if (token.type != ParsedTokenType_None) {
        return token;
    }
    
    token.type = ParsedTokenType_Series;    
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

#endif // _SIMPLE_TOKENIZER_H_