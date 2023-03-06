#ifndef __KUTPARSER_H__
#define __KUTPARSER_H__

#include <stddef.h>

typedef struct KutToken KutToken;
typedef enum KutTokenType KutTokenType;

enum KutTokenType {
    KUTTOKEN_INVALID,
    KUTTOKEN_IDENTIFIER,
    KUTTOKEN_NUMBER,
    KUTTOKEN_STRING,
    KUTTOKEN_ARGUMENT_SEPEARTOR,
    KUTTOKEN_END_STATEMENT,
    KUTTOKEN_START_EXPRESSION,
    KUTTOKEN_END_EXPRESSION,
    KUTTOKEN_START_TABLE,
    KUTTOKEN_END_TABLE,
    KUTTOKEN_START_FUNCTION,
    KUTTOKEN_END_FUNCTION,
    KUTTOKEN_START,
    KUTTOKEN_END,
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
int token_compare(KutToken token, size_t len, const char* str);

#define c_literal_length_pair(lit) sizeof(lit)-1, lit

extern const KutToken invalid_token;
extern const KutToken start_token;
extern const KutToken end_token;
extern const KutToken statement_end_token;

#endif