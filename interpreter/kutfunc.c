#include "kutfunc.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "kutvm.h"
#include "kuttable.h"
#include "kutreference.h"
#include "kutstring.h"

KutFunc* kutfunc_new(KutFunc* context, const KutFuncTemplate* template) {
    KutFunc* ret = calloc(1, sizeof(*ret) + template->register_count*sizeof(ret->registers[0]));
    *ret = (KutFunc){
        .capture_count = template->capture_count,
        .captures = calloc(template->capture_count, sizeof(ret->captures[0])),

        .instructions = template->instructions,
        .literals = template->literals,

        .reference_count = 1,

        .register_count = template->register_count,

        .call_stack = kuttable_new(2),

        .function_templates = template->function_templates,
    };

    for(size_t i = 0; i < template->capture_count; i++) {
        uint16_t capture_info = template->capture_infos[i];
        if(capture_info < 256) { // Register reference
            ret->captures[i] = kutreference_wrap(kutreference_new(&context->registers[capture_info]));
        } else { // Capture reference
            capture_info -= 256;
            kut_addref(&context->captures[capture_info]);
            ret->captures[i] = context->captures[capture_info];
        }
    }
    
    return ret;
}

KutFunc* kutfunc_cast(KutValue val) {
    if(istype(val, kutfunc)) {
        return val.data.data;
    }
    return NULL;
}

KutValue kutfunc_wrap(KutFunc* self) {
    return kut_wrap((KutData){.data = self}, &kutfunc_methods);
}

void kutfunc_addref(KutValue* _self) {
    KutFunc* self = _self ? kutfunc_cast(*_self) : NULL;
    if(self == NULL) {
        return;
    }
    if(self->reference_count != 0)
        self->reference_count += 1;
}

void kutfunc_decref(KutValue* _self) {
    KutFunc* self = _self ? kutfunc_cast(*_self) : NULL;
    if(self == NULL) {
        return;
    }
    if(self->reference_count == 0)
        return;
    if(self->reference_count == 1) {
        for(size_t i = 0; i < self->reference_count; i++) {
            kut_decref(&self->registers[i]);
        }
        for(size_t i = 0; i < self->capture_count; i++) {
            kut_decref(&self->captures[i]);
        }
        KutValue call_stack = kuttable_wrap(self->call_stack);
        kut_decref(&call_stack);
        free(self->captures);
        free(self);
        return;
    }
    self->reference_count -= 1;
    return;
}

KutValue kutfunc_run(KutValue* _self, KutTable* args) {
    KutFunc* self = _self ? kutfunc_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    memcpy(self->registers, args->data, args->len*sizeof(args->data[0]));
    bool finished = false;
    for(size_t i = 0; not finished; i++) {
        printf("Instruction %zu:\n", i);
        KutString* debg = kutstring_cast(kutfunc_debug(_self));
        printf("%.*s\n\n", kutstring_format(debg));
        free(debg);
        KutInstruction instruction = self->instructions[i];
        finished = instruction_handlers[instruction.r.instruction](self, instruction);
    }
    for(size_t i = 0; i < self->register_count; i++) {
        kut_decref(&self->registers[i]);
    }
    KutValue ret = self->ret;
    self->ret = kut_undefined;
    return ret;
}

