#ifndef __KUTREFERENCE_H__
#define __KUTREFERENCE_H__

#include "kutval.h"

KutValue kutreference_get(KutValue self);
KutValue kutreference_set(KutValue self, KutValue val);

KutValue kutreference_addref(KutData self, KutTable* args);
KutValue kutreference_decref(KutData self, KutTable* args);

#endif