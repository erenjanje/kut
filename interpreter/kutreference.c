#include "kutreference.h"
#include "kutstring.h"
#include <stdlib.h>
#include <stdio.h>

KutValue* kutreference_new(KutValue* ref) {
    if(istype(*ref, kutreference)) {
        return kutreference_cast(*ref);
    }
    KutValue* ret = calloc(1, sizeof(*ret));
    *ret = *ref;
    *ref = kutreference_wrap(ret);
    ret->reference_count = 2;
    return ret;
}

KutValue kutreference_wrap(KutValue* ref) {
    return kut_wrap((KutData){.data = ref}, &kutreference_methods);
}

KutValue* kutreference_cast(KutValue val) {
    if(istype(val, kutreference)) {
        return val.data.data;
    }
    return NULL;
}

void kutreference_addref(KutValue* _self) {
    KutValue* self = _self ? kutreference_cast(*_self) : NULL;
    if(self == NULL) {
        return;
    }
    if(self == NULL) {
        return;
    }
    if(self->reference_count != 0)
        self->reference_count += 1;
}

void kutreference_decref(KutValue* _self) {
    KutValue* self = _self ? kutreference_cast(*_self) : NULL;
    if(self == NULL) {
        return;
    }
    if(self->reference_count == 0)
        return;
    if(self->reference_count == 1) {
        kut_decref(self);
        free(self);
        return;
    }
    self->reference_count -= 1;
}

KutString* kutreference_tostring(KutValue* _self, size_t indent) {
    KutValue* self = _self ? kutreference_cast(*_self) : NULL;
    if(self == NULL) {
        return NULL;
    }
    KutString* inner_str = kut_tostring(self, 0);
    if(inner_str == NULL) {
        return NULL;
    }
    KutString* ret = kutstring_zero(inner_str->len + sizeof("<>")-1 + indent*(sizeof("\t")-1));
    memset(ret->data, '\t', indent);
    snprintf(ret->data+indent, ret->len+1, "<%.*s>", kutstring_format(inner_str));
    KutValue inner_wrapper = kutstring_wrap(inner_str);
    kut_decref(&inner_wrapper);
    return ret;
}

KutValue kutreference_dummy(KutValue* _self, KutTable* args) {
    return kut_undefined;
}

#include "kutreference.methods"
#include "kutstring.h"

KutDispatchedFn kutreference_dispatch(KutValue* self, KutString* message) {
    const struct KutDispatchGperfPair* entry = kutreference_dispatchLookup(message->data, message->len);
    if(not entry) {
        KutValue undef = kut_undefined;
        KutValue* inner = self ? kutreference_cast(*self) : &undef;
        return inner->methods->dispatch(inner, message);
    }
    return entry->method;
}

const KutMandatoryMethodsTable kutreference_methods = {
    .dispatch = kutreference_dispatch,
    .addref = kutreference_addref,
    .decref = kutreference_decref,
    .tostring = kutreference_tostring,
};