static const char* const kutfunc_instructionNames[] = {
    [KUTINSTRUCTION_ASSIGN_REGISTER] = "ASSIGN_REGISTER",
    [KUTINSTRUCTION_CREATE_CALLSTACK] = "CREATE_CALLSTACK",
    [KUTINSTRUCTION_LOAD_CLOSURE] = "LOAD_CLOSURE",
    [KUTINSTRUCTION_LOAD_INTEGER] = "LOAD_INTEGER",
    [KUTINSTRUCTION_LOAD_LITERAL] = "LOAD_LITERAL",
    [KUTINSTRUCTION_LOAD_NIL] = "LOAD_NIL",
    [KUTINSTRUCTION_LOAD_TABLE] = "LOAD_TABLE",
    [KUTINSTRUCTION_LOAD_TEMPLATE] = "LOAD_TEMPLATE",
    [KUTINSTRUCTION_LOAD_UNDEFINED] = "LOAD_UNDEFINED",
    [KUTINSTRUCTION_METHODCALL_CC] = "METHODCALL_CC",
    [KUTINSTRUCTION_METHODCALL_CR] = "METHODCALL_CR",
    [KUTINSTRUCTION_METHODCALL_IC] = "METHODCALL_IC",
    [KUTINSTRUCTION_METHODCALL_IR] = "METHODCALL_IR",
    [KUTINSTRUCTION_METHODCALL_PC] = "METHODCALL_PC",
    [KUTINSTRUCTION_METHODCALL_PR] = "METHODCALL_PR",
    [KUTINSTRUCTION_METHODCALL_RC] = "METHODCALL_RC",
    [KUTINSTRUCTION_METHODCALL_RR] = "METHODCALL_RR",
    [KUTINSTRUCTION_NO_OPERATION] = "NO_OPERATION",
    [KUTINSTRUCTION_POP_CLOSURE] = "POP_CLOSURE",
    [KUTINSTRUCTION_PUSH_CLOSURE] = "PUSH_CLOSURE",
    [KUTINSTRUCTION_PUSH_INTEGER] = "PUSH_INTEGER",
    [KUTINSTRUCTION_PUSH_LITERAL] = "PUSH_LITERAL",
    [KUTINSTRUCTION_PUSH_NIL] = "PUSH_NIL",
    [KUTINSTRUCTION_PUSH_REGISTER_1] = "PUSH_REGISTER_1",
    [KUTINSTRUCTION_PUSH_REGISTER_2] = "PUSH_REGISTER_2",
    [KUTINSTRUCTION_PUSH_REGISTER_3] = "PUSH_REGISTER_3",
    [KUTINSTRUCTION_PUSH_TABLE] = "PUSH_TABLE",
    [KUTINSTRUCTION_PUSH_TEMPLATE] = "PUSH_TEMPLATE",
    [KUTINSTRUCTION_PUSH_UNDEFINED] = "PUSH_UNDEFINED",
    [KUTINSTRUCTION_SETCLOSURE_CLOSURE] = "SETCLOSURE_CLOSURE",
    [KUTINSTRUCTION_SETCLOSURE_INTEGER] = "SETCLOSURE_CLOSURE",
    [KUTINSTRUCTION_SETCLOSURE_LITERAL] = "SETCLOSURE_CLOSURE",
    [KUTINSTRUCTION_SETCLOSURE_NIL] = "SETCLOSURE_CLOSURE", 
    [KUTINSTRUCTION_SETCLOSURE_TABLE] = "SETCLOSURE_CLOSURE",
    [KUTINSTRUCTION_SETCLOSURE_TEMPLATE] = "SETCLOSURE_CLOSURE",
    [KUTINSTRUCTION_SETCLOSURE_UNDEFINED] = "SETCLOSURE_CLOSURE",
};

