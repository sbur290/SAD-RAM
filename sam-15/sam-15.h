#ifndef SADRAM_H_INCLUDED
#define SADRAM_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <c3_always.h>
#include <hStructures.h>
#include <c3_errors.h>
#include <samVersion.h>

#define UDR_OFFSET(rP) (uint64_t)(((uint8_t*)rP) - ((uint8_t*)g_dataP))
#define CC const char*
#define CV const void*
#define SNPRINTF(buf) buf[sizeof(buf)-1] = 0; snprintf(buf, sizeof(buf)-1
#define CAT_THIS(buf) len = istrlen(buf); buf[sizeof(buf)-1] = 0; snprintf(&buf[len], sizeof(buf)-1-len

extern uint32_t  g_bugLevel;
extern FILE     *g_printFileP;
extern uint32_t  g_memRowSize;

typedef unsigned short     hilo16_t;
typedef unsigned long      hilo32_t;
typedef unsigned long long hilo64_t;
inline uint16_t HiLo16(uint16_t u16) {return ((u16 >> 8) & 0xFF) + ((u16 << 8) & 0xFF00);}
inline uint32_t HiLo32(uint32_t u32) {return (u32 >> 24) + ((u32 >> 8) & 0xFF00) + ((u32 << 8) & 0xFF0000) + ((u32 <<24) & 0xFF000000);}
inline uint64_t HiLo64(uint64_t u64) {return (((uint64_t)HiLo32((uint32_t)u64)) << 32) + HiLo32((uint32_t)(u64 >> 32));}
void   Printf(char const *fmtP,...);

#define ClosePrintFile() \
        {if (g_printFileP) fclose(g_printFileP); g_printFileP = NULL;}

#define MyIsValidHeapPointer(a,b,c) _CrtIsValidHeapPointer(a)

//cINDX contains: a pointer to the underlying data record (UDR)
//and             a copy of the key from that UDR.
//cINDX records are organized into cINDX[] arrays that are small enuf to fit on
//a row of memory (size = g_memRowSize).
#pragma pack (push, 1)
class cINDX                         //variable sized structure
   {public:                         //
    uint8_t        *dataP;          //index of underlying record.
    uint8_t         key[1];         //key of underlying record, actual size = m_keySize.
    cINDX *operator[](uint32_t inx);//returns &cINDX[inx] corrected for keySize
   }; //cINDX...
extern uint32_t cINDX_size, cINDX_padSz, cINDX_perRow;   //actual size, actual perRow

//Management of cINDX[]'s. cPAGE points to a pair of cINDX[] arrays (x1P and x2P)
class cPAGE                         //variable sized structure
   {public:                         //
    cINDX          *x1P, *x2P;      //pointers to arrays of cINDX elements
    uint16_t        count;          //number of cINDX[]'s in x1P[] + x2P[]
    hilo32_t        total;          //total # cINDX's in this and all preceding pages
    uint8_t         loKey[1];       //.... [m_keySize]; lowest key on x1P[] this index page
    cPAGE *operator[](uint32_t);    //returns &cPAGE[inx] corrected for keySize
   }; //cPAGE...
extern uint32_t cPAGE_size, cPAGE_padSz, cPAGE_perRow;   //actual size, actual perRow

//Highest level indexing structure is m_bookP[].
//cBOOK entries point to an array of count cPAGE[]'s (page1/2).
//total is counts total number of records up to this book entry (including this book)
class cBOOK                         //variable sized structure
   {public:                         //
    cPAGE          *page1P, *page2P;//array of pointers to cPAGE[]'s
    uint16_t        count;          //number of cPAGE[]'s in page1&2
    hilo32_t        total;          //total # cINDX's in this and preceding books
    uint8_t         loKey[1];       //lowest key in m_pages[0]
    cBOOK *operator[](uint32_t);    //returns &m_bookP[inx] corrected for keySiz
   }; //cBOOK...
extern uint32_t cBOOK_size, cBOOK_padSz, cBOOK_perRow;   //actual size, actual perRow
//Note: Vagaries in the C++ implementation of operator[] compel the syntax
//(*m_bookP)[inx] to address a cBOOK element. use #define MBOOK (*m_bookP)
//(*pageP)  [inx] to address a cPAGE element.
//(*inxP)   [inx] to address a cINDX element.

//Structure to contain addresses of user records prepared in advance of read/write
typedef struct {uint8_t *rowP;} sAHEAD;

//Offset and size of item passed to hSequencer
#define OFF_SIZE(struk, field) offsetof(struk, field), sizeof(struk::field)

