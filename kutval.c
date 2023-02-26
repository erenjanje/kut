#include "kutval.h"
#include "kutstring.h"

KutValue empty_dispatched(KutData self, KutTable* args) { // TODO: Remove me!
    return kut_undefined;
}

KutDispatchedFn kutundefined_dispatch(KutData self, KutString* message) {
    return empty_dispatched;
}

#include "kuttable.h"

#undef checkarg

bool checkarg(KutTable* args, size_t index, KutDispatchFn type) {
    return ((args->len >= index) and args->data[index].dispatch == type);
}

KutString* const addref_name = kutstring_literal("referans-ekle");
KutString* const decref_name = kutstring_literal("referans-çıkar");

void kut_addref(KutValue self) {
    if(self.dispatch) {
        self.dispatch(self.data, addref_name)(self.data, empty_table);
    }
}

void kut_decref(KutValue* self) {
    if(self->dispatch) {
        if(not self->dispatch(self->data, decref_name)(self->data, empty_table).data.boolean) {
            size_t ref = self->reference_count;
            *self = kut_undefined;
            self->reference_count = ref;
        }
    }
}

KutDispatchedFn kutboolean_dispatch(KutData self, KutString* message) {
    return empty_dispatched;
}

KutDispatchedFn kutinteger_dispatch(KutData self, KutString* message) {
    return empty_dispatched;
}
