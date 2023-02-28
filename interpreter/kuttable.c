#include "kuttable.h"
#include "kutstring.h"

#include <stdio.h>
#include <iso646.h>

static KutTable __empty_table_inside = {.reference_count = 1, .capacity = 0, .data = NULL, .len = 0};

KutTable* const empty_table = &__empty_table_inside;

KutTable* kuttable_new(size_t initial_capacity) {
    KutTable* ret = calloc(1, sizeof(*ret));
    initial_capacity = (initial_capacity < 2) ? 2 : initial_capacity;
    *ret = (KutTable){
        .capacity = initial_capacity,
        .len = 0,
        .reference_count = 1,
        .data = calloc(initial_capacity, sizeof(ret->data[0])),
    };
    return ret;
}

KutValue kuttable_wrap(KutTable* self) {
    return kut_wrap((KutData){.data = self}, kuttable_dispatch);
}

KutTable* kuttable_directPointer(size_t length, KutValue* data) {
    KutTable* tabl = calloc(1, sizeof(*tabl));
    tabl->capacity = length;
    tabl->len = length;
    tabl->data = data;
    tabl->reference_count = 0;
    return tabl;
}

KutTable* kuttable_cast(KutValue val) {
    if(istype(val, kuttable)) {
        return val.data.data;
    }
    return NULL;
}

KutValue kuttable_addref(KutValue* _self, KutTable* args) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    if(self->reference_count != 0)
        self->reference_count += 1;
    return kutboolean_wrap(true);
}

KutValue kuttable_decref(KutValue* _self, KutTable* args) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    for(size_t i = 0; i < self->len; i++) {
        kut_decref(&self->data[i]);
    }
    if(self->reference_count == 0)
        return kutboolean_wrap(true);
    if(self->reference_count == 1) {
        free(self->data);
        free(self);
        return kutboolean_wrap(false);
    }
    self->reference_count -= 1;
    return kutboolean_wrap(true);
}

static bool kuttable_reallocate(KutTable* self) {
    self->capacity += self->capacity/2;
    KutValue* tmp_ptr = realloc(self->data, self->capacity*sizeof(*self->data));
    if(tmp_ptr == null) {
        fprintf(stderr, "Memory Error!\n");
        free(self->data);
        return false;
    }
    self->data = tmp_ptr;
    return true;
}

void __kuttable_append(KutTable* self, KutValue val) {
    if(self->len == self->capacity)
        kuttable_reallocate(self);
    self->data[self->len] = val;
    kut_addref(&self->data[self->len]);
    self->len += 1;
}

KutValue kuttable_append(KutValue* _self, KutTable* args) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(_self == NULL or args->len < 1) {
        return kut_undefined;
    }
    __kuttable_append(self, args->data[0]);
    return kuttable_wrap(self);
}

KutValue kuttable_insert(KutValue* _self, KutTable* args) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(self == NULL or not (checkarg(args, 0, kutnumber) or args->len < 2)) {
        return kut_undefined;
    }
    size_t index = args->data[0].data.number;
    KutValue val = args->data[1];
    if(index > self->len)
        return kuttable_wrap(self);
    if(index == self->len) {
        __kuttable_append(self, val);
        return kuttable_wrap(self);
    }
    if(self->len == self->capacity)
        kuttable_reallocate(self);

    self->len += 1;
    for(size_t i = index+1; i < self->len; i++)
        self->data[i] = self->data[i-1];

    self->data[index] = val;
    kut_addref(&self->data[index]);
    return kut_undefined;
}

KutValue __kuttable_delete(KutTable* self, intmax_t index) {
    if(index < 0)
        index = self->len + index;
    if(index >= (intmax_t)self->len or index < 0)
        return kut_undefined;
    KutValue ret = self->data[index];
    for(size_t i = index; i < self->len-1; i++)
        self->data[i] = self->data[i+1];
    return ret;
}

KutValue kuttable_delete(KutValue* _self, KutTable* args) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(self == NULL or not checkarg(args, 0, kutnumber)) {
        return kut_undefined;
    }
    intmax_t index = args->data[0].data.number;
    return __kuttable_delete(self, index);
}

KutValue kuttable_clear(KutValue* _self, KutTable* args) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    for(size_t i = 0; i < self->len; i++) {
        kut_decref(&self->data[i]);
        self->data[i] = kut_undefined;
    }
    self->len = 0;
    return kut_undefined;
}

KutValue kuttable_tostring(KutValue* _self, KutTable* args) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    KutTable* strings = kuttable_new(self->len);
    strings->len = self->len;
    size_t total_length = sizeof("[]")-1;
    for(size_t i = 0; i < self->len; i++) {
        KutString* str = kut_tostring(&self->data[i]);
        if(str == NULL) {
            KutValue strings_table = kuttable_wrap(strings);
            kut_decref(&strings_table);
            return kut_undefined;
        }
        strings->data[i] = kutstring_wrap(str);
        total_length += str->len + ((i != self->len-1) ? sizeof(" ")-1 : sizeof("")-1);
    }
    size_t offset = 1; // Start at [
    KutString* ret = kutstring_zero(total_length);
    ret->data[0] = '[';
    for(size_t i = 0; i < self->len; i++) {
        KutString* added = kutstring_cast(strings->data[i]);
        // No check since we checked in the previous loop

        offset += snprintf(ret->data+offset, ret->len-offset+1, "%.*s%s", kutstring_format(added), (i != self->len-1) ? " " : "");
    }
    ret->data[ret->len-1] = ']';
    KutValue strings_table = kuttable_wrap(strings);
    kut_decref(&strings_table);
    return kutstring_wrap(ret);
}

#include "kuttable.methods"
#include "kutstring.h"

KutDispatchedFn kuttable_dispatch(KutValue* self, KutString* message) {
    const struct KutDispatchGperfPair* entry = kuttable_dispatchLookup(message->data, message->len);
    if(not entry)
        return empty_dispatched;
    return entry->method;
}