//Note: cINDX, cPAGE, and cBOOK are fixed length structures;
//however that size is not known until run time.
//The actual sizeof(cBOOK) = sizeof(cBOOK) + m_keySize - 1.
//The actual sizeof(cPAGE) = sizeof(cPAGE) + m_keySize - 1.
//The actual sizeof(cINDX) = sizeof(cINDX) + m_keySize - 1.
//Accordingly all address computations use the following two functions:
//    cINDX *BPIadr(page, rcdInPage); or (*inxP)[rcdInPage]
//    cPAGE *BPadr(page);            or (*pageP)[rcdInPage)

#ifdef _DEBUG
 #define BUGSET(a,b,c) memset(a,b,c)
#else 
 #define BUGSET(a,b,c)   {}
#endif //_DEBUG...

#define ADR_ARITHMETIC(type, baseP, eSize, inx) (type*)(((uint8_t*)baseP) + (eSize) * (inx))
typedef struct {uint32_t buk, page, inx;} LOCATOR;                                  //locator of a record
inline int iabs(int val) {return val < 0 ? -val : val;}
#pragma pack(pop)

#define DO_TIMING
#ifdef DO_TIMING
 #define TIME_FIELDS(name)    int g_timer_##name; cTIMEVALUE g_timeValue_##name; uint64_t g_totalTime_##name, g_startTime_##name, g_callsTo_##name
 #define TIME_ON(name)       {g_callsTo_##name++; g_startTime_##name = g_timeValue_##name.GetGmtv().m_time;}
 #define TIME_OFF(name)       g_totalTime_##name += (g_timeValue_##name.GetGmtv().m_time - g_startTime_##name);
 #define SHO_TIME_FIELD(name) ShoTimeField(#name, g_totalTime_##name, g_callsTo_##name)
#else
 #define TIME_FIELDS(name)
 #define TIME_ON(name)
 #define TIME_OFF(name)
 #define SHO_TIME_FIELD(name)
#endif

//Bit encoding of controls parameter to cSadram constructor.
#define OPTION_EARLY_BITS  0x0000000F //setting or early caching depth
#define OPTION_LSB         0x00000100 //Intel    byte ordering of key
#define OPTION_MSB         0x00000200 //Motorola byte ordering of key
#define OPTION_FLOAT       0x00000300 //key is floating point (size=4) or double(size=8)
#define OPTION_TYPE_BITS   0x00000300 //intel/motorola bits
#define OPTION_VBL_RCD     0x00000400 //==1 use variable length records (sam /variable)
#define OPTION_PACK        0x00000800 //==1 pack indexbase on completion of write
#define OPTION_STRIP       0x00001000 //==1 strip keys from indexbase on completion of write
#define OPTION_SIM         0x00002000 //==1 called from sim

//Configuration word (actually byte in this release)
#define CFG_INDX_BITS            1:0  //cell  cfg bits when processing hINDX
#define CFG_PAGE_BITS            3:2  //cell  cfg bits when processing hPAGE/hBOOK
#define gCFG_INDX_BITS           5:4  //group cfg bits when processing hINDX
#define gCFG_PAGE_BITS           7:6  //group cfg bits when processing hPAGE/hBOOK
#define hCFG_IDLE                0x00 //cell is inactive for comparison (CFG_INDX_BITS or CFG_PAGE_BITS)
#define hCFG_FRST                0x01 //cell is first cell in a bank    (CFG_INDX_BITS or CFG_PAGE_BITS)
#define hCFG_MIDL                0x02 //cell is neither first nor last  (CFG_INDX_BITS or CFG_PAGE_BITS)
#define hCFG_LAST                0x03 //cell is last  cell in a bank
#define gCFG_INDX_1ST            0x10 //first  hINDX         [gCFG_INDX_BITS] == gCFG_1ST   ([5:4] = 2'b01)
#define gCFG_INDX_LAST           0x20 //last   hINDX         [gCFG_INDX_BITS] == gCFG_LAST  ([5:4] = 2'b10)
#define gCFG_INDX_DEAD           0x30 //unused hINDX         [gCFG_INDX_BITS] == gCFG_DEAD  ([5:4] = 2'b11)
#define gCFG_PAGE_1ST            0x40 //first  hPAGE/hBOOK   [gCFG_PAGE_BITS] == gCFG_1ST   ([7:6] = 2'b01)
#define gCFG_PAGE_LAST           0x80 //last   hPAGE/hBOOK   [gCFG_PAGE_BITS] == gCFG_LAST  ([7:6] = 2'b10)
#define gCFG_PAGE_DEAD           0xC0 //unused hPAGE/hBOOK   [gCFG_PAGE_BITS] == gCFG_DEAD  ([7:6] = 2'b11)

//NOTE: minimum key size == two prevents CFG_1ST and CFG_LAST colliding
#define MIN_KEY_SIZE                2 //bytes
#define MAX_KEY_SIZE               64 //bytes

