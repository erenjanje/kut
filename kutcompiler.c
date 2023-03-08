#include "kutcompiler.h"

#include <string.h>
#include <stdlib.h>

static const KutVariableInfo invalid_info = {.position = -1};

static int kutcompiler_compareTokens(const KutToken t1, const KutToken t2) {
    return memcmp(t1.token, t2.token, t1.length < t2.length ? t1.length : t2.length);
}

static int kutcompiler_compareVariableInfos(const void* _i1, const void* _i2) {
    const KutVariableInfo* info1 = _i1;
    const KutVariableInfo* info2 = _i2;
    return kutcompiler_compareTokens(info1->token, info2->token);
}

static KutVariableInfo kutcompiler_getIdentifier(KutCompilerInfo* info, KutToken identifier) {
    KutVariableInfo query = {.token = identifier};
    KutVariableInfo* ref = bsearch(&query, info->variables, info->register_count+info->closure_count, sizeof(info->variables[0]), kutcompiler_compareVariableInfos);
    if(ref != NULL) {
        return *ref;
    }
    info = info->context;
    if(info == NULL) {
        return invalid_info;
    }
    ref = bsearch(&query, info->variables, info->register_count+info->closure_count, sizeof(info->variables[0]), kutcompiler_compareVariableInfos);
    if(ref != NULL) {
        return *ref;
    }
    return invalid_info;
}

static void kutcompiler_newIdentifier(KutCompilerInfo* info, KutToken identifier, bool is_closure) {
    KutVariableInfo inserted = {.is_closure = is_closure, .token = identifier};
    if(is_closure) {
        inserted.position = info->closure_count;
        info->closure_count += 1;
    } else {
        inserted.position = info->register_count;
        info->register_count += 1;
    }
    size_t variable_count = info->register_count + info->closure_count;
    info->variables = realloc(info->variables, variable_count*sizeof(info->variables[0]));
    info->variables[variable_count-1] = inserted;
    qsort(info->variables, variable_count, sizeof(info->variables[0]), kutcompiler_compareVariableInfos);
}

void kutcompiler_compileStatement(KutASTNode statement, KutCompilerInfo* info) {

}
