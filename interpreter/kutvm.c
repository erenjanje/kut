#include "kutvm.h"
#include "kuttable.h"
#include "kutreference.h"
#include "kutstring.h"
#include <stdio.h>

static KutValue* kutvm_getRegisterPointer(KutFunc* func, size_t reg) {
    if(reg >= func->register_count) {
        return NULL;
    }
    if(istype(func->registers[reg], kutreference)) {
        return kutreference_cast(func->registers[reg]);
    } else {
        return &func->registers[reg];
    }
}

static KutValue kutvm_getRegister(KutFunc* func, size_t reg) {
    if(reg >= func->register_count) {
        return kut_undefined;
    }
    if(istype(func->registers[reg], kutreference)) {
        return *kutreference_cast(func->registers[reg]);
    } else {
        return func->registers[reg];
    }
}

static void kutvm_setRegister(KutFunc* func, size_t reg, KutValue val) {
    KutValue* setter = kutvm_getRegisterPointer(func, reg);
    if(setter == NULL) {
        return;
    }
    size_t ref = setter->reference_count;
    *setter = val;
    setter->reference_count = ref;
}

bool kutvm_noOperation(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_methodcallIR(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_methodcallIC(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_pushRegister2(KutFunc* func, KutInstruction instruction) {
    KutValue tmp = kuttable_wrap(func->call_stack[func->current_call_stack]);
    kuttable_append(&tmp, kuttable_literal(kutvm_getRegister(func, instruction.r.reg0), kutvm_getRegister(func, instruction.r.reg1)));
    return false;
}
bool kutvm_pushRegister3(KutFunc* func, KutInstruction instruction) {
    KutValue tmp = kuttable_wrap(func->call_stack[func->current_call_stack]);
    kuttable_append(&tmp, kuttable_literal(kutvm_getRegister(func, instruction.r.reg0), kutvm_getRegister(func, instruction.r.reg1), kutvm_getRegister(func, instruction.r.reg2)));
    return false;
}
bool kutvm_assignRegister(KutFunc* func, KutInstruction instruction) {
    kut_set(kutvm_getRegisterPointer(func, instruction.r.reg0), kutvm_getRegisterPointer(func, instruction.r.reg1));
    return false;
}
bool kutvm_methodcallRR(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_methodcallRC(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_loadLiteral(KutFunc* func, KutInstruction instruction) {
    kut_set(kutvm_getRegisterPointer(func, instruction.l.reg), &func->literals[instruction.l.literal]);
    return false;
}
bool kutvm_loadClosure(KutFunc* func, KutInstruction instruction) {
    kut_set(kutvm_getRegisterPointer(func, instruction.l.reg), kutreference_cast(func->captures[instruction.l.literal]));
    return false;
}
bool kutvm_loadTemplate(KutFunc* func, KutInstruction instruction) {
    KutValue tmp = kutfunc_wrap(kutfunc_new(func, func->function_templates[instruction.l.literal]));
    kut_set(kutvm_getRegisterPointer(func, instruction.l.reg), &tmp);
    kut_decref(&tmp);
    return false;
}
bool kutvm_loadInteger(KutFunc* func, KutInstruction instruction) {
    KutValue tmp = kutnumber_wrap(instruction.l.literal);
    kut_set(kutvm_getRegisterPointer(func, instruction.l.reg), &tmp);
    return false;
}
bool kutvm_loadNil(KutFunc* func, KutInstruction instruction) {
    KutValue tmp = kut_nil;
    kut_set(kutvm_getRegisterPointer(func, instruction.l.reg), &tmp);
    return false;
}
bool kutvm_loadUndefined(KutFunc* func, KutInstruction instruction) {
    KutValue tmp = kut_undefined;
    kut_set(kutvm_getRegisterPointer(func, instruction.l.reg), &tmp);
    return false;
}
bool kutvm_loadTable(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_storeClosure(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_pushRegister1(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_methodcallPR(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_methodcallPC(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_pushLiteral(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_pushClosure(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_pushTemplate(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_pushInteger(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_pushNil(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_pushUndefined(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_pushTable(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_popClosure(KutFunc* func, KutInstruction instruction) {
    return false;
}

KutInstructionHandler instruction_handlers[] = {
    [KUTINSTRUCTION_NO_OPERATION] = kutvm_noOperation,
    [KUTINSTRUCTION_METHODCALL_IR] = kutvm_methodcallIR,
    [KUTINSTRUCTION_METHODCALL_IC] = kutvm_methodcallIC,
    [KUTINSTRUCTION_PUSH_REGISTER_2] = kutvm_pushRegister2,
    [KUTINSTRUCTION_PUSH_REGISTER_3] = kutvm_pushRegister3,
    [KUTINSTRUCTION_ASSIGN_REGISTER] = kutvm_assignRegister,
    [KUTINSTRUCTION_METHODCALL_RR] = kutvm_methodcallRR,
    [KUTINSTRUCTION_METHODCALL_RC] = kutvm_methodcallRC,
    [KUTINSTRUCTION_LOAD_LITERAL] = kutvm_loadLiteral,
    [KUTINSTRUCTION_LOAD_CLOSURE] = kutvm_loadClosure,
    [KUTINSTRUCTION_LOAD_TEMPLATE] = kutvm_loadTemplate,
    [KUTINSTRUCTION_LOAD_INTEGER] = kutvm_loadInteger,
    [KUTINSTRUCTION_LOAD_NIL] = kutvm_loadNil,
    [KUTINSTRUCTION_LOAD_UNDEFINED] = kutvm_loadUndefined,
    [KUTINSTRUCTION_LOAD_TABLE] = kutvm_loadTable,
    [KUTINSTRUCTION_STORE_CLOSURE] = kutvm_storeClosure,
    [KUTINSTRUCTION_PUSH_REGISTER_1] = kutvm_pushRegister1,
    [KUTINSTRUCTION_METHODCALL_PR] = kutvm_methodcallPR,
    [KUTINSTRUCTION_METHODCALL_PC] = kutvm_methodcallPC,
    [KUTINSTRUCTION_PUSH_LITERAL] = kutvm_pushLiteral,
    [KUTINSTRUCTION_PUSH_CLOSURE] = kutvm_pushClosure,
    [KUTINSTRUCTION_PUSH_TEMPLATE] = kutvm_pushTemplate,
    [KUTINSTRUCTION_PUSH_INTEGER] = kutvm_pushInteger,
    [KUTINSTRUCTION_PUSH_NIL] = kutvm_pushNil,
    [KUTINSTRUCTION_PUSH_UNDEFINED] = kutvm_pushUndefined,
    [KUTINSTRUCTION_PUSH_TABLE] = kutvm_pushTable,
    [KUTINSTRUCTION_POP_CLOSURE] = kutvm_popClosure,
};
