#include "kutcompiler.h"

#include <string.h>
#include <stdlib.h>

static const KutVariableInfo invalid_info = {.position = -1};

void kutcompiler_destroyInfo(KutCompilerInfo* info) {
    free(info->variables);
}

static int kutcompiler_compareTokens(const KutToken t1, const KutToken t2) {
    return memcmp(t1.token, t2.token, t1.length < t2.length ? t1.length : t2.length);
}

static int kutcompiler_compareVariableInfos(const void* _i1, const void* _i2) {
    const KutVariableInfo* info1 = _i1;
    const KutVariableInfo* info2 = _i2;
    return kutcompiler_compareTokens(info1->token, info2->token);
}

static KutVariableInfo kutcompiler_getIdentifier(KutCompilerInfo* info, KutToken identifier) {
    if(info == NULL) {
        return invalid_info;
    }
    KutVariableInfo query = {.token = identifier};
    KutVariableInfo* ref = bsearch(&query, info->variables, info->register_count+info->closure_count, sizeof(info->variables[0]), kutcompiler_compareVariableInfos);
    if(ref != NULL) {
        return *ref;
    }
    return invalid_info;
}

static KutVariableInfo kutcompiler_newIdentifier(KutCompilerInfo* info, KutToken identifier, bool is_closure, uint16_t closure_info) {
    if(info == NULL) {
        return invalid_info;
    }
    KutVariableInfo inserted = {.is_closure = is_closure, .token = identifier};
    if(is_closure) {
        inserted.position = info->closure_count;
        inserted.closure_info = closure_info;
        info->closure_count += 1;
    } else {
        inserted.position = info->register_count;
        info->register_count += 1;
    }
    size_t variable_count = info->register_count + info->closure_count;
    info->variables = realloc(info->variables, variable_count*sizeof(info->variables[0]));
    info->variables[variable_count-1] = inserted;
    qsort(info->variables, variable_count, sizeof(info->variables[0]), kutcompiler_compareVariableInfos);
    return inserted;
}

static KutVariableInfo kutcompiler_getVariable(KutCompilerInfo* info, KutToken query) {
    if(info == NULL) {
        return invalid_info;
    }
    KutVariableInfo result = kutcompiler_getIdentifier(info, query);
    if(memcmp(&result, &invalid_info, sizeof(result)) != 0) {
        return result;
    }
    result = kutcompiler_getVariable(info->context, query);
    if(memcmp(&result, &invalid_info, sizeof(result)) == 0) {
        return result;
    }
    return kutcompiler_newIdentifier(info, query, true, result.is_closure ? result.position+256 : result.position);
}

static KutVariableInfo kutcompiler_declareVariable(KutCompilerInfo* info, KutToken token) {
    return kutcompiler_newIdentifier(info, token, false, 0);
}

static inline bool kutcompiler_isDeclaration(KutASTNode statement) {
    return statement.type == KUTAST_STATEMENT
        and statement.children_count == 4
        and statement.children[0].type == KUTAST_IDENTIFIER
        and statement.children[1].type == KUTAST_IDENTIFIER and token_compare(statement.children[1].token, c_literal_length_pair("degiskeni")) == 0
        and statement.children[3].type == KUTAST_IDENTIFIER and token_compare(statement.children[3].token, c_literal_length_pair("olsun")) == 0;
}

#include <stdio.h>

void kutcompiler_compileStatement(KutASTNode statement, KutCompilerInfo* info) {
    if(kutcompiler_isDeclaration(statement)) {
        kutcompiler_declareVariable(info, statement.children[0].token);
        return;
    }
    for(size_t i = 0; i < statement.children_count; i++) {
        if(statement.children[i].type == KUTAST_IDENTIFIER) {
            KutVariableInfo var = kutcompiler_getVariable(info, statement.children[i].token);
        }
        if(statement.children[i].type == KUTAST_FUNCTION) {
            kutcompiler_compileFunction(statement.children[i], info);
        }
    }
}

void kutcompiler_compileFunction(KutASTNode func, KutCompilerInfo* info) {
    KutCompilerInfo self_info = {
        .context = info,
        .closure_count = 0,
        .register_count = 0,
        .variables = NULL,
    };
    for(size_t i = 0; i < func.argument_count; i++) {
        kutcompiler_declareVariable(&self_info, func.arguments[i].token);
    }
    if(func.children_count != 0 and func.children[0].type != KUTAST_STATEMENT) {
        //TODO: Single expression
        goto end;
    }
    for(size_t i = 0; i < func.children_count; i++) {
        kutcompiler_compileStatement(func.children[i], &self_info);
    }
end:
    kutcompiler_destroyInfo(&self_info);
}

void kutcompiler_compileIdentifier(KutASTNode identifier, KutCompilerInfo* info) {

}
