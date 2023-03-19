/*File simBug.cpp: derived class of cSamBug with custom glue logic to address 
  the data structures created by the FPGA simulation of Sadram. The simulation
  dumps BRAM using the $write function. This is read by the pipe Receive function,
  converted back to binary, and stored in host memory at g_bramP. 
  
  The variables hINDX_size, hINDX_padSz, hINDX_perRow,
                hPAGE_size, hPAGE_padSz, hPAGE_perRow,
                hBOOK_size, hBOOK_padSz, and hBOOK_perRow
  define the actual size of the FPGA structures.
  
  The addressing functions Badr, Padr, BPadr, and BPIadr calculate the addresses within g_bramP.
  The field access functions
    BookCount, BookTotal, BookPages, BookLoKey,
    PageCount, PageTotal, PageLoP, PageHiP, PageLoKey,
    and IndxKey,  
  access field within the structures.

  The function ShowAdr is called by cSamBug to display any address in the original BRAM,
  but otherwise the call cBugSam does all the heavy lifting.
*/

#include "simBug.h"

extern uint32_t cINDX_size, cINDX_padSz, cINDX_perRow;                       //=5 +keysize
extern uint32_t cPAGE_size, cPAGE_padSz, cPAGE_perRow;                       //=14+keysize
extern uint32_t cBOOK_size, cBOOK_padSz, cBOOK_perRow;                       //=10+keysize
extern uint32_t g_memRowSize;
extern uint8_t *g_bramP;
extern class cSadram *g_sP;

//Stubs for samBug, if it is every resurrected
uint8_t *cSadram::LocByNum(uint32_t)                     {assert(false); return NULL;}
cPAGE   *cSadram::BPadr(uint32_t,uint32_t)               {assert(false); return NULL;}
cINDX   *cSadram::BPIadr(uint32_t,uint32_t,uint32_t)     {assert(false); return NULL;}
void     cSadram::ClearKey(unsigned char *)              {assert(false);}

//aV < bV return -1, aV == bV return 0, aV > bV return +1
int CompareKey(const void *aV, const void *bV, int keySz)
   {uint8_t *aP=((uint8_t*)aV), *bP=((uint8_t*)bV);                             //typecast aV and bV
    uint32_t a32, b32;                                                          //
    if (keySz == 4)                                                             //optimization for uint32 field
       {a32 = HiLo32(*(uint32_t*)aP); b32 = HiLo32(*(uint32_t*)bP);             //
        return a32 < b32 ? -1 : a32 > b32 ? 1 : 0;                              //
       }                                                                        //
    //Compare aP against bP                                                     //
    for (; keySz-- > 0; aP++, bP++)                                             //
      {if (*aP < *bP) return -1;                                                //return -1 if aP < bP
       if (*aP > *bP) return +1;                                                //return +1 if aP > bP
      }                                                                         //
    return 0;                                                                   //return 0  if aP == bP
   } //CompareKey...

cBugSim::cBugSim(sPARAMS params)
   {g_bugSimP = this; m_signature = 'sim'; m_params = params;}

//Translate from &BRAM[vv] to address in g_bramP
void *cBugSim::AdrXlat(uintptr_t vv, int offset)
   {return (void*)&g_bramP[vv * g_sP->m_rowBytes + offset];}

//GetBits at srcP[bit] into ordinary C-variable
uint64_t cBugSim::GetBits(void *srcV, int bitOffset, int bitCount)
   {uint8_t *srcP = (uint8_t*)srcV;
    uint64_t result=0;                                                          //
    int      ss, dd;                                                            //source/dest bit counters
    assert(bitCount < 64);                                                      //no fields are >= 64 bits
    uint8_t *dP = (uint8_t*)&result, *sP=&srcP[bitOffset/8];                    //
    for (ss=bitOffset, dd=0; bitCount-- > 0; ss++, dd++)                        //
        {ss  &= 7; dd &= 7;                                                     //
         *dP |= (*sP & (1U << ss)) != 0 ? (1U << dd) : 0;                       //
         if (ss == 7) sP++;                                                     //
         if (dd == 7) dP++;                                                     //
        }                                                                       //
    return result;                                                              //
   } //cBugSim::GetBits...

