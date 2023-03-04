#include "kutparser.h"

#include <ctype.h>
#include <iso646.h>
#include <stdlib.h>

static const KutToken invalid_token = {.type = KTOK_INVALID};
static const KutToken statement_end_token = {.type = KTOK_END_STATEMENT, .length = 1};
static const char* const token_typename_table[] = {
    [KTOK_INVALID] = "KTOK_INVALID",
    [KTOK_IDENTIFIER] = "KTOK_IDENTIFIER",
    [KTOK_NUMBER] = "KTOK_NUMBER",
    [KTOK_STRING] = "KTOK_STRING",
    [KTOK_ARGUMENT_SEPEARTOR] = "KTOK_ARGUMENT_SEPEARTOR",
    [KTOK_END_STATEMENT] = "KTOK_END_STATEMENT",
    [KTOK_START_EXPRESSION] = "KTOK_START_EXPRESSION",
    [KTOK_END_EXPRESSION] = "KTOK_END_EXPRESSION",
    [KTOK_START_TABLE] = "KTOK_START_TABLE",
    [KTOK_END_TABLE] = "KTOK_END_TABLE",
    [KTOK_START_FUNCTION] = "KTOK_START_FUNCTION",
    [KTOK_END_FUNCTION] = "KTOK_END_FUNCTION",
};

static KutToken skip_line_comment(const char** _string, const char* endptr) {
    const char* string = *_string;
    while(string < endptr and *string != '\n') {
        string += 1;
    }
    if(string < endptr) {
        *_string = string+1;
    } else {
        return invalid_token;
    }
    return statement_end_token;
}

static KutToken skip_block_comment(const char** _string, const char* endptr) {
    size_t depth = 1;
    const char* string = *_string;
    string += 2; // First depth
    while((string+1) < endptr and depth != 0) {
        if(string[0] == '#' and string[1] == '[') {
            depth += 1;
            string += 2;
        } else if(string[0] == ']' and string[1] == '#') {
            depth -= 1;
            string += 2;
        } else {
            string += 1;
        }
    }
    if(depth != 0) {
        return invalid_token;
    }
    *_string = string;
    return statement_end_token;
}

static KutToken skip_comment(const char** _string, const char* endptr) {
    const char* string = *_string;
    if(string >= endptr) {
        return invalid_token;
    }
    if(string[1] == '[') {
        return skip_block_comment(_string, endptr);
    } else {
        return skip_line_comment(_string, endptr);
    }
}

static KutToken parse_number(const char** _string, const char* endptr) {
    const char* string = *_string;
    const char* number_end = NULL;
    double ret = strtod(string, &number_end);
    if(number_end > endptr) {
        return invalid_token;
    }
    *_string = number_end;
    return (KutToken){.type = KTOK_NUMBER, .num = ret, .token = string, .length = number_end-string};
}

static KutToken parse_string(const char** _string, const char* endptr) {
    const char* start = *_string;
    const char* string = *_string;
    string += 1; // Skip first quote, we came here because of it after all
    while(string < endptr) {
        if(*string == '\"') {
            *_string = string+1;
            return (KutToken){.type = KTOK_STRING, .token = start+1, .length = (string - start - 1)};
        } else if(*string == '\\') {
            string += 2;
        } else {
            string += 1;
        }
    }
    return invalid_token;
}

static KutToken parse_identifier(const char** _string, const char* endptr) {
    const char* string = *_string;
    const char* start = *_string;
    while(string != endptr) {
        if(isspace(*string) or *string == '(' or *string == ')' or *string == '[' or *string == ']' or *string == '{' or *string == '}' or *string == '\\') {
            *_string = string;
            return (KutToken){.type = KTOK_IDENTIFIER, .token = start, .length = string-start};
        }
        string += 1;
    }
    return invalid_token;
}

KutToken peek_token(const char* string, const char* endptr) {
    
}

KutToken next_token(const char** _string, const char* endptr) {
    const char* string = *_string;
    KutToken ret = invalid_token;
    while(string < endptr and (*string == ' ' or ((*string == '\\') and ((string+1) < endptr) and (string[1] == '\n')))) {
        if((*string == '\\') and ((string+1) < endptr) and (string[1] == '\n')) {
            string += 1;
        }
        string += 1;
    }
    if(string == endptr) {
        ret = invalid_token;
        return ret;
    }
    if(*string == '#') { // Comment start
        ret = skip_comment(&string ,endptr);
    } else if(*string == '\n') {
        ret = statement_end_token;
        string += 1;
    } else if(((*string == '+' or *string == '-') and (string+1) != endptr and isdigit(string[1])) or isdigit(*string)) { // Number
        ret = parse_number(&string, endptr);
    } else if(*string == '(') {
        ret =  (KutToken){.type = KTOK_START_EXPRESSION, .token = string, .length = 1};
        string += 1;
    } else if(*string == ')') {
        ret = (KutToken){.type = KTOK_END_EXPRESSION, .token = string, .length = 1};
        string += 1;
    } else if(*string == '[') {
        ret = (KutToken){.type = KTOK_START_TABLE, .token = string, .length = 1};
        string += 1;
    } else if(*string == ']') {
        ret = (KutToken){.type = KTOK_END_TABLE, .token = string, .length = 1};
        string += 1;
    } else if(*string == '{') {
        ret = (KutToken){.type = KTOK_START_FUNCTION, .token = string, .length = 1};
        string += 1;
    } else if(*string == '}') {
        ret = (KutToken){.type = KTOK_END_FUNCTION, .token = string, .length = 1};
        string += 1;
    } else if(*string == '|') {
        ret = (KutToken){.type = KTOK_ARGUMENT_SEPEARTOR, .token = string, .length = 1};
        string += 1;
    } else if(*string == '\"') {
        ret = parse_string(&string, endptr);
    } else {
        ret = parse_identifier(&string, endptr);
    }
    *_string = string;
    return ret;
}

#include <stdio.h>

void debug_token(KutToken tok) {
    printf("[%s]", token_typename_table[tok.type]);
    switch(tok.type) {
        case KTOK_INVALID:
            break;
        case KTOK_IDENTIFIER:
        case KTOK_NUMBER:
            printf("%.*s", (int)tok.length, tok.token);
            break;
        case KTOK_STRING:
            printf("\"%.*s\"", (int)tok.length, tok.token);
            break;
        case KTOK_ARGUMENT_SEPEARTOR:
        case KTOK_END_STATEMENT:
        case KTOK_START_EXPRESSION:
        case KTOK_END_EXPRESSION:
        case KTOK_START_TABLE:
        case KTOK_END_TABLE:
        case KTOK_START_FUNCTION:
        case KTOK_END_FUNCTION:
        case KTOK_START:
            break;
    }
    printf("\n");
}
