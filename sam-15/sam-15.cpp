/* Program sam-<version>.cpp : Version 11, Nov 2022.

Class cSadram simulates sort as implemented in Self Addressing DRAM (SADRAM).
When records are written an index comprising cINDX={key, recordAddress} is created
along with supporting structures called cPAGE and cBOOK.
On subsequent accesses these indexes are used to locate the data in sorted order.
In a real implementation this index structure is maintained by the hardware.
The offset and length of the key field are passed to the cSadram constructor,
and the record length is presumed to be in location[0] of the record (in the case of
variable length records) but the structure of the underlying record (type sRECORD
in the enclosing app) is otherwise completely opaque to cSadram.

Naming convention:
    s<name> refers to a structure.
    c<name> refers to a class
    h<name> refers to a function which is implemented in hardware.
    m_<name> refers to a class member (other than a function or operator),
    Upper case names with underscore refer to #defines, otherwise names use
    upper/lower case names refer to all all other objects.

The three structures (cBOOK, cPAGE, and cINDX) are deployed as arrays called
cBOOK[], cPAGE[], and cINDX[] respectively.
An cINDX points to the user data record. cINDX[] are pointed to by an cPAGE.
The cPAGE[]'s are pointed to by cBOOK. Both cINDX[] and cPAGE[] are sized to
fit into one row of DRAM memory (g_memRowSize) to facilitate hSequencer.
//WARNING: generated code#1
m_bookP
    │  ┌────────────────────────┐ ┌──────────────────────┐ ┌────────────────────────┐
    └─►│cBOOK:pagesP[0,1,2,3,∙∙∙│ │cBOOK:pagesP[0,1,2,∙∙∙│ │cBOOK:pagesP[0,1,2,3,∙∙∙│ ∙ ∙ ∙
       └──────────┬─┬─┬─┬───────┘ └──────┬─┬─┬───────────┘ └──────────┬─┬─┬─┬───────┘m_bUsed items
      ┌───────────┘ █ █ █                █ █ █                        █ █ █ █      pagesP = page1P/page2
      │  ┌───────────────────┐  ┌───────────────────┐  ┌─────────────────────┐
      └─►│cPAGE:x1P[0,1,2∙∙∙,│  │cPAGE:x1P[0,1,2,∙∙∙│  │cPAGE:x1P[0,1,2,3,∙∙∙│ ∙ ∙ ∙
         └─────────┬─┬─┬─┬───┘  └─────────┬─┬─┬─────┘  └────────┬─┬─┬─┬──────┘sBOOK:count items
                   │ █ █ █                │ █ █                 │ █ █ █
   ╔═══════════════╧═╗                  ╔═╧═╗                 ╔═╧═╗
   ║cINDX xP[0].dataP╠═►data record[0]  ║ 0 ╠═►               ║ 0 ╠═►
   ╠═════════════════╣                  ╠═══╣                 ╠═══╣
   ║cINDX xP[1].dataP╠═►data record[1]  ║ 1 ╠═►               ║ 1 ╠═►
   ╠═════════════════╣                  ╠═══╣                 ╠═══╣
   ║cINDX xP[2].dataP╠═►data record[2]  ║...╠═►               ║ 2 ╠═►
   ╠═════════════════╣                  ╚═══╝                 ╠═══╣
   ║      ∙ ∙ ∙      ╠═►                                      ║...╠═►
   ╚═════════════════╝cPAGE::count items                      ╚═══╝
//END WARNING: generated code#1

The two functions cSadram::hSequencer() and cSadram::hShiftIndexM() are implemented
entirely in hardware. These encapsulate the essense of Self Addressing Memory,
ie., they liberate the addressing structure from pure linearity.
hSequencer() compares each element of an incoming row against its immediate neighbour:
   row: item[0], item[1], item[2], ..., item[insertion], item[insertion+1],...
   hSequencer find where a new key belongs;     ↑
   new record belongs here                 ──────┘
This hSequencer is used in four places:
 1. To scan an cPAGE[] array for a given key (for locating the cINDX array)
 2. To scan an cINDX[] array for a given key (for writing and key search)
 3. To scan an cPAGE[] array for a given record number (for readback/update)
 4. To scan an cPAGE[], and cINDX[] to locate a record when reading by key.
It is anticipated that the results of these scans are present continuously
since the underlying cINDX[] and cPAGE[] structures should be present in memory cache.

The index pages (both cINDX[] and cPAGE[] arrays) are always in sort key order.
However, either array will be typically only partially filled. This arises because
the mitosis functions (IndexcDepairing() and PageMitosis()) break a full row (comprising
either an cINDX[] or cPAGE[]) into two pages (page0 and page1). The introduction of
a new cINDX into an cINDX[], or a new cPAGE into an cPAGE[] may force these
arrays into mitosis, resulting in partially filled rows.

xxxMitosis() splits a full array (ie one already containing cINDX_perRow or cPAGE  items)
into two pages (page0, and page1). The split is at the cINDX/cPAGE boundary immediately
following the halfway point (= mitosis point). After mitosis may have different sizes:
     page0.size = mitosis             cINDX/cPAGE records,
and  page1.size = cINDX_perRow - mitosis cINDX/cPAGE records.
In the worst case Mitosis() 'doubles' the total number of cINDX[]/cPAGE[] arrays needed.
In the worst case the indexBase at completion of a sort will contain:
   - one   subordinate array  with mitosis          cINDX's,
   - (n-1) subordinate arrays with (perRow-mitosis) cINDX's.
ie, rcdCount == mitosis + (perRow-mitosis) * (n-1)
Solving for n: n = 1 + (rcdCount - mitosis) / (perRow - mitosis)
This calculation is performed by WorstCase(rcdCount, perRow).

Both cINDX and cPAGE are fixed sized structures; however that size is not known at compile
time since each structure contains a key (of size m_keySize) as specified by the user.
cPAGE::loKey[m_keySize] is the smallest key in the cPAGE::indexP arrays.
Actual size of cINDX = sizeof(cINDX) + m_keySize - 1. Use BPIadr() functions, or [] override.
Actual size of cPAGE = sizeof(cPAGE) + m_keySize - 1. Use BPadr()  functions, or [] override.
Actual size of cBOOK = sizeof(cBOOK) + m_keySize - 1. Use BPadr()  functions, or [] override.

Revision history
samsort-1.cpp  original C++ code
samsort-2.cpp  with paged cINDX structure (ie. cINDX[]'s confined to rowSize)
samsort-3.cpp  modelling of hSequencer
samsort-4.cpp  added second level index (M_bookP)
samsort-5.cpp  added LocateByKey() to LocByRcdNum() for reading data
samsort-6.cpp  added loKey to m_book to speed up sequential access.
sam-8.cpp      added TIME_* functions & classes.
               replaced BPIadr(inxP, inx) and BPadr(pageP, page) with operator[].
               added PackIndexes(), but lacking loading factor.
               added index stripping (sam /strip).
*/

#include <malloc.h>
#include "sam-15.h"
#include "samHelper.h"
#include "samBug.h"
#include <C3_codeTimer.h>

#define MBOOK (*m_bookP)                                                        //C++ operator[] cockup
bool g_alwaysMessageBoxB=false;

extern uint32_t         g_memRowSize,  //physical size of underlying hardware row (in bytes)
                        g_stopOnRcdNum,//debugging: start debugging before writing this record.
                        g_bugLevel;                                             //
extern class cBugIndex *g_bugP;                                                 //
extern cCodeTimer      *g_timerP;                                               //
class cBugSim          *g_bugSimP;                                              //
bool                    g_doubleBookB=false;
extern void _Assert(const char *msgP, const char *noteP, int line, const char *fileNameP);
extern cSadram         *g_sP;                                                   //for convenience of samBug.cpp
cSamError               g_err;                                                  //

int iabs(int val) {return (val < 0) ? -val : val;}                              //

//Time fields used to capture elapsed time of individual events
//Macro declares: g_timer_##name, g_timeValue_##name, g_totalTime_##name, g_startTime_##name, and g_callsTo_##name
#ifdef USE_TIMERS
TIME_FIELDS(Insert_Key);      //Underscores to control display in Excel; see Gather()
TIME_FIELDS(Indx_Mitosis);    //                "
TIME_FIELDS(Page_Mitosis);    //                "
TIME_FIELDS(Locate_Page);     //                "
TIME_FIELDS(Page_Sequencer);  //                "
TIME_FIELDS(Pack_Index);      //                "
#endif