//-------- Fundamental structure addressing functions
cBOOK *cBugSim::Badr(uint32_t buk)
   {return (cBOOK*)(((uint8_t*)g_sP->m_bookP) + buk * hBOOK_padSz + hBOOK_align);} 

cPAGE *cBugSim::BPadr(uint32_t buk, uint32_t page)
    {hBOOK    *hbookP = (hBOOK*)Badr(buk);                                       //point to cBOOK entry
     hPAGE    *hpageP = (hPAGE*)AdrXlat((int)page >= hPAGE_perRow 
                                       ? GetP2(hbookP) : GetP1(hbookP), hPAGE_align);//get cBOOK::pagesP
     return (cPAGE*)hPadr(hpageP, page % hPAGE_perRow);                          //cPAGE within cPAGE[]
    } //cBugSim::BPadr...

cINDX *cBugSim::BPIadr(uint32_t buk, uint32_t page, uint32_t inx)
   {hPAGE   *hpageP=(hPAGE*)BPadr(buk, page);                                   //
    uint32_t inxAdr=(uint32_t)(inx >= cINDX_perRow ? GetP2(hpageP) : GetP1(hpageP));//
    hINDX   *inxP  = (hINDX*) AdrXlat(inxAdr, hINDX_align);                     //index into cINDX[]
    return  (cINDX*) (((uint8_t*) inxP) + hINDX_padSz * (inx % hINDX_perRow));
   } //cBugSim::BPIadr...

hPAGE *cBugSim::hPadr(hPAGE *hP, int page) {return ADR_ARITHMETIC(hPAGE, hP, hPAGE_padSz, page);}

extern uint32_t g_memRowSize;
void cBugSim::ShowAdr(void *vP)
    {uintptr_t vv = (uintptr_t)vP;
     vv = ((vv - (uintptr_t)g_bramP) / Max(g_memRowSize,1));
     Printf("@bram[%d]", (uint32_t)vv);
    } //cBugSim::ShowAdr...

//Check of bram dump data for valid address pointers (into bram)
bool cBugSim::ValidateBram(uint8_t *bramP, int bUsed, int bramRows)
    {hBOOK   *hBookP;                                                           //
     hPAGE   *pgP;                                                              //
     uint32_t buk, pagesUsed, pg=0, l, r;                                       //
     const char *fmtP="";                                                       //
     for (buk=0; ((int)buk) < bUsed; buk++)                                     //
        {hBookP    = (hBOOK*)Badr(buk);                                         //
         pagesUsed = BookCount((cBOOK*)hBookP);                                 //
         fmtP      = "(pagesUsed)=%d >= %d(=bramRow)";                          //
         if ((l=pagesUsed  ) >= (r=bramRows))                      goto err;    //
         fmtP      = "(page Ref)=%d >= %d(=bramRow)";                           //
         if ((l=(uint32_t)GetP1(hBookP)) >= (r=bramRows) ||                     //
             (l=(uint32_t)GetP2(hBookP)) >= (r=bramRows))          goto err;    //
         for (pg=0; pg < pagesUsed; pg++)                                       //
             {pgP = (hPAGE*)BPadr(buk, pg);                                     //
              fmtP = "(page)=%d >= %d(=bramRow)";                               //
              if ((l=(uint32_t)GetP1(pgP))>= (r=bramRows))         goto err;    //
              if ((l=(uint32_t)GetP2(pgP))>= (r=bramRows))         goto err;    //
         }   }                                                                  //
     return true;                                                               //
err: Printf("*** Invalid address in bram structure: buk=%d, pg=%d, ", buk, pg); // 
     Printf(fmtP, l, r);                                                        // 
     Printf("\n");                                                              //   
     return false;                                                              //
    } //cBugSim::ValidateBram...

//end of file
