#include "kutval.h"
#include "kutstring.h"

KutValue empty_dispatched(KutValue* self, KutTable* args) { // TODO: Remove me!
    return kut_undefined;
}

static KutString* const addref_name = kutstring_literal("referans-ekle");
static KutString* const decref_name = kutstring_literal("referans-çıkar");
static KutString* const tostring_name = kutstring_literal("metin-yap");

static KutString* undefined_name = kutstring_literal("undefined");

KutString* kutundefined_tostring(KutValue* self, size_t indent) {
    return undefined_name;
}

KutDispatchedFn kutundefined_dispatch(KutValue* self, KutString* message) {
    return empty_dispatched;
}

const KutMandatoryMethodsTable kutundefined_methods = {
    .dispatch = kutundefined_dispatch,
    .addref = NULL,
    .decref = NULL,
    .tostring = kutundefined_tostring,
};

const KutMandatoryMethodsTable kutnil_methods = {
    .dispatch = kutundefined_dispatch,
    .addref = NULL,
    .decref = NULL,
    .tostring = NULL,
};

#include "kuttable.h"

#undef checkarg

bool checkarg(KutTable* args, size_t index, const KutMandatoryMethodsTable* type) {
    return ((args->len >= index) and args->data[index].methods == type);
}

void kut_addref(KutValue* self) {
    if(self->methods and self->methods->addref) {
        self->methods->addref(self);
    }
}

void kut_decref(KutValue* self) {
    if(self->methods and self->methods->decref) {
        self->methods->decref(self);
    }
    size_t ref = self->reference_count;
    *self = kut_nil;
    self->reference_count = ref;
}

KutString* const nil_name = kutstring_literal("nil");

KutString* kut_tostring(KutValue* self, size_t indent) {
    if(self == NULL) {
        return NULL;
    }
    if(self->methods == NULL) {
        return nil_name;
    }
    KutString* val = self->methods->tostring(self, indent);
    return val;
}

bool kut_equal(KutValue* self, KutValue* other) {
    if(self->methods and self->methods->equal) {
        return self->methods->equal(self, other);
    } else if(other->methods and other->methods->equal) {
        return other->methods->equal(self, other);
    }
    return true; // Both nil
}

void kut_set(KutValue* destination, KutValue* source) {
    if((destination == NULL) or (source == NULL) or (destination == source)) {
        return;
    }
    size_t ref = destination->reference_count;
    kut_addref(source);
    kut_decref(destination);
    *destination = *source;
    destination->reference_count = ref;
}

void kut_swap(KutValue* val1, KutValue* val2) {
    if(val1 == NULL or val2 == NULL or val1 == val2) {
        return;
    }
    size_t ref1 = val1->reference_count;
    size_t ref2 = val2->reference_count;
    KutValue tmp = *val1;
    *val1 = *val2;
    *val2 = tmp;
    val1->reference_count = ref1;
    val2->reference_count = ref2;
}

KutDispatchedFn kutboolean_dispatch(KutValue* self, KutString* message) {
    return empty_dispatched;
}

KutString* const true_literal = kutstring_literal("true");
KutString* const false_literal = kutstring_literal("false");

KutString* kutboolean_tostring(KutValue* _self, size_t indent) {
    bool self = _self ? kutboolean_cast(*_self) : false;
    return self ? true_literal : false_literal;
}

const KutMandatoryMethodsTable kutboolean_methods = {
    .dispatch = kutboolean_dispatch,
    .addref = NULL,
    .decref = NULL,
    .tostring = kutboolean_tostring,
};
