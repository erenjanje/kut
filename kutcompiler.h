#ifndef __KUTCOMPILER_H__
#define __KUTCOMPILER_H__

#include "interpreter/kutval.h"
#include "kutparser.h"
#include "kutast.h"

typedef struct KutVariableInfo KutVariableInfo;
typedef struct KutCompilerInfo KutCompilerInfo;

struct KutVariableInfo {
    KutToken token;
    bool is_closure;
    int32_t position;
};

struct KutCompilerInfo {
    KutCompilerInfo* context;
    size_t register_count;
    size_t closure_count;
    KutVariableInfo* variables;
};

void kutcompiler_compileStatement(KutASTNode statement, KutCompilerInfo* info);

#endif