void kutfunc_debugInstruction(KutInstruction instruction) {
    printf("%-20s ", kutfunc_instructionNames[instruction.r.instruction]);
    switch(instruction.r.instruction) {
                case KUTINSTRUCTION_NO_OPERATION: printf(""); break;
                case KUTINSTRUCTION_METHODCALL_IR: printf("reg : %5d", instruction.r.reg0); break;
                case KUTINSTRUCTION_METHODCALL_IC: printf(""); break;
                case KUTINSTRUCTION_PUSH_REGISTER_2: printf("reg1: %5d reg2: %5d", instruction.r.reg0, instruction.r.reg1); break;
                case KUTINSTRUCTION_PUSH_REGISTER_3: printf("reg1: %5d reg2: %5d reg3: %5d", instruction.r.reg0, instruction.r.reg1, instruction.r.reg2); break;
                case KUTINSTRUCTION_CREATE_CALLSTACK: printf(""); break;
                case KUTINSTRUCTION_ASSIGN_REGISTER: printf("src : %5d dest: %5d", instruction.r.reg0, instruction.r.reg1); break;
                case KUTINSTRUCTION_METHODCALL_RR: printf("self: %5d ret : %5d", instruction.r.reg0, instruction.r.reg1); break;
                case KUTINSTRUCTION_METHODCALL_RC: printf("ret : %5d", instruction.r.reg0); break;
                case KUTINSTRUCTION_LOAD_LITERAL: printf("reg : %5d lit : %5d", instruction.l.reg, instruction.l.literal); break;
                case KUTINSTRUCTION_LOAD_CLOSURE: printf("reg : %5d clos: %5d", instruction.l.reg, instruction.l.literal); break;
                case KUTINSTRUCTION_LOAD_TEMPLATE: printf("reg : %5d temp: %5d", instruction.l.reg, instruction.l.literal); break;
                case KUTINSTRUCTION_LOAD_INTEGER: printf("reg : %5d int : %5d", instruction.l.reg, instruction.l.literal); break;
                case KUTINSTRUCTION_LOAD_NIL: printf("reg : %5d", instruction.l.reg); break;
                case KUTINSTRUCTION_LOAD_UNDEFINED: printf("reg : %5d", instruction.l.reg); break;
                case KUTINSTRUCTION_LOAD_TABLE: printf("reg : %5d", instruction.l.reg); break;
                case KUTINSTRUCTION_PUSH_REGISTER_1: printf("reg : %5d", instruction.r.reg0); break;
                case KUTINSTRUCTION_METHODCALL_PR: printf("self: %5d", instruction.l.reg); break;
                case KUTINSTRUCTION_METHODCALL_PC: printf(""); break;
                case KUTINSTRUCTION_PUSH_LITERAL: printf("lit : %5d", instruction.l.literal); break;
                case KUTINSTRUCTION_PUSH_CLOSURE: printf("clo : %5d", instruction.l.literal); break;
                case KUTINSTRUCTION_PUSH_TEMPLATE: printf("temp: %5d", instruction.l.literal); break;
                case KUTINSTRUCTION_PUSH_INTEGER: printf("int : %5d", instruction.l.literal); break;
                case KUTINSTRUCTION_PUSH_NIL: printf(""); break;
                case KUTINSTRUCTION_PUSH_UNDEFINED: printf(""); break;
                case KUTINSTRUCTION_PUSH_TABLE: printf(""); break;
                case KUTINSTRUCTION_POP_CLOSURE: printf("clo : %5d", instruction.l.literal); break;
                case KUTINSTRUCTION_METHODCALL_CR: printf("ret : %5d self: %5d", instruction.l.literal, instruction.l.reg); break;
                case KUTINSTRUCTION_METHODCALL_CC: printf("ret: %5d", instruction.l.literal); break;
                case KUTINSTRUCTION_SETCLOSURE_LITERAL: printf(""); break;
                case KUTINSTRUCTION_SETCLOSURE_CLOSURE: printf(""); break;
                case KUTINSTRUCTION_SETCLOSURE_TEMPLATE: printf(""); break;
                case KUTINSTRUCTION_SETCLOSURE_INTEGER: printf("clo : %5d int : %5d", instruction.l.literal, instruction.l.reg); break;
                case KUTINSTRUCTION_SETCLOSURE_NIL: printf("clo : %5d", instruction.l.literal); break;
                case KUTINSTRUCTION_SETCLOSURE_UNDEFINED: printf("clo : %5d", instruction.l.literal); break;
                case KUTINSTRUCTION_SETCLOSURE_TABLE: printf("clo : %5d", instruction.l.literal); break;
    }
    printf("\n");
}

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

static KutString* tostring_name = kutstring_literal("metin-yap");

