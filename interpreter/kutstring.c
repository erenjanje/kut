#include "kutstring.h"

#include <string.h>
#include <iso646.h>
#include <stdio.h>

#include "kuttable.h"

KutString* kutstring_new(const char* str, size_t len) {
    KutString* ret = calloc(1, sizeof(*ret) + (len+1)*sizeof(ret->data[0]));
    ret->len = len;
    ret->reference_count = 1;
    memcpy(ret->data, str, len);
    return ret;
}

KutValue kutstring_wrap(KutString* str) {
    return kut_wrap((KutData){.data = str}, &kutstring_methods);
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

static void kutstring_addref(KutValue* _self) {
    KutString* self = _self ? kutstring_cast(*_self) : NULL;
    if(self == NULL) {
        return;
    }
    if(self->reference_count != 0)
        self->reference_count += 1;
}

static void kutstring_decref(KutValue* _self) {
    KutString* self = _self ? kutstring_cast(*_self) : NULL;
    if(self == NULL) {
        return;
    }
    if(self->reference_count == 0)
        return;
    if(self->reference_count == 1) {
        free(self);
        return;
    }
    self->reference_count -= 1;
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
    if(not checkarg(args, 0, &kutstring_methods)) {
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

static KutString* kutstring_tostring(KutValue* _self, size_t indent) {
    KutString* self = _self ? kutstring_cast(*_self) : NULL;
    if(self == NULL) {
        return NULL;
    }
    // TODO: Escape the string
    KutString* ret = kutstring_zero(self->len + sizeof("\"\"")-1 + indent*(sizeof("\t")-1));
    memset(ret->data, '\t', indent);
    snprintf(ret->data+indent, ret->len+1, "\"%.*s\"", kutstring_format(self));
    return ret;
}

static bool kutstring_equal(KutValue* _self, KutValue* _other) {
    KutString* self = _self ? kutstring_cast(*_self) : NULL;
    KutString* other = _other ? kutstring_cast(*_other) : NULL;
    if(self == NULL or other == NULL) {
        return false;
    }
    return self->len == other->len and (memcmp(self->data, other->data, self->len) == 0);
}

#include "kutstring.methods"

KutDispatchedFn kutstring_dispatch(KutValue* self, KutString* message) {
    const struct KutDispatchGperfPair* entry = kutstring_dispatchLookup(message->data, message->len);
    if(not entry)
        return empty_dispatched;
    return entry->method;
}

const KutMandatoryMethodsTable kutstring_methods = {
    .dispatch = kutstring_dispatch,
    .addref = kutstring_addref,
    .decref = kutstring_decref,
    .tostring = kutstring_tostring,
    .equal = kutstring_equal,
};
