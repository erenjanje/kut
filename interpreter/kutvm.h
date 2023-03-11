#ifndef __VM_H__
#define __VM_H__

#include "kutval.h"
#include "kutfunc.h"

typedef struct KutVM KutVM;

bool kutvm_noOperation(KutFunc* func, KutInstruction instruction);
bool kutvm_methodcallIC(KutFunc* func, KutInstruction instruction);
bool kutvm_pushRegister2(KutFunc* func, KutInstruction instruction);
bool kutvm_pushRegister3(KutFunc* func, KutInstruction instruction);
bool kutvm_assignRegister(KutFunc* func, KutInstruction instruction);
bool kutvm_methodcallRC(KutFunc* func, KutInstruction instruction);
bool kutvm_loadLiteral(KutFunc* func, KutInstruction instruction);
bool kutvm_loadClosure(KutFunc* func, KutInstruction instruction);
bool kutvm_loadTemplate(KutFunc* func, KutInstruction instruction);
bool kutvm_loadInteger(KutFunc* func, KutInstruction instruction);
bool kutvm_loadNil(KutFunc* func, KutInstruction instruction);
bool kutvm_loadUndefined(KutFunc* func, KutInstruction instruction);
bool kutvm_loadTable(KutFunc* func, KutInstruction instruction);
bool kutvm_pushRegister1(KutFunc* func, KutInstruction instruction);
bool kutvm_methodcallPC(KutFunc* func, KutInstruction instruction);
bool kutvm_pushLiteral(KutFunc* func, KutInstruction instruction);
bool kutvm_pushClosure(KutFunc* func, KutInstruction instruction);
bool kutvm_pushTemplate(KutFunc* func, KutInstruction instruction);
bool kutvm_pushInteger(KutFunc* func, KutInstruction instruction);
bool kutvm_pushNil(KutFunc* func, KutInstruction instruction);
bool kutvm_pushUndefined(KutFunc* func, KutInstruction instruction);
bool kutvm_pushTable(KutFunc* func, KutInstruction instruction);
bool kutvm_popClosure(KutFunc* func, KutInstruction instruction);

typedef bool (*KutInstructionHandler)(KutFunc* func, KutInstruction instruction);

extern KutInstructionHandler instruction_handlers[];

#endif