KutValue kutfunc_debug(KutValue* _self) {
    KutFunc* self = _self ? kutfunc_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    KutValue registers = kuttable_wrap(kuttable_directPointer(self->register_count, self->registers));
    KutValue captures = kuttable_wrap(kuttable_directPointer(self->capture_count, self->captures));
    KutValue call_stack = kuttable_wrap(self->call_stack);
    KutString* register_string = kut_tostring(&registers, 0);
    KutString* capture_string = kut_tostring(&captures, 0);
    KutString* call_stack_string = kut_tostring(&call_stack, 0);
    if(register_string == NULL) {
        return kut_undefined;
    }
    size_t total_length = snprintf(NULL, 0, "func@%p\n\n\n\n", self) + register_string->len + capture_string->len + call_stack_string->len;
    for(size_t i = 0; i < self->instruction_count; i++) {
        KutInstruction instruction = self->instructions[i];
        switch(instruction.r.instruction) {
            case KUTINSTRUCTION_NO_OPERATION:           total_length += snprintf(NULL, 0, ""); break;
            case KUTINSTRUCTION_METHODCALL_IR:          total_length += snprintf(NULL, 0, "reg : %5d", instruction.r.reg0); break;
            case KUTINSTRUCTION_METHODCALL_IC:          total_length += snprintf(NULL, 0, ""); break;
            case KUTINSTRUCTION_PUSH_REGISTER_2:        total_length += snprintf(NULL, 0, "reg1: %5d reg2: %5d", instruction.r.reg0, instruction.r.reg1); break;
            case KUTINSTRUCTION_PUSH_REGISTER_3:        total_length += snprintf(NULL, 0, "reg1: %5d reg2: %5d reg3: %5d", instruction.r.reg0, instruction.r.reg1, instruction.r.reg2); break;
            case KUTINSTRUCTION_CREATE_CALLSTACK:       total_length += snprintf(NULL, 0, ""); break;
            case KUTINSTRUCTION_ASSIGN_REGISTER:        total_length += snprintf(NULL, 0, "src : %5d dest: %5d", instruction.r.reg0, instruction.r.reg1); break;
            case KUTINSTRUCTION_METHODCALL_RR:          total_length += snprintf(NULL, 0, "self: %5d ret : %5d", instruction.r.reg0, instruction.r.reg1); break;
            case KUTINSTRUCTION_METHODCALL_RC:          total_length += snprintf(NULL, 0, "ret : %5d", instruction.r.reg0); break;
            case KUTINSTRUCTION_LOAD_LITERAL:           total_length += snprintf(NULL, 0, "reg : %5d lit : %5d", instruction.l.reg, instruction.l.literal); break;
            case KUTINSTRUCTION_LOAD_CLOSURE:           total_length += snprintf(NULL, 0, "reg : %5d clos: %5d", instruction.l.reg, instruction.l.literal); break;
            case KUTINSTRUCTION_LOAD_TEMPLATE:          total_length += snprintf(NULL, 0, "reg : %5d temp: %5d", instruction.l.reg, instruction.l.literal); break;
            case KUTINSTRUCTION_LOAD_INTEGER:           total_length += snprintf(NULL, 0, "reg : %5d int : %5d", instruction.l.reg, instruction.l.literal); break;
            case KUTINSTRUCTION_LOAD_NIL:               total_length += snprintf(NULL, 0, "reg : %5d", instruction.l.reg); break;
            case KUTINSTRUCTION_LOAD_UNDEFINED:         total_length += snprintf(NULL, 0, "reg : %5d", instruction.l.reg); break;
            case KUTINSTRUCTION_LOAD_TABLE:             total_length += snprintf(NULL, 0, "reg : %5d", instruction.l.reg); break;
            case KUTINSTRUCTION_PUSH_REGISTER_1:        total_length += snprintf(NULL, 0, "reg : %5d", instruction.r.reg0); break;
            case KUTINSTRUCTION_METHODCALL_PR:          total_length += snprintf(NULL, 0, "self: %5d", instruction.l.reg); break;
            case KUTINSTRUCTION_METHODCALL_PC:          total_length += snprintf(NULL, 0, ""); break;
            case KUTINSTRUCTION_PUSH_LITERAL:           total_length += snprintf(NULL, 0, "lit : %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_PUSH_CLOSURE:           total_length += snprintf(NULL, 0, "clo : %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_PUSH_TEMPLATE:          total_length += snprintf(NULL, 0, "temp: %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_PUSH_INTEGER:           total_length += snprintf(NULL, 0, "int : %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_PUSH_NIL:               total_length += snprintf(NULL, 0, ""); break;
            case KUTINSTRUCTION_PUSH_UNDEFINED:         total_length += snprintf(NULL, 0, ""); break;
            case KUTINSTRUCTION_PUSH_TABLE:             total_length += snprintf(NULL, 0, ""); break;
            case KUTINSTRUCTION_POP_CLOSURE:            total_length += snprintf(NULL, 0, "clo : %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_METHODCALL_CR:          total_length += snprintf(NULL, 0, "ret : %5d self: %5d", instruction.l.literal, instruction.l.reg); break;
            case KUTINSTRUCTION_METHODCALL_CC:          total_length += snprintf(NULL, 0, "ret: %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_SETCLOSURE_LITERAL:     total_length += snprintf(NULL, 0, ""); break;
            case KUTINSTRUCTION_SETCLOSURE_CLOSURE:     total_length += snprintf(NULL, 0, ""); break;
            case KUTINSTRUCTION_SETCLOSURE_TEMPLATE:    total_length += snprintf(NULL, 0, ""); break;
            case KUTINSTRUCTION_SETCLOSURE_INTEGER:     total_length += snprintf(NULL, 0, "clo : %5d int : %5d", instruction.l.literal, instruction.l.reg); break;
            case KUTINSTRUCTION_SETCLOSURE_NIL:         total_length += snprintf(NULL, 0, "clo : %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_SETCLOSURE_UNDEFINED:   total_length += snprintf(NULL, 0, "clo : %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_SETCLOSURE_TABLE:       total_length += snprintf(NULL, 0, "clo : %5d", instruction.l.literal); break;
        }
    }

    KutString* ret = kutstring_zero(total_length);
    size_t offset = snprintf(ret->data, total_length+1, "func@%p\n", self);
    memcpy(ret->data+offset, register_string->data, register_string->len);
    offset += register_string->len;
    ret->data[offset] = '\n';
    offset += 1;

    memcpy(ret->data+offset, capture_string->data, capture_string->len);
    offset += capture_string->len;
    ret->data[offset] = '\n';
    offset += 1;

    memcpy(ret->data+offset, call_stack_string->data, call_stack_string->len);
    offset += call_stack_string->len;
    ret->data[offset] = '\n';
    offset += 1;
    for(size_t i = 0; i < self->instruction_count; i++) {
        KutInstruction instruction = self->instructions[i];
        offset += snprintf(ret->data+offset, ret->len-offset+1, "   %s", kutfunc_instructionNames[instruction.r.instruction]);
        switch(instruction.r.instruction) {
            case KUTINSTRUCTION_NO_OPERATION:           snprintf(ret->data+offset, ret->len-offset+1, ""); break;
            case KUTINSTRUCTION_METHODCALL_IR:          snprintf(ret->data+offset, ret->len-offset+1, "reg : %5d", instruction.r.reg0); break;
            case KUTINSTRUCTION_METHODCALL_IC:          snprintf(ret->data+offset, ret->len-offset+1, ""); break;
            case KUTINSTRUCTION_PUSH_REGISTER_2:        snprintf(ret->data+offset, ret->len-offset+1, "reg1: %5d reg2: %5d", instruction.r.reg0, instruction.r.reg1); break;
            case KUTINSTRUCTION_PUSH_REGISTER_3:        snprintf(ret->data+offset, ret->len-offset+1, "reg1: %5d reg2: %5d reg3: %5d", instruction.r.reg0, instruction.r.reg1, instruction.r.reg2); break;
            case KUTINSTRUCTION_CREATE_CALLSTACK:       snprintf(ret->data+offset, ret->len-offset+1, ""); break;
            case KUTINSTRUCTION_ASSIGN_REGISTER:        snprintf(ret->data+offset, ret->len-offset+1, "src : %5d dest: %5d", instruction.r.reg0, instruction.r.reg1); break;
            case KUTINSTRUCTION_METHODCALL_RR:          snprintf(ret->data+offset, ret->len-offset+1, "self: %5d ret : %5d", instruction.r.reg0, instruction.r.reg1); break;
            case KUTINSTRUCTION_METHODCALL_RC:          snprintf(ret->data+offset, ret->len-offset+1, "ret : %5d", instruction.r.reg0); break;
            case KUTINSTRUCTION_LOAD_LITERAL:           snprintf(ret->data+offset, ret->len-offset+1, "reg : %5d lit : %5d", instruction.l.reg, instruction.l.literal); break;
            case KUTINSTRUCTION_LOAD_CLOSURE:           snprintf(ret->data+offset, ret->len-offset+1, "reg : %5d clos: %5d", instruction.l.reg, instruction.l.literal); break;
            case KUTINSTRUCTION_LOAD_TEMPLATE:          snprintf(ret->data+offset, ret->len-offset+1, "reg : %5d temp: %5d", instruction.l.reg, instruction.l.literal); break;
            case KUTINSTRUCTION_LOAD_INTEGER:           snprintf(ret->data+offset, ret->len-offset+1, "reg : %5d int : %5d", instruction.l.reg, instruction.l.literal); break;
            case KUTINSTRUCTION_LOAD_NIL:               snprintf(ret->data+offset, ret->len-offset+1, "reg : %5d", instruction.l.reg); break;
            case KUTINSTRUCTION_LOAD_UNDEFINED:         snprintf(ret->data+offset, ret->len-offset+1, "reg : %5d", instruction.l.reg); break;
            case KUTINSTRUCTION_LOAD_TABLE:             snprintf(ret->data+offset, ret->len-offset+1, "reg : %5d", instruction.l.reg); break;
            case KUTINSTRUCTION_PUSH_REGISTER_1:        snprintf(ret->data+offset, ret->len-offset+1, "reg : %5d", instruction.r.reg0); break;
            case KUTINSTRUCTION_METHODCALL_PR:          snprintf(ret->data+offset, ret->len-offset+1, "self: %5d", instruction.l.reg); break;
            case KUTINSTRUCTION_METHODCALL_PC:          snprintf(ret->data+offset, ret->len-offset+1, ""); break;
            case KUTINSTRUCTION_PUSH_LITERAL:           snprintf(ret->data+offset, ret->len-offset+1, "lit : %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_PUSH_CLOSURE:           snprintf(ret->data+offset, ret->len-offset+1, "clo : %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_PUSH_TEMPLATE:          snprintf(ret->data+offset, ret->len-offset+1, "temp: %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_PUSH_INTEGER:           snprintf(ret->data+offset, ret->len-offset+1, "int : %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_PUSH_NIL:               snprintf(ret->data+offset, ret->len-offset+1, ""); break;
            case KUTINSTRUCTION_PUSH_UNDEFINED:         snprintf(ret->data+offset, ret->len-offset+1, ""); break;
            case KUTINSTRUCTION_PUSH_TABLE:             snprintf(ret->data+offset, ret->len-offset+1, ""); break;
            case KUTINSTRUCTION_POP_CLOSURE:            snprintf(ret->data+offset, ret->len-offset+1, "clo : %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_METHODCALL_CR:          snprintf(ret->data+offset, ret->len-offset+1, "ret : %5d self: %5d", instruction.l.literal, instruction.l.reg); break;
            case KUTINSTRUCTION_METHODCALL_CC:          snprintf(ret->data+offset, ret->len-offset+1, "ret: %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_SETCLOSURE_LITERAL:     snprintf(ret->data+offset, ret->len-offset+1, ""); break;
            case KUTINSTRUCTION_SETCLOSURE_CLOSURE:     snprintf(ret->data+offset, ret->len-offset+1, ""); break;
            case KUTINSTRUCTION_SETCLOSURE_TEMPLATE:    snprintf(ret->data+offset, ret->len-offset+1, ""); break;
            case KUTINSTRUCTION_SETCLOSURE_INTEGER:     snprintf(ret->data+offset, ret->len-offset+1, "clo : %5d int : %5d", instruction.l.literal, instruction.l.reg); break;
            case KUTINSTRUCTION_SETCLOSURE_NIL:         snprintf(ret->data+offset, ret->len-offset+1, "clo : %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_SETCLOSURE_UNDEFINED:   snprintf(ret->data+offset, ret->len-offset+1, "clo : %5d", instruction.l.literal); break;
            case KUTINSTRUCTION_SETCLOSURE_TABLE:       snprintf(ret->data+offset, ret->len-offset+1, "clo : %5d", instruction.l.literal); break;
        }
    }
    KutValue regstr = kutstring_wrap(register_string);
    kut_decref(&regstr);
    regstr = kutstring_wrap(capture_string);
    kut_decref(&regstr);
    regstr = kutstring_wrap(call_stack_string);
    kut_decref(&regstr);
    free(kuttable_cast(registers));
    free(kuttable_cast(captures));
    return kutstring_wrap(ret);
}

KutString* kutfunc_tostring(KutValue* _self, size_t indent) {
    KutFunc* self = _self ? kutfunc_cast(*_self) : NULL;
    if(self == NULL) {
        return NULL;
    }
    size_t len = snprintf(NULL, 0, "func/%"PRIXPTR, self) + indent*(sizeof("\t")-1);
    KutString* ret = kutstring_zero(len);
    memset(ret->data, '\t', indent);
    snprintf(ret->data+indent, ret->len+1, "func/%"PRIXPTR, self);
    return ret;
}

#include "kutfunc.methods"
#include "kutstring.h"

KutDispatchedFn kutfunc_dispatch(KutValue* self, KutString* message) {
    const struct KutDispatchGperfPair* entry = kutfunc_dispatchLookup(message->data, message->len);
    if(not entry)
        return empty_dispatched;
    return entry->method;
}

#if 1
KutInstruction kutinstruction_noOperation(void) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_NO_OPERATION, .reg = 0, .literal = 0}};
}
KutInstruction kutinstruction_methodcallIR(uint8_t reg) {
    return (KutInstruction){.r = {.instruction = KUTINSTRUCTION_METHODCALL_IR, .reg0 = reg, .reg1 = 0, .reg2 = 0}};
}
KutInstruction kutinstruction_methodcallIC(void) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_METHODCALL_IC, .reg = 0, .literal = 0}};
}
KutInstruction kutinstruction_pushRegister2(uint8_t reg1, uint8_t reg2) {
    return (KutInstruction){.r = {.instruction = KUTINSTRUCTION_PUSH_REGISTER_2, .reg0 = reg1, .reg1 = reg2, .reg2 = 0}};
}
KutInstruction kutinstruction_pushRegister3(uint8_t reg1, uint8_t reg2, uint8_t reg3) {
    return (KutInstruction){.r = {.instruction = KUTINSTRUCTION_PUSH_REGISTER_3, .reg0 = reg1, .reg1 = reg2, .reg2 = reg3}};
}
KutInstruction kutinstruction_createCallstack(void) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_CREATE_CALLSTACK, .reg = 0, .literal = 0}};
}

// Result is assigned to a register

KutInstruction kutinstruction_assignRegister(uint8_t dest, uint8_t src) {
    return (KutInstruction){.r = {.instruction = KUTINSTRUCTION_ASSIGN_REGISTER, .reg0 = dest, .reg1 = src, .reg2 = 0}};
}
KutInstruction kutinstruction_methodcallRR(uint8_t self, uint8_t ret) {
    return (KutInstruction){.r = {.instruction = KUTINSTRUCTION_METHODCALL_RR, .reg0 = ret, .reg1 = self, .reg2 = 0}};
}
KutInstruction kutinstruction_methodcallRC(uint8_t ret) {
    return (KutInstruction){.r = {.instruction = KUTINSTRUCTION_METHODCALL_RC, .reg0 = ret, .reg1 = 0, .reg2 = 0}};
}
KutInstruction kutinstruction_loadLiteral(uint8_t reg, uint16_t literal) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_LOAD_LITERAL, .reg = reg, .literal = literal}};
}
KutInstruction kutinstruction_loadClosure(uint8_t reg, uint16_t closure) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_LOAD_CLOSURE, .reg = reg, .literal = closure}};
}
KutInstruction kutinstruction_loadTemplate(uint8_t reg, uint16_t template) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_LOAD_TEMPLATE, .reg = reg, .literal = template}};
}
KutInstruction kutinstruction_loadInteger(uint8_t reg, uint16_t integer) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_LOAD_INTEGER, .reg = reg, .literal = integer}};
}
KutInstruction kutinstruction_loadNil(uint8_t reg) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_LOAD_NIL, .reg = reg, .literal = 0}};
}
KutInstruction kutinstruction_loadUndefined(uint8_t reg) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_LOAD_UNDEFINED, .reg = reg, .literal = 0}};
}
KutInstruction kutinstruction_loadTable(uint8_t reg) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_LOAD_TABLE, .reg = reg, .literal = 0}};
}

