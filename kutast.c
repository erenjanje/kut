#include "kutast.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static KutASTNode kutast__new(const char** _string, const char* endptr, const KutASTNodeType type, const KutTokenType ender_token) {
    const char* string = *_string;
    KutToken token = start_token;
    KutASTNode ret = {0};
    ret.children = calloc(1, sizeof(ret.children[0]));
    ret.type = type;
    while((token = next_token(&string, endptr)).type != KUTTOKEN_END) {
        ret.children = realloc(ret.children, (ret.children_count+1)*sizeof(ret.children[0]));
        memset(&ret.children[ret.children_count], 0, sizeof(ret.children[ret.children_count]));
        ret.children[ret.children_count].token = token;
        if(token.type == ender_token) {
            goto end;
        }
        switch(token.type) {
            case KUTTOKEN_IDENTIFIER:
                ret.children[ret.children_count].type = KUTAST_IDENTIFIER;
            break;

            case KUTTOKEN_NUMBER:
                ret.children[ret.children_count].type = KUTAST_NUMBER_LITERAL;
            break;

            case KUTTOKEN_STRING:
                ret.children[ret.children_count].type = KUTAST_STRING_LITERAL;
            break;

            case KUTTOKEN_END_STATEMENT:
            case KUTTOKEN_ARGUMENT_SEPEARTOR:
            case KUTTOKEN_END_EXPRESSION:
            case KUTTOKEN_END_FUNCTION:
            case KUTTOKEN_END_TABLE:
                ret.children[ret.children_count].type = KUTAST_INVALID;
                ret.children_count += 1;
                ret.type = KUTAST_INVALID;
                goto end;
            break;

            case KUTTOKEN_START_EXPRESSION:
                ret.children[ret.children_count] = kutast_newExpression(&string, endptr);
            break;

            case KUTTOKEN_START_TABLE:
                ret.children[ret.children_count] = kutast_newTable(&string, endptr);
            break;

            case KUTTOKEN_START_FUNCTION:
                ret.children[ret.children_count] = kutast_newFunction(&string, endptr);
            break;
        }
        ret.children_count += 1;
    }

end:
    *_string = string;
    return ret;
}

KutASTNode kutast_newStatement(const char** _string, const char* endptr) {
    return kutast__new(_string, endptr, KUTAST_STATEMENT, KUTTOKEN_END_STATEMENT);
}

KutASTNode kutast_newExpression(const char** _string, const char* endptr) {
    return kutast__new(_string, endptr, KUTAST_EXPRESSION, KUTTOKEN_END_EXPRESSION);
}

KutASTNode kutast_newTable(const char** _string, const char* endptr) {
    return kutast__new(_string, endptr, KUTAST_TABLE, KUTTOKEN_END_TABLE);
}

void kutast_parseMultiLineFunction(const char** _string, const char* endptr, KutASTNode* func) {
    const char* string = *_string;
    KutASTNode statement = (KutASTNode){.type = KUTAST_NUMBER_LITERAL}; // Everything except invalid

    while((statement = kutast_newStatement(&string, endptr)).type != KUTAST_INVALID) {
        func->children = realloc(func->children, (func->children_count+1)*sizeof(func->children[func->children_count]));
        func->children[func->children_count] = statement;
        func->children_count += 1;
    }

    kutast_destroy(statement);

    *_string = string;
}

