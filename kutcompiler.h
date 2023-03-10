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
    KutVariableInfo* variables;
    size_t template_pos;
    KutTemplateArray* templates;
};

void kutcompiler_new(KutCompilerInfo* context, KutCompilerInfo* info);
void kutcompiler_destroyInfo(KutCompilerInfo* info);
void kutcompiler_compileStatement(KutASTNode statement, KutCompilerInfo* info);
void kutcompiler_compileFunction(KutASTNode func, KutCompilerInfo* info, bool is_reg, uint8_t reg);
void kutcompiler_compileIdentifier(KutASTNode identifier, KutCompilerInfo* info, bool is_reg, uint8_t reg);

#endif