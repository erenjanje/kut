#ifndef __KUTSTRING_H__
#define __KUTSTRING_H__

#include "kutval.h"
#include <string.h>

struct KutString {
    size_t reference_count;
    size_t len;
    char data[];
};

KutString* kutstring_new(char* str, size_t len);
KutString* kutstring_newCString(char* str);
KutValue kutstring_wrap(KutString* str);
KutString* kutstring_cast(KutValue val);

/// Hideous, isn't it? Well, It creates a new anonymous struct with fixed
/// length `data` field and puts the literal's data into it and then makes `name` a `KutString*` pointing to it.
/// Note that the reference count is 0, since this should not be freed at all as it resides in the data/bss segment.
#define kutstring_literal(lit) (KutString*)(struct {size_t ref, len; char data[sizeof((lit))];}[]){{.ref = 0, .len = sizeof((lit))-1, .data = lit}}

KutValue kutstring_addref(KutData self, KutTable* args);
KutValue kutstring_decref(KutData self, KutTable* args);

KutValue kutstring_equal(KutData self, KutTable* args);
bool kutstring_equalString(KutString* self, KutString* other);
bool kutstring_equalCString(KutString* self, const char* other, size_t length);

KutValue kutstring_compare(KutData self, KutTable* args);
bool kutstring_compareString(KutString* self, KutString* other);
bool kutstring_compareCString(KutString* self, const char* other, size_t length);

#define kutstring_format(str) (int)((str)->len), (str)->data

#endif