/*Class constructor; calculate architectural parameters and allocate index structures.
 The idea is that software will organize the allocation before passing the task to hardware.
 We want to know exactly what space will be required for m_bookP[m_bMax].
 For concreteness, let us assume a rowSize=1024 Bytes, keySize=9, and rcdCount=2531
 (2531 is the smallest rcdCount which causes page mitosis with this rowSize, keySize
 and current key randomization). From these basic values we can calculate:
      cINDX_size   = sizeof(cINDX) = 13 {rcdNum(4), and key(9)}
 and  cPAGE_size   = sizeof(cPAGE) = 31 {2*pointer(8), count(2), total(4), and key(9)}
      cBOOK_size   = sizeof(cBOOK) = 16 {pointer(9), count(2), total(4), filler(1)}
      cINDX_perRow = 1024/13 = 78, ie., a maximum of 78 cINDX's will fit on a memory row.
      cPAGE_perRow = 1024/23 = 44, ie., a maximum of 44 cPAGE's  will fit on a memory row.
      cPAGE::xP -> cINDX[78], ie., each cPAGE points to an array of 78 cINDX's; index mitosis
                                     may leave as as few as 39 = 78/2 entries in each array.
      cBOOK::page1[0,1,2...]  each entry points to an array of 44 cPAGE's; page mitosis
                                     may leave as few as 22 = 44/2 entries in each array.
      In the worst case m_bookP will need 3 entries (ie. m_bookP[3]) to manage
      2531 cINDX's, since (m_bMax=3) * 22 * 39 = 2575 >= 2531; solving for m_bMax:
      m_bMax     = 2531 / (22*39) = 2.94 which rounds up to 3;
      ie., m_bMax = DivUp(m_rcdCount, (cINDX_perRow/2)*(cPAGE_perRow/2))
*/
cSadram::cSadram(uint32_t rcdCount, int keyOffset, int keySz, int rcdSz, sHDW_PARAMS *hParamsP, uint32_t options)
   {int sz;                                                                     //
    g_sP           = this;                                                      //for convenience of VS watch windows
    m_finishedB    = false;                                                     //
    m_options      = options;                                                   //direction of KeyCompare(...), etc.
    m_keyOffset    = keyOffset;                                                 //offset of key in underlying data record
    m_keySize      = keySz;                                                     //size   of key in underlying data record
    m_rcdSize      = rcdSz;                                                     //
    m_rcdCount     = rcdCount;                                                  //
    m_rawP         = NULL;                                                      //
    m_rowBytes     = g_memRowSize;                                              //
    m_targetBusSz  = hParamsP->targetBusSz;                                     //units = sequencerCells 
    hINDX_align    = 2;                                                         //obsolete ?
    hPAGE_align    = 2;                                                         //obsolete ?
    hBOOK_align    = 6;                                                         //obsolete ?
    if (ComputeGeometry(m_keySize) < 0) return;                                 //
    //Initialize early caching controls                                         //
    if ((m_earlyCache=(m_options & OPTION_EARLY_BITS)) == 0) m_aheadP = NULL;   //
    else{m_aheadP = (sAHEAD*)malloc(sz=m_earlyCache*sizeof(sAHEAD));            //
         BUG1("0x%p earlyCache", m_aheadP);                                     //
         memset(m_aheadP, -1, sz);                                              //not just debugging
        }                                                                       //
    m_earlyHits             = m_earlyTries = 0;                                 //early cache statistics
    m_xMax                  = WorstCase(rcdCount, cINDX_perRow);                //# cINDX[]'s for each cPAGE
                                                                                //
    //Calculate geometry of cBOOK's, allocate and initialize first entry        //
    InitializeBook(rcdCount);                                                   //
    g_bugP                  = new cBugIndex(this, m_keySize);                   //for g_bugP->Bug()
    m_rcdNum                = -1;                                               //source (unsorted) rcd number
    #ifdef USE_TIMERS                                                           //
    //Initialize timing parameters                                              //
        TIME_THIS(Insert_Key,     ;);                                           //make sure all counters are present
        TIME_THIS(Indx_Mitosis,   ;);                                           //   in g_timerP->m_info[]
        TIME_THIS(Page_Mitosis,   ;);                                           //
        TIME_THIS(Locate_Page,    ;);                                           //
        TIME_THIS(Page_Sequencer, ;);                                           //
    #endif                                                                      //
   } //cSadram::cSadram...

//Calculate howmany arrays are required to accommodate count items each with a
//size of perRow. In the worst case array[0] = mitosis, array[1,2,...] = rmdr.
//See worst case analysis at the top of this file.
int cSadram::WorstCase(int count, int perRow)
   {int mitosis = (perRow+1)/2, rmdr = (perRow-mitosis);                        //
  //if (count <= perRow) return 1;                                              //'small case': fits in one row
    return 1 + (count - mitosis + rmdr - 1) / rmdr;                             //ordinary case:
   } //cSadram::WorstCase...

#ifdef _DEBUG
//AllocateRow with tracking
void *cSadram::AllocateRo(const char *bugMsgP, const char *fileNameP,
                          int line, int param1, int param2)
    {void *vP=malloc(g_memRowSize);                                             //
     BUG "@mem 0x%p: alloc ", vP);                                              //
     BUG bugMsgP, param1, param2);                                              //
     BUG " \t%s line %04d\n", fileNameP, line);                                 //
     return vP;                                                                 //
    } //cSadram::AllocateRo...
#else
//AllocateRow with no tracking
void *cSadram::AllocateRo(void) {return malloc(g_memRowSize);}
#endif

//Initialize cBOOK *m_bookP to the proper size of each element.
//bookCount = number of sBOOK entries in m_bookP.
//pgSize    = number of cPAGE's in each cPAGE[] array
//Assign cPAGE to first cBOOK and inxP to first cINDX within that page.
//Further access to cBOOK uses operator[] to accommodate length of sBOOK entries.
void cSadram::InitializeBook(uint32_t rcdCount)
   {cBOOK    *bookP;                                                            //
    cPAGE    *pageP;                                                            //
    uint32_t  buk;                                                              //
                                                                                //
    #ifdef USE_DYNAMIC_ALLOCATION                                               //
        m_bMax   = 1;                                                           //forces ReallocateBook
    #else                                                                       //
        m_bMax   = Max(DivUp(m_rcdCount,(cINDX_perRow/2)*(cPAGE_perRow/2)),1);  //see above comments
    #endif                                                                      //
    if (m_options & OPTION_SIM) {m_bookP = NULL; return;}                       //space assigned by SetBook from sim-10 project
    m_bookP      = (cBOOK*)malloc(m_bMax * cBOOK_size);                         //ie., alloc m_bookP[bMax]
    BUG2("0x%p: alloc m_bookP, max=%d", m_bookP,m_bMax);                        //
    BUGSET(m_bookP, 0x00, m_bMax * cBOOK_size);                                 //
    //allocate and clear entire m_book array                                    //
    for (buk=0; buk < m_bMax; buk++)                                            //
        {bookP         = MBOOK[buk];                                            //
         bookP->page1P =(cPAGE*)cSadram::AllocateRow("m_book[%d].page1",buk,0); //
         bookP->page2P = NULL;                                                  //
         memset(m_bookP->page1P, 0, m_rowBytes);                                //needed for PackIndexes(:)
         pageP         = bookP->page1P;                                         //
         BUGSET(pageP, 0, g_memRowSize);                                        //this is critical to /pack
         bookP->count  = 0;                                                     //
         bookP->total  = (buk+1)*0x11111111;                                    //debugging signature
         ClearKey(bookP->loKey);                                                //
         pageP->x1P    = NULL;                                                  //
         pageP->x2P    = NULL;                                                  //
         pageP->count  = 0;                                                     //
         pageP->total  = (buk+1)*0x10101010;                                    //debugging signature
         ClearKey(pageP->loKey);                                                //
        }                                                                       //
    //First entry only, preset the index substructures                          //
    bookP          = MBOOK[0];                                                  //
    pageP          = bookP->page1P;                                             //m_book[0].pages
    pageP->x1P     = (cINDX*)cSadram::AllocateRow("book[0].page1P[0].x1P", 0, 0);//
    pageP->x2P     = (cINDX*)cSadram::AllocateRow("book[0].page1P[0].x2P", 0, 0);//
    pageP->total   = 0x12345678;                                                //debugging signature
//  pageP          = bookP->page2;                                              //m_book[0].pages
//  pageP->x1P     = (cINDX*)cSadram::AllocateRow("book[0].page2[0].x1P", 0, 0);//
//  pageP->x2P     = (cINDX*)cSadram::AllocateRow("book[0].page2[0].x2P", 0, 0);//
//  pageP->total   = 0x12345679;                                                //debugging signature
    bookP->count   = 1;                                                         //ie., the page just allocated
    m_bUsed        = 1;                                                         //
   } //cSadram::InitializeBook...

