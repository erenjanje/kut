#ifndef __KUTREFERENCE_H__
#define __KUTREFERENCE_H__

#include "kutval.h"

KutValue* kutreference_new(KutValue* ref);
KutValue kutreference_wrap(KutValue* ref);
KutValue* kutreference_cast(KutValue val);

void kutreference_addref(KutValue* self);
void kutreference_decref(KutValue* self);


#endif