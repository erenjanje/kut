#include "kutstring.h"

#include <string.h>
#include <iso646.h>

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

KutValue kutstring_addref(KutData self, KutTable* args) {
    KutString* str = self.data;
    if(str->reference_count != 0)
        str->reference_count += 1;
    return kutboolean_wrap(true);
}

KutValue kutstring_decref(KutData self, KutTable* args) {
    KutString* str = self.data;
    if(str->reference_count == 0)
        return kutboolean_wrap(true);
    if(str->reference_count == 1) {
        free(str);
        return kutboolean_wrap(false);
    }
    str->reference_count -= 1;
    return kutboolean_wrap(true);
}

KutValue kutstring_equal(KutData self, KutTable* args) {
    KutString* str = self.data;
    if(not checkarg(args, 0, kutstring)) {
        return kut_undefined;
    }
    return kutboolean_wrap(kutstring_equalString(str, (KutString*)args->data[0].data.data));
}

bool kutstring_equalString(KutString* self, KutString* other) {
    return kutstring_equalCString(self, other->data, other->len);
}

bool kutstring_equalCString(KutString* self, const char* other, size_t length) {
    return (memcmp(self->data, other, ((self->len < length) ? self->len : length)) == 0);
}

KutValue kutstring_compare(KutData self, KutTable* args) {
    KutString* str = self.data;
    if(not checkarg(args, 0, kutstring)) {
        return kut_undefined;
    }
    return kutnumber_wrap(kutstring_compareString(str, (KutString*)args->data[0].data.data));
}

bool kutstring_compareString(KutString* self, KutString* other) {
    return kutstring_compareCString(self, other->data, other->len);
}

bool kutstring_compareCString(KutString* self, const char* other, size_t length) {
    return memcmp(self->data, other, ((self->len < length) ? self->len : length));
}

#include "kutstring.methods"

KutDispatchedFn kutstring_dispatch(KutData self, KutString* message) {
    const struct KutDispatchGperfPair* entry = kutstring_dispatchLookup(message->data, message->len);
    if(not entry)
        return empty_dispatched;
    return entry->method;
}