#define whatClass cSadram
#include "..\source-15\computeGeometry.cpp"
#undef  whatClass

//Set some cSadram variables post cSadram class constructor glue logic for cSimDriver.
void cSadram::PushBook(cBOOK *bukP, int bUsed)                             //
   {m_bookP_save    = m_bookP;     m_bookP     = bukP;                     //
    m_bUsed_save    = m_bUsed;     m_bUsed     = bUsed;                    //
    m_rcdCount_save = m_rcdNum;    m_rcdNum    = m_rcdCount-1;             //indexbase presumed loaded
    m_finished_save = m_finishedB; m_finishedB = true;                     //
   }  //cSadram::PushBook...
                                                                           //
void cSadram::PopBook(void)                                                //
   {m_bookP        = m_bookP_save;                                         //
    m_bUsed        = m_bUsed_save;                                         //
    m_rcdNum       = m_rcdCount_save;                                      //
    m_finishedB    = m_finished_save;                                      //
   } //cSadram::PopBook...

//class destructor. Deallocate entire indexBase.
cSadram::~cSadram()
   {uint32_t  buk, page;                                                        //
    cBOOK    *bookP;                                                            //
    cPAGE    *pgP;                                                              //
    #ifdef _DEBUG                                                               //
        _CrtCheckMemory();                                                      //
    #endif                                                                      //
    if (m_options & OPTION_SIM) {FREE(m_bookP);} else                           //
       {for (buk=0; m_bookP != NULL && buk < m_bMax; buk++)                     //m_bMAX ! all pages are pre-allocated
            {bookP  = MBOOK[buk];                                               //
             for(page=0; page < bookP->count; page++)                           //
                {pgP = BPadr(buk, page);                                        //cBOOK[book]::pages[page]. x1P & x2P
                 BUG3("0x%p: free book[%d].page[%d].x1P", pgP->x1P, buk, page); //
                 BUG3("0x%p: free book[%d].page[%d].x2P", pgP->x2P, buk, page); //
                 free(pgP->x1P); free(pgP->x2P);                                //
                }                                                               //
             BUG2("0x%p: free book[%d].pagesL  ", bookP->page1P, buk);          //
             BUG2("0x%p: free book[%d].pagesH  ", bookP->page2P, buk);          //
             free(bookP->page1P); free(bookP->page2P);                          //cBOOK[book]::page1 & page2
            }                                                                   //
        FREE(m_bookP);                                                          //
       }                                                                        //
    FREE(m_aheadP);                                                             //
    FREE(m_rawP);                                                               //
   } //cSadram::~cSadram...

//aV < bV return -1, aV == bV return 0, aV > bV return +1
int cSadram::CompareKey(const void *aV, const void *bV, int keySz)
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
   } //cSadram::CompareKey...

//Calculate address within cINDX, cPAGE, and cBOOK structures.
//DO NOT USE 'pageP[page]' or 'inxP[inx]' or 'm_bookP[book]'
//            since cINDX, cPAGE, and cBOOK are variable length.
cBOOK *cSadram::Badr(uint32_t buk)
   {Assert(buk  < m_bMax, NULL); return ADR_ARITHMETIC(cBOOK, m_bookP, cBOOK_size, buk);}

//Return address of book[buk].pages[page]
cPAGE *cSadram::BPadr(uint32_t buk, uint32_t page)
   {Assert(buk  < m_bMax, NULL); //else  ReallocateBook(buk);
    Assert(page < 2*cPAGE_perRow, NULL);
    cBOOK *bookP=MBOOK[buk];
    return page >= cBOOK_perRow && g_doubleBookB
         ? ADR_ARITHMETIC(cPAGE, bookP->page2P, cPAGE_size, page-cBOOK_perRow)
         : ADR_ARITHMETIC(cPAGE, bookP->page1P, cPAGE_size, page);
   } //cSadram::BPadr...

cINDX *cSadram::BPIadr(uint32_t buk, uint32_t page, uint32_t inx)
   {cPAGE *pageP = BPadr(buk, page);
    return inx >= cINDX_perRow
         ? ADR_ARITHMETIC(cINDX, pageP->x2P, cINDX_size, inx-cINDX_perRow)
         : ADR_ARITHMETIC(cINDX, pageP->x1P, cINDX_size, inx);
   } //cSadram::BPIadr...

//Calculate the adr of cBOOK[ii]; (cBOOK_size is unknown at compile time).
cBOOK *cBOOK::operator[](uint32_t buk)  {return ADR_ARITHMETIC(cBOOK, this, cBOOK_size, buk);}

//Calculate the adr of cPAGE[ii]; (cPAGE_size is unknown at compile time).
cPAGE *cPAGE::operator[](uint32_t page) {return ADR_ARITHMETIC(cPAGE, this, cPAGE_size, page);}

//Calculate the adr of cINDX[ii]; (cINDX_size is unknown at compile time).
cINDX *cINDX::operator[](uint32_t inx)  {return ADR_ARITHMETIC(cINDX, this, cINDX_size, inx);}

void cSadram::ClearKey(uint8_t *keyP) {memset(keyP, 0, m_keySize);}

//Return pointer to last key on (buk,page)
uint8_t *cSadram::HiKey(uint32_t buk, uint32_t page)
   {cPAGE *pageP=BPadr(buk, page); return BPIadr(buk, page, pageP->count-1)->key;}

//Step forward from (page, inx) by inc records; Adjust buk, page, and inx if required
cINDX *cSadram::StepForward(uint32_t *bukP, uint32_t *pageP, uint32_t *indxP, uint32_t inc)
   {cPAGE   *pgP;                                                               //
    uint32_t buk=*bukP, page=*pageP, inx=*indxP;                                //dereference all pointers
    do {pgP = BPadr(buk, page);                                                 //
        if ((inx+inc) < pgP->count)                                             //within page ?
           {inx += inc; break;}                                                 //no page overflow: inx += inc has done it.
        else                                                                    //
           {inc -= (pgP->count - inx); inx = 0;                                 //adjust to end of page
            if (++page >= MBOOK[buk]->count)                                    //step page, check for overflow
               {page = 0; if (++buk >= m_bUsed) return NULL;}                   //adjust to next cPAGE array
           }                                                                    //
       } while (inc != 0);                                                      //
    *bukP = buk; *pageP = page; *indxP = inx;                                   //
    return BPIadr(buk, page, inx);                                              //
   } //cSadram::StepForward...

//return rcdOfset of Underlying Data Record in sorted order.
//Each cPAGE record has the total record count we can use hSequencer to locate
//the index page; then compute the cINDX location from the relativized sortNum.
uint8_t *cSadram::LocByNum(uint32_t sortNum)
   {uint32_t  buk=0, page, prev=0, ignore, rcdNum;                              //
    hilo32_t  sortHilo=HiLo32(sortNum);                                         //
    cBOOK    *bookP;                                                            //
    cPAGE    *pageP;                                                            //
    cINDX    *inxP;                                                             //
    bool      bb;                                                               //
    if (m_options & OPTION_STRIP)                                               //
       {return sortNum < m_rcdCount ? (uint8_t*)m_rawP[sortNum] : NULL;}        //
    for (rcdNum=sortNum, buk=0; buk < m_bUsed; buk++)                           //
        {if (rcdNum >= HiLo32((bookP=MBOOK[buk])->total))             continue; //next cPAGE[] of m_book
         bb = hSequencer(bookP->page1P, bookP->count, cPAGE_size,              //inputs
                         (uint8_t*)&sortHilo, OFF_SIZE(cPAGE, total),           //inputs: searching on rcdNum
                         &ignore, &page);                                       //outputs
         if (!bb) return NULL;                                                  //no record with that rcdNum
         //relativize rcdNum to this page by subtracting previous page.total    //
         if (buk != 0 && page == 0)                                             //
             rcdNum -= HiLo32(BPadr(buk-1, MBOOK[buk-1]->count-1)->total);      //
         else                                                                   //
         if (page != 0)                                                         //
             rcdNum -= HiLo32(BPadr(buk, page-1)->total);                       //
         pageP = BPadr(buk, page);                                              //
         Assert(rcdNum >= 0 && rcdNum < pageP->count, NULL);                    //
         inxP   = BPIadr(buk, page, rcdNum);                                    //
         if (m_earlyCache != 0)                                                 //
             EarlyCache(buk, page, rcdNum);                                     //
         return inxP->dataP;                                                    //return offset within UDR's
        }                                                                       //
    return NULL;                                                                //
   } //cSadram::LocByNum...

