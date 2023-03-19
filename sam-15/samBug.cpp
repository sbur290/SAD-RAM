/* sambug.cpp : Sept 2
 This modules contains debugging routines for class cSadram.
 It also contains a group of routines which verify the internal
 index tables and sorted data:
     bool CheckSequence     (void);          verify order of keys
     bool CheckPresent   (int duplicate); verify all records are present in sorted data
     bool CheckRecords  (void);          verify records are in order
     void CheckMemory    (void);          verify guard words around memory blocks
 They all return true if successful.
*/

#define _CRT_SECURE_NO_WARNINGS 1

//#include "sam-15.h"
#include "samHelper.h"

extern const char      *g_errMsgP;
extern uint32_t         g_memRowSize;
extern uint32_t         g_bugLevel;
extern class cBugIndex *g_bugP;
extern char             g_exeFileDir[_MAX_PATH];
class cSadram          *g_sP=NULL;
extern uint32_t         cBOOK_size, cPAGE_size, cINDX_size;
#define PrintError(causeP, buk, page, inx) _PrintError(__FUNCTION__, __LINE__, causeP, buk, page, inx)
static bool             g_ignoreErrsB=false;

cBugIndex::cBugIndex(cSadram *sP, int keySize) {m_signature = 'bug'; m_keySize = keySize; g_sP = sP;}

int cBugIndex::CompareKey(const void *aV, const void *bV, int keySz)
   {uint8_t *aP=((uint8_t*)aV), *bP=((uint8_t*)bV);                             //typecast aV and bV
    uint32_t a32, b32;                                                          //
    if (keySz == 0) keySz = m_keySize;                                          //
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
   } //cBugIndex::CompareKey...

//These four virtual routines calculate cBOOK, cPAGE and cINDX addresses
cBOOK *cBugIndex::Badr(uint32_t buk)
   {return (cBOOK*)(((uint8_t*)g_sP->m_bookP) + buk * cBOOK_size);}

cPAGE *cBugIndex::Padr(cPAGE *pageP, uint32_t page)
   {return (cPAGE*)(((uint8_t*)pageP) + page * cPAGE_size);}

cPAGE *cBugIndex::BPadr(uint32_t buk, uint32_t page)
   {return g_sP->BPadr(buk, page);}

cINDX *cBugIndex::BPIadr(uint32_t buk, uint32_t page, uint32_t inx)
   {return g_sP->BPIadr(buk, page, inx);}

void cBugIndex::ShowAdr(void *adrP)
  {char buf[32];
   snprintf(buf, sizeof(buf), "%p", adrP);
   Printf("@0x%s", &buf[strspn(buf, "0")]);
  } //cBugIndex::ShowAdr...

uint8_t *cBugIndex::HiKey(uint32_t buk, uint32_t page)
   {cPAGE *pageP= BPadr(buk, page);
    cINDX *inxP = BPIadr(buk, page, PageCount(pageP)-1);
    return IndxKey(inxP);
   } //cBugIndex::HiKey...

//Create a messasge box with error :(just to rub the programmer's nose in it)
bool cBugIndex::_PrintError(const char *fncP, int line, const char *causeP, uint32_t buk, uint32_t page, uint32_t inx)
    {char buf[250], caption[64];                                                //
     snprintf(buf, sizeof(buf), "\t%s [%d.%d.%d] ****\n\n"                      //
              "Yes:\tcontinue and ignore this error henceforth\n"               //return true
              "No:\tcontinue testing for this error\n"                          //return false
              "Cancel:\t Terminate program\n", causeP, buk, page, inx);         //does not return :)
     Printf("\n**** Error in %s() line %04d: %s", fncP, line, buf);             //
     snprintf(caption, sizeof(caption), "Error in %s line #%d", fncP, line);    //
     switch (MessageBoxA(NULL, buf, caption, MB_YESNOCANCEL))                   //yes means ignore the error
          {case IDYES:    return g_ignoreErrsB = true;                          //
           default:                                                             //
           case IDNO:     return g_ignoreErrsB =false;                          //
           case IDCANCEL: exit(1);                                              //
          }                                                                     //
    } //_PrintError...

