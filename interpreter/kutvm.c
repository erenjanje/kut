#include "kutvm.h"
#include "kuttable.h"
#include "kutreference.h"
#include "kutstring.h"
#include <stdio.h>

static KutValue* kutvm_getRegisterPointer(KutFunc* func, size_t reg) {
    if(reg >= func->template->register_count) {
        return NULL;
    }
    if(istype(func->registers[reg], kutreference)) {
        return kutreference_cast(func->registers[reg]);
    } else {
        return &func->registers[reg];
    }
}

static KutValue kutvm_getRegister(KutFunc* func, size_t reg) {
    if(reg >= func->template->register_count) {
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

static void kutvm_methodcall(KutFunc* func, KutValue* ret, size_t arg_count) {
    KutTable args = {
        .capacity = arg_count-2,
        .len = arg_count-2,
        .reference_count = 0,
        .data = &func->call_stack->data[func->call_stack->len - arg_count + 1], // +1 from self, we don't give self inside the args table, -1 from message
    };
    KutValue* self = &func->call_stack->data[func->call_stack->len - arg_count];
    KutValue message_val = func->call_stack->data[func->call_stack->len-1];
    KutString* message = kutstring_cast(message_val);
    if(message == NULL) {
        fprintf(stderr, "Message is not a string!\n");
        exit(EXIT_FAILURE);
    }
    KutValue result = self->methods->dispatch(self, message)(self, &args);
    func->call_stack->len -= arg_count;
    for(size_t i = 0; i < arg_count; i++) {
        kut_decref(&func->call_stack->data[i]);
    }
    // TODO: Add error handling

    if(ret != NULL) {
        kut_set(ret, &result);
    } else {
        kut_decref(&result);
    }
}

static void kutvm_newtable(KutFunc* func, KutValue* ret, size_t length) {
    func->call_stack->len -= length;
    KutTable* table = kuttable_new(length);
    table->len = length;
    for(size_t i = 0; i < length; i++) {
        table->data[i] = func->call_stack->data[i];
    }
    *ret = kuttable_wrap(table);
}

bool kutvm_noOperation(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_methodcallIC(KutFunc* func, KutInstruction instruction) {
    kutvm_methodcall(func, NULL, instruction.r.reg0);
    return false;
}
bool kutvm_pushRegister2(KutFunc* func, KutInstruction instruction) {
    KutValue tmp = kuttable_wrap(func->call_stack);
    kuttable_append(&tmp, kuttable_literal(kutvm_getRegister(func, instruction.r.reg0), kutvm_getRegister(func, instruction.r.reg1)));
    return false;
}
bool kutvm_pushRegister3(KutFunc* func, KutInstruction instruction) {
    KutValue tmp = kuttable_wrap(func->call_stack);
    kuttable_append(&tmp, kuttable_literal(kutvm_getRegister(func, instruction.r.reg0), kutvm_getRegister(func, instruction.r.reg1), kutvm_getRegister(func, instruction.r.reg2)));
    return false;
}
bool kutvm_assignRegister(KutFunc* func, KutInstruction instruction) {
    kut_set(kutvm_getRegisterPointer(func, instruction.r.reg0), kutvm_getRegisterPointer(func, instruction.r.reg1));
    return false;
}
bool kutvm_methodcallRC(KutFunc* func, KutInstruction instruction) {
    KutValue* ret_reg = kutvm_getRegisterPointer(func, instruction.r.reg0);
    kutvm_methodcall(func, ret_reg, instruction.r.reg1);
    return false;
}
bool kutvm_loadLiteral(KutFunc* func, KutInstruction instruction) {
    kut_set(kutvm_getRegisterPointer(func, instruction.l.reg), &func->template->literals->data[instruction.l.literal]);
    return false;
}
bool kutvm_loadClosure(KutFunc* func, KutInstruction instruction) {
    kut_set(kutvm_getRegisterPointer(func, instruction.l.reg), kutreference_cast(func->closures[instruction.l.literal]));
    return false;
}
bool kutvm_loadTemplate(KutFunc* func, KutInstruction instruction) {
    KutValue tmp = kutfunc_wrap(kutfunc_new(func, &func->template->function_templates->data[instruction.l.literal]));
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
    kutvm_newtable(func, &func->registers[instruction.l.reg], instruction.l.literal);
    return false;
}
bool kutvm_storeClosure(KutFunc* func, KutInstruction instruction) {
    return false;
}
bool kutvm_pushRegister1(KutFunc* func, KutInstruction instruction) {
    __kuttable_append(func->call_stack, func->registers[instruction.r.reg0]);
    return false;
}
bool kutvm_methodcallPC(KutFunc* func, KutInstruction instruction) {
    KutValue ret = kut_undefined;
    kutvm_methodcall(func, &ret, instruction.r.reg0);
    __kuttable_append(func->call_stack, ret);
    return false;
}
bool kutvm_pushLiteral(KutFunc* func, KutInstruction instruction) {
    __kuttable_append(func->call_stack, func->template->literals->data[instruction.l.literal]);
    return false;
}
bool kutvm_pushClosure(KutFunc* func, KutInstruction instruction) {
    __kuttable_append(func->call_stack, *kutreference_cast(func->closures[instruction.l.literal]));
    return false;
}
bool kutvm_pushTemplate(KutFunc* func, KutInstruction instruction) {
    KutValue tmp = kutfunc_wrap(kutfunc_new(func, &func->template->function_templates->data[instruction.l.literal]));
    __kuttable_append(func->call_stack, tmp);
    kut_decref(&tmp);
    return false;
}
bool kutvm_pushInteger(KutFunc* func, KutInstruction instruction) {
    __kuttable_append(func->call_stack, kutnumber_wrap(instruction.l.literal));
    return false;
}
bool kutvm_pushNil(KutFunc* func, KutInstruction instruction) {
    __kuttable_append(func->call_stack, kut_nil);
    return false;
}
bool kutvm_pushUndefined(KutFunc* func, KutInstruction instruction) {
    __kuttable_append(func->call_stack, kut_undefined);
    return false;
}
bool kutvm_pushTable(KutFunc* func, KutInstruction instruction) {
    KutValue tmp = kut_undefined;
    kutvm_newtable(func, &tmp, instruction.l.literal);
    return false;
}
bool kutvm_popClosure(KutFunc* func, KutInstruction instruction) {
    func->call_stack->len -= 1;
    KutValue popped = func->call_stack->data[func->call_stack->len];
    kut_set(kutreference_cast(func->closures[instruction.l.literal]), &popped);
    kut_decref(&popped);
    return false;
}

KutInstructionHandler instruction_handlers[] = {
    [KUTINSTRUCTION_NO_OPERATION] = kutvm_noOperation,
    [KUTINSTRUCTION_METHODCALL_IC] = kutvm_methodcallIC,
    [KUTINSTRUCTION_PUSH_REGISTER_2] = kutvm_pushRegister2,
    [KUTINSTRUCTION_PUSH_REGISTER_3] = kutvm_pushRegister3,
    [KUTINSTRUCTION_ASSIGN_REGISTER] = kutvm_assignRegister,
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