inline uint8_t *RowAdr(uint8_t *adrP)
   {return (uint8_t*)(((uintptr_t)adrP) & ~((uintptr_t)g_memRowSize-1));}

//Read pages ahead of DataPage(inxP->rcdNum); rmdr = #cINDX's to end of page
//gather statistics of early caching efficiency.
//m_aheadP[] will contain a list comprising {thisPage, data[rcdNum+1], +2,...)}
//This code only gathers statistics on the probable effect of Early Caching.
void cSadram::EarlyCache(uint32_t buk, uint32_t page, uint32_t rcdNum)
   {uint32_t   ii, jj, ignore, count = BPadr(buk, page)->count;                 //
    uint8_t   *dataP = BPIadr(buk, page, rcdNum)->dataP, *rowP=RowAdr(dataP);   //address of current record
    m_earlyTries++;                                                             //statistics
    if (hEqualizer(m_aheadP, m_earlyCache, sizeof(sAHEAD),                      //inputs
                   (uint8_t*)&rowP, OFF_SIZE(sAHEAD, rowP),                     //inputs
                   &ignore, &ii))                                               //outputs
        m_earlyHits++;                                                          //keep statistics
    m_aheadP[0].rowP = rowP;                                                    //
    for (jj=0; ++rcdNum < count && jj < m_earlyCache-1; )                       //
       {dataP = BPIadr(buk, page, rcdNum)->dataP;                               //
        if (RowAdr(dataP) != m_aheadP[jj].rowP)                                 //
            m_aheadP[++jj].rowP = RowAdr(dataP);                                //
       }                                                                        //
    if (false)                                                                  //
       {for (ii=0; ii < m_earlyCache; ii++) Printf("0x%p ", m_aheadP[ii].rowP); //
        Printf("%d/%d\n", m_earlyHits, m_earlyTries);                           //
       }                                                                        //
   } //cSadram::EarlyCache...

//Copy index structures and compact to fill each cBOOK:pagesP and cINDX[]'s.
bool cSadram::PackIndexes(uint32_t loading)
   {uint32_t  rcdCount=m_rcdCount, bUsed,                                       //
              limit = (loading=Max(Min(loading, 100), 50)) * m_rowBytes;        //limit to data in each row
    bool      okB=true, stripB=(m_options & OPTION_STRIP) != 0;                 //
                                                                                //
    bUsed = PackPages(limit, rcdCount);                                         //
    bUsed = (stripB) ? StripKeys(bUsed) : PackIndx (limit, bUsed);              //
                                                                                //
    #ifdef _DEBUG                                                               //
    if (!stripB)                                                                //
       {uint32_t save_bUsed=m_bUsed;                                            //
        m_bUsed = bUsed;                                                        //for g_bugP->Bug()
        if (!(okB=g_bugP->CheckIndexbase(rcdCount)))                            //okB=final result
           g_bugP->Bug("After Pack Indexes, before delete pages");              //
        m_bUsed = save_bUsed;                                                   //
       }                                                                        //
    #else                                                                       //
        okB = true;                                                             //okB=final results
    #endif //_DEBUG...                                                          //
                                                                                //
    DeleteEmptyPages(stripB ? 1 : bUsed);                                       //
    m_bUsed = bUsed;                                                            //finally !:
    #ifdef _DEBUG                                                               //
    if (false)                                                                  //
        BUG_INDEXES("After Deletions");                                         //
                                                                                //
    #endif //_DEBUG...                                                          //
    goto xit;                                                                   //
xit:if (false)                                                                  //
       {if (g_printFileP) {fclose(g_printFileP); g_printFileP = NULL;}}         //
    return okB;                                                                 //
   } //cSadram::PackIndexes...

//Pack cBOOK[i].pagesP[].
//cBOOK::pagesP are pre-allocated and not free'd until ~cSADRAM time.
//This routine packs all cPAGE references in the pagesP array.
//In doing so it adjusts the count field in each cPAGE.
//As each book.pagesP entry is moved, the source record count <= 0 and.
//x1P & x2P are set to NULL (because they are now owned by the destination.
//Returns the number of cBOOK's still active.
//At the end of the day the debugging output will look like:
//book[55].pages@0x000002381AFD80A0, loKey=578D01B2\x00, contains 8 cPAGE[]'s, 177 keys(9387)
//book[56].pages@0x000002381AFD99A0, loKey=58C501A8\x00, contains 8 cPAGE[]'s, 178 keys(9565)
//book[57].pages@0x000002381AFD6660, loKey=5A9F00B1\x00, contains 8 cPAGE[]'s, 149 keys(9714)
//book[58].pages@0x000002381AFCD920, loKey=5D030029\x00, contains 8 cPAGE[]'s, 165 keys(9879)
//book[59].pages@0x000002381AFD62A0, loKey=5E7300F1\x00, contains 7 cPAGE[]'s, 121 keys(10000)
//ie., every pagesP has the same number of cPAGE[]'s (except the last one).
uint32_t cSadram::PackPages(uint32_t limit, uint32_t rcdCount)
   {uint32_t  bookCount, bUsed, sBook, sPage, dBook, dPage;                     //
    cBOOK    *sBookP,      *dBookP;                                             //
    cPAGE    *sPageP=NULL, *dPageP=NULL;                                        //
                                                                                //
    #ifdef _DEBUG                                                               //
       if (!g_bugP->CheckIndexbase(rcdCount)) return false;                     //
       if (false)                                                               //
          BUG_INDEXES("Before Pack Pages");                                     //
    #endif //_DEBUG...                                                          //
    for (dPage=dBook=sBook=0; sBook < m_bUsed; sBook++)                         //for all cBOOK's used
        {dBookP = MBOOK[dBook];                                                 //
         sBookP = MBOOK[sBook];                                                 //
         for (sPage=0, bookCount=sBookP->count; sPage < bookCount; sPage++)     //for all cPAGE[]'s in book.pagesP
             {sPageP = BPadr(sBook, sPage);                                     //
              dPageP = BPadr(dBook, dPage);                                     //
              if (sPageP != dPageP)                                             //
                 {memmove(dPageP, sPageP, cPAGE_size);                        //copy to free locn in page.
                  sPageP->count = 0;                                            //invalidate source book.pagesP[]
                  sPageP->x1P   = sPageP->x2P = NULL;                           //x1P and x2P are now owned by dPageP
                 }                                                              //(might be used later in loop)
              dBookP->count = ++dPage;                                          //
              if (dPage >= cPAGE_perRow)                                       //past end of cBOOK::pagesP
                 {dPage = 0; dBookP = MBOOK[++dBook];}                          //
         }   } //for sPage=..., for dPage=...                                   //
    bUsed = dBook+1 - (dPage == 0 ? 1 : 0);                                     //(dpage...) for new dPage opened but not used
    #ifdef _DEBUG                                                               //
       {uint32_t save_bUsed=m_bUsed;                                            //
        m_bUsed = bUsed;                                                        //for g_bugP->Bug()
        if (!g_bugP->CheckIndexbase(rcdCount)) return false;                   //   work correctly
        if (false)                                                              //
            g_bugP->Bug("After pack pages");                                    //
        m_bUsed = save_bUsed;                                                   //.. but not just yet
       }                                                                        //
    #endif //_DEBUG...                                                          //
    return bUsed;
   } //cSadram::PackPages...