#if 1 //def _DEBUG
#ifdef FULLY_ALPHABETIC_KEYS
bool cBugIndex::BugKey(const char *titleP, const uint8_t *keyP)
   {int bufSize=4*64+istrlen(titleP)+5;                                         //max key size * 4 + strlen(titleP + 3
    char *buf=(char*)alloca(bufSize);                                           //
    BugKey(buf, bufSize, titleP, keyP);                                         //
    Printf("%s", buf);                                                          //
    return true;                                                                //
   } //cBugIndex::BugKey...                                                     //

bool cBugIndex::BugKey(char *bufP, int bufSize, const char *titleP, const uint8_t *keyP)
   {bool alphaKeyB=m_alphaKeyB;                                                 //
    int ii, keySz=g_sP->m_keySize;                                              //
    if (bufSize < strlen(titleP) + keySz*4 + 2 +1)                              //title + '\x00' + 2 quotes
       {strcpy(bufP, "***bufSize"); return true;}                               //
    bufP[0] = 0;                                                                //
    if (alphaKeyB)                                                              //check key is alphabetic
       for (ii=0; ii < keySz; ii++)                                             //  as promised
           if (keyP[ii] < 0x20 || keyP[ii] >= 0x80) {alphaKeyB = false; break;} //
    snprintf(bufP, bufSize-1, "%s", titleP);                                    //
    bufP   += (int)strlen(bufP);                                                //
    *bufP++ = '\"';                                                             //
    if (alphaKeyB)                                                              //
       {memmove(bufP, keyP, keySz); bufP += keySz;}                             //
    else                                                                        //
       {while (keySz-- >= 0) {sprintf(bufP, "\\x%02X", *keyP++); bufP += 4;}}   //
    *bufP++ = '\"'; *bufP = 0;                                                  //
    return true;                                                                //
   } //cBugIndex::BugKey...
#else
void cBugIndex::BugKey(const char *titleP, uint8_t *keyP)
   {Printf("%s", titleP);                                                       //
    for (int ii=sizeof(sizeof(sRECORD::key)); --ii >= 4; )                      //
        if (keyP[ii] >= 20 && keyP[ii] < 0x80) Printf("%c", keyP[ii]);          //fixed text part
        else                                   Printf("\\%02X", keyP[ii]);      //       "
    Printf("_0x%02X%02X%02X%02X", keyP[3], keyP[2], keyP[1], keyP[0]);          //random (hex part)
   } //cBugIndex::BugKey...
#endif //FULLY_ALPHABETIC_KEYS...

//Print key and data part of underlying data record;
void cBugIndex::BugData(const char *titleP, uint32_t rcdNum, CV rcdV, bool shoDataB)
   {sRECORD *rP=(sRECORD*)rcdV;                                                 //
    char     buf[40];                                                           //
    snprintf(buf, sizeof(buf), "%p", rP);                                       //
    if (titleP) Printf("%s", titleP);                                           //
    if (shoDataB) Printf("userdata[%02d]@0x%s", rcdNum, &buf[strspn(buf,"0")]); //%02d is good enuf for debugging
    if (titleP != NULL || shoDataB) Printf(", ");                               //
    BugKey("key=", rP->key);                                                    //
    Printf(", \"%s\"", rP->txt);                                                //
   } //BugData...

