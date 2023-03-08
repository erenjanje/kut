#ifndef __KUTERROR_H__
#define __KUTERROR_H__

#include "kutval.h"

typedef struct KutStackTrace KutStackTrace;
typedef struct KutError KutError;

struct KutStackTrace {
    KutString* filename;
    KutString* function;
    size_t line_number;
    KutStackTrace* next;
};

struct KutError {
    size_t reference_count;
    KutStackTrace* trace;
    KutStackTrace* last_trace;
    uint64_t code;
    KutString* message;
};

KutError* kuterror_newDirect(uint64_t code, KutString* message, const char* filename, const char* function_name, int line_number);
void kuterror_addTraceDirect(KutError* err, const char* filename, const char* function_name, int line_number);
#define kuterror_new(code, message) kuterror_newDirect((code), (message), __FILE__, __FUNC__, __LINE__)
#define kuterror_addTrace(err) kuterror_addTraceDirect((err), __FILE__, __FUNC__, __LINE__)

#endif