//Copy index structures and compact to fill each cBOOK:pagesP and cINDX[]'s.
//This a particularly nasty piece of code:
//1. Compaction occurs in place and so structures are written at the same time
//   that they are being read. You must be exquisitely careful with pageP->count
//   to avoid treading on your own toes (see Pack Indx().saveDpageCount.)
//2. The cPAGE and cBOOK structures mare be modified in the inner loops to
//   protect against the out loops terminating without properly updating them.
uint32_t cSadram::PackIndx(uint32_t limit, uint32_t bUsed)
   {uint32_t  bookCount, inxCount, total=0, dPageCount,                         //
              sBook, sPage, sIndx, dBook, dPage, dIndx, saveDpageCount;         //
    cBOOK    *sBookP, *dBookP;                                                  //
    cPAGE    *sPageP=NULL, *dPageP=NULL, *saveDpageP=NULL;                      //
    cINDX    *sInxP =NULL, *dInxP =NULL;                                        //
                                                                                //
    //This compacts each cINDX in indexbase.                                    //
    //Read each cINDX from [sBook.sPage]++ and write back to [dBook.dPage]++.   //
    dPageP = m_bookP->page1P;                                                 //first pagesP
    for (dPage=dBook=dIndx=sBook=total=0; sBook < bUsed; sBook++)               //
        {dBookP = MBOOK[dBook];                                                 //
         sBookP = MBOOK[sBook];                                                 //
         for (sPage=0, bookCount=sBookP->count; sPage < bookCount; sPage++)     //
             {sPageP = (*sBookP->page1P)[sPage];                              //
              for (sIndx=0, inxCount=sPageP->count; sIndx < inxCount; sIndx++)  //
                  {sInxP = BPIadr(sBook, sPage, sIndx);                         //
                   dInxP = BPIadr(dBook, dPage, dIndx);                         //
                   if (sInxP != dInxP)                                          //
                       memmove(dInxP, sInxP, cINDX_size);                     //move cINDX structure
                   if (dIndx == 0)                                              //
                      {memmove(BPadr(dBook,dPage)->loKey,sInxP->key, m_keySize);//move cPAGE::loKeys
                       if (dPage == 0)                                          //            "
                          memmove(dBookP->loKey,         sInxP->key, m_keySize);//move cBOOK::loKeys
                      }                                                         //
                   dPageCount    = ++dIndx;                                     //
                   saveDpageP    = dPageP;                                      //snake venom
                   saveDpageCount= dPageCount;                                  //snake venom
                   dBookP->total = dPageP->total = HiLo32(++total);             //set these now in case the loops exits
                   dBookP->count = dPage+1;                                     //
                   //Step dIndx and maybe dBook, and dPage                      //
                   if (dIndx >= 2*cINDX_perRow)                                //past end of cPAGE::x1P and cPAGE::x2P
                      {//This is a real snake pit. Delay setting dPageP->count  //
                       //until the end of the sPage loop.                       //
                       //Problem occurs if the first page is full. In this case //
                       //[dBook.dPage.dInx]=[sBook.sPage.sIndx] & sPageP->count //
                       //gets clobberred before the sIndx loop starts.          //
                       if (saveDpageP)                                          //
                           saveDpageP->count = saveDpageCount;                  //
                       saveDpageP    = NULL;                                    //
                       dPageP        = BPadr(dBook, ++dPage);                   //
                       dIndx         = 0;                                       //
                       if (dPage >= cPAGE_perRow)                              //past end of cBOOK::page1P
                          {dPage     = 0;                                       //
                           dBookP    = MBOOK[++dBook];                          //next array of cBOOK::pagesP
                           dPageP    = dBookP->page1P;                          //
                          }                                                     //
                       dPageCount    = 0;                                       //
                      }                                                         //
                  } //for (sIndx=0...                                           //
             } //for (sPage=...                                                 //
        } //for (dPage=...                                                      //
    if (saveDpageP)                                                             //snake venom
        saveDpageP->count = saveDpageCount;                                     //snake venom
    if (dIndx == 0 && dPage != 0)                                               //overrun in lup-de-lups
       {if (--dPage == 0)                                                       //
           if (dBook != 0) dPage = MBOOK[--dBook]->count-1;                     //
       }                                                                        //
    bUsed = dBook+1 - (dPage == 0 && dIndx == 0 ? 1 : 0);                       // - () for dPage opened but not used
    for (; dBook < m_bUsed; dPage=-1, dBook++)                                  //invalidate cBOOK::pages[i] after [dBook.dPage]
        {while(++dPage < cPAGE_perRow) BPadr(dBook, dPage)->count = 0;}         //first iteration invalidates the tail of cBOOK::page1[].
    return bUsed;                                                               //subsequent iterations invalidate whole  cBOOK::page1[].
   } //cSadram::PackIndx...

uint32_t cSadram::StripKeys(uint32_t bUsed)
   {uint32_t  sBook, sPage, sIndx;
    uint64_t  *destP;                                                           //
    cBOOK     *sBookP=NULL;                                                     //
    cPAGE     *sPageP=NULL;                                                     //
    cINDX     *sInxP =NULL;                                                     //
                                                                                //
    //This compacts each cINDX in indexbase down to a simple pointer.           //
    //Read each cINDX from [sBook.sPage] and store dataP at destP.              //
    destP = m_rawP = (uint64_t*)malloc(m_rcdCount * sizeof(void*));             //
    for (sBook=0; sBook < bUsed; sBook++)                                       //
        {sBookP = MBOOK[sBook];                                                 //
         for (sPage=0; sPage < sBookP->count; sPage++)                          //
             {sPageP = (*sBookP->page1P)[sPage];                                 //
              for (sIndx=0; sIndx < sPageP->count; sIndx++)                     //
                  {sInxP = BPIadr(sBook, sPage, sIndx);                         //
                   memmove(destP++, &sInxP->dataP, sizeof(void *));             //move pointer to user data
                  } //for (sIndx=0...                                           //
              sPageP->count = 0;                                                //invalidate
             } //for (sPage=...                                                 //
        } //for (sBook=...                                                      //
    return 0;                                                                   //
   } //cSadram::StripKeys...

//Free all pages marked as invalid, ie. with count == 0
void cSadram::DeleteEmptyPages(uint32_t bUsed)
   {uint32_t  book, page;                                                       //
    cBOOK    *bookP;                                                            //
    cPAGE    *pageP=NULL;                                                       //
                                                                                //
    for (book=bUsed-1; book < m_bUsed; book++)                                  //starts a little early eh ??
        {bookP = MBOOK[book];                                                   //
         for (page=0; page < cPAGE_perRow; page++)                              //
             {if ((pageP=(*bookP->page1P)[page]) == NULL)   continue;            //huh ?
              if (pageP->count != 0)                        continue;           // != 0 means valid
              if (pageP->x1P == NULL && pageP->x2P == NULL) continue;           // not allocated
              BUG3("0x%p [%d].page1[%d].x1P", pageP->x1P, book, page);          //
              BUG3("0x%p [%d].page1[%d].x2P", pageP->x2P, book, page);          //
              if (pageP->x1P) free(pageP->x1P); pageP->x1P = NULL;              //
              if (pageP->x2P) free(pageP->x2P); pageP->x2P = NULL;              //free unused cINDX[]
        }    }                                                                  //
    return;                                                                     //
   } //cSadram::DeleteEmptyPages...

