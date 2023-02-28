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
    return kut_wrap((KutData){.data = self}, kutfunc_dispatch);
}

KutValue kutfunc_addref(KutValue* _self, KutTable* args) {
    KutFunc* self = _self ? kutfunc_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    if(self->reference_count != 0)
        self->reference_count += 1;
    return kutboolean_wrap(true);
}

KutValue kutfunc_decref(KutValue* _self, KutTable* args) {
    KutFunc* self = _self ? kutfunc_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    if(self->reference_count == 0)
        return kutboolean_wrap(true);
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
        return kutboolean_wrap(false);
    }
    self->reference_count -= 1;
    return kutboolean_wrap(true);
}

KutValue kutfunc_run(KutValue* _self, KutTable* args) {
    KutFunc* self = _self ? kutfunc_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    bool finished = false;
    for(size_t i = 0; not finished; i++) {
        printf("Instruction %zu:\n", i);
        KutString* debg = kutstring_cast(kutfunc_debug(_self, args));
        printf("%.*s\n\n", kutstring_format(debg));
        free(debg);
        KutInstruction instruction = self->instructions[i];
        finished = instruction_handlers[instruction.r.instruction](self, instruction);
    }
    for(size_t i = 0; i < self->register_count; i++) {
        kut_decref(&self->registers[i]);
    }
    for(size_t i = 0; i < self->capture_count; i++) {
        kut_decref(&self->captures[i]);
    }
    KutValue ret = self->ret;
    self->ret = kut_undefined;
    return ret;
}

KutInstruction kutfunc_emptyInstruction(KutEmptyInstructionName name) {
    return (KutInstruction){.l = {.instruction = name, .reg = 0, .literal = 0}};
}

KutInstruction kutfunc_registerInstruction(KutRegisterInstructionName name, uint8_t reg0, uint8_t reg1, uint8_t reg2) {
    return (KutInstruction){.r = {.instruction = name, .reg0 = reg0, .reg1 = reg1, .reg2 = reg2}};
}

KutInstruction kutfunc_literalInstruction(KutLiteralInstructionName name, uint8_t reg, uint16_t literal) {
    return (KutInstruction){.l = {.instruction = name, .reg = reg, .literal = literal}};
}

const char* kutfunc_serializeInstruction(KutInstruction instruction) {
    switch(instruction.r.instruction) {
            case KI_NOPERATION: return "NOPERATION\t";
            case KI_METHODCALL: return "METHODCALL\t";
            case KI_RETURNCALL: return "RETURNCALL\t";
            case KI_PUSHVALUE1: return "PUSHVALUE1\t";
            case KI_PUSHVALUE2: return "PUSHVALUE2\t";
            case KI_PUSHVALUE3: return "PUSHVALUE3\t";
            case KI_MVREGISTER: return "MVREGISTER\t";
            case KI_SWAPREGIST: return "SWAPREGIST\t";
            case KI_GETLITERAL: return "GETLITERAL\t";
            case KI_GETCLOSURE: return "GETCLOSURE\t";
            case KI_SETCLOSURE: return "SETCLOSURE\t";
            case KI_GETTMPLATE: return "GETTMPLATE\t";
            default: return "UNKNOWN_INSTRUCTION\n";
    }
}

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

static KutString* tostring_name = kutstring_literal("metin-yap");

