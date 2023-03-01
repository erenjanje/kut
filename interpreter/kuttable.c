#include "kuttable.h"
#include "kutstring.h"
#include "kutfunc.h"

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
    return kut_wrap((KutData){.data = self}, &kuttable_methods);
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

void kuttable_addref(KutValue* _self) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(self == NULL) {
        return;
    }
    if(self->reference_count != 0)
        self->reference_count += 1;
}

void kuttable_decref(KutValue* _self) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(self == NULL) {
        return;
    }
    if(self->reference_count == 0)
        return;
    if(self->reference_count == 1) {
        for(size_t i = 0; i < self->len; i++) {
            kut_decref(&self->data[i]);
        }
        free(self->data);
        free(self);
        return;
    }
    self->reference_count -= 1;
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
    if(self == NULL or not (checkarg(args, 0, &kutnumber_methods) or args->len < 2)) {
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
    if(self == NULL or not checkarg(args, 0, &kutnumber_methods)) {
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

KutValue kuttable_foreach(KutValue* _self, KutTable* args) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    KutFunc* func = NULL;
    if(checkarg(args, 0, &kutfunc_methods)) {
        func = kutfunc_cast(args->data[0]);
    }
    if(func == NULL) {
        return kut_undefined;
    }
    KutValue wrapper = kutfunc_wrap(func);
    KutTable* loop_args = kuttable_new(2);
    loop_args->len = 1;
    for(size_t i = 0; i < self->len; i++) {
        kut_set(&loop_args->data[0], &self->data[i]);
        KutValue tmpret = kutfunc_run(&wrapper, loop_args);
        kut_decref(&tmpret);
    }
    KutValue argswrap = kuttable_wrap(loop_args);
    kut_decref(&argswrap);
    return kut_undefined;
}

KutValue kuttable_map(KutValue* _self, KutTable* args) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    KutFunc* func = NULL;
    if(checkarg(args, 0, &kutfunc_methods)) {
        func = kutfunc_cast(args->data[0]);
    }
    if(func == NULL) {
        return kut_undefined;
    }
    KutValue wrapper = kutfunc_wrap(func);
    KutTable* loop_args = kuttable_new(2);
    KutTable* ret = kuttable_new(self->len);
    ret->len = self->len;
    loop_args->len = 1;
    for(size_t i = 0; i < self->len; i++) {
        kut_set(&loop_args->data[0], &self->data[i]);
        KutValue val = kutfunc_run(&wrapper, loop_args);
        ret->data[i] = val;
    }
    KutValue argswrap = kuttable_wrap(loop_args);
    kut_decref(&argswrap);
    return kuttable_wrap(ret);
}

KutValue kuttable_filter(KutValue* _self, KutTable* args) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    KutFunc* func = NULL;
    if(checkarg(args, 0, &kutfunc_methods)) {
        func = kutfunc_cast(args->data[0]);
    }
    if(func == NULL) {
        return kut_undefined;
    }
    KutValue wrapper = kutfunc_wrap(func);
    KutTable* loop_args = kuttable_new(2);
    KutTable* ret = kuttable_new(self->len);
    loop_args->len = 1;
    for(size_t i = 0; i < self->len; i++) {
        kut_set(&loop_args->data[0], &self->data[i]);
        KutValue val = kutfunc_run(&wrapper, loop_args);
        if(kutboolean_cast(val)) {
            ret->data[ret->len] = self->data[i];
            ret->len += 1;
        } else {
            kut_decref(&val);
        }
    }
    KutValue argswrap = kuttable_wrap(loop_args);
    kut_decref(&argswrap);
    return kuttable_wrap(ret);
}

KutValue kuttable_reduce(KutValue* _self, KutTable* args) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(self == NULL) {
        return kut_undefined;
    }
    KutFunc* func = NULL;
    if(checkarg(args, 0, &kutfunc_methods)) {
        func = kutfunc_cast(args->data[0]);
    }
    if(func == NULL) {
        return kut_undefined;
    }
    KutValue wrapper = kutfunc_wrap(func);
    KutTable* loop_args = kuttable_new(2);
    KutValue ret = self->data[0];
    loop_args->len = 2;
    for(size_t i = 0; i < self->len; i++) {
        kut_set(&loop_args->data[0], &ret);
        kut_set(&loop_args->data[1], &self->data[i]);
        KutValue tmp_ret = kutfunc_run(&wrapper, loop_args);
        kut_set(&ret, &tmp_ret);
        kut_decref(&tmp_ret);
    }
    KutValue argswrap = kuttable_wrap(loop_args);
    kut_decref(&argswrap);
    return ret;
}

KutString* kuttable_tostring(KutValue* _self, size_t indent) {
    KutTable* self = _self ? kuttable_cast(*_self) : NULL;
    if(self == NULL) {
        return NULL;
    }
    KutTable* strings = kuttable_new(self->len);
    strings->len = self->len;
    size_t total_length = sizeof("[]")-1 + indent*(sizeof("\t")-1);
    for(size_t i = 0; i < self->len; i++) {
        KutString* str = kut_tostring(&self->data[i], 0);
        if(str == NULL) {
            KutValue strings_table = kuttable_wrap(strings);
            kut_decref(&strings_table);
            return NULL;
        }
        strings->data[i] = kutstring_wrap(str);
        total_length += str->len + ((i != self->len-1) ? sizeof(" ")-1 : sizeof("")-1);
    }
    size_t offset = indent+1; // Start at [
    KutString* ret = kutstring_zero(total_length);
    memset(ret->data, '\t', indent);
    ret->data[indent] = '[';
    for(size_t i = 0; i < self->len; i++) {
        KutString* added = kutstring_cast(strings->data[i]);
        // No check since we checked in the previous loop

        offset += snprintf(ret->data+offset, ret->len-offset+1, "%.*s%s", kutstring_format(added), (i != self->len-1) ? " " : "");
    }
    ret->data[ret->len-1] = ']';
    KutValue strings_table = kuttable_wrap(strings);
    kut_decref(&strings_table);
    return ret;
}

#include "kuttable.methods"
#include "kutstring.h"

KutDispatchedFn kuttable_dispatch(KutValue* self, KutString* message) {
    const struct KutDispatchGperfPair* entry = kuttable_dispatchLookup(message->data, message->len);
    if(not entry)
        return empty_dispatched;
    return entry->method;
}

const KutMandatoryMethodsTable kuttable_methods = {
    .dispatch = kuttable_dispatch,
    .addref = kuttable_addref,
    .decref = kuttable_decref,
    .tostring = kuttable_tostring,
};
