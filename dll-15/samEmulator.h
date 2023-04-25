#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <fcntl.h>
#include <direct.h>
#include <crtdbg.h>
#include <hStructures.h>
#include <c3_errors.h>
#include <opName.h>
#include <hStructures.h>
#include <samVersion.h>
#pragma warning( disable:6053) //strings maybe not 0x00 terminated on snprintf
#pragma warning( disable:6387) //
#pragma warning( disable:6288) //  keep your stupid opinion to yourself
#pragma warning( disable:6011) //                "
#pragma warning( disable:6001) //                "

#define XCHG(a,b,t)         {t = a; a = b; b = t;}                              //exchange a & b using temporary t
const int   checkE_bufSz=400;                                                   //

#define HIGHLIGHT(pp) {strcpy(pp, "\\highlight1 "); pp += strlen(pp);}          //highlight following text
#define LOWLIGHT(pp)  {strcpy(pp, "\\highlight0 "); pp += strlen(pp);}          //revert to default coloring

//Commands to ExecutePgm(mode = ...
typedef enum 
     {EX_RUN=0,         // 0  Run sam program at full speed
      EX_BYOP,          // 1  Single step (opcode by opcode) 
      EX_2PC,           // 2  Run util $pc = stop-value
      EX_2REG0,         // 3  Run util $0 = stop-value
      EX_2REG1,         // 4  Run util $1 = stop-value
      EX_2REG2,         // 5  Run util $2 = stop-value
      EX_2REG3,         // 6  Run util $3 = stop-value
      EX_2REG4,         // 7  Run util $4 = stop-value
      EX_2REG5,         // 8  Run util $5 = stop-value
      EX_2REG6,         // 9  Run util $6 = stop-value
      EX_2REG7,         //10  Run util $7 = stop-value
      EX_2LINE=11,      //11  Run until lineNum == stop-value
      EX_2RET,          //12  Run up to next ret opcode
      EX_BYLINE,        //13  Run until line number changes
      EX_STEP_INTO,     //14  Step into subroutine 
      EX_2CALL          //15  Run to next call
     } EX_MODE;