//print all entries in index structures and return true if indexbase is in proper sequence.
//If startPg, and endPg != DFAULT these delimit the pages displayed; otherwise the whole indexBase.
//g_bugLevel == 0 Display a single line containing #books, #pages, and #indexes
//g_bugLevel == 1 Display heading information from m_bookP[ii]
//g_bugLevel == 2 Display allocation of each book and book.page1 and .page2
//g_bugLevel == 3 Display each key int indexbase and the corresponding data record
//Presentation of addresses uses showAdrP since it depends upon who caller.
//showAdrP translates address to bram addresses.
bool cBugIndex::Bug(const char *titleP, bool doubleBookB,                       //
                    void (*showAdrP)(void *), const char *userDataFileP)        //
   {bool       bb;                                                              //
    uint32_t   ii, inx, bukTotal, inxCount=0, inxTotal, endBuk, endPg, page,    //
               rcdOfset, rcdNum, rcds, totalRcds=0, pgUsed, bUsed, buk, startPg;//
    cINDX     *inxP;                                                            //
    cPAGE     *pageP;                                                           //
    cBOOK     *bookP=NULL;                                                      //
    sRECORD   *rP;                                                              //
    uint8_t   *lastKeyP=NULL, *lastLoKeyP=NULL;                                 //
    FILE      *fileP=NULL;                                                      //
                                                                                //
    _CrtCheckMemory();                                                          //
    g_ignoreErrsB = false;                                                      //
    #ifdef FULLY_ALPHABETIC_KEYS                                                //
       m_alphaKeyB = true;                                                      //
    #else                                                                       //
       m_alphaKeyB = false;                                                     //
    #endif                                                                      //
//g_printFileP = fopen("straightShot.dmp", "wb");                               //
    memset(&m_fail, 0, sizeof(m_fail)); m_fails = 0;                            //clear failure log
    if (titleP) Printf("\n-------- %s: ", titleP);                              //
    if ((g_sP->m_options & OPTION_STRIP) && g_sP->m_finishedB) return BugRaw(titleP);//thin soup here
    if ((rcdNum=g_sP->m_rcdNum) == (uint32_t)(-1))                              //
       {Printf("Indexbase is empty; "); return true;}                           //
    bUsed    = g_sP->m_bUsed;                                                   //
    startPg  = 0; endPg  = doubleBookB ? 2* cPAGE_perRow : cPAGE_perRow;        //
    buk      = 0; endBuk = bUsed-1;                                             //
                                                                                //
    Printf("cBOOK");                                                            //
    for (ii=pgUsed=0; ii < bUsed; ii++) pgUsed += BookCount(Badr(ii));          //total pages used
    Printf(" used=%d, cPAGEs used=%d, cINDXs used=%d\n", bUsed,pgUsed,rcdNum+1);//
                                                                                //
    if (g_bugLevel == 0) return true;                                           //
    Printf("cBOOK(size=%d, perRow=%d), cPAGE(size=%d, perRow=%d), cINDX(size=%d, perRow=%d)\n",
            cBOOK_size, cBOOK_perRow, cPAGE_size, cPAGE_perRow, cINDX_size, cINDX_perRow);
    if (g_bugLevel <= 2)                                                        //
       {for (totalRcds=0; buk <= endBuk; buk++, page=0, endPg=2*cPAGE_perRow)   // 2* for g_doubleBookB
            {bookP = Badr(buk);                                                 //
             Printf("  book[%02d] ", buk); showAdrP(BookPageL(bookP));          //
             BugKey(", loKey=", BookLoKey(bookP));                              //
             Printf(", %d cPAGEs", page=ii=BookCount(bookP));                   //
             for (rcds=0; ((int)--ii) >= 0;) rcds += PageCount(BPadr(buk, ii)); //records in this book
             Printf(", %d cINDXs (running total=%d)\n", rcds, totalRcds+=rcds); //
             if (g_bugLevel == 1) continue;                                     //
             for (endPg=Min(endPg, page), page=0; page < endPg; page++)         //list of #cPAGEs in M_bookP[buk]
                 {pageP     = BPadr(buk, page);                                 //
                  bb        = lastLoKeyP && CompareKey(lastLoKeyP, PageLoKey(pageP))>0;//
                  Printf("    book[%02d].page[%d]", buk, page); showAdrP(pageP);//
                  BugKey(", loKey=", lastLoKeyP=PageLoKey(pageP));              //
                  BugKey(", hiKey=", HiKey(buk, page));                         //
                  if (g_bugLevel == 1)                                          //
                     {ii = PageCount(pageP);                                    //
                      Printf(", #cINDXs=%d%s", ii,                              //
                              ii ==   cINDX_perRow ? " (1/2 full)" :            //
                              ii == 2*cINDX_perRow ? " (full)"     : "");       //
                      if (g_sP->m_finishedB)                                    //
                        Printf(", total #cINDXs=%d\n", PageTotal(pageP));//
                      if (bb && !g_ignoreErrsB)
                              PrintError("Lokey sequence", buk, page, 0);       //
                     }                                                          //
                  else                                                          //
                     {Printf(", .x1P"); showAdrP(PageLoP(pageP));               //
                      Printf(", .x2P"); showAdrP(PageHiP(pageP));               //
                      Printf(", %d cINDXs\n", PageCount(pageP));                //
            }    }   }                                                          //
        if (totalRcds != (rcdNum+1) && g_sP->m_finishedB)                       //
           Printf("Record count = %d does not match records found (=%d)\n",     //
                          totalRcds, rcdNum+1);                                 //
        return true;                                                            //
       } //bugLevel <= 2...                                                     //
                                                                                //
    //buglevel == 3: show indexbase and underlying records                      //
    if (userDataFileP != NULL)                                                  //
       {//This little gimmick opens a file with the user data, so we can display//
        //sRECORDs pointed to by cINDX::dataP when called from sim.exe.         //
        fileP = fopen(userDataFileP, "rb");                                     //
       }                                                                        //
    for (inxTotal=bukTotal=0; buk <= endBuk; buk++, startPg=0)                  //
        {if (buk >= g_sP->m_bMax) break;                                        //request for an invalid page fr caller
         for (page=BookCount(bookP=Badr(buk)),inxCount=ii=0; ii <page; ii++)    //count # cPAGEs in this cBOOK.pagesP
             inxCount += PageCount(pageP=BPadr(buk, ii));                       //
         inxTotal += inxCount; bukTotal += page;                                //
         Printf("  Book[%d] contains cPAGE[%d], cINDX[%d]",                     //
                   buk, ii, inxCount);                                          //
         if (g_sP->m_finishedB)                                                 //otherwise total is invalid
                        Printf(", total records=%d", BookTotal(bookP));         //
         BugKey(", lokey=", BookLoKey(Badr(buk)));                              //
         Printf("\n");                                                          //
         for (page=startPg; (int)page <= Min(BookCount(bookP)-1, endPg); page++)//for each page in requested range
            {pageP = BPadr(buk, page);                                          //Display page fields
             Printf("    book[%d].pages[%d]", buk, page);                       //
             BugKey(".loKey=",   PageLoKey(pageP));                             //
             BugKey(", .hiKey=", HiKey(buk, page));                             //
             ii = PageCount(pageP);                                             //
             Printf(", #cINDXs=%d%s", ii, cINDX_perRow*2 == ii ? "(full)" : "");//full signal
             if (!g_ignoreErrsB &&                                              //
                 CompareKey(PageLoKey(pageP), IndxKey(BPIadr(buk, page, 0)))!=0)//loKey on parent cPAGE == cINDX.key
                {if (m_fails++ == 0) {m_fail.buk = buk; m_fail.page = page;}    //
                 if (!PrintError("cPAGE sequence", buk, page, 0)) goto xit;     //oh dear ):
                }                                                               //
             if (g_sP->m_finishedB) Printf(", total=%d", PageTotal(pageP));      //
             Printf("\n");                                                      //
             //detailed cINDX.                                                  //
             for (inx=0; inx < PageCount(pageP); inx++)                         //
                 {Printf("      [%d.%d.%d]: %s", buk, page, inx, inx < 10 ? " " : "");
                  inxP     = BPIadr(buk, page, inx);                            //
                  rcdOfset = (uint32_t)IndxRelAdr(inxP);                        //
                  if (userDataFileP != NULL)                                    //user record is not in memory
                     {if (fileP != NULL)                                        //
                         {fseek(fileP, rcdOfset, SEEK_SET);                     //must read from userDataFileP
                          fread(rP=&m_record, sizeof(m_record),1, fileP);       //
                         }                                                      //
                      else strcpy((rP=&m_record)->txt, "** Unknown record");    //whazat ?
                     }                                                          //
                  else                                                          //
                      rP = ADR_ARITHMETIC(sRECORD, g_dataP,1, rcdOfset);        // user record is already in memory
                  Printf("rcd Offset=%5d, ", rcdOfset);                         //
                  BugData(NULL, rcdOfset, rP, userDataFileP != NULL);           //
                  if (lastKeyP && CompareKey(lastKeyP, IndxKey(inxP)) > 0)      //keys must be GEQ
                     Printf(" *** sequence error");
                  lastKeyP = IndxKey(inxP);                                     //
                  Printf("\n");                                                 //
        }   }    }                                                              //
xit: if (fileP) fclose(fileP);                                                  //
//   if (g_printFileP) fclose(g_printFileP); g_printFileP = NULL;
     return m_fails == 0;                                                       //
    } //cBugIndex::Bug...

