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
    size_t current_call_stack;
    KutTable** call_stack;
    size_t capture_count;
    KutValue* captures;
    size_t register_count;
    KutValue registers[];
};

enum KutInstructionName {
    /// @brief No operation, empty instruction
    KUTINSTRUCTION_NO_OPERATION,
    
    /// @brief Methodcall, return value is ignored, `self` is given register
    KUTINSTRUCTION_METHODCALL_IR,

    /// @brief Methodcall, return value is ignored, `self` is given in the call stack
    KUTINSTRUCTION_METHODCALL_IC,
    
    /// @brief Pushes 2 registers to the call stack
    KUTINSTRUCTION_PUSH_REGISTER_2,
    
    /// @brief Pushes 3 registers to the call stack
    KUTINSTRUCTION_PUSH_REGISTER_3,


    /// @brief Symbolic instruction
    KUTINSTRUCTION_REGISTER_START = KUTINSTRUCTION_PUSH_REGISTER_3,

    /// @brief Assigns the second register `src`s value to the first register `dest`
    KUTINSTRUCTION_ASSIGN_REGISTER,
    
    /// @brief Methodcall, return value is given to the register, `self` is given register
    KUTINSTRUCTION_METHODCALL_RR,
    
    /// @brief Methodcall, return value is given to the register, `self` is given in the call stack
    KUTINSTRUCTION_METHODCALL_RC,
    
    /// @brief Loads the literal into the given register
    KUTINSTRUCTION_LOAD_LITERAL,
    
    /// @brief Loads the closure's value into the given register
    KUTINSTRUCTION_LOAD_CLOSURE,
    
    /// @brief Loads a new function from the template into the given register
    KUTINSTRUCTION_LOAD_TEMPLATE,
    
    /// @brief Loads a 16-bit immediate integer to the given register
    KUTINSTRUCTION_LOAD_INTEGER,
    
    /// @brief Loads nil into the given register
    KUTINSTRUCTION_LOAD_NIL,
    
    /// @brief Loads nil into the given register
    KUTINSTRUCTION_LOAD_UNDEFINED,
    
    /// @brief Loads the call stack as a table into the given register
    KUTINSTRUCTION_LOAD_TABLE,

    /// @brief Returns the value in the register
    KUTINSTRUCTION_RETURN_REGISTER,

    /// @brief Stores the value of the register in the give closure
    KUTINSTRUCTION_STORE_CLOSURE,


    /// @brief Symbolic instruction
    KUTINSTRUCTION_PUSH_START = KUTINSTRUCTION_STORE_CLOSURE,

    /// @brief Pushes 1 register to the call stack
    KUTINSTRUCTION_PUSH_REGISTER_1,

    /// @brief Methodcall, return value is directly pushed to the previous call stack, `self` is given register
    KUTINSTRUCTION_METHODCALL_PR,

    /// @brief Methodcall, return value is directly pushed to the previous call stack, `self` is given in the call stack
    KUTINSTRUCTION_METHODCALL_PC,

    /// @brief Pushes the literal directly to the call stack
    KUTINSTRUCTION_PUSH_LITERAL,

    /// @brief Pushes the closure's value directly to the call stack
    KUTINSTRUCTION_PUSH_CLOSURE,

    /// @brief Pushes new function from the template directly to the call stack
    KUTINSTRUCTION_PUSH_TEMPLATE,

    /// @brief Pushes a 16-bit immediate integer directly to the call stack
    KUTINSTRUCTION_PUSH_INTEGER,

    /// @brief Pushes nil directly to the call stack
    KUTINSTRUCTION_PUSH_NIL,

    /// @brief Pushes undefined directly to the call stack
    KUTINSTRUCTION_PUSH_UNDEFINED,

    /// @brief Pushes the call stack as a table directly to the previous call stack
    KUTINSTRUCTION_PUSH_TABLE,

    /// @brief Returns the value at the top of the stack
    KUTINSTRUCTION_RETURN_STACK,

    /// @brief Pops a value from the call stack and assigns it to the given closure
    KUTINSTRUCTION_POP_CLOSURE,
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
        int8_t reg;
        uint16_t literal;
    } l;
};

KutFunc* kutfunc_new(KutFunc* context, const KutFuncTemplate* template);
KutValue kutfunc_wrap(KutFunc* self);
KutFunc* kutfunc_cast(KutValue val);

KutValue kutfunc_run(KutValue* self, KutTable* args);
KutValue kutfunc_debug(KutValue* _self);

const char* kutfunc_serializeInstruction(KutInstruction instruction);
void kutfunc_debugInstruction(KutInstruction instruction);

KutInstruction kutinstruction_noOperation(void);
KutInstruction kutinstruction_methodcallIR(uint8_t reg, uint8_t args);
KutInstruction kutinstruction_methodcallIC(uint8_t args);
KutInstruction kutinstruction_pushRegister2(uint8_t reg1, uint8_t reg2);
KutInstruction kutinstruction_pushRegister3(uint8_t reg1, uint8_t reg2, uint8_t reg3);

// Result is assigned to a register

KutInstruction kutinstruction_assignRegister(uint8_t src, uint8_t dest);
KutInstruction kutinstruction_methodcallRR(uint8_t self, uint8_t ret, uint8_t args);
KutInstruction kutinstruction_methodcallRC(uint8_t ret, uint8_t args);
KutInstruction kutinstruction_loadLiteral(uint8_t reg, uint16_t literal);
KutInstruction kutinstruction_loadClosure(uint8_t reg, uint16_t closure);
KutInstruction kutinstruction_loadTemplate(uint8_t reg, uint16_t template);
KutInstruction kutinstruction_loadInteger(uint8_t reg, uint16_t integer);
KutInstruction kutinstruction_loadNil(uint8_t reg);
KutInstruction kutinstruction_loadUndefined(uint8_t reg);
KutInstruction kutinstruction_loadTable(uint8_t reg, uint16_t size);
KutInstruction kutinstruction_storeClosure(uint8_t reg, uint16_t closure);

// Result is directly pushed to the call stack

KutInstruction kutinstruction_pushRegister1(uint8_t reg);
KutInstruction kutinstruction_methodcallPR(uint8_t self, uint8_t args);
KutInstruction kutinstruction_methodcallPC(uint8_t args);
KutInstruction kutinstruction_pushLiteral(uint16_t literal);
KutInstruction kutinstruction_pushClosure(uint16_t closure);
KutInstruction kutinstruction_pushTemplate(uint16_t template);
KutInstruction kutinstruction_pushInteger(uint16_t integer);
KutInstruction kutinstruction_pushNil(void);
KutInstruction kutinstruction_pushUndefined(void);
KutInstruction kutinstruction_pushTable(uint16_t size);
KutInstruction kutinstruction_popClosure(uint16_t closure);

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