#ifdef _DEBUG
   #define BUG_INDEXES g_bugP->Bug
#else
   #define BUG_INDEXES {}/##/
#endif
typedef enum {ITEM_CFG=0, ITEM_INDX=1, ITEM_PAGE=2, ITEM_BOOK=3} eITEM_TYPE;

#define BRAM_ADR_BITS 5

#define Error(erC, context, param) _Error(erC,  context, param, __FILE__, __LINE__, __FUNCTION__)
#define ErrorN(erC, p1)            _ErrorN(erC, p1,             __FILE__, __LINE__, __FUNCTION__)
extern cSamError g_err;                                                       //

class cSadram
   {public:                                                                     //
    cSadram             (uint32_t rcdCount, int keyOfset, int keySz, int rcdSz, sHDW_PARAMS *hParamsP, uint32_t options);//class constructor
   ~cSadram             ();                                                     //class destructor
    int _ErrorN         (int erC, uint32_t p1, CC fileNameP, int line, const char *fncP);
    int _Error          (int erC, CC contextP, CC paramsP, CC fileNameP, int line, const char *fncP);
    uint8_t *LocByKey   (const uint8_t *keyP, int *instanceP);                  //return &UDR[]         lookup record by key
    uint8_t *LocByNum   (uint32_t sortNum);                                     //return &UDR[sortNum]  lookup record by record number
    uint8_t *ReadFirst  (void) {m_seqRecord = -1; return ReadNext();};          //Read first sorted record.
    uint8_t *ReadNext   (void) {return LocByNum(++m_seqRecord);}                //read next sorted record.
    void     WriteRecord(void *dstP, void *srcP, uint32_t rcdSz);               //Write a record
    void     WriteFinish(void);                                                 //
    void     PushBook   (cBOOK *bukP, int bUsed);                               //glue logic for CompilerWrap
    void     PopBook    (void);                                                 //glue logic for CompilerWrap
    uint32_t RoundupToTargetBus(uint32_t valu) {return DivUp(valu, m_targetBusSz) * m_targetBusSz;} //
    bool     PackIndexes(uint32_t loading);                                     //pack indexbase
    private:                                                                    //
    uint32_t       m_keySize,   m_keyOffset, m_rcdSize, m_rcdCount, m_options,  //copy user supplied parameters
                   m_rowBytes,  m_targetBusSz,          m_rcdCount_save,        //row size of underlying hardware
                   m_xMax,                                                      //computed cINDX geometry
                   m_bMax,      m_bUsed,    m_bUsed_save,                       //computed cBOOK geometry
                   m_rcdNum,    m_bytesWrit,m_seqRecord,                        //record number of record written
                   m_earlyCache,m_earlyHits, m_earlyTries,                      //early cache statistics
                   m_baseSz[4], m_sizes[4], m_align[4], m_padSz[4], m_perRow[4];//architectural parameters by eITEM_TYPE
    cBOOK         *m_bookP, *m_bookP_save;                                      //highest level indexbase structure
    uint64_t      *m_rawP;                                                      //raw record numbers when indexbase is stripped
    sAHEAD        *m_aheadP;                                                    //
    bool           m_finishedB, m_finished_save;                                //set when sort is complete and total fields are valid.
    int      CompareKey (CV aV, CV bV, int keySz);                              //return -1 if aP <  bP.
                                                                                //keySz == 0 defaults to keySz=m_keySize
    void     DeleteEmptyPages(uint32_t bUsed);                                  //called from packIndexes
    cBOOK   *Badr       (uint32_t buk);                                         //calculate &m_book[buk]
    cPAGE   *BPadr      (uint32_t buk, uint32_t page);                          //calculate &m_book[buk]->page1&2[page]
    cINDX   *BPIadr     (uint32_t buk, uint32_t page, uint32_t inx);            //calculate &m_book[buk]->xP[pageNum][inx]
    uint8_t *HiKey      (uint32_t buk, uint32_t page);                          //last key on m_book[buk].page1&2[page]
    #ifdef _DEBUG                                                               //
     #define AllocateRow(fmt,p1, p2) AllocateRo(fmt, __FILE__, __LINE__, p1, p2)//
     static void*AllocateRo (CC bugMsgP, const char *fileNameP, int line,       //
                              int param1=0, int param2=0);                      //
    #else                                                                       //
     #define AllocateRow(a,b,c) AllocateRo()                                    //
     static void *AllocateRo(void);                                             //
    #endif                                                                      //
    private:                                                                    //
    int      Align2TargetBus(int posn, int itemSize);                           //
    void     ClearKey   (uint8_t *keyP);                                        //clear to zero memory at keyP for m_keySize
    int      ComputeGeometry(int keySize);                                         //
    static int DivUp    (int aa, int bb) {return (aa+bb-1)/bb;}                 //
    bool     DoubleSeq  (cPAGE    *pageP, uint32_t count,                       //two calls of hSequencer with pageP->x1P and pageP->x2P
                         uint32_t itemSz, uint32_t perRow,                      //size of item and number per row
                         uint8_t  *keyP,  uint32_t keyOff, uint32_t keySz,      //key, locn & size: inputs
                         uint32_t *prevP, uint32_t *locnP);                     //outputs
    void     InsertKey  (uint32_t rcdNum, void *rcdP);                          //extract key from rcdP and insert in indexbase
    uint32_t PackIndx   (uint32_t limit, uint32_t bUsed);                       //subroutine of PackIndx
    uint32_t PackPages  (uint32_t loading, uint32_t rcdCount);                  //subroutine of PackIndx
    uint32_t StripKeys  (uint32_t bUsed);                                       //subroutine of PackIndx
    uint32_t RoundupToTargetbus(uint32_t size);                                 //
    int    WorstCase    (int count, int perRow);                                //Calculate space required in worst-case
    //Functions implemented in hardware                                         //
    bool   hSequencer   (void *rowP, uint32_t rowCnt, uint32_t itemSz,          //input  params
                         uint8_t  *keyP,uint32_t keyOffset, uint32_t keySz,     //
                         uint32_t *prevP, uint32_t *inxP);                      //output params
    //Same as hSequencer except demands an equal comparison.                    //
    bool   hEqualizer   (void *rowV, uint32_t rowCnt, uint32_t itemSz,          //inputs
                         uint8_t *keyP, uint32_t keyOffset,uint32_t keySz,      //inputs
                         uint32_t  *prevP, uint32_t *locnP);                    //outputs
    bool   hInsertPage  (uint32_t buk, uint32_t page1);                         //
    void   hMoveKey     (uint8_t *destKey, uint8_t *srcKeyP);                   //
    void   hShiftIndxRight(uint32_t buk, uint32_t page, uint32_t inx);          //right shift cINDX[]
    void   hShiftPageRight(uint32_t buk, uint32_t pages, uint32_t page1);       //right shift cPAGE[]
    void   hShiftBookRight(uint32_t buk1);                                      //right shift cBOOK[]
    void   IndxMitosis  (uint32_t buk, uint32_t page);                          //clone full cINDX[]
    void   PageMitosis  (uint32_t buk);                                         //close full cPAGE[]
    void   InitializeBook(uint32_t rcdCount);                                   //initialize m_bookP
    void   LocatePage   (uint8_t *keyP, uint32_t *bukP, uint32_t *pgP);         //output buk, and page for specified keyP
    cINDX *StepForward  (uint32_t *bukP, uint32_t *pgP, uint32_t *inxP, uint32_t inc);//Step location {buk, page, inx} by inc
    void   ReallocateBook(uint32_t buk);                                        //only used with dynamic allocation
    void EarlyCache     (uint32_t buk, uint32_t page, uint32_t rcdNum);         //Simulate early caching
    friend class cHelper;                                                       //
    friend class cGenerateVerilog;                                              //
    friend class cBugIndex;                                                     //
    friend class cBugSim;                                                       //
    friend class CompilerWrap;                                                    //
    friend class cEmulator;                                                     //
    friend class cAssert;                                                       //
   }; //class cSadram...
 
