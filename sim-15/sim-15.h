#ifndef _COMMANDER_H_INCLUDED
#define _COMMANDER_H_INCLUDED
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <windows.h>
#include <string.h>
#include <malloc.h>
#include <crtdbg.h>
#include <fcntl.h>
#include <tchar.h>
#include <time.h>
#include <samParams.h>
#include "C3_preProcessor.h"
#include <hStructures.h>
#include <opName.h>

#define CC const char*
#define CV const void*
#define TF(a) ((a) ? "true " : "false")
#define SNPRINTF(buf) buf[sizeof(buf)-1] = 0; snprintf(buf, sizeof(buf)-1
#define BRAM_ADR_BITS 5

#pragma pack (push, 1)
typedef struct                                                                  //Basic 16-bit opcode
   {const char *nameP;                                                          //
    uint16_t    act;                                                            //
    int8_t      ctrl;                                                           //procesing routine#
   } OPCODE_TBL;

#define CAZE_ILLEGAL_OP   case 'A'
#define CAZE_TOP_OF_OP    case 'E'
#define CAZE_DUMP_ROW     case 'F'
#define CAZE_SEQ_GROUP    case 'G'

//States of state machine implemented in samControl.sv
typedef enum                                                                    //
   {SC_START = 0,                                                               //
    SC_IDLE  = 1,                                                               //
    SC_GETOP = 2,                                                               //
    SC_DO_OP = 3,                                                               //
    SC_SCAN  = 4,                                                               //
    SC_READ  = 5,                                                               //
    SC_READ1 = 6,                                                               //
    SC_XCHG  = 7,                                                               //
    SC_R2R   = 8,                                                               //
    SC_WRYT  = 9,                                                               //
    SC_WRYT1 =10,                                                               //
    SC_SCIN  =11,                                                               //
    SC_SCIN1 =12,                                                               //
    SC_ARITH1=13,                                                               //
    SC_XTOS  =14,                                                               //
    SC_NU    =15,                                                               //
    SC_RDF1  =16,                                                               //
    SC_RDF2  =17,                                                               //
    SC_RDF3  =18,                                                               //
    SC_WDF1  =19,                                                               //
   _SC_END   =99                                                                //
   } SC_CODES;                                                                  //

#pragma pack(pop)

extern class cSadram  *g_sP;
extern char           *g_failureP;
const int              TARGETBUS_SIZE=8, CELL_SZ=8;

#define Error(erC, context, param) _Error(erC,  context, param, __FILE__, __LINE__, __FUNCTION__)

#define ErrorN(erC, p1)            _ErrorN(erC, p1,             __FILE__, __LINE__, __FUNCTION__)
//Address of bit in bit vector; stored LSBit first
#define BIT_VECTOR(vector, bit)        (vector[(bit)/8] && (1 << ((bit) & 7)))

