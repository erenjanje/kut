#include "kutfunc.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

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

const char* kutfunc_serializeInstruction(KutInstruction instruction) {
    switch(instruction.r.instruction) {
            case KI_NOPERATION: return "NOPERATION\t";
            case KI_NEWCLLSTAK: return "NEWCLLSTAK\t";
            case KI_METHODCALL: return "METHODCALL\t";
            case KI_RETURNCALL: return "RETURNCALL\t";
            case KI_PUSHVALUE1: return "PUSHVALUE1\t";
            case KI_PUSHVALUE2: return "PUSHVALUE2\t";
            case KI_PUSHVALUE3: return "PUSHVALUE3\t";
            case KI_MVREGISTER: return "MVREGISTER\t";
            case KI_SWAPREGIST: return "SWAPREGIST\t";
            case KI_BRANCHWITH: return "BRANCHWITH\t";
            case KI_GETLITERAL: return "GETLITERAL\t";
            case KI_GETCLOSURE: return "GETCLOSURE\t";
            case KI_SETCLOSURE: return "SETCLOSURE\t";
            case KI_GETTMPLATE: return "GETTMPLATE\t";
            case KI_LOAD16LITR: return "LOAD16LITR\t";
            case KI_LOADNILVAL: return "LOADNILVAL\t";
            case KI_LOADUNDEFN: return "LOADUNDEFN\t";
            default: return "UNKNOWN_INSTRUCTION\t";
    }
}