KutValue kutfunc_debug(KutValue* _self, KutTable* args) {
    KutFunc* self = _self ? kutfunc_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    KutValue registers = kuttable_wrap(kuttable_directPointer(self->register_count, self->registers));
    KutString* register_string = kut_tostring(&registers);
    if(register_string == NULL) {
        return kut_undefined;
    }
    size_t total_length = snprintf(NULL, 0, "func@%p\n\n", self) + register_string->len;
    for(size_t i = 0; self->instructions[i].r.instruction != KI_NOPERATION; i++) {
        KutInstruction current_instruction = self->instructions[i];
        total_length += snprintf(NULL, 0, "   %s", kutfunc_serializeInstruction(current_instruction));
        switch(current_instruction.l.instruction) {
            case KI_NOPERATION:
                total_length += snprintf(NULL, 0, "\n");
                break;
            
            case KI_METHODCALL:
                total_length += snprintf(NULL, 0, "retn: %5u\tself: %5u\tmssg: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1, current_instruction.r.reg2);
                break;
            case KI_RETURNCALL:
                total_length += snprintf(NULL, 0, "retn: %5u\n", current_instruction.r.reg0);
                break;
            case KI_PUSHVALUE1:
                total_length += snprintf(NULL, 0, "val0: %5u\n", current_instruction.r.reg0);
                break;
            case KI_PUSHVALUE2:
                total_length += snprintf(NULL, 0, "val1: %5u\tval2: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1);
                break;
            case KI_PUSHVALUE3:
                total_length += snprintf(NULL, 0, "val1: %5u\tval2: %5u\tval3: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1, current_instruction.r.reg2);
                break;
            case KI_MVREGISTER:
                total_length += snprintf(NULL, 0, "dest: %5u\tsorc: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1);
                break;
            case KI_SWAPREGIST:
                total_length += snprintf(NULL, 0, "reg1: %5u\treg2: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1);
                break;
            
            case KI_GETLITERAL:
            case KI_GETCLOSURE:
            case KI_SETCLOSURE:
            case KI_GETTMPLATE:
                total_length += snprintf(NULL, 0, "regs: %5u\tlitr: %5u\n", current_instruction.l.reg, current_instruction.l.literal);
                break;
        }
    }

    KutString* ret = kutstring_zero(total_length);
    size_t offset = 0;
    memcpy(ret->data+offset, register_string->data, register_string->len);
    offset += register_string->len;
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
            
            case KI_GETLITERAL:
            case KI_GETCLOSURE:
            case KI_SETCLOSURE:
            case KI_GETTMPLATE:
                offset += snprintf(ret->data+offset, ret->len-offset+1, "regs: %5u\tlitr: %5u\n", current_instruction.l.reg, current_instruction.l.literal);
                break;
        }
    }
    KutValue regstr = kutstring_wrap(register_string);
    kut_decref(&regstr);
    free(kuttable_cast(registers));
    return kutstring_wrap(ret);
}

KutValue kutfunc_tostring(KutValue* _self, KutTable* args) {
    KutFunc* self = _self ? kutfunc_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    size_t len = snprintf(NULL, 0, "func/%"PRIXPTR, self);
    KutString* ret = kutstring_zero(len);
    snprintf(ret->data, ret->len+1, "func/%"PRIXPTR, self);
    return kutstring_wrap(ret);
}

#include "kutfunc.methods"
#include "kutstring.h"

KutDispatchedFn kutfunc_dispatch(KutValue* self, KutString* message) {
    const struct KutDispatchGperfPair* entry = kutfunc_dispatchLookup(message->data, message->len);
    if(not entry)
        return empty_dispatched;
    return entry->method;
}

KutInstruction kutfunc_noperation(void) {
    return (KutInstruction){.l = {.instruction = KI_NOPERATION, .reg = 0, .literal = 0}};
}

KutInstruction kutfunc_methodcall(uint8_t return_position, uint8_t self, uint8_t message) {
    return (KutInstruction){.r = {.instruction = KI_METHODCALL, .reg0 = return_position, .reg1 = self, .reg2 = message}};
}

KutInstruction kutfunc_returncall(uint8_t returned_value) {
    return (KutInstruction){.r = {.instruction = KI_RETURNCALL, .reg0 = returned_value, .reg1 = 0, .reg2 = 0}};
}

KutInstruction kutfunc_pushvalue1(uint8_t value) {
    return (KutInstruction){.r = {.instruction = KI_PUSHVALUE1, .reg0 = value, .reg1 = 0, .reg2 = 0}};
}

KutInstruction kutfunc_pushvalue2(uint8_t value1, uint8_t value2) {
    return (KutInstruction){.r = {.instruction = KI_PUSHVALUE2, .reg0 = value1, .reg1 = value2, .reg2 = 0}};
}

KutInstruction kutfunc_pushvalue3(uint8_t value1, uint8_t value2, uint8_t value3) {
    return (KutInstruction){.r = {.instruction = KI_PUSHVALUE3, .reg0 = value1, .reg1 = value2, .reg2 = value3}};
}

KutInstruction kutfunc_mvregister(uint8_t destination, uint8_t source) {
    return (KutInstruction){.r = {.instruction = KI_MVREGISTER, .reg0 = destination, .reg1 = source, .reg2 = 0}};
}

KutInstruction kutfunc_swapregist(uint8_t reg1, uint8_t reg2) {
    return (KutInstruction){.r = {.instruction = KI_SWAPREGIST, .reg0 = reg1, .reg1 = reg2, .reg2 = 0}};
}

KutInstruction kutfunc_getliteral(uint8_t reg, uint16_t literal) {
    return (KutInstruction){.l = {.instruction = KI_GETLITERAL, .reg = reg, .literal = literal}};
}

KutInstruction kutfunc_getclosure(uint8_t reg, uint16_t literal) {
    return (KutInstruction){.l = {.instruction = KI_GETCLOSURE, .reg = reg, .literal = literal}};
}

KutInstruction kutfunc_setclosure(uint8_t reg, uint16_t literal) {
    return (KutInstruction){.l = {.instruction = KI_SETCLOSURE, .reg = reg, .literal = literal}};
}

KutInstruction kutfunc_gettmplate(uint8_t reg, uint16_t literal) {
    return (KutInstruction){.l = {.instruction = KI_GETTMPLATE, .reg = reg, .literal = literal}};
}
