#include "kuttable.h"

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

KutValue kuttable_addref(KutData self, KutTable* args) {
    KutTable* _self = self.data;
    if(_self->reference_count != 0)
        _self->reference_count += 1;
    return kutboolean_wrap(true);
}

KutValue kuttable_decref(KutData self, KutTable* args) {
    KutTable* _self = self.data;
    if(_self->reference_count == 0)
        return kutboolean_wrap(true);
    if(_self->reference_count == 1) {
        free(_self->data);
        free(_self);
        return kutboolean_wrap(false);
    }
    _self->reference_count -= 1;
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
    self->len += 1;
    kut_addref(val);
}

KutValue kuttable_append(KutData self, KutTable* args) {
    KutTable* _self = self.data;
    if(args->len < 1) {
        return kut_undefined;
    }
    __kuttable_append(_self, args->data[0]);
    return kuttable_wrap(_self);
}

KutValue kuttable_insert(KutData _self, KutTable* args) {
    KutTable* self = _self.data;
    if(not (checkarg(args, 0, kutnumber) or args->len < 2)) {
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
    kut_addref(val);
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

KutValue kuttable_delete(KutData _self, KutTable* args) {
    KutTable* self = _self.data;
    if(not checkarg(args, 0, kutnumber)) {
        return kut_undefined;
    }
    intmax_t index = args->data[0].data.number;
    return __kuttable_delete(self, index);
}

KutValue kuttable_clear(KutData _self, KutTable* args) {
    KutTable* self = _self.data;
    for(size_t i = 0; i < self->len; i++) {
        kut_decref(&self->data[i]);
        self->data[i] = kut_undefined;
    }
    self->len = 0;
    return kut_undefined;
}

#include "kuttable.methods"
#include "kutstring.h"

KutDispatchedFn kuttable_dispatch(KutData self, KutString* message) {
    const struct KutDispatchGperfPair* entry = kuttable_dispatchLookup(message->data, message->len);
    if(not entry)
        return empty_dispatched;
    return entry->method;
}
