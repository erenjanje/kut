#include "kutfunc.h"

#include <stdlib.h>
#include <stdbool.h>

#include "kutvm.h"
#include "kuttable.h"

static KutValue move_register(KutFunc* self, uint8_t reg) {
    KutValue* ret = calloc(1, sizeof(*ret));
    *ret = self->registers[reg];
    self->registers[reg] = kutreference_wrap(ret);
    ((KutValue*)self->registers[reg].data.data)->reference_count = 2;
    return kutreference_wrap(ret);
}

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
            ret->captures[i] = move_register(context, capture_info);
        } else { // Capture reference
            capture_info -= 256;
            kut_addref(context->captures[capture_info]);
            ret->captures[i] = context->captures[capture_info];
        }
    }
    
    return ret;
}

KutValue kutfunc_wrap(KutFunc* self) {
    return kut_wrap((KutData){.data = self}, kutfunc_dispatch);
}

KutValue kutfunc_addref(KutData _self, KutTable* args) {
    KutFunc* self = _self.data;
    if(self->reference_count != 0)
        self->reference_count += 1;
    return kutboolean_wrap(true);
}

KutValue kutfunc_decref(KutData _self, KutTable* args) {
    KutFunc* self = _self.data;
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

KutValue kutfunc_run(KutData _self, KutTable* args) {
    KutFunc* self = _self.data;
    bool finished = false;
    for(size_t i = 0; not finished; i++) {
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

void __kutfunc_print(KutFunc* func) {
    KutInstruction current_instruction = func->instructions[0];
    const KutInstruction finishing_instruction = kutfunc_emptyInstruction(KI_NOPERATION);
    size_t i = 0;
    do {
        current_instruction = func->instructions[i];
        i += 1;
        printf("%s", kutfunc_serializeInstruction(current_instruction));
        switch(current_instruction.l.instruction) {
            case KI_NOPERATION:
                printf("\n");
                break;
            
            case KI_METHODCALL:
                printf("retn: %5u,\tself: %5u,\tmssg: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1, current_instruction.r.reg2);
                break;
            case KI_RETURNCALL:
                printf("retn: %5u\n", current_instruction.r.reg0);
                break;
            case KI_PUSHVALUE1:
                printf("val0: %5u\n", current_instruction.r.reg0);
                break;
            case KI_PUSHVALUE2:
                printf("val1: %5u,\tval2: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1);
                break;
            case KI_PUSHVALUE3:
                printf("val1: %5u,\tval2: %5u,\tval3: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1, current_instruction.r.reg2);
                break;
            case KI_MVREGISTER:
                printf("dest: %5u,\tsorc: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1);
                break;
            case KI_SWAPREGIST:
                printf("reg1: %5u,\treg2: %5u\n", current_instruction.r.reg0, current_instruction.r.reg1);
                break;
            
            case KI_GETLITERAL:
            case KI_GETCLOSURE:
            case KI_SETCLOSURE:
            case KI_GETTMPLATE:
                printf("regs: %5u,\tlitr: %5u\n", current_instruction.l.reg, current_instruction.l.literal);
                break;
        }
    } while(memcmp(&current_instruction, &finishing_instruction, sizeof(KutInstruction)));
}

#include "kutstring.h"

KutValue kutfunc_print(KutData _self, KutTable* args) {
    KutFunc* self = _self.data;
    printf("REGS: ");
    for(size_t i = 0; i < self->register_count; i++) {
        KutValue reg = self->registers[i];
        if(istype(reg, kutnumber)) {
            printf("%g ", reg.data.number);
        } else if(istype(reg, kutstring)) {
            printf("\"%.*s\" ", kutstring_format(((KutString*)reg.data.data)));
        } else if(istype(reg, kutundefined)) {
            printf("undefined ");
        } else if(istype(reg, kutboolean)) {
            printf("%s ", reg.data.boolean ? "true" : "false");
        } else if(reg.dispatch == NULL) {
            printf("nil ");
        } else if(istype(reg, kutfunc)) {
            printf("func-%p ", reg.data.data);
        } else if(istype(reg, kutreference)) {
            printf("ref-%p", reg.data.data);
        } else {
            printf("other ");
        }
    }
    printf("\n");
    __kutfunc_print(self);
    return kut_undefined;
}

#include "kutfunc.methods"
#include "kutstring.h"

KutDispatchedFn kutfunc_dispatch(KutData self, KutString* message) {
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
