#include "kutreference.h"
#include <stdlib.h>

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

KutValue kutreference_get(KutValue _self) {
    kutreference_decref(_self.data, NULL);
    KutValue* self = _self.data.data;
    return *self;
}

KutValue kutreference_set(KutValue _self, KutValue val) {
    kutreference_decref(_self.data, NULL);
    KutValue* self = _self.data.data;
    *self = val;
    return val;
}

KutValue kutreference_addref(KutData self, KutTable* args) {
    KutValue* _self = self.data;
    if(_self->reference_count != 0)
        _self->reference_count += 1;
    return kutboolean_wrap(true);
}

KutValue kutreference_decref(KutData self, KutTable* args) {
    KutValue* _self = self.data;
    if(_self->reference_count == 0)
        return kutboolean_wrap(true);
    if(_self->reference_count == 1) {
        free(_self);
        return kutboolean_wrap(false);
    }
    _self->reference_count -= 1;
    return kutboolean_wrap(true);
}

#include "kutreference.methods"
#include "kutstring.h"

KutDispatchedFn kutreference_dispatch(KutData self, KutString* message) {
    const struct KutDispatchGperfPair* entry = kutreference_dispatchLookup(message->data, message->len);
    if(not entry) {
        KutValue inner = *(KutValue*)self.data;
        return inner.dispatch(inner.data, message);
    }
    return entry->method;
}
