#ifndef __KUTCOMPILER_H__
#define __KUTCOMPILER_H__

#include "interpreter/kutval.h"
#include "interpreter/kutfunc.h"
#include "kutparser.h"
#include "kutast.h"

typedef struct KutVariableInfo KutVariableInfo;
typedef struct KutCompilerInfo KutCompilerInfo;

struct KutVariableInfo {
    KutToken token;
    bool is_closure;
    int32_t position;
    uint16_t closure_info;
};

struct KutCompilerInfo {
    KutCompilerInfo* context;
    size_t register_count;
    size_t closure_count;
    KutVariableInfo* variables;
    KutFuncTemplate* template;
};

void kutcompiler_destroyInfo(KutCompilerInfo* info);
void kutcompiler_compileStatement(KutASTNode statement, KutCompilerInfo* info);
void kutcompiler_compileFunction(KutASTNode func, KutCompilerInfo* info);
void kutcompiler_compileIdentifier(KutASTNode identifier, KutCompilerInfo* info);

#endif