bool cBugIndex::BugRaw(const char *titleP)
   {uint64_t *rawP=g_sP->m_rawP;                                                //
    sRECORD  *rP;                                                               //
    Printf("\n");                                                               //
    switch (g_bugLevel)                                                         //
      {case 3:                                                                  //
       case 2:                                                                  //
       case 1:                                                                  //
          for (uint32_t rcdNum=0; rcdNum < g_sP->m_rcdNum+1; rcdNum++, rawP++)  //
              {rP = (sRECORD*)(*rawP);                                          //
               Printf("userdata[%02d]@0x%16p", rcdNum, rP);                     //%02d is good enuf for debugging
               BugKey(", key=", rP->key);                                       //
               Printf(", \"%s\"\n", rP->txt);                                   //
      }       }                                                                 //
    return true;
   } //bool cBugIndex::BugRaw(const char *titleP)

//Ongoing verification of index indexBase.
//Return true if indexBase is in sequence; set g_bugLevel if a failure is observed
bool cBugIndex::CheckIndexbase(bool ccB)
   {uint32_t   buk, page, inx, minus1=-1, rcdCount, len;                        //
    char       msg[125];                                                        //
    cINDX     *inxP;                                                            //
    cPAGE     *pageP;                                                           //
    cBOOK     *bookP;                                                           //
    uint8_t   *lastKey=(uint8_t*)alloca(g_sP->m_keySize);                       //for sequence check during tour of indexBase
    #define APPEND len=(int)strlen(msg); BugKey(&msg[len],(int)(sizeof(msg)-len)//
                                                                                //
    #ifdef _DEBUG                                                               //
    _CrtCheckMemory();                                                          //That will stop it farting in church.
    #endif                                                                      //
    memset(&m_fail, 0, sizeof(m_fail)); m_fails = 0;                            //in case of failure
    if (g_sP->m_rcdNum == minus1) return true;                                  //empty indexbase
    g_sP->ClearKey(lastKey);                                                    //
    if ((buk=g_sP->m_bUsed) > g_sP->m_bMax)                                     //
       {Printf("Invalid buk index(=%d) >= max(=%d)****\n", buk, g_sP->m_bMax);  //
        m_fail.buk = buk;                                                       //
        return false;                                                           //
       }                                                                        //
    for (buk=rcdCount=0; buk < g_sP->m_bUsed; buk++)                            //
        {bookP = Badr(buk);                                                     //
         for (page=0; page < BookCount(bookP); page++)                          //for each page
             {pageP     = BPadr(buk, page);                                     //
              rcdCount += PageCount(pageP);                                     //
              if (CompareKey(PageLoKey(pageP), IndxKey(PageLoP(pageP))) != 0)   //loKey on parent cPAGE == key in page.cINDX[0]
                 {if (!PrintError("cPAGE sequence", buk, page, 0)) return true; // oh dear
                  BugKey(", key=", PageLoKey(pageP));                           //
                  if (m_fails++ == 0) {m_fail.buk = buk; m_fail.page = page;}   //
                  g_bugLevel = Max(g_bugLevel, 2);                              //
                  if (m_fails > 10) {Printf("\n"); return false;}               //be serious
                  continue;                                                     //
                 } //compare cPAGE failure...                                   //
              //detailed cINDX.                                                 //
              for (inx=0; inx < PageCount(pageP); inx++)                        //
                  {inxP = BPIadr(buk, page, inx);                               //
                   if (CompareKey(lastKey, inxP->key) > 0)                      //keys must be increasing
                      {snprintf(msg, sizeof(msg),                               //
                            "[%d,%d,%d] Sequence Error:", buk, page, inx);      //
                       APPEND, "\n\tkey=", inxP->key);                          //
                       APPEND, ",\n\tprevious=", lastKey);                      //
                       m_fail.buk = buk; m_fail.page = page; m_fail.inx = inx;  //
                       if (!PrintError(msg, buk, page, inx)) return false;      // oh dear ):
                       BugKey("Sequence error at key=", inxP->key);             //
                       if (m_fails++ == 0)                                      //
                          {m_fail.inx = inx; m_fail.buk = buk; m_fail.page = page;}//log failure
                           g_bugLevel = Max(g_bugLevel, 3);                     //
                           if (m_fails > 10) {Printf("\n"); return false;}      //be serious
                           continue;                                            //
                      } //compare cINDX failure...                              //
                   memmove(lastKey, inxP->key, g_sP->m_keySize);                //
        }    }    }                                                             //
     if (m_fails != 0) Printf("\n"); else                                       //
     if (rcdCount != (g_sP->m_rcdNum+1))                                        //
        {Printf("Record count = %d does not match records encountered (=%d)\n", //
                    g_sP->m_rcdNum+1, rcdCount);                                //
         m_fails++;                                                             //
        }                                                                       //
     return m_fails == 0;                                                       //
     #undef APPEND                                                              //
    } //cBugIndex::CheckIndexbase...

