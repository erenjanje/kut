#ifndef __KUTVAL_H__
#define __KUTVAL_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <iso646.h>

typedef struct KutValue KutValue;
typedef struct KutTable KutTable;
typedef struct KutString KutString;

typedef union KutData {
    bool boolean;
    double number;
    void* data;
} KutData;

typedef KutValue (*KutDispatchedFn)(KutValue* self, KutTable* args);
typedef KutDispatchedFn (*KutDispatchFn)(KutValue* self, KutString* message);
typedef struct KutMandatoryMethodsTable KutMandatoryMethodsTable;
typedef KutString* (*KutToStringMethodFn)(KutValue* self, size_t indent);
typedef void (*KutAddrefMethodFn)(KutValue* self);
typedef void (*KutDecrefMethodFn)(KutValue* self);
typedef bool (*KutEqualMethodFn)(KutValue* self, KutValue* other);

struct KutMandatoryMethodsTable {
    KutDispatchFn dispatch;
    KutAddrefMethodFn addref;
    KutDecrefMethodFn decref;
    KutToStringMethodFn tostring;
    KutEqualMethodFn equal;
};

KutValue empty_dispatched(KutValue* self, KutTable* args); // TODO: Remove me!

// KutDispatchedFn kutundefined_dispatch(KutValue* self, KutString* message);
// KutDispatchedFn kutboolean_dispatch(KutValue* self, KutString* message);
// KutDispatchedFn kutnumber_dispatch(KutValue* self, KutString* message);
// KutDispatchedFn kutreference_dispatch(KutValue* self, KutString* message);
// KutDispatchedFn kutstring_dispatch(KutValue* self, KutString* message);
// KutDispatchedFn kutfunc_dispatch(KutValue* self, KutString* message);
// KutDispatchedFn kuttable_dispatch(KutValue* self, KutString* message);

extern const KutMandatoryMethodsTable kutnil_methods;
extern const KutMandatoryMethodsTable kutundefined_methods;
extern const KutMandatoryMethodsTable kutboolean_methods;
extern const KutMandatoryMethodsTable kutnumber_methods;
extern const KutMandatoryMethodsTable kutreference_methods;
extern const KutMandatoryMethodsTable kutstring_methods;
extern const KutMandatoryMethodsTable kutfunc_methods;
extern const KutMandatoryMethodsTable kuttable_methods;
extern const KutMandatoryMethodsTable kuterror_methods;

extern KutTable* const empty_table;

struct KutValue {
    size_t reference_count;
    KutData data;
    const KutMandatoryMethodsTable* methods;
};

struct KutDispatchGperfPair {
    const char* name;
    KutDispatchedFn method;
};

#define istype(v, T) ((v).methods == &T##_methods)
bool checkarg(KutTable* args, size_t index, const KutMandatoryMethodsTable* type);

static const KutValue kut_nil = {.methods = NULL};
static const KutValue kut_undefined = {.methods = &kutundefined_methods};

static inline KutValue kut_wrap(KutData data, const KutMandatoryMethodsTable* methods) { return (KutValue){.reference_count = 0, .data = data, .methods = methods}; }

static inline KutValue kutboolean_wrap(bool boolean) { return kut_wrap((KutData){.boolean = boolean}, &kutboolean_methods); }
static inline KutValue kutnumber_wrap(double number) { return kut_wrap((KutData){.number = number}, &kutnumber_methods); }

static inline double kutnumber_cast(KutValue val) { return istype(val, kutnumber) ? val.data.number : 0; }
static inline bool kutboolean_cast(KutValue val) { return istype(val, kutboolean) ? val.data.boolean : false; }

void kut_addref(KutValue* self);
void kut_decref(KutValue* self);
KutString* kut_tostring(KutValue* self, size_t indent);
bool kut_equal(KutValue* self, KutValue* other);

void kut_set(KutValue* destination, KutValue* source);
void kut_swap(KutValue* val1, KutValue* val2);

typedef union KutInstruction KutInstruction;

#endif