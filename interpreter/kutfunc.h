#ifndef __KUTFUNC_H__
#define __KUTFUNC_H__

#include "kutval.h"

typedef struct KutFuncTemplate KutFuncTemplate;
typedef struct KutFunc KutFunc;

struct KutFuncTemplate {
    const KutInstruction* instructions;
    const KutValue* literals;
    const KutFuncTemplate** function_templates;
    size_t register_count;
    size_t capture_count;
    uint16_t capture_infos[];
};

struct KutFunc {
    size_t reference_count;
    const KutInstruction* instructions;
    const KutValue* literals;
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

    __KI_METHODCALL,
    __KI_RETURNCALL,
    __KI_PUSHVALUE1,
    __KI_PUSHVALUE2,
    __KI_PUSHVALUE3,
    __KI_MVREGISTER,
    __KI_SWAPREGIST,

    __KI_GETLITERAL,
    __KI_GETCLOSURE,
    __KI_SETCLOSURE,
    __KI_GETTMPLATE,
};

typedef enum KutEmptyInstructionName {
    KI_NOPERATION = __KI_NOPERATION
} KutEmptyInstructionName;

typedef enum KutRegisterInstructionName {
    KI_METHODCALL = __KI_METHODCALL,
    KI_RETURNCALL = __KI_RETURNCALL,
    KI_PUSHVALUE1 = __KI_PUSHVALUE1,
    KI_PUSHVALUE2 = __KI_PUSHVALUE2,
    KI_PUSHVALUE3 = __KI_PUSHVALUE3,
    KI_MVREGISTER = __KI_MVREGISTER,
    KI_SWAPREGIST = __KI_SWAPREGIST,
} KutRegisterInstructionName;

typedef enum KutLiteralInstructionName {
    KI_GETLITERAL = __KI_GETLITERAL,
    KI_GETCLOSURE = __KI_GETCLOSURE,
    KI_SETCLOSURE = __KI_SETCLOSURE,
    KI_GETTMPLATE = __KI_GETTMPLATE,
} KutLiteralInstructionName;

enum KutSpecialRegister {
    KSP_RET,
    KSP_ZR0,
};

union KutInstruction {
    struct KutRegisterInstruction {
        uint8_t instruction;
        uint8_t reg0;
        uint8_t reg1;
        uint8_t reg2;
    } r;

    struct KutLiteralInstruction {
        uint8_t instruction;
        uint8_t reg;
        uint16_t literal;
    } l;
};

KutFunc* kutfunc_new(KutFunc* context, const KutFuncTemplate* template);
KutValue kutfunc_wrap(KutFunc* self);

KutValue kutfunc_run(KutData self, KutTable* args);
KutValue kutfunc_print(KutData self, KutTable* args);

KutInstruction kutfunc_emptyInstruction(KutEmptyInstructionName name);
KutInstruction kutfunc_registerInstruction(KutRegisterInstructionName name, uint8_t reg0, uint8_t reg1, uint8_t reg2);
KutInstruction kutfunc_literalInstruction(KutLiteralInstructionName name, uint8_t reg, uint16_t literal);

const char* kutfunc_serializeInstruction(KutInstruction instruction);

KutInstruction kutfunc_noperation(void);
KutInstruction kutfunc_methodcall(uint8_t return_position, uint8_t self, uint8_t message);
KutInstruction kutfunc_returncall(uint8_t returned_value);
KutInstruction kutfunc_pushvalue1(uint8_t value);
KutInstruction kutfunc_pushvalue2(uint8_t value1, uint8_t value2);
KutInstruction kutfunc_pushvalue3(uint8_t value1, uint8_t value2, uint8_t value3);
KutInstruction kutfunc_mvregister(uint8_t destination, uint8_t source);
KutInstruction kutfunc_swapregist(uint8_t reg1, uint8_t reg2);
KutInstruction kutfunc_getliteral(uint8_t reg, uint16_t literal);
KutInstruction kutfunc_getclosure(uint8_t reg, uint16_t literal);
KutInstruction kutfunc_setclosure(uint8_t reg, uint16_t literal);
KutInstruction kutfunc_gettmplate(uint8_t reg, uint16_t literal);

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