//Called from InsertKey before a new key is inserted into indexBase
void cBugIndex::BugB4Insert(uint32_t rcdNum, void *rcdP)
   {Bug("\nBefore InsertKey");                                                  //
    Printf(CONDITIONAL_CRLF "Inserting rcd #%d", rcdNum);                       //
    BugData("", rcdNum, rcdP, false);                                           //false = print user data only
    Printf("\n");                                                               //
   } //cBugIndex::BugB4Insert...

//Debugging: Step thru the indexing structure and verify they are in sequence.
bool cBugIndex::CheckSequence(SAM_CONTROLS *ctlP)
   {bool       verboseB=ctlP->verboseB;                                         //
    uint32_t   buk, page, inx;                                                  //
    cINDX     *inxP=NULL, *prevInxP=NULL;                                       //
    cPAGE     *pageP;                                                           //
                                                                                //
    if (verboseB) Printf("Checking cINDX[]s and cPAGE[]s are in sequence.\n");  //
    for (buk=0; buk < g_sP->m_bUsed; buk++)                                     //
      for (page=0; page < BookCount(Badr(buk)); page++)                         //
        {pageP = BPadr(buk, page);                                              //
         for (inx=0; inx < PageCount(pageP); inx++, prevInxP=inxP)              //
            {inxP = BPIadr(buk, page, inx);                                     //
             if (inx != 0 && CompareKey(prevInxP->key, inxP->key) > 0)          //compare with preceeding key
                {return PrintError("Seq failure", buk, page, inx);}             //false = sequence failure, true=ignore error
        }   }                                                                   //
    return true;                                                                //
   } //cBugIndex::CheckSequence...

