
#ifndef BBBANK_H
#define BBBANK_H

#include "bbsys.h"

struct bbBank;

bbBank* bbCreateBank(int size);
void bbFreeBank(bbBank* b);
int bbBankSize(bbBank* b);
void bbResizeBank(bbBank* b, int size);
void bbCopyBank(bbBank* src, int src_p, bbBank* dest, int dest_p, int count);
int bbPeekByte(bbBank* b, int offset);
int bbPeekShort(bbBank* b, int offset);
int bbPeekInt(bbBank* b, int offset);
float bbPeekFloat(bbBank* b, int offset);
void bbPokeByte(bbBank* b, int offset, int value);
void bbPokeShort(bbBank* b, int offset, int value);
void bbPokeInt(bbBank* b, int offset, int value);
void bbPokeFloat(bbBank* b, int offset, float value);
int bbReadBytes(bbBank* b, bbStream* s, int offset, int count);
int bbWriteBytes(bbBank* b, bbStream* s, int offset, int count);
int bbCallDLL(BBStr* dll, BBStr* fun, bbBank* in, bbBank* out);
bool bank_create();
bool bank_destroy();
void bank_link(void(*rtSym)(const char*, void*));

#endif
