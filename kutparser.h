#ifndef __KUTPARSER_H__
#define __KUTPARSER_H__

#include <stddef.h>

typedef struct KutToken KutToken;
typedef enum KutTokenType KutTokenType;

enum KutTokenType {
    KTOK_INVALID,
    KTOK_IDENTIFIER,
    KTOK_NUMBER,
    KTOK_STRING,
    KTOK_ARGUMENT_SEPEARTOR,
    KTOK_END_STATEMENT,
    KTOK_START_EXPRESSION,
    KTOK_END_EXPRESSION,
    KTOK_START_TABLE,
    KTOK_END_TABLE,
    KTOK_START_FUNCTION,
    KTOK_END_FUNCTION,
    KTOK_START,
};

struct KutToken {
    KutTokenType type;
    size_t length;
    const char* token;
    double num;
};
KutToken peek_token(const char* string, const char* endptr);
KutToken next_token(const char** string, const char* endptr);
void debug_token(KutToken tok);

#endif