KutASTNode kutast_newFunction(const char** _string, const char* endptr) {
    const char* string = *_string;
    KutToken token = start_token;
    KutASTNode ret = {0};
    ret.children = calloc(1, sizeof(ret.children[0]));
    ret.arguments = calloc(1, sizeof(ret.arguments[0]));
    ret.type = KUTAST_FUNCTION;
    while((token = next_token(&string, endptr)).type != KUTTOKEN_END) {
        ret.arguments = realloc(ret.arguments, (ret.argument_count+1)*sizeof(ret.arguments[0]));
        memset(&ret.arguments[ret.argument_count], 0, sizeof(ret.arguments[ret.argument_count]));
        ret.arguments[ret.argument_count].token = token;
        switch(token.type) {
            case KUTTOKEN_IDENTIFIER:
                ret.arguments[ret.argument_count].type = KUTAST_IDENTIFIER;
            break;

            case KUTTOKEN_NUMBER:
                ret.arguments[ret.argument_count].type = KUTAST_NUMBER_LITERAL;
            break;

            case KUTTOKEN_STRING:
                ret.arguments[ret.argument_count].type = KUTAST_STRING_LITERAL;
            break;

            case KUTTOKEN_ARGUMENT_SEPEARTOR:
                goto next;
            break;

            case KUTTOKEN_END_STATEMENT: {
                KutASTNode first_statement = (KutASTNode){
                    .type = KUTAST_STATEMENT,
                    .children_count = ret.argument_count,
                    .children = ret.arguments,
                    .arguments = NULL,
                    .argument_count = 0,
                };
                ret.arguments = calloc(1, sizeof(ret.arguments[0]));
                ret.arguments[0] = first_statement;
                ret.argument_count = 1;
                size_t tmp = ret.argument_count;
                ret.argument_count = ret.children_count;
                ret.children_count = tmp;
                KutASTNode* temp = ret.arguments;
                ret.arguments = ret.children;
                ret.children = temp;
                kutast_parseMultiLineFunction(&string, endptr, &ret);
                goto end;
            } break;

            case KUTTOKEN_END_EXPRESSION:
            case KUTTOKEN_END_TABLE:
                ret.arguments[ret.argument_count].type = KUTAST_INVALID;
                ret.argument_count += 1;
                goto end;
            break;

            case KUTTOKEN_START_EXPRESSION:
                ret.arguments[ret.argument_count] = kutast_newExpression(&string, endptr);
            break;

            case KUTTOKEN_START_TABLE:
                ret.arguments[ret.argument_count] = kutast_newTable(&string, endptr);
            break;

            case KUTTOKEN_START_FUNCTION:
                ret.arguments[ret.argument_count] = kutast_newFunction(&string, endptr);
            break;
            
            case KUTTOKEN_END_FUNCTION: {
                size_t tmp = ret.argument_count;
                ret.argument_count = ret.children_count;
                ret.children_count = tmp;
                KutASTNode* temp = ret.arguments;
                ret.arguments = ret.children;
                ret.children = temp;
                goto end;
            } break;
        }
        ret.argument_count += 1;
    }
next:
    while((token = next_token(&string, endptr)).type != KUTTOKEN_END) {
        ret.children = realloc(ret.children, (ret.children_count+1)*sizeof(ret.children[0]));
        memset(&ret.children[ret.children_count], 0, sizeof(ret.children[ret.children_count]));
        ret.children[ret.children_count].token = token;
        switch(token.type) {
            case KUTTOKEN_IDENTIFIER:
                ret.children[ret.children_count].type = KUTAST_IDENTIFIER;
            break;

            case KUTTOKEN_NUMBER:
                ret.children[ret.children_count].type = KUTAST_NUMBER_LITERAL;
            break;

            case KUTTOKEN_STRING:
                ret.children[ret.children_count].type = KUTAST_STRING_LITERAL;
            break;

            case KUTTOKEN_END_FUNCTION:
                goto end;
            break;

            case KUTTOKEN_ARGUMENT_SEPEARTOR:
                ret.type = KUTAST_INVALID;
                goto end;
            break;

            case KUTTOKEN_END_STATEMENT: {
                KutASTNode first_statement = (KutASTNode){
                    .type = KUTAST_STATEMENT,
                    .children_count = ret.children_count,
                    .children = ret.children,
                    .arguments = NULL,
                    .argument_count = 0,
                };
                ret.children = calloc(1, sizeof(ret.children[0]));
                ret.children[0] = first_statement;
                ret.children_count = 1;
                kutast_parseMultiLineFunction(&string, endptr, &ret);
                goto end;
            } break;

            case KUTTOKEN_END_EXPRESSION:
            case KUTTOKEN_END_TABLE:
                ret.children[ret.children_count].type = KUTAST_INVALID;
                ret.children_count += 1;
                goto end;
            break;

            case KUTTOKEN_START_EXPRESSION:
                ret.children[ret.children_count] = kutast_newExpression(&string, endptr);
            break;

            case KUTTOKEN_START_TABLE:
                ret.children[ret.children_count] = kutast_newTable(&string, endptr);
            break;

            case KUTTOKEN_START_FUNCTION:
                ret.children[ret.children_count] = kutast_newFunction(&string, endptr);
            break;
        }
        ret.children_count += 1;
    }

end:
    *_string = string;
    return ret;
}

void kutast_destroy(KutASTNode node) {
    for(size_t i = 0; i < node.children_count; i++) {
        kutast_destroy(node.children[i]);
    }
    for(size_t i = 0; i < node.argument_count; i++) {
        kutast_destroy(node.arguments[i]);
    }
    if(node.children) {
        free(node.children);
    };
    if(node.arguments) {
        free(node.arguments);
    }
}

static void kutast__debug(KutASTNode node, size_t depth, const char* indent) {
    for(size_t i = 0; i < depth; i++) {
        printf("%s", indent);
    }
    switch(node.type) {
        case KUTAST_INVALID:
            printf("INVALID\n");
        break;
        case KUTAST_IDENTIFIER:
            printf("%.*s\n", (int)node.token.length, node.token.token);
        break;
        case KUTAST_STRING_LITERAL:
            printf("\"%.*s\"\n", (int)node.token.length, node.token.token);
        break;
        case KUTAST_NUMBER_LITERAL:
            printf("%g\n", node.token.num);
        break;
        case KUTAST_STATEMENT:
            printf("STATEMENT\n");
            for(size_t i = 0; i < node.children_count; i++) {
                kutast__debug(node.children[i], depth+1, indent);
            }
        break;
        case KUTAST_EXPRESSION:
            printf("EXPRESSION()\n");
            for(size_t i = 0; i < node.children_count; i++) {
                kutast__debug(node.children[i], depth+1, indent);
            }
        break;
        case KUTAST_TABLE:
            printf("TABLE[]\n");
            for(size_t i = 0; i < node.children_count; i++) {
                kutast__debug(node.children[i], depth+1, indent);
            }
        break;
        case KUTAST_FUNCTION:
            printf("FUNCTION{");
            for(size_t i = 0; i < node.argument_count; i++) {
                printf("%.*s%s", (int)node.arguments[i].token.length, node.arguments[i].token.token, i == node.argument_count-1 ? "" : " ");
            }
            printf("}\n");
            for(size_t i = 0; i < node.children_count; i++) {
                kutast__debug(node.children[i], depth+1, indent);
            }
        break;
    }
}

void kutast_debug(KutASTNode node, const char* indent) {
    kutast__debug(node, 0, indent);
}
