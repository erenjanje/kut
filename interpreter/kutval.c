#include "kutval.h"
#include "kutstring.h"

KutValue empty_dispatched(KutValue* self, KutTable* args) { // TODO: Remove me!
    return kut_undefined;
}

static KutString* const addref_name = kutstring_literal("referans-ekle");
static KutString* const decref_name = kutstring_literal("referans-çıkar");
static KutString* const tostring_name = kutstring_literal("metin-yap");

static KutString* undefined_name = kutstring_literal("undefined");

KutValue kutundefined_tostring(KutValue* self, KutTable* args) {
    return kutstring_wrap(undefined_name);
}

KutDispatchedFn kutundefined_dispatch(KutValue* self, KutString* message) {
    if(kutstring_equalString(message, tostring_name)) {
        return kutundefined_tostring;
    }
    return empty_dispatched;
}

#include "kuttable.h"

#undef checkarg

bool checkarg(KutTable* args, size_t index, KutDispatchFn type) {
    return ((args->len >= index) and args->data[index].dispatch == type);
}

void kut_addref(KutValue* self) {
    if(self->dispatch) {
        self->dispatch(self, addref_name)(self, empty_table);
    }
}

void kut_decref(KutValue* self) {
    if(self->dispatch) {
        if(not kutboolean_cast(self->dispatch(self, decref_name)(self, empty_table))) {
            size_t ref = self->reference_count;
            self->reference_count = ref;
        }
    }
    *self = kut_undefined;
}

KutString* const nil_name = kutstring_literal("nil");

KutString* kut_tostring(KutValue* self) {
    if(self == NULL) {
        return NULL;
    }
    if(self->dispatch == NULL) {
        return nil_name;
    }
    KutValue val = self->dispatch(self, tostring_name)(self, empty_table);
    return kutstring_cast(val);
}

KutDispatchedFn kutboolean_dispatch(KutValue* self, KutString* message) {
    return empty_dispatched;
}