void kutfunc_debugInstruction(KutInstruction instruction) {
    printf("%-16s ", kutfunc_serializeInstruction(instruction));
    switch(instruction.r.instruction) {
        case KI_NOPERATION:
            break;
        case KI_NEWCLLSTAK:
            break;
        case KI_METHODCALL:
            printf("ret  %5d self %5d msg  %5d", instruction.r.reg0, instruction.r.reg1, instruction.r.reg2);
            break;
        case KI_RETURNCALL:
            printf("ret  %5d", instruction.r.reg0);
            break;
        case KI_PUSHVALUE1:
            printf("val  %5d", instruction.r.reg0);
            break;
        case KI_PUSHVALUE2:
            printf("val1 %5d val2 %5d", instruction.r.reg0, instruction.r.reg1);
            break;
        case KI_PUSHVALUE3:
            printf("val1 %5d val2 %5d val3 %5d", instruction.r.reg0, instruction.r.reg1, instruction.r.reg2);
            break;
        case KI_MVREGISTER:
            printf("dest %5d src  %5d", instruction.r.reg0, instruction.r.reg1);
            break;
        case KI_SWAPREGIST:
            printf("reg1 %5d reg2 %5d", instruction.r.reg0, instruction.r.reg1);
            break;
        case KI_BRANCHWITH:
            printf("cond %5d if   %5d else %5d", instruction.r.reg0, instruction.r.reg1, instruction.r.reg2);
            break;
        case KI_GETLITERAL:
            printf("reg  %5d lit  %5d", instruction.l.reg, instruction.l.literal);
            break;
        case KI_GETCLOSURE:
            printf("reg  %5d lit  %5d", instruction.l.reg, instruction.l.literal);
            break;
        case KI_SETCLOSURE:
            printf("reg  %5d lit  %5d", instruction.l.reg, instruction.l.literal);
            break;
        case KI_GETTMPLATE:
            printf("reg  %5d lit  %5d", instruction.l.reg, instruction.l.literal);
            break;
        case KI_LOAD16LITR:
            printf("reg  %5d lit  %5d", instruction.l.reg, instruction.l.literal);
            break;
        case KI_LOADNILVAL:
            printf("reg  %5d", instruction.l.reg);
            break;
        case KI_LOADUNDEFN:
            printf("reg  %5d", instruction.l.reg);
            break;
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
    for(size_t i = 0; self->instructions[i].r.instruction != KI_NOPERATION; i++) {
        KutInstruction current_instruction = self->instructions[i];
        total_length += snprintf(NULL, 0, "   %s", kutfunc_serializeInstruction(current_instruction));
        switch(current_instruction.l.instruction) {
            case KI_NOPERATION:
                total_length += snprintf(NULL, 0, "\n");
                break;
            
            case KI_METHODCALL:
                total_length += snprintf(NULL, 0, "ret : %5u\tself: %5u\tmsg : %5u\n", current_instruction.r.reg0, current_instruction.r.reg1, current_instruction.r.reg2);
                break;
            case KI_RETURNCALL:
                total_length += snprintf(NULL, 0, "ret : %5u\n", current_instruction.r.reg0);
                break;
            case KI_PUSHVALUE1:
                total_length += snprintf(NULL, 0, "val : %5u\n", current_instruction.r.reg0);
                break;
            case KI_PUSHVALUE2:
                total_length += snprintf(NULL, 0, "val1: %5u\tval2: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1);
                break;
            case KI_PUSHVALUE3:
                total_length += snprintf(NULL, 0, "val1: %5u\tval2: %5u\tval3: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1, current_instruction.r.reg2);
                break;
            case KI_MVREGISTER:
                total_length += snprintf(NULL, 0, "dest: %5u\tsrc : %5u\n", current_instruction.r.reg0, current_instruction.r.reg1);
                break;
            case KI_SWAPREGIST:
                total_length += snprintf(NULL, 0, "reg1: %5u\treg2: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1);
                break;
            case KI_BRANCHWITH:
                total_length += snprintf(NULL, 0, "cond: %5u\tif  : %5u\telse: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1, current_instruction.r.reg2);
                break;
            
            case KI_GETLITERAL:
            case KI_GETCLOSURE:
            case KI_SETCLOSURE:
            case KI_GETTMPLATE:
            case KI_LOAD16LITR:
                total_length += snprintf(NULL, 0, "regs: %5u\tlitr: %5u\n", current_instruction.l.reg, current_instruction.l.literal);
                break;
            
            case KI_LOADNILVAL:
            case KI_LOADUNDEFN:
                total_length += snprintf(NULL, 0, "reg : %5u\n", current_instruction.l.reg);
                break;
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
    for(size_t i = 0; self->instructions[i].r.instruction != KI_NOPERATION; i++) {
        KutInstruction current_instruction = self->instructions[i];
        offset += snprintf(ret->data+offset, ret->len-offset+1, "   %s", kutfunc_serializeInstruction(current_instruction));
        switch(current_instruction.l.instruction) {
            case KI_NOPERATION:
                offset += snprintf(ret->data+offset, ret->len-offset+1, "\n");
                break;
            
            case KI_METHODCALL:
                offset += snprintf(ret->data+offset, ret->len-offset+1, "retn: %5u\tself: %5u\tmssg: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1, current_instruction.r.reg2);
                break;
            case KI_RETURNCALL:
                offset += snprintf(ret->data+offset, ret->len-offset+1, "retn: %5u\n", current_instruction.r.reg0);
                break;
            case KI_PUSHVALUE1:
                offset += snprintf(ret->data+offset, ret->len-offset+1, "val0: %5u\n", current_instruction.r.reg0);
                break;
            case KI_PUSHVALUE2:
                offset += snprintf(ret->data+offset, ret->len-offset+1, "val1: %5u\tval2: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1);
                break;
            case KI_PUSHVALUE3:
                offset += snprintf(ret->data+offset, ret->len-offset+1, "val1: %5u\tval2: %5u\tval3: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1, current_instruction.r.reg2);
                break;
            case KI_MVREGISTER:
                offset += snprintf(ret->data+offset, ret->len-offset+1, "dest: %5u\tsorc: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1);
                break;
            case KI_SWAPREGIST:
                offset += snprintf(ret->data+offset, ret->len-offset+1, "reg1: %5u\treg2: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1);
                break;
            case KI_BRANCHWITH:
                offset += snprintf(ret->data+offset, ret->len-offset+1, "cond: %5u\tif  : %5u\telse: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1, current_instruction.r.reg2);
                break;
            
            case KI_GETLITERAL:
            case KI_GETCLOSURE:
            case KI_SETCLOSURE:
            case KI_GETTMPLATE:
            case KI_LOAD16LITR:
                offset += snprintf(ret->data+offset, ret->len-offset+1, "regs: %5u\tlitr: %5u\n", current_instruction.l.reg, current_instruction.l.literal);
                break;

            case KI_LOADNILVAL:
            case KI_LOADUNDEFN:
                offset += snprintf(ret->data+offset, ret->len-offset+1, "reg : %5u\n", current_instruction.l.reg);
                break;
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