/*Check that all records are seen during read.
 Get original recordNum from sRECORD::txt and set bit in present.
(source records have a txt field comprising "Original Record ###" ###=recordNum
 present[] must be all 1's when done.
 duplRcd is the record forced to have a duplicate key by samApp. */
bool cBugIndex::CheckPresent(SAM_CONTROLS *ctlP, uint32_t duplRcd)
   {bool        verboseB=ctlP->verboseB;                                        //
    uint32_t    sortNum, ii, rcdCnt=g_sP->m_rcdCount,                           //== records writ (zero based)
                pSz=(rcdCnt+7)/8;                                               //size of bit vector
    uint8_t    *present=(uint8_t*)calloc(pSz+1, 1),bits;                        //bit vector: bit set to 1 if record found
    sRECORD    *r0P, *r1P=NULL;                                                 //
    const char *msgP="**** Error in LocByRcdNum()";                             //
                                                                                //
    if (verboseB) Printf("Checking all records are present in sorted data.\n"); //
    if (rcdCnt > duplRcd) present[duplRcd/8] = 1 << duplRcd;                    //forced duplicate record
    for (sortNum=0; sortNum < rcdCnt; sortNum++)                                //
       {if ((r0P=(sRECORD*)g_sP->LocByNum(sortNum)) == NULL)          goto bad; //read data in sorted order
        if ((ii=r0P->rcdNum) > g_sP->m_rcdCount)                      goto bad; //what ??
        bits = 1 << (ii & 7);                                                   //
        if ((present[ii/8] & bits) == 0) {present[ii/8] |= bits;      continue;}//set bit in present[]
        if (_strnicmp(r0P->txt, "DuplicateRecord", 15) == 0)          continue; //intentional duplicate
        Printf("CheckPresent. Unexpected Duplicate record ****");               //
        for (ii=0; ii < sortNum; ii++)                                          //only consider earlier records
            {r1P  = (sRECORD*)g_sP->LocByNum(ii);                               //
             if (CompareKey(r1P->key, r0P->key) == 0)                           //
                {Printf(", duplicate of record %d", ii); break;}                //
            }                                                                   //
        for (ii=0; ii < 2; ii++, r0P=r1P)                                       //
            {if (r0P == NULL) continue;                                         //
             Printf("\nudr[%d]=%s", r0P->rcdNum, r0P->txt);                     //
             BugKey(", key='", r0P->key);                                       //
             Printf("'\n");                                                     //
            }                                                                   //
        goto xit;                                                               //
       } //for (sortdLocn=...                                                   //
    msgP = "Not all Records found in sorted data";                              //possible error
    //Now present[] should be all 1's                                           //
    if (present[pSz-1] != (1 << (rcdCnt&7))-1 && (rcdCnt & 7) != 0)   goto bad; //last location unless exact multiple of 8 rcds were writ
    if (rcdCnt == 8 && present[0] != 0xFF)                            goto bad; //Special case 8
    if (rcdCnt > 8)                                                             //
       {if (present[0] != 0xFF)                                       goto bad; //
        if (memcmp(present, &present[1], pSz-2) != 0)                 goto bad; //is the remainder all 0xFF ?
       }                                                                        //
    free(present); return true;                                                 //happy trails
bad:Printf("CheckPresent(). %s; sorted rcd#=%d, rcdCount=%d, rowSize=%d ****\n",//
                             msgP, sortNum, rcdCnt, g_memRowSize);              //
xit:free(present); return false;                                                //unhappy
   } //cBugIndex::CheckPresent...