//Return rcd pointer of record with specifed key value.
//We use hSequencer to locate the index page; then compute the cINDX location
//by applying hSequencer to the cINDX[] in that page.
//The parameter instance identifies the record in a group of identical keys:
//Set instance == 0 before 1st call; LocByKey will step this to 1,2,3... if there
//are additional duplicate records. Finally instance == -1 signals the last dup.
uint8_t *cSadram::LocByKey(const uint8_t *keyP, int *instanceP)
   {uint32_t buk=0, page, prevPage=0, inx, prevInx, cc, instance;               //
    uint64_t result;                                                            //
    uint8_t *resultP;                                                           //
    int      ii;                                                                //
    cBOOK   *bukP;                                                              //
    cPAGE   *pageP;                                                             //
    cINDX   *inxP, *jnxP;                                                       //
    uint8_t *key=(uint8_t*)alloca(m_keySize+1);                                 //
                                                                                //
    memmove(key, keyP, m_keySize);                                              //copy key, we are going to corrupt
    key[m_keySize] = 0;                                                         //debugging
    //decrement key so that it will be < cPAGE::loKey or cINDX::dataP           //
    for (ii=m_keySize, cc=1; --ii >= 0;) {key[ii] -= cc; cc = key[ii] == 0xFF;} //corrupt key
    if (false)                                                                  //
       {g_bugP->Bug("LocByKey");                                                //
        g_bugP->BugKey("Searching for key ", keyP); Printf("\n");               //
       }                                                                        //
    if (hSequencer(m_bookP, m_bUsed, cBOOK_size, key,                           //
                    offsetof(cBOOK, loKey), m_keySize,                          //
                    &buk, (uint32_t*)&result) && buk > 0)                       //be serious
         buk--;                                                                 //start for loop from previous page
    else buk = 0;                                                               //start for loop from beginning
    resultP  = ADR_ARITHMETIC(uint8_t, g_dataP, result, 1);                     // ??*?
    for (; buk < m_bUsed; buk++)                                                //
        {bukP = MBOOK[buk];                                                     //
         if (!hSequencer(bukP->page1P, bukP->count, cPAGE_size, key,             //inputs: location/length of key in row
                         offsetof(cPAGE, loKey), m_keySize,                     //inputs: location/length of key in row
                         &prevPage, &page))                                     //outputs, exit on failure
             {if (buk != (m_bUsed-1)) continue;}                                //otherwise last page - no upper bound
         if (buk != 0 && page == 0)                                             //
            {if (CompareKey(key, HiKey(buk-1, MBOOK[buk-1]->count-1), m_keySize) < 0)//
                {bukP = MBOOK[--buk]; page = bukP->count-1;                     //
            }   }                                                               //
         else                                                                   //
         if (page != 0 && CompareKey(key, HiKey(buk, prevPage), m_keySize) < 0) page--;    //possible overrun of page
         pageP = (*bukP->page1P)[page];                                          //
         if (!DoubleSeq(pageP, pageP->count, cINDX_size, hINDX_perRow,          //
                        key, offsetof(cINDX, key), m_keySize,                   //inputs: location/length of key in row
                        &prevInx, &inx)) continue;                              //outputs, exit on failure
         inxP = BPIadr(buk, page, inx);                                         //location of original hit
         if (CompareKey(keyP, inxP->key,m_keySize) == 0)                        //found the exact key requested
            {resultP = inxP->dataP;                                             //probable result, unless duplicates are involved
             result  = UDR_OFFSET(resultP);                                     //
             if ((instance=*instanceP) < 0) return resultP;                     //no duplicates considered
             jnxP = StepForward(&buk, &page, &inx, instance);                   //locn of requested key
             inxP = StepForward(&buk, &page, &inx, 1);                          //is the next one a duplicate ?
             if (jnxP != NULL && inxP == NULL)                                  //last record of indexBase
                {*instanceP = -1; return jnxP->dataP;}                          //
             if (jnxP == NULL || inxP == NULL ||                                //
                                     CompareKey(keyP, inxP->key, m_keySize)!=0) //end of file or end of duplicates
                  *instanceP = -1;                                              //clear instance
             else *instanceP = instance+1;                                      //update incremented instance #
             return jnxP->dataP;                                                //return rcdNum at originalHit + instance
            }                                                                   //
         break;                                                                 //
        }                                                                       //
    return NULL;                                                                //
   } //cSadram::LocByKey...

//The following function is performed by hardware not software. The row of data is maintained
//in sorted order, and since the comparisons are all between a single datum and the array
//elements it can be performed in a single clock cycle.
//Each element is compared with keyP; the point at which the comparison shifts from < to >=
//is the insertion point. Every element thereafter is also >= keyP; every element before is < keyP.
//
//if an insertion point is correctly found:
//      hSequencer returns true,
//      *locnP = insertion point
//      *prevP = preceding record
//if an insertion point is not found:
//      hSequencer returns false,
//      *locnP = 'eof', ie. insertion point past last record.
//      *prevP = rcdNum of last valid cINDX entry.
bool cSadram::hSequencer(void     *rowV,  uint32_t rowCnt,    uint32_t itemSz,  //array description inputs
                         uint8_t  *keyP,  uint32_t keyOffset, uint32_t keySz,   //key description inputs
                         uint32_t *prevP, uint32_t *locnP)                      //outputs
   {uint8_t   *kP=((uint8_t*)rowV)+keyOffset;                                   //location of 1st key field
    uint32_t  locn;                                                             //
    //Check if key is larger than highest key in array                          //
    if (rowCnt >= 2 &&                                                          //
        CompareKey(keyP, kP + (rowCnt-1)*itemSz, m_keySize) >= 0)               //highest key in array
       {*locnP = rowCnt; *prevP = rowCnt-1; return false;}                      //
                                                                                //
    for (*prevP=*locnP=locn=0; locn < rowCnt; kP+=itemSz, *locnP=++locn)        //for each item in this vector
        {if (CompareKey(keyP, kP, keySz) < 0) return true;                      //
         *prevP = locn;                                                         //
        }                                                                       //
    return false;                                                               //not found condition
   } //cSadram::hSequencer...

//hSequencer adapted to paired structure
bool cSadram::DoubleSeq(cPAGE    *pageP, uint32_t count,
                        uint32_t itemSz, uint32_t perRow,                       //inputs
                        uint8_t  *keyP,  uint32_t keyOff, uint32_t keySz,       //key, location & size: inputs
                        uint32_t *prevP, uint32_t *locnP)                       //outputs
   {bool     bb;                                                                //
//  uint32_t perRow = m_rowBytes/itemSz;                                        //
    if (hSequencer(pageP->x1P, Min(count, perRow), itemSz, keyP, keyOff, keySz, prevP, locnP))
                                                                   return true; //
    if ((int) (count -= perRow) <= 0) return false;                             //data only on x1P
    bb = hSequencer(pageP->x2P, count, itemSz, keyP, keyOff, keySz, prevP, locnP); //
    *prevP = *prevP + perRow;                                                   //index relative to
    *locnP = *locnP + perRow;                                                   //first cINDX[] in pcPAGE
    return bb;                                                                  //
   } //cSadram::DoubleSeq...

//This is equivalent to hSequencer except it requires == comparison.
//Not intended to be implemented in silicon, just a variant on hSeqencer.
bool cSadram::hEqualizer(void *rowV, uint32_t rowCnt, uint32_t itemSz,          //inputs
                         uint8_t *keyP, uint32_t keyOffset,uint32_t keySz,      //inputs
                         uint32_t  *prevP, uint32_t *locnP)                     //outputs
   {uint8_t   *kP;                                                              //
    uint32_t  locn;                                                             //
    for (kP=((uint8_t*)rowV)+keyOffset, *prevP=*locnP=locn=0;                   //
                  locn < rowCnt; kP+=itemSz, *locnP=++locn)                     //for each item in this vector
        {if (CompareKey(keyP, kP, keySz) == 0) return true;                     //
         *prevP = locn;                                                         //
        }                                                                       //
    return false;                                                               //not found condition
   } //cSadram::hEqualizer...

//Right shift cINDX's at [insert thru end-of-array] by one location.
void cSadram::hShiftIndxRight(uint32_t buk, uint32_t page, uint32_t insert)
   {cPAGE    *pageP= BPadr(buk, page);                                          //
    uint32_t  hi   = pageP->count;                                              //
    for (; hi > insert; hi--)                                                   //
        memmove(BPIadr(buk, page, hi), BPIadr(buk, page, hi-1), cINDX_size);  //right shift higher indexed entries
   } //cSadram::hShiftIndxRight...

//Right shift cPAGE at [insert] by one location.
void cSadram::hShiftPageRight(uint32_t buk, uint32_t page, uint32_t insert)
   {for (; page > insert; page--)                                             //move succeeding page pointers right
         memmove(BPadr(buk, page), BPadr(buk, page-1), cPAGE_size);           //
   } //cSadram::hShiftPageRight...

//right shift m_bookP. m_bookP remains compact throught insertions
void cSadram::hShiftBookRight(uint32_t insert)
    {for (uint32_t ii=m_bMax; --ii > insert;)                                  //
          memmove(MBOOK[ii], MBOOK[ii-1], cBOOK_size);                         //
    } //cSadram::hShiftBookRight...

