#include "kutcompiler.h"
#include "interpreter/kutstring.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static const KutVariableInfo invalid_info = {.position = -1};

void kutcompiler_new(KutCompilerInfo* context, KutCompilerInfo* info) {
    *info = (KutCompilerInfo) {
        .context = context,
        .variables = NULL,
    };
    KutValueArray* arr = NULL;
    if(context == NULL) {
        info->templates = calloc(1, sizeof(*info->templates));
        arr = calloc(1, sizeof(*arr));
    } else {
        info->templates = context->templates;
        arr = info->templates->data[info->templates->len-1].literals;
    }
    info->templates->data = realloc(info->templates->data, (info->templates->len+1)*sizeof(info->templates->data[0]));
    info->template_pos = info->templates->len;
    memset(&info->templates->data[info->template_pos], 0, sizeof(info->templates->data[0]));
    info->templates->data[info->template_pos].literals = arr;
    info->templates->len += 1;
}

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
    KutVariableInfo* ref = bsearch(&query, info->variables, info->templates->data[info->template_pos].register_count+info->templates->data[info->template_pos].closure_count, sizeof(info->variables[0]), kutcompiler_compareVariableInfos);
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
        inserted.position = info->templates->data[info->template_pos].closure_count;
        inserted.closure_info = closure_info;
        info->templates->data[info->template_pos].closure_count += 1;
    } else {
        inserted.position = info->templates->data[info->template_pos].register_count;
        info->templates->data[info->template_pos].register_count += 1;
    }
    size_t variable_count = info->templates->data[info->template_pos].register_count + info->templates->data[info->template_pos].closure_count;
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

static inline bool kutcompiler_isAssignment(KutASTNode statement) {
    return statement.type == KUTAST_STATEMENT
        and statement.children_count == 3
        and statement.children[0].type == KUTAST_IDENTIFIER
        and statement.children[2].type == KUTAST_IDENTIFIER and token_compare(statement.children[2].token, c_literal_length_pair("olsun")) == 0;
}

/// @param statement An assignment statement (first check using `kutcompiler_isAssignment`!)
/// @return Whether the assignment is a register-to-register assignment
static inline bool kutcompiler_isRegisterAssignment(KutASTNode statement) {
    return statement.children[1].type == KUTAST_IDENTIFIER;
}

void kutcompiler_pushInstruction(KutCompilerInfo* info, KutInstruction instruction) {
    KutInstruction** instructions = &info->templates->data[info->template_pos].instructions;
    size_t* instruction_count = &info->templates->data[info->template_pos].instruction_count;
    *instructions = realloc(*instructions, (*instruction_count+1)*sizeof((*instructions)[0]));
    (*instructions)[*instruction_count] = instruction;
    *instruction_count += 1;
}

size_t kutcompiler_pushLiteral(KutCompilerInfo* info, KutValue val) {
    KutValue** literals = &info->templates->data[info->template_pos].literals->data;
    size_t* literal_count = &info->templates->data[info->template_pos].literals->len;
    *literals = realloc(*literals, (*literal_count+1)*sizeof((*literals)[0]));
    (*literals)[*literal_count] = val;
    *literal_count += 1;
    return *literal_count - 1;
}

static inline bool kutcompiler_isInteger(double num) {
    return num == floor(num);
}

void kutcompiler_pushNumber(KutCompilerInfo* info, double num) {
    if(kutcompiler_isInteger(num) and INT16_MIN <= num and num <= INT16_MAX) {
        kutcompiler_pushInstruction(info, kutinstruction_pushInteger(num));
    } else {
        kutcompiler_pushInstruction(info, kutinstruction_pushLiteral(kutcompiler_pushLiteral(info, kutnumber_wrap(num))));
    }
}

void kutcompiler_compilePushExpression(KutASTNode expression, KutCompilerInfo* info) {
    if(expression.children_count == 0) {
        return;
    }
    for(size_t i = 0; i < expression.children_count-1; i++) {
        switch(expression.children[i].type) {
            case KUTAST_IDENTIFIER: {
                KutVariableInfo var = kutcompiler_getVariable(info, expression.children[i].token);
                if(memcmp(&var, &invalid_info, sizeof(var)) == 0) {
                    fprintf(stderr, "%d Undefined symbol %.*s\n", __LINE__, (int)var.token.length, var.token.token);
                    continue;
                }
                if(var.is_closure) {
                    kutcompiler_pushInstruction(info, kutinstruction_pushClosure(var.position));
                } else {
                    kutcompiler_pushInstruction(info, kutinstruction_pushRegister1(var.position));
                }
            } break;
            case KUTAST_FUNCTION:
            break;
            case KUTAST_EXPRESSION:
            break;
            case KUTAST_NUMBER_LITERAL:
                kutcompiler_pushNumber(info, expression.children[i].token.num);
            break;
        }
    }
    KutValue method_name = kutstring_wrap(kutstring_new(expression.children[expression.children_count-1].token.token, expression.children[expression.children_count-1].token.length));
    size_t method_name_pos = kutcompiler_pushLiteral(info, method_name);
    kutcompiler_pushInstruction(info, kutinstruction_pushLiteral(method_name_pos));
    kutcompiler_pushInstruction(info, kutinstruction_methodcallPC(expression.children_count));
    // kut_decref(&method_name);
}

void kutcompiler_compileStatement(KutASTNode statement, KutCompilerInfo* info) {
    if(statement.children_count == 1) {
        printf("%d %s\n", __LINE__, /* (int)statement.children[0].token.length,*/ statement.children[0].token.token);
    }
    if(statement.children_count == 0) {
        return;
    } else if(kutcompiler_isDeclaration(statement)) {
        kutcompiler_declareVariable(info, statement.children[0].token);
        return;
    } else if(kutcompiler_isAssignment(statement)) {
        return;
    }
    for(size_t i = 0; i < statement.children_count-1; i++) {
        if(statement.children[i].type == KUTAST_IDENTIFIER) {
            KutVariableInfo var = kutcompiler_getVariable(info, statement.children[i].token);
            if(memcmp(&var, &invalid_info, sizeof(var)) == 0) {
                fprintf(stderr, "%d Undefined symbol \"%.*s\"\n", __LINE__, (int)var.token.length, var.token.token);
                continue;
            }
            if(var.is_closure) {
                kutcompiler_pushInstruction(info, kutinstruction_pushClosure(var.position));
            } else {
                kutcompiler_pushInstruction(info, kutinstruction_pushRegister1(var.position));
            }
        } else if(statement.children[i].type == KUTAST_FUNCTION) {
            kutcompiler_compileFunction(statement.children[i], info);
        } else if(statement.children[i].type == KUTAST_EXPRESSION) {
            kutcompiler_compilePushExpression(statement.children[i], info);
        }
    }
    KutValue method_name = kutstring_wrap(kutstring_new(statement.children[statement.children_count-1].token.token, statement.children[statement.children_count-1].token.length));
    size_t method_name_pos = kutcompiler_pushLiteral(info, method_name);
    kutcompiler_pushInstruction(info, kutinstruction_pushLiteral(method_name_pos));
    kutcompiler_pushInstruction(info, kutinstruction_methodcallIC(statement.children_count));
}

void kutcompiler_compileFunction(KutASTNode func, KutCompilerInfo* info) {
    KutCompilerInfo self_info;
    kutcompiler_new(info, &self_info);
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
