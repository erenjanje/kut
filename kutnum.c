#include "kutnum.h"
#include "kuttable.h"

#include <iso646.h>
#include <math.h>
#include <string.h>

static KutValue kutnum_add(KutData data, KutTable* args) {
    double num = data.number;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutnumber_wrap(num + args->data[0].data.number);
}

static KutValue kutnum_sub(KutData data, KutTable* args) {
    double num = data.number;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutnumber_wrap(num - args->data[0].data.number);
}

static KutValue kutnum_mul(KutData data, KutTable* args) {
    double num = data.number;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutnumber_wrap(num * args->data[0].data.number);
}

static KutValue kutnum_div(KutData data, KutTable* args) {
    double num = data.number;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutnumber_wrap(num / args->data[0].data.number);
}

static KutValue kutnum_sin(KutData data, KutTable* args) {
    double num = data.number;
    return kutnumber_wrap(sin(num));
}

static KutValue kutnum_cos(KutData data, KutTable* args) {
    double num = data.number;
    return kutnumber_wrap(cos(num));
}

static KutValue kutnum_tan(KutData data, KutTable* args) {
    double num = data.number;
    return kutnumber_wrap(tan(num));
}

static KutValue kutnum_cot(KutData data, KutTable* args) {
    double num = data.number;
    return kutnumber_wrap(1.0/tan(num));
}

static KutValue kutnum_asin(KutData data, KutTable* args) {
    double num = data.number;
    return kutnumber_wrap(asin(num));
}

static KutValue kutnum_acos(KutData data, KutTable* args) {
    double num = data.number;
    return kutnumber_wrap(acos(num));
}

static KutValue kutnum_atan(KutData data, KutTable* args) {
    double num = data.number;
    return kutnumber_wrap(atan(num));
}

static KutValue kutnum_atan2(KutData data, KutTable* args) {
    double num = data.number;
    if(args->len < 1 or not istype(args->data[0], kutnumber)) {
        return kut_undefined;
    }
    return kutnumber_wrap(atan2(num, args->data[0].data.number));
}

static KutValue kutnum_acot(KutData data, KutTable* args) {
    double num = data.number;
    return kutnumber_wrap(atan(1.0/num));
}

static KutValue kutnum_abs(KutData data, KutTable* args) {
    double num = data.number;
    return kutnumber_wrap(fabs(num));
}

static KutValue kutnum_addref(KutData data, KutTable* args) {
    return kutboolean_wrap(true);
}

static KutValue kutnum_decref(KutData data, KutTable* args) {
    return kutboolean_wrap(true);
}

#include "kutnum.methods"

#include "kutstring.h"

KutDispatchedFn kutnumber_dispatch(KutData self, KutString* message) {
    const struct KutDispatchGperfPair* entry = kutnum_dispatchLookup(message->data, message->len);
    if(not entry)
        return empty_dispatched;
    return entry->method;
}