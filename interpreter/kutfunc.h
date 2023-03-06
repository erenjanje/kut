#ifndef __KUTFUNC_H__
#define __KUTFUNC_H__

#include "kutval.h"

typedef struct KutFuncTemplate KutFuncTemplate;
typedef struct KutFunc KutFunc;

struct KutFuncTemplate {
    size_t instruction_count;
    KutInstruction* instructions;
    size_t literal_count;
    KutValue* literals;
    size_t template_count;
    const KutFuncTemplate** function_templates;
    size_t register_count;
    size_t capture_count;
    uint16_t capture_infos[];
};

struct KutFunc {
    size_t reference_count;
    size_t instruction_count;
    const KutInstruction* instructions;
    size_t literal_count;
    KutValue* literals;
    size_t template_count;
    const KutFuncTemplate** function_templates;
    KutValue ret;
    KutTable* call_stack;
    size_t capture_count;
    KutValue* captures;
    size_t register_count;
    KutValue registers[];
};

enum KutInstructionName {
    __KI_NOPERATION,
    __KI_NEWCLLSTAK,

    __KI_METHODCALL,
    __KI_RETURNCALL,
    __KI_PUSHVALUE1,
    __KI_PUSHVALUE2,
    __KI_PUSHVALUE3,
    __KI_MVREGISTER,
    __KI_SWAPREGIST,
    __KI_BRANCHWITH,

    __KI_GETLITERAL,
    __KI_GETCLOSURE,
    __KI_SETCLOSURE,
    __KI_GETTMPLATE,
    __KI_LOAD16LITR,
    __KI_LOADNILVAL,
    __KI_LOADUNDEFN,
};

typedef enum KutEmptyInstructionName {
    KI_NOPERATION = __KI_NOPERATION,
    KI_NEWCLLSTAK = __KI_NEWCLLSTAK,
} KutEmptyInstructionName;

typedef enum KutRegisterInstructionName {
    KI_METHODCALL = __KI_METHODCALL,
    KI_RETURNCALL = __KI_RETURNCALL,
    KI_PUSHVALUE1 = __KI_PUSHVALUE1,
    KI_PUSHVALUE2 = __KI_PUSHVALUE2,
    KI_PUSHVALUE3 = __KI_PUSHVALUE3,
    KI_MVREGISTER = __KI_MVREGISTER,
    KI_SWAPREGIST = __KI_SWAPREGIST,
    KI_BRANCHWITH = __KI_BRANCHWITH,
} KutRegisterInstructionName;

typedef enum KutLiteralInstructionName {
    KI_GETLITERAL = __KI_GETLITERAL,
    KI_GETCLOSURE = __KI_GETCLOSURE,
    KI_SETCLOSURE = __KI_SETCLOSURE,
    KI_GETTMPLATE = __KI_GETTMPLATE,
    KI_LOAD16LITR = __KI_LOAD16LITR,
    KI_LOADNILVAL = __KI_LOADNILVAL,
    KI_LOADUNDEFN = __KI_LOADUNDEFN,
} KutLiteralInstructionName;

enum KutSpecialRegister {
    KSP_RET,
    KSP_ZR0,
};

union KutInstruction {
    struct KutRegisterInstruction {
        uint8_t instruction;
        int8_t reg0;
        int8_t reg1;
        int8_t reg2;
    } r;

    struct KutLiteralInstruction {
        uint8_t instruction;
        int8_t reg;
        uint16_t literal;
    } l;
};

KutFunc* kutfunc_new(KutFunc* context, const KutFuncTemplate* template);
KutValue kutfunc_wrap(KutFunc* self);
KutFunc* kutfunc_cast(KutValue val);

KutValue kutfunc_run(KutValue* self, KutTable* args);
KutValue kutfunc_debug(KutValue* _self);

KutInstruction kutfunc_emptyInstruction(KutEmptyInstructionName name);
KutInstruction kutfunc_registerInstruction(KutRegisterInstructionName name, int8_t reg0, int8_t reg1, int8_t reg2);
KutInstruction kutfunc_literalInstruction(KutLiteralInstructionName name, int8_t reg, uint16_t literal);


const char* kutfunc_serializeInstruction(KutInstruction instruction);
void kutfunc_debugInstruction(KutInstruction instruction);

KutInstruction kutfunc_noperation(void);
KutInstruction kutfunc_newcllstak(void);
KutInstruction kutfunc_methodcall(int8_t return_position, int8_t self, int8_t message);
KutInstruction kutfunc_returncall(int8_t returned_value);
KutInstruction kutfunc_pushvalue1(int8_t value);
KutInstruction kutfunc_pushvalue2(int8_t value1, int8_t value2);
KutInstruction kutfunc_pushvalue3(int8_t value1, int8_t value2, int8_t value3);
KutInstruction kutfunc_mvregister(int8_t destination, int8_t source);
KutInstruction kutfunc_swapregist(int8_t reg1, int8_t reg2);
KutInstruction kutfunc_branchwith(int8_t condition, int8_t ifthen, int8_t otherwise);
KutInstruction kutfunc_getliteral(int8_t reg, uint16_t literal);
KutInstruction kutfunc_getclosure(int8_t reg, uint16_t literal);
KutInstruction kutfunc_setclosure(int8_t reg, uint16_t literal);
KutInstruction kutfunc_gettmplate(int8_t reg, uint16_t literal);
KutInstruction kutfunc_load16litr(int8_t reg, uint16_t literal);
KutInstruction kutfunc_loadnilval(int8_t reg);
KutInstruction kutfunc_loadundefn(int8_t reg);

#define kutfunc_templateLiteral(_instructions, _literals, _register_count, _infos, ...) \
    (const KutFuncTemplate*)(const struct {const KutInstruction* instructions; const KutValue* literals; const KutFuncTemplate** function_templates; size_t register_count, capture_count; uint16_t captures[sizeof((uint16_t[]){_infos, ##__VA_ARGS__})/sizeof(uint16_t)];}[])\
    {{\
        .instructions = _instructions,\
        .literals = _literals,\
        .function_templates = NULL,\
        .register_count = _register_count,\
        .capture_count = sizeof((uint16_t[]){_infos, ##__VA_ARGS__})/sizeof(uint16_t),\
        .captures = {_infos, ##__VA_ARGS__}}\
    }

#endif