#include "kuterror.h"

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include "kutstring.h"

KutError* kuterror_newDirect(uint64_t code, KutString* message, const char* filename, const char* function_name, int line_number) {
    KutStackTrace* trace = calloc(1, sizeof(*trace));
    *trace = (KutStackTrace){
        .filename = kutstring_newCString(filename),
        .function = kutstring_newCString(function_name),
        .line_number = line_number,
        .next = NULL,
    };
    KutError* ret = calloc(1, sizeof(*ret));
    *ret = (KutError){
        .code = code,
        .message = message,
        .reference_count = 1,
        .trace = trace,
        .last_trace = trace,
    };
    kut_addref(message);
    return ret;
}

void kuterror_addTraceDirect(KutError* err, const char* filename, const char* function_name, int line_number) {
    KutStackTrace* added_trace = calloc(1, sizeof(*added_trace));
    *added_trace = (KutStackTrace){
        .filename = kutstring_newCString(filename),
        .function = kutstring_newCString(function_name),
        .line_number = line_number,
        .next = NULL,
    };
    err->last_trace->next = added_trace;
    err->last_trace = added_trace;
}

KutValue kuterror_wrap(KutError* err) {
    return kut_wrap((KutData){.data = err}, &kuterror_methods);
}

KutError* kuterror_cast(KutValue val) {
    if(istype(val, kuterror)) {
        return val.data.data;
    }
    return NULL;
}

void kuterror_addref(KutValue* _self) {
    KutError* self = _self ? kuterror_cast(*_self) : NULL;
    if(self == NULL) {
        return;
    }
    if(self->reference_count != 0) {
        self->reference_count += 1;
    }
}

void kuterror_decref(KutValue* _self) {
    KutError* self = _self ? kuterror_cast(*_self) : NULL;
    if(self == NULL) {
        return;
    }
    if(self->reference_count == 0) {
        return;
    }
    if(self->reference_count == 1) {
        KutStackTrace* current_trace = self->trace;
        while(current_trace != NULL) {
            KutStackTrace* next_trace = current_trace->next;
            free(current_trace);
            current_trace = next_trace;
        }
        KutValue msg = kutstring_wrap(self->message);
        kutstring_methods.decref(&msg);
        free(self);
        return;
    }
    self->reference_count -= 1;
}

KutString* kuterror_tostring(KutValue* _self) {
    KutError* self = _self ? kuterror_cast(*_self) : NULL;
    if(self == NULL) {
        return NULL;
    }
    size_t len = snprintf(NULL, 0, "Error %"PRIu64" occured at %.*s:%.*s:%zu: \"%.*s\"\n", 
        self->code,
        kutstring_format(self->trace->filename),
        kutstring_format(self->trace->function),
        self->trace->line_number,
        kutstring_format(self->message)
    );
    KutStackTrace* current_trace = self->trace->next;
    while(current_trace != NULL) {
        len += snprintf(NULL, 0, "Trace: %.*s:%.*s:%zu\n", current_trace->filename, current_trace->function, current_trace->line_number);
        current_trace = current_trace->next;
    }
    current_trace = self->trace;
    KutString* ret = kutstring_zero(len);
    size_t offset = 0;
    snprintf(ret->data+offset, ret->len-offset+1, "Error %"PRIu64" occured at %.*s:%.*s:%zu: \"%.*s\"\n", 
        self->code,
        kutstring_format(self->trace->filename),
        kutstring_format(self->trace->function),
        self->trace->line_number,
        kutstring_format(self->message)
    );
    while(current_trace != NULL) {
        snprintf(ret->data+offset, ret->len-offset+1, "Trace: %.*s:%.*s:%zu\n",
            current_trace->filename, current_trace->function, current_trace->line_number
        );
        current_trace = current_trace->next;
    }
    return ret;
}

KutDispatchedFn kuterror_dispatch(KutValue* _self) {
    return empty_dispatched;
}

const KutMandatoryMethodsTable kuterror_methods = {
    .dispatch = kuterror_dispatch,
    .addref = kuterror_addref,
    .decref = kuterror_decref,
    .tostring = kuterror_tostring,
};