// Result is directly pushed to the call stack

KutInstruction kutinstruction_pushRegister1(uint8_t reg) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_PUSH_REGISTER_1, .reg = reg, .literal = 0}};
}
KutInstruction kutinstruction_methodcallPR(uint8_t self) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_METHODCALL_PR, .reg = self, .literal = 0}};
}
KutInstruction kutinstruction_methodcallPC(void) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_METHODCALL_PC, .reg = 0, .literal = 0}};
}
KutInstruction kutinstruction_pushLiteral(uint16_t literal) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_PUSH_LITERAL, .reg = 0, .literal = literal}};
}
KutInstruction kutinstruction_pushClosure(uint16_t closure) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_PUSH_CLOSURE, .reg = 0, .literal = closure}};
}
KutInstruction kutinstruction_pushTemplate(uint16_t template) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_PUSH_TEMPLATE, .reg = 0, .literal = template}};
}
KutInstruction kutinstruction_pushInteger(uint16_t integer) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_PUSH_INTEGER, .reg = 0, .literal = integer}};
}
KutInstruction kutinstruction_pushNil(void) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_PUSH_NIL, .reg = 0, .literal = 0}};
}
KutInstruction kutinstruction_pushUndefined(void) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_PUSH_UNDEFINED, .reg = 0, .literal = 0}};
}
KutInstruction kutinstruction_pushTable(void) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_PUSH_TABLE, .reg = 0, .literal = 0}};
}