const int       checkE_bufSz=400;                                               //
class cSimDriver
   {public:                                                                     //
    bool            m_showLineB,  m_doneB,      m_errorB,   m_grpStoppedB,      //
                    m_noReadCheckB, m_expectB,  m_actualB,  m_singleStepB,      //
                    m_showAllB,   m_compileB,   m_skipXsimB,m_dryRunB,          //global options
                    m_expandOnlyB,m_bugMemB,    m_emulateB, m_patchDrvB,        //patch 'MICROCODE_SIZE' in samDefines.sv
                    m_doubleBookB,m_nu[3];                                      //using both cBOOK.pageH and cBOOK.pageL
    int             m_bUsed,      m_rowBytes,   m_errCount, m_targetBusSz,      //
                    m_curRow,     m_holdCheckE, m_insPt,    m_holdLine,         //
                    m_bugThisRow, m_insertPt,   m_keySize,  m_bitsInOffsetField,//
                    m_microCodeSz,m_testOps,    m_maxGroups,m_countWrites,      //
                    m_baseSz[4],  m_sizes[4],   m_align[4],                     //architectural parameters 
                                                m_padSz[4], m_perRow[4],        //   indexed by eITEM_TYPE
                    m_expectErr;                                                //
    HANDLE          m_childProcessHandle;                                       // 
    char           *m_lineMapP, *m_symbolTblP;                                  //m_messagesP is in OpName
    int             m_msgSize,  m_lineMapSize, m_symbolSize;                    //
    eENVIRONMENT    m_environment;                                              //
    char           *m_includeDirP, *m_srcFileNameP,                             //include directory, source file
                   *m_captureFileP;                                             //
    time_t          m_start;                                                    //
    char            c_err[250], *m_testRowP, *m_holdReg0P;                      //
    class cOpName  *m_opNameP;                                                  //
    int             ComputeGeometry(int keySize);                               //
    static char    *FullFileName(const char *nameP);                            //
    sPARAMS         m_params;                                                   //
    char           *m_lastRegsP[MAX_REGS], m_expect[128], m_actual[128];        //
    cSimDriver                  ();                                             //
    ~cSimDriver                 ();                                             //
    int             AddMessage  (const char *msgP, int len);                    //
    uint64_t        Anumber     (const char *pp, char **ppP=NULL, int *bitCountP=NULL);//
    int             BuildFileNames(void);                                       //
    int             SendCmd     (const char *cmdP);                             //
    char           *AppendKey   (char *bufP, int bufSize, const char *targetP, int keySize);
    char           *ConditionNames(uint16_t cc);                                //
    static char    *Endof       (char *srcP) {return srcP ? &srcP[strlen(srcP)] : NULL;}//
    static int     _Error       (int erC, CC contextP, CC paramsP, CC fileNameP, int line, const char *fncP);
    static int     _ErrorN      (int erC, uint32_t p1,             CC fileNameP, int line, const char *fncP); //
    void            FormatAsString(char *bufP, int bufSize, const char *regP);  //
    int             GetError    (void) {return m_errCount;}                     //
    BOOL            GetChildStatus(LPDWORD wdP) {return GetExitCodeProcess(m_childProcessHandle, wdP);}
    static int      Help        (const char *p1, const char *p2);               //
    int             ReadAllFile (const char *fileNameP, const char *suffixP, char **bufPP);
    int             StartSimulator(int keySize);                                //
    char           *strdupl     (const char *srcP, int len=-1);                 //
    bool            VerilogParam(const char *paramNameP, int *valP, bool paramB=true);
    private:                                                                    //
    class cCompile *m_compilerP;                                                //
    HANDLE          m_hPipeParentRcv;                                           //
    HANDLE          m_hPipeParentRcvError;                                      //
    HANDLE          m_hPipeChildRcv;                                            //
    bool            m_processing, m_compiling, m_withinAstep, m_cloning;        //
    uint8_t        *m_grpMaskBinP;                                              //binary of grpMask
    char           *m_samDefinesSrcP;                                           //
    const char     *m_itemName[4];                                              //
    const char     *m_testNamesP[2];                                            //
    char           *m_expectingP;                                               //for #expecting: #received :
    char            m_microcode[_MAX_PATH],   m_msgFile[_MAX_PATH],             //names of subordinate files
                    m_lineMap  [_MAX_PATH],   m_capFile[_MAX_PATH],             //            "
                    m_symbolTbl[_MAX_PATH];                                     //
    char            m_hdr    [checkE_bufSz],  m_body  [checkE_bufSz],           //buffers for CheckE formatting
                    m_flags  [checkE_bufSz],  m_tgtBuf[checkE_bufSz],           //        "
                    m_stakBuf[checkE_bufSz],  m_printBuf[checkE_bufSz];         //        "
    int             m_lastRowNum;                                               //
    int             Align2TargetBus(int posn, int itemBytes);                   //
    char            BreakoutLine(char *lineP, char **breakOutP, uint64_t *valP, bool *validP, int max);
    bool            BugThis     (unsigned char *ramP, int bramRows);
    bool            Check       (char *lineP);
    int             Check1      (char *lineP, char *fields[20], uint64_t *vP, bool *invalidP, int item); //from test1.sv
    int             CheckC      (char *lineP, char *fields[20], uint64_t *vP, bool *invalidP, int item);
    int             CheckG      (char *lineP, char *fields[20], uint64_t *vP, bool *invalidP, int item);
    int             CheckA      (char *lineP, char *fields[20], uint64_t *vP, bool *invalidP, int item);
    int             CheckE      (char *lineP, char *fields[20], uint64_t *vP, bool *invalidP, int item);
    int             CheckF      (char *lineP, char *fields[20], uint64_t *vP, bool *invalidP, int item);
    int             CheckI      (char *lineP, char *fields[20], uint64_t *vP, bool *invalidP, int item);
    int             CheckR      (char *lineP, char *fields[20], uint64_t *vP, bool *invalidP, int item);
    int             CheckK      (char *lineP, char *fields[20], uint64_t *vP, bool *invalidP, int item);
    int             CheckS      (char *lineP, char *fields[20], uint64_t *vP, bool *invalidP, int item);
    int             OpPrint     (int pc, OPCODE op, uint64_t *regsP, int bugLevel);
    const char      CmpName     (const char *codeP); 
    int             CompareKey  (const void *aV, const void *bV, int keySz);
    int             Compile     (const char *fileNameP);
    bool            DisplayableHex(const char *keyP, char *keyOutP);
    char           *FindMessage (int msgNum);
    int             FindMessage(const char *msgP, int len);
    char           *Fixctime    (time_t *nP);
    char           *FormatBin32 (uint32_t bin);
    char           *FormatKey   (char *keyP);
    char           *FmtGroupMask(const char *grpMaskP, int itemSize);
    uint64_t        GetParam    (char *lineP, const char *whatP);
    static int      HelpTests   (const char *p1);
    int             PrintRow    (int row);
    void            PrintRowRaw (int row);
    void            PrintRowBkwds(const char *titleP, const char *rowP);
    void            PrintLine   (const char *lineP, const char *msgP="\n");
    void            PrintGrpMask(const char *titleP);
    int             ReadParams  (sHDW_PARAMS *paramsP);
    void            RemoveSourceDir(char *lineP);
    char           *ReverseUnhex(char *keyP);
    char           *ReverseKey  (char *keyP);
    uint32_t        RoundupToTargetbus(uint32_t valu);
    char           *RowFromBlockramData(int rowNum);
    static DWORD WINAPI RcvFromChildThread     (LPVOID lpvThreadParam);
    static DWORD WINAPI RcvErrorFromChildThread(LPVOID lpvThreadParam);
    DWORD WINAPI    Receive     (HANDLE hPipe);
    DWORD WINAPI    ReceiveErr  (HANDLE hPipe);
    void            ShowCheckHdr(int pc, const char*nameP, uint64_t *vP);
    int             ShowNtError (const char *fncP, int line);
    const char     *StateName   (SC_CODES state);
    bool            strBegins   (const char *lineP, const char *whatP);
    void            LaunchChild (HANDLE hChildStdOut, HANDLE hChildStdIn, HANDLE hChildStdErr);
    bool            ValidateBram(uint8_t *bramP);
    int             WriteParamsData (void);
    char const     *GetHereLabel    (uint16_t);
    const char     *FileNameOnly    (int fileNum);
    char           *StripFileName   (char *fileNameP);
    char const     *GetSourceLineP  (int);
    int             GetSourceLineNum(int);
   public: int      CheckArchitecture(void);
    friend class cCompile; 
    friend class cEmulateMicrocode;
   }; //cSimDriver...

#endif// _COMMANDER_H_INCLUDED...
 