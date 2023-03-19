#ifndef _SAM_APP_H_INCLUDED
#define _SAM_APP_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <crtdbg.h>
#include <fcntl.h>
#include "samOptions.h"

#pragma pack (push, 1)
//Arbitrary structure. These are the records which will be sorted
//Each record has a key field, which defines the sort order
typedef struct //variable size structure
   {unsigned short rcdLen;    //
    unsigned int   rcdNum;    //for convenience of CheckLocByKey()
    char           txt[68];   //might be smaller if sam /variable is called
    unsigned char  key[64];   //actual size must be larger than 2 bytes
    char           pad[20];   //variable length records screw with length by 16 bytes max
   } sRECORD;                 //actual size == 94+keySize
#pragma pack(pop)

inline int SizeofRecord(unsigned int keySz) {return sizeof(sRECORD)-sizeof(sRECORD::key) + keySz;}
#define AllocateLocalRecord(keySz)  ((sRECORD*)alloca(SizeofRecord(keySz)))
#define AllocateGlobalRecord(keySz) ((sRECORD*)malloc(SizeofRecord(keySz)))
extern sRECORD       *g_dataP;
extern unsigned int   g_keySize;
#define RecordAdr(ii) ((sRECORD *) (((uint8_t*)g_dataP) + SizeofRecord(g_keySize) * ii))

void Printf(char const *fmtP,...);

#define FULLY_ALPHABETIC_KEYS //keys are in ascii, otherwise binary
#endif //_SAM_APP_H_INCLUDED
//end of file...
