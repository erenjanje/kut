#include "kutcompiler.h"
#include "interpreter/kutstring.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>

static const KutVariableInfo invalid_info = {.position = -1};

typedef void (*KutCompileCallbackFn)(KutASTNode node, KutCompilerInfo* info, bool is_reg, uint8_t reg);
static const KutCompileCallbackFn callbacks[KUTAST_FUNCTION+1];

static inline void kutcompiler_compile(KutASTNode node, KutCompilerInfo* info, bool is_reg, uint8_t reg) {
    callbacks[node.type](node, info, is_reg, reg);
}

#define KUTCOMPILER_IF_MESSAGE_POS 0
#define KUTCOMPILER_LOOP_MESSAGE_POS 1

KutString* kutcompiler_ifMessage = kutstring_literal("kosulla");
KutString* kutcompiler_loopMessage = kutstring_literal("dön");

void kutcompiler_new(KutCompilerInfo* context, KutCompilerInfo* info) {
    *info = (KutCompilerInfo) {
        .context = context,
        .variables = NULL,
    };
    KutValueArray* arr = NULL;
    if(context == NULL) {
        info->templates = calloc(1, sizeof(*info->templates));
        arr = calloc(1, sizeof(*arr));
        arr->len = 2; // loop and condition messages
        arr->data = calloc(2, sizeof(arr->data[0]));
        arr->data[KUTCOMPILER_IF_MESSAGE_POS] = kutstring_wrap(kutcompiler_ifMessage);
        arr->data[KUTCOMPILER_LOOP_MESSAGE_POS] = kutstring_wrap(kutcompiler_loopMessage);
    } else {
        info->templates = context->templates;
        arr = info->templates->data[info->templates->len-1].literals;
    }
    info->templates->data = realloc(info->templates->data, (info->templates->len+1)*sizeof(info->templates->data[0]));
    info->template_pos = info->templates->len;
    memset(&info->templates->data[info->template_pos], 0, sizeof(info->templates->data[0]));
    info->templates->data[info->template_pos].literals = arr;
    info->templates->data[info->template_pos].function_templates = info->templates;
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

#define check_token(tok, value) (((tok).type == KUTAST_IDENTIFIER and token_compare((tok).token, c_literal_length_pair(value))) == 0)

/**
 * <var_name> degiskeni <value> olsun
*/
static inline bool kutcompiler_isDeclaration(KutASTNode statement) {
    return statement.type == KUTAST_STATEMENT
        and statement.children_count == 4
        and statement.children[0].type == KUTAST_IDENTIFIER
        and check_token(statement.children[1], "degiskeni")
        and check_token(statement.children[3], "olsun");
}

/**
 * <var_name> <value> olsun
*/
static inline bool kutcompiler_isAssignment(KutASTNode statement) {
    return statement.type == KUTAST_STATEMENT
        and statement.children_count == 3
        and statement.children[0].type == KUTAST_IDENTIFIER
        and check_token(statement.children[2], "olsun");
}

/**
 * eger <condition> ise <then_function> degilse <else_function>
*/
static inline bool kutcompiler_isIf(KutASTNode statement) {
    return statement.type == KUTAST_STATEMENT
        and statement.children_count == 6
        and check_token(statement.children[0], "eger")
        and check_token(statement.children[2], "ise")
        and check_token(statement.children[4], "degilse");
}

/**
 * <iterable> icindeki her-bir <loop_var_name> icin <loop_body>
*/
static inline bool kutcompiler_isLoop(KutASTNode statement) {
    return statement.type == KUTAST_STATEMENT
        and statement.children_count == 6
        and check_token(statement.children[1], "icindeki")
        and check_token(statement.children[2], "her-bir")
        and statement.children[3].type == KUTAST_IDENTIFIER
        and check_token(statement.children[4], "icin")
        and statement.children[5].type == KUTAST_FUNCTION;
}

/// @param statement An assignment statement (first check using `kutcompiler_isAssignment`!)
/// @return Whether the assignment is a register-to-register assignment
static inline bool kutcompiler_isRegisterAssignment(KutASTNode statement) {
    return statement.children[1].type == KUTAST_IDENTIFIER;
}

size_t kutcompiler_pushInstruction(KutCompilerInfo* info, KutInstruction instruction) {
    KutInstruction** instructions = &info->templates->data[info->template_pos].instructions;
    size_t* instruction_count = &info->templates->data[info->template_pos].instruction_count;
    *instructions = realloc(*instructions, (*instruction_count+1)*sizeof((*instructions)[0]));
    (*instructions)[*instruction_count] = instruction;
    *instruction_count += 1;
    return (*instruction_count)-1;
}

size_t kutcompiler_pushLiteral(KutCompilerInfo* info, KutValue val) {
    KutValue** literals = &info->templates->data[info->template_pos].literals->data;
    size_t* literal_count = &info->templates->data[info->template_pos].literals->len;
    for(size_t i = 0; i < *literal_count; i++) {
        if(kut_equal(&((*literals)[i]), &val)) {
            kut_decref(&val);
            return i;
        }
    }
    *literals = realloc(*literals, (*literal_count+1)*sizeof((*literals)[0]));
    (*literals)[*literal_count] = val;
    *literal_count += 1;
    return *literal_count - 1;
}

static inline bool kutcompiler_isInteger(double num) {
    return num == floor(num);
}

void kutcompiler_compileNumber(KutASTNode _num, KutCompilerInfo* info, bool is_reg, uint8_t reg) {
    double num = _num.token.num;
    if(kutcompiler_isInteger(num) and INT16_MIN <= num and num <= INT16_MAX) {
        kutcompiler_pushInstruction(info, is_reg ? kutinstruction_loadInteger(reg, num) : kutinstruction_pushInteger(num));
    } else {
        size_t literal_pos = kutcompiler_pushLiteral(info, kutnumber_wrap(num));
        kutcompiler_pushInstruction(info, is_reg ? kutinstruction_loadLiteral(reg, literal_pos) : kutinstruction_pushLiteral(literal_pos));
    }
}

void kutcompiler_compileString(KutASTNode str, KutCompilerInfo* info, bool is_reg, uint8_t reg) {
    KutValue string = kutstring_wrap(kutstring_new(str.token.token, str.token.length));
    size_t literal_pos = kutcompiler_pushLiteral(info, string);
    kutcompiler_pushInstruction(info, is_reg ? kutinstruction_loadLiteral(reg, literal_pos) : kutinstruction_pushLiteral(literal_pos));
}

void kutcompiler_compileExpression(KutASTNode expression, KutCompilerInfo* info, bool is_reg, uint8_t reg) {
    if(expression.children_count == 0) {
        return;
    }
    for(size_t i = 0; i < expression.children_count-1; i++) {
        kutcompiler_compile(expression.children[i], info, false, 0);
    }
    KutValue method_name = kutstring_wrap(kutstring_new(expression.children[expression.children_count-1].token.token, expression.children[expression.children_count-1].token.length));
    size_t method_name_pos = kutcompiler_pushLiteral(info, method_name);
    kutcompiler_pushInstruction(info, kutinstruction_pushLiteral(method_name_pos));
    kutcompiler_pushInstruction(info, is_reg ? kutinstruction_methodcallRC(reg, expression.children_count) : kutinstruction_methodcallPC(expression.children_count));
    // kut_decref(&method_name);
}

void kutcompiler_compileStatement(KutASTNode statement, KutCompilerInfo* info) {
    if(statement.children_count == 1) {
        printf("%d %s\n", __LINE__, /* (int)statement.children[0].token.length,*/ statement.children[0].token.token);
    }
    if(statement.children_count == 0) {
        return;
    } else if(kutcompiler_isDeclaration(statement)) {
        KutVariableInfo var = kutcompiler_declareVariable(info, statement.children[0].token);
        kutcompiler_compile(statement.children[2], info, true, var.position);
        return;
    } else if(kutcompiler_isAssignment(statement)) {
        KutVariableInfo var = kutcompiler_getVariable(info, statement.children[0].token);
        if(not var.is_closure) {
            kutcompiler_compile(statement.children[1], info, true, var.position);
        } else {
            kutcompiler_compile(statement.children[1], info, false, 0);
            kutcompiler_pushInstruction(info, kutinstruction_popClosure(var.position));
        }
        return;
    } else if(kutcompiler_isIf(statement)) {
        kutcompiler_compile(statement.children[1], info, false, 0);
        kutcompiler_compile(statement.children[3], info, false, 0);
        kutcompiler_compile(statement.children[5], info, false, 0);
        kutcompiler_pushInstruction(info, kutinstruction_pushLiteral(KUTCOMPILER_IF_MESSAGE_POS));
        kutcompiler_pushInstruction(info, kutinstruction_methodcallIC(4));
        return;
    } else if(kutcompiler_isLoop(statement)) {
        kutcompiler_compile(statement.children[0], info, false, 0);

        statement.children[5].argument_count += 1;
        statement.children[5].arguments = realloc(statement.children[5].arguments, statement.children[5].argument_count*sizeof(statement.children[5].arguments[0]));
        memmove(&statement.children[5].arguments[1], statement.children[5].arguments, (statement.children[5].argument_count-1)*sizeof(statement.children[5].arguments[0]));
        
        statement.children[5].arguments[0] = statement.children[3];
        kutcompiler_compileFunction(statement.children[5], info, false, 0);
        kutcompiler_pushInstruction(info, kutinstruction_pushLiteral(KUTCOMPILER_LOOP_MESSAGE_POS));
        kutcompiler_pushInstruction(info, kutinstruction_methodcallIC(3));
        return;
    }
    for(size_t i = 0; i < statement.children_count-1; i++) {
        kutcompiler_compile(statement.children[i], info, false, 0);
    }
    KutValue method_name = kutstring_wrap(kutstring_new(statement.children[statement.children_count-1].token.token, statement.children[statement.children_count-1].token.length));
    size_t method_name_pos = kutcompiler_pushLiteral(info, method_name);
    kutcompiler_pushInstruction(info, kutinstruction_pushLiteral(method_name_pos));
    kutcompiler_pushInstruction(info, kutinstruction_methodcallIC(statement.children_count));
}

void kutcompiler_compileFunction(KutASTNode func, KutCompilerInfo* info, bool is_reg, uint8_t reg) {
    KutCompilerInfo self_info;
    kutcompiler_new(info, &self_info);
    for(size_t i = 0; i < func.argument_count; i++) {
        kutcompiler_declareVariable(&self_info, func.arguments[i].token);
    }
    if(func.children_count != 0 and func.children[0].type != KUTAST_STATEMENT) { // Single expression
        if(func.children[0].type == KUTAST_IDENTIFIER and func.children_count == 1) {
            KutVariableInfo var = kutcompiler_getVariable(&self_info, func.children[0].token);
            if(var.is_closure) {
                kutcompiler_pushInstruction(&self_info, kutinstruction_pushClosure(var.position));
                kutcompiler_pushInstruction(&self_info, kutinstruction_returnStack());
            } else {
                kutcompiler_pushInstruction(&self_info, kutinstruction_returnRegister(var.position));
            }
            goto end;
        }
        func.type = KUTAST_EXPRESSION;
        kutcompiler_compileExpression(func, &self_info, false, 0);
        kutcompiler_pushInstruction(&self_info, kutinstruction_returnStack());
        goto end;
    }
    for(size_t i = 0; i < func.children_count; i++) {
        kutcompiler_compileStatement(func.children[i], &self_info);
    }
end: {}
    size_t variable_count = self_info.templates->data[self_info.template_pos].closure_count + self_info.templates->data[self_info.template_pos].register_count;
    self_info.templates->data[self_info.template_pos].capture_infos = calloc(self_info.templates->data[self_info.template_pos].closure_count, sizeof(uint16_t));
    for(size_t i = 0; i < variable_count; i++) {
        if(self_info.variables[i].is_closure) {
            self_info.templates->data[self_info.template_pos].capture_infos[i] = self_info.variables[i].closure_info;
        }
    }
    kutcompiler_pushInstruction(info, is_reg ? kutinstruction_loadTemplate(reg, self_info.template_pos) : kutinstruction_pushTemplate(self_info.template_pos));
    kutcompiler_destroyInfo(&self_info);
}

void kutcompiler_compileIdentifier(KutASTNode identifier, KutCompilerInfo* info, bool is_reg, uint8_t reg) {
    KutVariableInfo var = kutcompiler_getVariable(info, identifier.token);
    if(memcmp(&var, &invalid_info, sizeof(var)) == 0) {
        fprintf(stderr, "Unknown identifier: %.*s\n", (int)identifier.token.length, identifier.token.token);
        exit(EXIT_FAILURE);
    }
    if(var.is_closure) {
        if(is_reg) {
            kutcompiler_pushInstruction(info, kutinstruction_loadClosure(reg, var.position));
        } else {
            kutcompiler_pushInstruction(info, kutinstruction_pushClosure(var.position));
        }
    } else {
        if(is_reg) {
            kutcompiler_pushInstruction(info, kutinstruction_assignRegister(reg, var.position));
        } else {
            kutcompiler_pushInstruction(info, kutinstruction_pushRegister1(var.position));
        }
    }
}

void kutcompiler_compileTable(KutASTNode table, KutCompilerInfo* info, bool is_reg, uint8_t reg) {
    for(size_t i = 0; i < table.children_count; i++) {
        kutcompiler_compile(table.children[i], info, false, 0);
    }
    kutcompiler_pushInstruction(info, is_reg ? kutinstruction_loadTable(reg, table.children_count) : kutinstruction_pushTable(table.children_count));
}

void kutcompiler_compileInvalid(KutASTNode invalid, KutCompilerInfo* info, bool is_reg, uint8_t reg) {
    fprintf(stderr, "Invalid token\n");
}

static const KutCompileCallbackFn callbacks[] = {
    [KUTAST_NUMBER_LITERAL] = kutcompiler_compileNumber,
    [KUTAST_STRING_LITERAL] = kutcompiler_compileString,
    [KUTAST_EXPRESSION] = kutcompiler_compileExpression,
    [KUTAST_FUNCTION] = kutcompiler_compileFunction,
    [KUTAST_IDENTIFIER] = kutcompiler_compileIdentifier,
    [KUTAST_TABLE] = kutcompiler_compileTable,
    [KUTAST_INVALID] = kutcompiler_compileInvalid,
};