// Result is assigned to a closure

KutInstruction kutinstruction_methodcallCR(uint16_t self, uint8_t ret) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_METHODCALL_CR, .reg = ret, .literal = self}};
}
KutInstruction kutinstruction_methodcallCC(uint16_t self) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_METHODCALL_CC, .reg = 0, .literal = self}};
}
KutInstruction kutinstruction_setclosureInteger(uint16_t closure, uint8_t integer) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_SETCLOSURE_INTEGER, .reg = integer, .literal = closure}};
}
KutInstruction kutinstruction_setclosureNil(uint16_t closure) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_SETCLOSURE_NIL, .reg = 0, .literal = closure}};
}
KutInstruction kutinstruction_setclosureUndefined(uint16_t closure) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_SETCLOSURE_UNDEFINED, .reg = 0, .literal = closure}};
}
KutInstruction kutinstruction_setclosureTable(uint16_t closure) {
    return (KutInstruction){.l = {.instruction = KUTINSTRUCTION_SETCLOSURE_TABLE, .reg = 0, .literal = closure}};
}
#endif

const KutMandatoryMethodsTable kutfunc_methods = {
    .dispatch = kutfunc_dispatch,
    .addref = kutfunc_addref,
    .decref = kutfunc_decref,
    .tostring = kutfunc_tostring,
};
