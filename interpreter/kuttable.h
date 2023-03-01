#ifndef __KUTTABLE_H__
#define __KUTTABLE_H__

#include "kutval.h"
#include <stddef.h>
#include <stdlib.h>

#define null NULL

struct KutTable {
    size_t reference_count;
    size_t capacity;
    size_t len;
    KutValue* data;
};

KutTable* kuttable_new(size_t initial_capacity);
KutValue kuttable_wrap(KutTable* self);
KutTable* kuttable_directPointer(size_t length, KutValue* data);
KutTable* kuttable_cast(KutValue val);

void __kuttable_append(KutTable* self, KutValue val);
KutValue __kuttable_delete(KutTable* self, intmax_t index);

KutValue kuttable_append(KutValue* self, KutTable* index);
KutValue kuttable_insert(KutValue* self, KutTable* index);
KutValue kuttable_delete(KutValue* self, KutTable* index);
KutValue kuttable_clear(KutValue* self, KutTable* args);
KutValue kuttable_foreach(KutValue* self, KutTable* args);

#define kuttable_literal(first, ...) (KutTable*)(KutTable[]){{\
    .reference_count = 0,\
    .capacity = sizeof((KutValue[]){first, ##__VA_ARGS__})/sizeof(KutValue),\
    .len = sizeof((KutValue[]){first, ##__VA_ARGS__})/sizeof(KutValue),\
    .data = (KutValue[]){first, ##__VA_ARGS__}\
}}

#endif