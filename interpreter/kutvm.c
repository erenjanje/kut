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

bool kutvm_noperation(KutFunc* func, KutInstruction instruction) {
    return false;
}

bool kutvm_methodcall(KutFunc* func, KutInstruction instruction) {
    size_t return_position = instruction.r.reg0;
    size_t self = instruction.r.reg1;
    size_t msg = instruction.r.reg2;
    KutString* message = kutstring_cast(kutvm_getRegister(func, msg));
    if(message == NULL) {
        fprintf(stderr, "Message should be a string!\n");
        return false;
    }
    KutValue* selfptr = kutvm_getRegisterPointer(func, self);
    KutValue ret = selfptr->methods->dispatch(selfptr, message)(selfptr, func->call_stack);
    KutValue* retptr = kutvm_getRegisterPointer(func, return_position);
    kut_set(retptr, &ret);
    kut_decref(&ret);
    return false;
}

bool kutvm_returncall(KutFunc* func, KutInstruction instruction) {
    kut_set(&func->ret, kutvm_getRegisterPointer(func, instruction.r.reg0));
    return true;
}

bool kutvm_pushvalue1(KutFunc* func, KutInstruction instruction) {
    __kuttable_append(func->call_stack, kutvm_getRegister(func, instruction.r.reg0));
    return false;
}

bool kutvm_pushvalue2(KutFunc* func, KutInstruction instruction) {
    __kuttable_append(func->call_stack, kutvm_getRegister(func, instruction.r.reg0));
    __kuttable_append(func->call_stack, kutvm_getRegister(func, instruction.r.reg1));
    return false;
}

bool kutvm_pushvalue3(KutFunc* func, KutInstruction instruction) {
    __kuttable_append(func->call_stack, kutvm_getRegister(func, instruction.r.reg0));
    __kuttable_append(func->call_stack, kutvm_getRegister(func, instruction.r.reg1));
    __kuttable_append(func->call_stack, kutvm_getRegister(func, instruction.r.reg2));
    return false;
}

bool kutvm_getliteral(KutFunc* func, KutInstruction instruction) {
    kut_set(kutvm_getRegisterPointer(func, instruction.l.reg), &func->literals[instruction.l.literal]);
    return false;
}

bool kutvm_getclosure(KutFunc* func, KutInstruction instruction) {
    kut_set(kutvm_getRegisterPointer(func, instruction.l.reg), kutreference_cast(func->captures[instruction.l.literal]));
    return false;
}

bool kutvm_setclosure(KutFunc* func, KutInstruction instruction) {
    kut_set(kutreference_cast(func->captures[instruction.l.literal]), kutvm_getRegisterPointer(func, instruction.l.reg));
    return false;
}

bool kutvm_mvregister(KutFunc* func, KutInstruction instruction) {
    kut_set(kutvm_getRegisterPointer(func, instruction.r.reg0), kutvm_getRegisterPointer(func, instruction.r.reg1));
    return false;
}

bool kutvm_swapregist(KutFunc* func, KutInstruction instruction) {
    kut_swap(kutvm_getRegisterPointer(func, instruction.r.reg0), kutvm_getRegisterPointer(func, instruction.r.reg1));
    return false;
}

bool kutvm_gettmplate(KutFunc* func, KutInstruction instruction) {
    kut_decref(&func->registers[instruction.l.reg]);
    const KutFuncTemplate* template = func->function_templates[instruction.l.literal];
    KutValue f = kutfunc_wrap(kutfunc_new(func, template));
    kut_set(kutvm_getRegisterPointer(func, instruction.l.reg), &f);
    kut_decref(&f);
    return false;
}

KutInstructionHandler instruction_handlers[] = {
    [KI_NOPERATION] = kutvm_noperation,
    [KI_METHODCALL] = kutvm_methodcall,
    [KI_RETURNCALL] = kutvm_returncall,
    [KI_PUSHVALUE1] = kutvm_pushvalue1,
    [KI_PUSHVALUE2] = kutvm_pushvalue2,
    [KI_PUSHVALUE3] = kutvm_pushvalue3,
    [KI_MVREGISTER] = kutvm_mvregister,
    [KI_SWAPREGIST] = kutvm_swapregist,
    [KI_GETLITERAL] = kutvm_getliteral,
    [KI_GETCLOSURE] = kutvm_getclosure,
    [KI_SETCLOSURE] = kutvm_setclosure,
    [KI_GETTMPLATE] = kutvm_gettmplate,
};