//Search m_bookP for the page on which keyP belongs.
//Straightforward call to hSequencer except for one boundary condition:
//If the page is the first one in the cPAGE[]'s pointed to by m_bookP[].pages
//then it is possible that the record really belongs on the previous page.
//eg. page[0] 'a', 'b, 'c',
//    page[1] 'x', 'y', 'z'
//keyP = 'm' will return page == 1 from hSequencer since the lowest key on page[1]
//is the first cINDX for which cINDX:key > keyP. This is wrong because hSequencer
//does not look into page[0] but relies on the cPAGE[]. Accordingly, if page == 0
//from hSequencer(), then check the previous page at {buk-1, lastpage[buk-1]).
void cSadram::LocatePage(uint8_t *keyP, uint32_t *bukP, uint32_t *pgP)
   {uint32_t  book, ignore, lastPage, page;                                     //
    cBOOK    *bookP;                                                            //
                                                                                //
    hSequencer(m_bookP, m_bUsed, cBOOK_size,                                     //row, count, and itemSize inputs
               keyP, offsetof(cBOOK, loKey), m_keySize,                         //key, locn & size inputs
               &book, &ignore);                                                 //outputs
    bookP = MBOOK[book];                                                        //
    if (hSequencer(bookP->page1P, bookP->count, cPAGE_size,                      //page, count & sizeof(cPAGE)
                   keyP, offsetof(cPAGE, loKey), m_keySize,                     //key, locn & size inputs
                   &page, &lastPage))                                           //outputs
        {if (book > 0 && page == 0 &&                                           //overcooked it ?
             CompareKey(keyP, HiKey(book-1, lastPage), m_keySize) <= 0)         //hiKey on previous page
                page = MBOOK[--book]->count-1;                                  //use last key on previous page
        }                                                                       //
    *bukP= book;                                                                //
    *pgP = page;                                                                //
   } //cSadram::LocatePage...

/*Create an entry in one of the page1[].inxP (=arrays of cINDX[]'s; one per record).
 cINDX[]'s, cPAGE[]'s and cBOOK[]'s are in key order.
 This means the entire indexBase is in key sequence.
 cPAGE[] & cINDX[] are confined to a single row of DRAM. This is critical because it
 enables the hardware to scan the row for the insert-page/index-location in parallel
 as the row is read from DRAM. The classification proceeds in two steps:
     1. hSequencer finds the cPAGE on which targetKey (=dstV.key) belongs.
        Note that this does NOT require that the target cINDX arrays be accessed.
 and 2. hSequencer finds the insertion point within that cPAGE at which targetKey belongs.
 This is more treacherous than may appear. With multiple cPAGE[]'s, it is possible to skip
 over the proper page. Consider inserting targetKey=='b' in the following indexBase:
     MBOOK[0].page1 = {{cPAGE[0], cPAGE[1]},...}
     cPAGE[0]        = {'a', 'f', 'm'}      cPAGE[0].loKey == 'a'
     cPAGE[1]        = {'r', 'u', 's', 't'} cPAGE[1].loKey == 'r'
 The proper insertion point is page[0].location[1] between 'a' and 'f'.
 hSequencer looks for loKey >= targetKey, so the initial scan of the loKey fields of
 m_bookP skips over page[0] (because loKey (=='a') < targetKey (== 'b')) and returns page[1].
 Oops! page[0] is the proper page because loKey(= 'a') <= targetKey(='b') <= hiKey(='m').
 This situation is corrected with some post processing following the first call to hSequencer.
 The signature condition is:
   'buk > 1 && page == 0' && page0.lastkey < targetKey.
 However, we do not keep page.hiKey in the interests of storage efficiency but rely on
 the loKey field of the next cPAGE[] to bracket the keys on any page.

 If necessary split the index page in two pieces to accommodate the new index record.
 Otherwise right shift all records above the insertion point on this cPAGE[page].xP
 and insert new cINDX record.
*/
void cSadram::InsertKey(uint32_t rcdNum, void *dstV)
   {uint32_t     book=0, insertPage, insertInx, inxCount, ignore;               //
    uint8_t     *dstP=(uint8_t*)dstV, *keyP=dstP+m_keyOffset;                   //
    cPAGE       *pageP;                                                         //
    cINDX       *inxP;                                                          //
top:if (m_rcdNum == 0)                                                          //empty indexbase
        pageP = BPadr(book=0, insertPage=insertInx=0);                          //
    else                                                                        //
       {TIME_THIS(Locate_Page, LocatePage(keyP, &book, &insertPage));           //locate cPAGE within cBOOK
        //Record belongs on m_bookP[book]->page1[insertPage].                   //
        //Search thru this page for cINDX::key > target                         //
        pageP = BPadr(book, insertPage);                                        //
        if ((inxCount=pageP->count) < 2*cINDX_perRow)                           //number of cINDX's on this page
            TIME_THIS(Page_Sequencer,                                           //
                      DoubleSeq(pageP, inxCount, cINDX_size, hINDX_perRow,      //searches cPage:x1P and cPAGE::x2P
                                keyP, offsetof(cINDX, key), m_keySize,          //target key inputs
                                &ignore, &insertInx));                          //outputs
       }                                                                        //
    //insertPage, and insertLocn specify where the new key should be added      //
    if (pageP->count >= 2*cINDX_perRow)                                          //both cPage.x1P and .x2P are full
       {TIME_THIS(Indx_Mitosis, IndxMitosis(book, insertPage)); goto top;}      //rearrange the pages
    if (insertInx == 0)                                                         //
       {hMoveKey(pageP->loKey, keyP);                                           //save base of cINDX[] in cPAGE
        if (insertPage == 0) hMoveKey(MBOOK[book]->loKey, keyP);                //save base of cPAGE[] in cBOOK
       }                                                                        //
    hShiftIndxRight(book, insertPage, insertInx);                               //make room for new cINDX
    pageP->count++;                                                             //inc #cINDX's in page::x1P + page::x2P
    //Fill in fields of new cINDX                                               //
    inxP            = BPIadr(book, insertPage, insertInx);                      //
    inxP->dataP     = (uint8_t*)dstV;                                           //save pointer to user data
    hMoveKey(inxP->key, keyP);                                                  //save key from user data
   } //cSadram::InsertKey...

//The sequencer-array is loaded from the SamCache/DRAM.
//The key is loaded into the search logic and sent out across the sequencer-array.
//An opcode then causes the sequencer array to insert the key in the proper location.
//The sequencer-array is written to SamCache.
//The sequencer-array is eventually written to DRAM.
void cSadram::hMoveKey(uint8_t *destKey, uint8_t *srcKeyP)
   {memmove(destKey, srcKeyP, m_keySize);}

void cSadram::IndxMitosis(uint32_t buk, uint32_t page0)
   {uint32_t    page1=page0+1, mitosis=(1+cINDX_perRow)/2;                      //clone is within book[buk]
    cPAGE      *page0P=BPadr(buk, page0), *page1P;                              //this & next page addresses
    cINDX      *inxP;                                                           //
#ifdef _DEBUG                                                                   //
    bool         bb;                                                            //
    if (g_bugLevel >= 1)                                                        //
       bb = BUG_INDEXES("Before IndxMitosis");                                  //
#endif //DEBUG...                                                               //
    //create a new page in front of page1 and initialize the first cINDX record //
    if (!hInsertPage(buk, page1)) return;                                       //insert new page; fails if PageMitosis is called
    page0P->count     = cINDX_perRow;                                           //new count of original page
    page1P            = BPadr(buk, page1);                                      //
    page1P->count     = cINDX_perRow;                                            //count new page
    inxP              = page0P->x2P;                                            //
    page0P->x2P       = page1P->x1P;                                            //
    page1P->x1P       = inxP;                                                   //
    memmove(page1P->loKey, inxP->key, m_keySize);                               //create loKey on new page
#ifdef _DEBUG                                                                   //
 // BUGSET(page0P->x2P, 0x77, cPAGE_size);                                      //
 // BUGSET(page1P->x2P, 0x99, cPAGE_size);                                      //
    if (g_bugLevel >= 1)                                                        //
       BUG_INDEXES("After IndxMitosis");                                        //summary page0 and page1
#endif //DEBUG...
   } //cSadram::IndxMitosis...

