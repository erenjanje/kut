#ifndef __VM_H__
#define __VM_H__

#include "kutval.h"
#include "kutfunc.h"

typedef struct KutVM KutVM;

bool kutvm_noperation(KutFunc* func, KutInstruction instruction);
bool kutvm_methodcall(KutFunc* func, KutInstruction instruction);
bool kutvm_returncall(KutFunc* func, KutInstruction instruction);
bool kutvm_pushvalue1(KutFunc* func, KutInstruction instruction);
bool kutvm_pushvalue2(KutFunc* func, KutInstruction instruction);
bool kutvm_pushvalue3(KutFunc* func, KutInstruction instruction);
bool kutvm_getliteral(KutFunc* func, KutInstruction instruction);
bool kutvm_getclosure(KutFunc* func, KutInstruction instruction);
bool kutvm_setclosure(KutFunc* func, KutInstruction instruction);
bool kutvm_mvregister(KutFunc* func, KutInstruction instruction);
bool kutvm_swapregist(KutFunc* func, KutInstruction instruction);
bool kutvm_gettmplate(KutFunc* func, KutInstruction instruction);

typedef bool (*KutInstructionHandler)(KutFunc* func, KutInstruction instruction);

extern KutInstructionHandler instruction_handlers[];

#endif