#endif //_DEBUG...

//Verify that the index indexBase is in the proper sorted sequence
bool cBugIndex::CheckRecords(SAM_CONTROLS *ctlP)
   {bool      verboseB=ctlP->verboseB;                                          //
    sRECORD  *rP, *lastRcdP=NULL;                                               //
    uint32_t  ii;                                                               //
    uint64_t  locn, lastLocn;                                                   //
    char      note[60];                                                         //
    int       result;                                                           //
                                                                                //
    if (verboseB) Printf("Checking sorted records are in proper sequence.\n");  //
    for (ii=0; ii < g_sP->m_rcdCount; ii++, lastRcdP=rP, lastLocn=locn)         //
        {rP   = (sRECORD*)g_sP->LocByNum(ii);                                   //location of next UDR
         locn = UDR_OFFSET(rP);                                                 //
         snprintf(note, sizeof(note), "locn = %d, bytesWrit=%d",                //
                                          (int)locn, (int)g_sP->m_bytesWrit);   //
         if (locn < 0 || locn >= g_sP->m_bytesWrit) {printf("Error 9995: %s\n", note); exit(1);}                               //
         if (ii == 0) continue;                                                 //wait for second record
         result = CompareKey(rP->key, lastRcdP->key);                           //check key sequence
         if (result  < 0 ||                                                     //absolute decrease - bad
             result == 0 && (locn < lastLocn))                                  //keys are equal; not an error if records are still in
            {Printf("CheckRecords. Sequence error in sorted data, rcdNum=%d****\n", locn);//   original order, ie. locn < lastLocn
             return false;                                                      //unhappy
        }   }                                                                   //
    return true;                                                                //happy
   } //cBugIndex::CheckRecords...

void cBugIndex::CheckMemory(SAM_CONTROLS *ctlP)
   {bool     verboseB=ctlP->verboseB;                                           //
    long     lo, hi, *lP;                                                       //
    uint32_t buk, page;                                                         //
    cBOOK   *bookP;                                                             //
                                                                                //
    if (verboseB) Printf("Checking cINDXs and cPAGEs memory allocation.\n");    //
    for (buk=0; buk < g_sP->m_bUsed; buk++)                                     //
        {bookP = Badr(buk);                                                     //
         for (page=0; page < BookCount(bookP); page++)                          //
            {lP = (long*)BPIadr(buk, page, 0);                                  //
             lo = *(lP-1); hi = *(lP+g_memRowSize/4);                           //
             assert(lo == 0xFDFDFDFD && hi == 0xFDFDFDFD);                      //
        }   }                                                                   //
   } //cBugIndex::CheckMemory...

void cBugIndex::BugToFile(const char *fileNameP, int bugLevel)
   {int   bug=g_bugLevel;
    char  fn[_MAX_PATH];
    FILE *tFileP, *dFileP;
    snprintf(fn, sizeof(fn), "%s\\%s", g_exeFileDir, fileNameP);
    if ((dFileP=fopen(fn, "wb")) == NULL) return;
    tFileP       = g_printFileP;
    g_printFileP = dFileP;
    g_bugLevel   = bugLevel;
    g_bugP->Bug(NULL);
    g_printFileP = tFileP;
    fclose(dFileP);
    g_bugLevel   = bug;
   } //cBugIndex::BugToFile...

//end of file...
