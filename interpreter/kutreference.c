#include "kutreference.h"
#include "kutstring.h"
#include <stdlib.h>
#include <stdio.h>

KutValue* kutreference_new(KutValue* ref) {
    KutValue* ret = calloc(1, sizeof(*ret));
    *ret = *ref;
    *ref = kutreference_wrap(ret);
    ret->reference_count = 2;
    return ret;
}

KutValue kutreference_wrap(KutValue* ref) {
    return kut_wrap((KutData){.data = ref}, kutreference_dispatch);
}

KutValue* kutreference_cast(KutValue val) {
    if(istype(val, kutreference)) {
        return val.data.data;
    }
    return NULL;
}

KutValue kutreference_addref(KutValue* _self, KutTable* args) {
    KutValue* self = _self ? kutreference_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    if(self == NULL) {
        return kut_undefined;
    }
    if(self->reference_count != 0)
        self->reference_count += 1;
    return kutboolean_wrap(true);
}

KutValue kutreference_decref(KutValue* _self, KutTable* args) {
    KutValue* self = _self ? kutreference_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    if(self->reference_count == 0)
        return kutboolean_wrap(true);
    if(self->reference_count == 1) {
        free(self);
        return kutboolean_wrap(false);
    }
    self->reference_count -= 1;
    return kutboolean_wrap(true);
}

KutValue kutreference_tostring(KutValue* _self, KutTable* args) {
    KutValue* self = _self ? kutreference_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    KutString* inner_str = kut_tostring(self);
    if(inner_str == NULL) {
        return kut_undefined;
    }
    KutString* ret = kutstring_zero(inner_str->len+sizeof("<>")-1);
    snprintf(ret->data, ret->len+1, "<%.*s>", kutstring_format(inner_str));
    return kutstring_wrap(ret);
}

#include "kutreference.methods"
#include "kutstring.h"

KutDispatchedFn kutreference_dispatch(KutValue* self, KutString* message) {
    const struct KutDispatchGperfPair* entry = kutreference_dispatchLookup(message->data, message->len);
    if(not entry) {
        KutValue undef = kut_undefined;
        KutValue* inner = self ? kutreference_cast(*self) : &undef;
        return inner->dispatch(inner, message);
    }
    return entry->method;
}