//Insert new page before page1 in one cPAGE[] vector of M_bookP[buk]
//Move cINDX[]'s on m_bookP->page1[page1] and insert new page:
//       inx[n-2]─►inx[n-1], inx[n-3]─►inx[n-2], ... inx[page1]─►inx[page1+1],
//       where n = bukP->pages. The pointer at bukP->page1[page1] is then set to the new page
//Then replace page1 with new (empty) page. Return false if PageMitosis was invoked.
bool cSadram::hInsertPage(uint32_t book, uint32_t page1)
   {cPAGE    *pageP;                                                            //
    cBOOK    *bookP=MBOOK[book];                                                //parent of cBOOK::page1
    uint32_t  pages=bookP->count;                                               //count of cPAGE's in this cPAGE[] array
                                                                                //
    if (pages >= cPAGE_perRow)                                                  //
       {TIME_THIS(Page_Mitosis, PageMitosis(book));                             //
        return false;                                                           //forces hSequencer to restart from top:
       }                                                                        //
    hShiftPageRight(book, pages, page1);                                        //move succeeding page pointers right
    bookP->count++;                                                             //step count of cPAGE[]'s
    pageP         = BPadr(book, page1);                                         //
    pageP->x1P    = (cINDX*)AllocateRow("book[%d].pages[%d].x1P", book, page1); //
    pageP->x2P    = (cINDX*)AllocateRow("book[%d].pages[%d].x2P", book, page1); //
    pageP->count  = 0;                                                          //
    return true;                                                                //
   } //cSadram::hInsertPage...

//This code should never be called because the m_bookP[] is pre-allocated.
//However, if m_bMax is artificially set to 1 the allocation of m_bookP becomes
//dynamic and this routine is called to add another entry to m_bookP.
void cSadram::ReallocateBook(uint32_t buk)
    {Printf("*** ReallocateBook");                                              //
     #ifdef USE_DYNAMIC_ALLOCATION                                              //
     m_bookP = (cBOOK*) realloc(m_bookP, (++m_bMax)*cBOOK_size);                //allocate new space in m_bookP
     MBOOK[m_bMax-1]->count = 0;                                                //with zero cPAGE[]'s
     MBOOK[m_bMax-1]->page1= (cPAGE*)AllocateRow("Reallocate[%d]", m_bMax-1,0); //
     return;                                                                    //
     #else //~USE_DYNAMIC_ALLOCATION                                            //
 //   Printf(": Index Loading=%d%%, PageLoading=%d%%, rcdNum=%d\n",              //
 //         g_helpP->IndexLoading(), g_helpP->PageLoading(), m_rcdNum);         //
     Printf("\n");                                                              //
     if (false)                                                                 //
        BUG_INDEXES("ReallocateBook");                                          //
     Assert(false, "Not implemented");                                          //
     #endif //~USE_DYNAMIC_ALLOCATION                                           //
    } //cSadram::ReallocateBook...

void cSadram::PageMitosis(uint32_t buk0)
   {uint32_t     buk1=buk0+1, mitosis, rmdr;                                    //
    cBOOK       *buk0P, *buk1P;                                                 //this & next page addresses
    cPAGE       *nuPageP, *pgP;                                                 //
    #ifdef _DEBUG                                                               //
    if (g_bugLevel >= 2)                                                        //
       {Printf("Before PageMitosis (buk[%d]) ", buk0);                          //
        BUG_INDEXES();                                                          //
       }                                                                        //
    #endif //_DEBUG...                                                          //
    if (m_bUsed >= m_bMax) ReallocateBook(buk0);                                //
    buk0P         = MBOOK[buk0];                                                //delayed in case Reallocate was called by ReallocateBook
    buk1P         = MBOOK[buk1];                                                //
    mitosis       = (1+cPAGE_perRow)/2;                                         //mitosis point within m_bookP[buk]
    rmdr          = cPAGE_perRow - mitosis;                                     //records following mitosis pt
    nuPageP       = MBOOK[m_bMax-1]->page1P;                                     //steal last page from m_bookP
    hShiftBookRight(buk1);                                                      //
    buk1P->page1P = nuPageP;                                                     //
    memmove(buk1P->page1P,pgP=(*buk0P->page1P)[mitosis], cPAGE_size * rmdr);      //copy upper cPAGE[]'s to buk1
    memset(pgP, 0, cPAGE_size * rmdr);                                          //and clear upper cPAGE[]'s
    buk0P->count  = mitosis;                                                    //buk0 has lower sMitosis cPAGE[]'s
    buk1P->count  = rmdr;                                                       //buk1 has the remaining cPAGE[]'s
    hMoveKey(buk1P->loKey, buk1P->page1P->loKey);                                //
    m_bUsed++;                                                                  //
    #ifdef _DEBUG                                                               //
    if (false)                                                                  //
       {Printf("After PageMitosis (book=%d of %d sBOOK's) ", buk0, m_bUsed);    //
        BUG_INDEXES();                                                          //
       }                                                                        //
    #endif //_DEBUG...                                                          //
   } //cSadram::PageMitosis...

//Write record and build index structure
void cSadram::WriteRecord(void *dstP, void *srcP, uint32_t rcdSz)
   {memmove(dstP, srcP, rcdSz);                                                 //move the underlying data record
    TIME_THIS(Insert_Key, InsertKey(++m_rcdNum, dstP));                         //insert key in proper place
    m_bytesWrit += rcdSz;                                                       //
#ifdef _DEBUG                                                                   //
    if (g_bugLevel >= 2)                                                        //
        BUG_INDEXES("After WriteRecord");                                       //
#endif //DEBUG...
   } //cSadram::WriteRecord...

//Calculate total fields in each cBOOK and cPAGE entry.
//This enables hSequencer to locate the proper page of cINDX[]'s without examining each cINDX.
void cSadram::WriteFinish(void)
   {uint32_t  page, buk;                                                        //
    cPAGE    *pageP;                                                            //
    cBOOK    *bookP;                                                            //
    uint32_t  sum;                                                              //
                                                                                //
    if ((m_options & (OPTION_PACK + OPTION_STRIP)) != 0)                        //
       TIME_THIS(Pack_Index, PackIndexes(100));                                 //
    if ((m_options & OPTION_STRIP) == 0)                                        //
       {for (sum=buk=0; buk < m_bUsed; buk++)                                   //
           {for(bookP=MBOOK[buk], page=0; page < bookP->count; page++)          //
               {pageP        = BPadr(buk, page);                                //
                sum         += pageP->count;                                    //
                pageP->total = HiLo32(sum);                                     //cumulative total in each cPAGE
               }                                                                //
            bookP->total     = HiLo32(sum);                                     //cumulative total in each cBOOK entry
       }   }                                                                    //
    m_finishedB = true;                                                         //courtesy for samHelper
   } //cSadram::WriteFinish...

int cSadram::_ErrorN(int erC, uint32_t p1, CC fileNameP, int line, const char *fncP)
   {char b1[20];
    SNPRINTF(b1), "%d", p1);
    return _Error(erC, "", b1, fileNameP, line, fncP);
   } //cSadram::_ErrorN..

int cSadram::_Error(int erC, CC contextP, CC paramsP, CC fileNameP, int line, const char *fncP)
   {char erBuf[MAX_ERROR_MESSAGE_SIZE+256], fn[_MAX_PATH+10]; int len;          //
    erC = g_err.LogError(erC, contextP, paramsP);                               //
    SNPRINTF(fn), "%s #%04d, function=%s", (char*)fileNameP, line, fncP);       //
    if (g_alwaysMessageBoxB || g_err.Severity(erC) == XSEVERITY_MESSAGEBOX)     //
       {g_err.ShortError(erC, erBuf, sizeof(erBuf));                            //
        len = istrlen(erBuf);                                                   //
        Printf("%s\x07 %s\n", erBuf, fn);                                       //beep :(
        strncat(erBuf, "\tPress Yes for more information\n"                     //
                       "\tPress No  to ignore error and continue\n"             //
                       "\tPress Cancel to abort further processing",            //
               sizeof(erBuf)-len-1);                                            //
        switch (MessageBoxA(NULL, erBuf, fn,MB_YESNOCANCEL))                    //
             {case IDNO:     return 0;                                          //
              case IDCANCEL: return erC;                                        //
             }                                                                  //
        erBuf[len] = 0;                                                         //strip 'Press yes...'
        g_err.AddContext(fn);                                                   //
        g_err.FullError(erC, erBuf, sizeof(erBuf));                             //
       }                                                                        //
    else                                                                        //
       {g_err.AddContext(fn);                                                   //
        g_err.ShortError(erC, erBuf, sizeof(erBuf));                            //
       }                                                                        //
    Printf("\n%s\n", erBuf);                                                    //
    return erC;                                                                 //
   } //cSadram::_Error...

//end of file...
