#ifndef __KUTAST_H__
#define __KUTAST_H__

#include "kutparser.h"

typedef enum KutASTNodeType KutASTNodeType;
typedef union KutASTNodeData KutASTNodeData;
typedef struct KutASTNode KutASTNode;

enum KutASTNodeType {
    KUTAST_INVALID,
    KUTAST_IDENTIFIER,
    KUTAST_STRING_LITERAL,
    KUTAST_NUMBER_LITERAL,
    KUTAST_STATEMENT,
    KUTAST_EXPRESSION,
    KUTAST_TABLE,
    KUTAST_FUNCTION,
};

struct KutASTNode {
    KutASTNodeType type;
    KutToken token;
    size_t children_count;
    KutASTNode* children;
    size_t argument_count;
    KutASTNode* arguments;
};

KutASTNode kutast_newStatement(const char** string, const char* endptr);
KutASTNode kutast_newExpression(const char** string, const char* endptr);
KutASTNode kutast_newTable(const char** string, const char* endptr);
KutASTNode kutast_newFunction(const char** string, const char* endptr);
void kutast_destroy(KutASTNode node);
void kutast_debug(KutASTNode node, const char* indent);

#endif