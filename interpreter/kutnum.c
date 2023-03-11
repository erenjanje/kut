#include "kutnum.h"
#include "kuttable.h"
#include "kutstring.h"

#include <iso646.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

static KutValue kutnum_lt(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutboolean_wrap(num < kutnumber_cast(args->data[0]));
}

static KutValue kutnum_gt(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutboolean_wrap(num > kutnumber_cast(args->data[0]));
}

static KutValue kutnum_le(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutboolean_wrap(num <= kutnumber_cast(args->data[0]));
}

static KutValue kutnum_ge(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutboolean_wrap(num >= kutnumber_cast(args->data[0]));
}

static KutValue kutnum_add(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutnumber_wrap(num + kutnumber_cast(args->data[0]));
}

static KutValue kutnum_sub(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutnumber_wrap(num - kutnumber_cast(args->data[0]));
}

static KutValue kutnum_mul(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutnumber_wrap(num * kutnumber_cast(args->data[0]));
}

static KutValue kutnum_div(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutnumber_wrap(num / kutnumber_cast(args->data[0]));
}

static KutValue kutnum_pow(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutnumber_wrap(pow(num, kutnumber_cast(args->data[0])));
}

static KutValue kutnum_sin(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    return kutnumber_wrap(sin(num));
}

static KutValue kutnum_cos(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    return kutnumber_wrap(cos(num));
}

static KutValue kutnum_tan(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    return kutnumber_wrap(tan(num));
}

static KutValue kutnum_cot(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    return kutnumber_wrap(1.0/tan(num));
}

static KutValue kutnum_asin(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    return kutnumber_wrap(asin(num));
}

static KutValue kutnum_acos(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    return kutnumber_wrap(acos(num));
}

static KutValue kutnum_atan(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    return kutnumber_wrap(atan(num));
}

static KutValue kutnum_atan2(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutnumber_wrap(atan2(num, kutnumber_cast(args->data[0])));
}

static KutValue kutnum_acot(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    return kutnumber_wrap(atan(1.0/num));
}

static KutValue kutnum_abs(KutValue* data, KutTable* args) {
    double num = data ? kutnumber_cast(*data) : 0.0;
    return kutnumber_wrap(fabs(num));
}

static KutString* kutnum_tostring(KutValue* data, size_t indent) {
    double num = data ? kutnumber_cast(*data) : nan("notvalid");
    size_t len = snprintf(NULL, 0, "%g", num) + indent*(sizeof("\t")-1);
    KutString* ret = kutstring_zero(len);
    memset(ret->data, '\t', indent);
    snprintf(ret->data+indent, ret->len+1, "%g", num);
    return ret;
}

static bool kutnum_equal(KutValue* data, KutValue* other) {
    double num = data ? kutnumber_cast(*data) : nan("notvalid");
    double other_num = other ? kutnumber_cast(*other) : nan("notvalid");
    return num == other_num;
}

#include "kutnum.methods"

#include "kutstring.h"

KutDispatchedFn kutnumber_dispatch(KutValue* self, KutString* message) {
    const struct KutDispatchGperfPair* entry = kutnum_dispatchLookup(message->data, message->len);
    if(not entry)
        return empty_dispatched;
    return entry->method;
}

const KutMandatoryMethodsTable kutnumber_methods = {
    .dispatch = kutnumber_dispatch,
    .addref = NULL,
    .decref = NULL,
    .tostring = kutnum_tostring,
    .equal = kutnum_equal,
};
