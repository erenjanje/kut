#include "kutvm.h"
#include "kuttable.h"
#include "kutreference.h"
#include <stdio.h>

bool kutvm_noperation(KutFunc* func, KutInstruction instruction) {
    return false;
}

bool kutvm_methodcall(KutFunc* func, KutInstruction instruction) {
    size_t return_position = instruction.r.reg0;
    KutValue self = func->registers[instruction.r.reg1];
    KutValue msg = func->registers[instruction.r.reg2];
    if(not istype(msg, kutstring)) {
        fprintf(stderr, "Message should be a string!\n");
        return false;
    }
    KutString* message = msg.data.data;
    KutValue ret = self.dispatch(self.data, message)(self.data, func->call_stack);
    kut_decref(&func->registers[return_position]);
    func->registers[return_position] = ret;
    kuttable_clear((KutData){.data = func->call_stack}, NULL);
    return false;
}

bool kutvm_returncall(KutFunc* func, KutInstruction instruction) {
    kut_decref(&func->ret);
    kut_addref(func->registers[instruction.r.reg0]);
    func->ret = func->registers[instruction.r.reg0];
    return true;
}

bool kutvm_pushvalue1(KutFunc* func, KutInstruction instruction) {
    __kuttable_append(func->call_stack, func->registers[instruction.r.reg0]);
    return false;
}

bool kutvm_pushvalue2(KutFunc* func, KutInstruction instruction) {
    __kuttable_append(func->call_stack, func->registers[instruction.r.reg0]);
    __kuttable_append(func->call_stack, func->registers[instruction.r.reg1]);
    return false;
}

bool kutvm_pushvalue3(KutFunc* func, KutInstruction instruction) {
    __kuttable_append(func->call_stack, func->registers[instruction.r.reg0]);
    __kuttable_append(func->call_stack, func->registers[instruction.r.reg1]);
    __kuttable_append(func->call_stack, func->registers[instruction.r.reg2]);
    return false;
}

bool kutvm_getliteral(KutFunc* func, KutInstruction instruction) {
    kut_decref(&func->registers[instruction.l.reg]);
    kut_addref(func->literals[instruction.l.literal]);
    func->registers[instruction.l.reg] = func->literals[instruction.l.literal];
    return false;
}

bool kutvm_getclosure(KutFunc* func, KutInstruction instruction) {
    kut_decref(&func->registers[instruction.l.reg]);
    kut_addref(func->captures[instruction.l.literal]);
    func->registers[instruction.l.reg] = func->captures[instruction.l.literal];
    return false;
}

bool kutvm_setclosure(KutFunc* func, KutInstruction instruction) {
    kut_decref(&func->captures[instruction.l.literal]);
    *((KutValue*)func->registers[instruction.l.literal].data.data) = func->registers[instruction.l.reg];
    return false;
}

bool kutvm_mvregister(KutFunc* func, KutInstruction instruction) {
    if(instruction.r.reg0 == instruction.r.reg1)
        return false;
    kut_decref(&func->registers[instruction.r.reg0]);
    kut_addref(func->registers[instruction.r.reg1]);
    func->registers[instruction.r.reg0] = func->registers[instruction.r.reg1];
    return false;
}

bool kutvm_swapregist(KutFunc* func, KutInstruction instruction) {
    if(instruction.r.reg0 == instruction.r.reg1)
        return false;
    KutValue tmp = func->registers[instruction.r.reg0];
    func->registers[instruction.r.reg0] = func->registers[instruction.r.reg1];
    func->registers[instruction.r.reg1] = tmp;
    return false;
}

bool kutvm_gettmplate(KutFunc* func, KutInstruction instruction) {
    kut_decref(&func->registers[instruction.l.reg]);
    const KutFuncTemplate* template = func->function_templates[instruction.l.literal];
    func->registers[instruction.l.reg] = kutfunc_wrap(kutfunc_new(func, template));
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