class cEmulator
   {public:                                                                     //
    int         m_errorCode;                                                    //
    private:                                                                    //
    uint8_t    *m_bramP;                                                        //
    int         m_bugLevel,    m_pgmSize,     m_curRow,     m_bramRows,         //
                m_indxPerRow,  m_pagesPerRow, m_keySize,    m_clkCount,         //
                m_lineMapSize, m_grpsPerRow,  m_symbolSize, m_sourceSize,       //
                m_insPt,       m_holdLine,    m_holdCheckE, m_pc,     m_sp,     //
                m_curOvly,     m_lastExit,    m_expectedVersusActual, m_dirn,   //
                m_targetBusSz, m_rowBytes,    m_curLine,    m_rowFormat,        //
                m_oldLine,     m_retLevel,                                      //
                m_baseSz[4], m_sizes[4], m_align[4], m_padSz[4], m_perRow[4];   //architectural parameters [eITEM_TYPE]
    CONDITION_CODES m_samStatus;                                                //
    sSYSTEM_STAT m_sysStat;                                                     //
    bool        m_stepR,   m_stepA,   m_singleStepB, m_atLeastOneLineB,         //
                m_actualB, m_expectB, m_guiB,        m_nuB[1];                  //
    uint64_t    m_stak[32], samRegs[8], m_lastRegs[HOWMANY(samRegs)],m_holdReg0;//
    char       *m_microCodeP, *m_symbolTblP, *m_sourceTextP,                    //
                m_actual [checkE_bufSz], m_expect  [checkE_bufSz],              //
                m_hdr    [checkE_bufSz], m_body    [checkE_bufSz],              //buffers for CheckE formatting
                m_flags  [checkE_bufSz], m_tgtBuf  [checkE_bufSz],              //        "
                m_stakBuf[checkE_bufSz], m_printBuf[checkE_bufSz],              //        "
                m_savePrint[checkE_bufSz];                                      //        "
    sSRC_REF   *m_lineMapP;                                                     //
    OPCODE     *m_pgmP;                                                         //
    cOpName    *m_opNameP;                                                      //
    EX_MODE     m_saveMode;                                                     //
    public:                                                                     //
    static uint16_t m_breaks[16];                                               //static to allow access
    static int      m_maxBreaks;                                                //before class is created
    char       *m_objNameOnlyP, *m_samFilePrefixP;                              // 
    const char *m_compilerMsgP;                                                 //
    int         m_repete;                                                       //
    uint32_t    m_highlight;                                                    //which words to highlight in rowData
    cEmulator                  (int keySz, CC objFileP, CC srcFileP, int bugLvl, bool guiB);//
   ~cEmulator                  ();                                              //
    int         AdjustLineNum  (int line);                                      //
    uint64_t    Arithmetic     (OPCODE op);                                     //
    int         Align2TargetBus(int posn, int itemBytes);                       //
    int         Bugger         (const char *fmtP, int p1, int p2, int p3);      //
    void        Bugout         (CC fmtP,...);                                   //
    int         CheckE         (void);                                          //
    int         CheckF         (uint64_t row, int rowType);                     //
    int         CompareKey     (const void *aV, const void *bV, int keySz);     //
    int         ComputeGeometry(int keySize);                                   //
    char       *ConditionNames (uint16_t cc);                                   //
    int         Configuration  (int pc, OPCODE op);                             //
    int         DebugGui       (int locn, int line, const char *sP);            //
    bool        DisplayableHex (const uint8_t *key8P, char *outP);              //
    int         DoOpPrint      (int pc, OPCODE op, int valu);                   //
    int         Execute1       (int pc, OPCODE op);                             //
    int         ExecutePgm     (EX_MODE mode, uint64_t line=-1);                //
    static const char *FileNameOnly(const char *fullNameP);                     //
    void        FormatAsString (char *bufP, int bufSize, uint64_t u64);         //
    int         GetAssembler   (char *bufP, int height, int width);             //
    int         GetClkCount    (char *bufP);                                    //
    int         GetInsPoint    (char *bufP);                                    //
    int         GetExpectedVersusActual(char *bufP);                            //
    int         GetPc          (char *bufP);                                    //
    int         GetSp          (char *bufP);                                    //
    int         GetCurRow      (char *bufP);                                    //
    int         GetCurOvly     (char *bufP);                                    //
    int         GetFileName    (char *bufP);                                    //
    int         GetFormattedRow(char *bufP, bool keysOnlyB);                    //
    const char *GetLabel       (int pc);                                        //
    int         GetOpName      (char *buf1P, char *buf2P);                      //
    int         GetSamStatus   (char *buf1P, char *buf2P);                      //
    int         GetSourceAll   (char *bufP, int line, int bufSz,bool fromF9B);  //
    int         GetSourceAtPc  (char *bufP, int *lineP);                        //
    const char *GetSourceLineP (int pc, int *lineP=NULL, int *lenP=NULL);       //
    int         GetGuiB        (void) {return m_guiB;}                          //
    int         GetMessages    (char *bufP);                                    //
    const char *GetTgtLabel    (int adr);                                       //
    uint32_t    Hex2Bin        (CC pp);                                         //
    bool        hSequencer     (void   *rowV,uint32_t rowCnt,   uint32_t itemSz,//array description inputs
                               uint8_t *keyP,uint32_t keyOffset,uint32_t keySz, //key description inputs
                               uint32_t *prevP,uint32_t *locnP);                //outputs
    void        InsertKey      (OPCODE op, int itemCnt, int itemSz, int locn, uint8_t *keyP);//
    bool        Isalpha        (char ch);                                       //
    bool        IsExpectOp     (OPCODE op);                                     //
    int         OpBug          (int pc, OPCODE op);                             //
    int         ReadTheFile    (CC nameOnlyP, CC extP, void **bufPP);           //
    int         ReadTheFile    (CC nameOnlyP, CC extP, char **bufPP)            //
                          {return ReadTheFile(nameOnlyP, extP, (void **)bufPP);}//
    int         ReadTheFile    (CC nameOnlyP, CC extP, uint8_t **bufPP)         //
                          {return ReadTheFile(nameOnlyP, extP, (void **)bufPP);}//
    uint32_t    RoundupToTargetbus(uint32_t size);                              //
    char       *RtfHeading     (char *bufP);                                    //
    int         RWfield        (int pc, OPCODE op);                             //
    static int  SamCommandLine (const char **paramsPP,int count,char*fNameOutP);//
    int         SetBreakPoint  (const char *pgmP, int line);                    //
    int         SetBugLevel    (int lvl);                                       //
    int         SetCurRow      (int row);                                       //
    int         SetPc          (int pc);                                        //
    int         SetSp          (int sp);                                        //
    int         SetSamReg      (int reg, uint64_t u64);                         //
    int         SetRowFormat   (int fmt);                                       //
    int         ShowRow        (int style);                                     //
    bool        strBegins      (const char *strP, const char *whatP);           //
    void        TestDujour     (const char *srcP);                              //
    int         GetCompilerMsg (char *bufP);                                    //
    public:                                                                     //
    int         GetSamReg      (int reg,  char *bufP);                          //
    int         GetStack       (int depth,char *bufP);                          //
    int         LongLiteral    (int pc, uint64_t *resultP);                     //
    int         PutSamReg      (int reg, uint64_t data);                        //
    int         PutStack       (int depth, uint64_t data);                      //
    int         PutSamStatus   (uint8_t u8);                                    //
    static int  RestoreSettings(int *h, int *w, int *top, uint32_t*options, int*regN, uint64_t*regVal, uint16_t *breaksP=NULL);
    static int  SaveSettings   (int  h, int  w, int  top, uint32_t options, int regN, uint64_t regVal);
    int         StartLogging   (bool startB, char *fileNameP);                  //
    int         XlatPC2Line    (int pc, int *srcOffsetP=NULL);                  //
   }; //cEmulator...

//end of file ...
