#ifndef SIMBUG_H_INCLUDED_
#define SIMBUG_H_INCLUDED_
#include <stdarg.h>
#include <malloc.h>
#include <assert.h>
#include "..\sam-15\sam-15.h"
#include "..\sam-15\samBug.h"
#include <samParams.h>
#include <hStructures.h>

extern class cBugSim *g_bugSimP;

class cBugSim : public cBugIndex
   {public:                                                                     //
    sPARAMS m_params;                                                           //
    cBugSim            (sPARAMS params);                                        //
    //virtual function overriding structure accesses in cSamBug                 //
    //Basic addressing functions                                                //
    cBOOK *Badr  (uint32_t buk);                                                //
    cPAGE *BPadr (uint32_t buk, uint32_t page);                                 //
    cINDX *BPIadr(uint32_t buk, uint32_t page, uint32_t inx);                   //
    hPAGE *hPadr (hPAGE *hP, int page);                                         //internal utility
    //simple fields can be referenced without any fuss                          //
    uint16_t BookCount(cBOOK *bP) {return ((hBOOK*)bP)->kount;}                 //
    uint64_t BookTotal(cBOOK *bP) {return ((hBOOK*)bP)->total;}                 //
    uint32_t PageCount(cPAGE *pP) {return ((hPAGE*)pP)->kount;}                 //
    uint64_t PageTotal(cPAGE *pP) {return ((hPAGE*)pP)->total;}                 //
    uint64_t IndxRelAdr(cINDX*xP) {return ((hINDX*)xP)->data; }                 //
    //address fields must be translated into the host address within g_imageP   //
    cPAGE   *BookPageL(cBOOK *bP) {return (cPAGE*)AdrXlat(GetP1((hPAGE*)bP));}  //
    cPAGE   *BookPageH(cBOOK *bP) {return (cPAGE*)AdrXlat(GetP2((hPAGE*)bP));}  //
    cINDX   *PageLoP  (cPAGE *pP) {return (cINDX*)AdrXlat(GetP1((hPAGE*)pP));}  //
    cINDX   *PageHiP  (cPAGE *pP) {return (cINDX*)AdrXlat(GetP2((hPAGE*)pP));}  //
    uint8_t *BookLoKey(cBOOK *bP) {return ((uint8_t*)bP) + hBOOK_BASE;}         //Keys are located on byte
    uint8_t *PageLoKey(cPAGE *pP) {return ((uint8_t*)pP) + hPAGE_BASE;}         //  boundary
    uint8_t *IndxKey  (cINDX *xP) {return ((uint8_t*)xP) + hINDX_BASE;}         //    in all cITEMs
    //Various utility functions                                                 //
    static void ShowAdr     (void *vP);                                         //
    void       *AdrXlat     (uintptr_t,  int offset=0);                         //
    uint64_t    GetBits     (void *srcP, int bitOffset, int bitCount);          //not used
    bool        ValidateBram(uint8_t *bramP, int bUsed, int bramRows);          //
   }; //cBugSim...
#endif //SIMBUG_H_INCLUDED_