#ifdef _DEBUG                                                                   //
   #define Assert(expr, note) if (!(expr)) _Assert(#expr, note, __LINE__, __FILE__)//
   extern FILE *g_bugFileP;                                                     //
   void _Assert(const char *exprP, const char *noteP, int line, const char *fNameP);//
   #define BUG             if (g_bugFileP) fprintf(g_bugFileP,
   #define BUG1(f,a)       BUG "@mem "f" \t%s line %04d\n",a,        __FILE__,__LINE__);
   #define BUG2(f,a,b)     BUG "@mem "f" \t%s line %04d\n",a,b,      __FILE__,__LINE__);
   #define BUG3(f,a,b,c)   BUG "@mem "f" \t%s line %04d\n",a,b,c,    __FILE__,__LINE__);
   #define BUG4(f,a,b,c,d) BUG "@mem "f" \t%s line %04d\n",a,b,c, d, __FILE__,__LINE__);
   #define FREE(vv) {void *vP = vv; BUG2("0x%p: free %-12s\t",vP, #vv); free(vP); vv = NULL;}
#else                                                                           //
   #define Assert(a, b) assert(a)                                               //
   #define FREE(vP) {free(vP);}                                                 //
   #define BUG  /##/
   #define BUG1 /##/
   #define BUG2 /##/
   #define BUG3 /##/
   #define BUG4 /##/
#endif //_DEBUG...

#endif //SADRAM_H_INCLUDED...

//end of file...
