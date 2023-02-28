#include "kutstring.h"

#include <string.h>
#include <iso646.h>
#include <stdio.h>

#include "kuttable.h"

KutString* kutstring_new(char* str, size_t len) {
    KutString* ret = calloc(1, sizeof(*ret) + (len+1)*sizeof(ret->data[0]));
    ret->len = len;
    ret->reference_count = 1;
    memcpy(ret->data, str, len);
    return ret;
}

KutValue kutstring_wrap(KutString* str) {
    return kut_wrap((KutData){.data = str}, kutstring_dispatch);
}

KutString* kutstring_newCString(char* str) {
    return kutstring_new(str, strlen(str)-1);
}

KutString* kutstring_cast(KutValue val) {
    if(istype(val, kutstring)) {
        return val.data.data;
    }
    return NULL;
}

KutString* kutstring_zero(size_t len) {
    KutString* ret = calloc(1, sizeof(*ret) + (len+1)*sizeof(ret->data[0]));
    ret->len = len;
    ret->reference_count = 1;
    return ret;
}

KutValue kutstring_addref(KutValue* _self, KutTable* args) {
    KutString* self = _self ? kutstring_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    if(self->reference_count != 0)
        self->reference_count += 1;
    return kutboolean_wrap(true);
}

KutValue kutstring_decref(KutValue* _self, KutTable* args) {
    KutString* self = _self ? kutstring_cast(*_self) : NULL;
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

KutValue kutstring_equal(KutValue* _self, KutTable* args) {
    KutString* self = _self ? kutstring_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    if(not checkarg(args, 0, kutstring)) {
        return kut_undefined;
    }
    return kutboolean_wrap(kutstring_equalString(self, (KutString*)args->data[0].data.data));
}

bool kutstring_equalString(KutString* self, KutString* other) {
    return kutstring_equalCString(self, other->data, other->len);
}

bool kutstring_equalCString(KutString* self, const char* other, size_t length) {
    return (memcmp(self->data, other, ((self->len < length) ? self->len : length)) == 0);
}

KutValue kutstring_compare(KutValue* _self, KutTable* args) {
    KutString* self = _self ? kutstring_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    if(not checkarg(args, 0, kutstring)) {
        return kut_undefined;
    }
    return kutnumber_wrap(kutstring_compareString(self, (KutString*)args->data[0].data.data));
}

bool kutstring_compareString(KutString* self, KutString* other) {
    return kutstring_compareCString(self, other->data, other->len);
}

bool kutstring_compareCString(KutString* self, const char* other, size_t length) {
    return memcmp(self->data, other, ((self->len < length) ? self->len : length));
}

KutValue kutstring_tostring(KutValue* _self, KutTable* args) {
    KutString* self = _self ? kutstring_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    // TODO: Escape the string
    KutString* ret = kutstring_zero(self->len+sizeof("\"\"")-1);
    snprintf(ret->data, ret->len+1, "\"%.*s\"", kutstring_format(self));
    return kutstring_wrap(ret);
}

#include "kutstring.methods"

KutDispatchedFn kutstring_dispatch(KutValue* self, KutString* message) {
    const struct KutDispatchGperfPair* entry = kutstring_dispatchLookup(message->data, message->len);
    if(not entry)
        return empty_dispatched;
    return entry->method;
}
