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
#include <samVersion.h>

#define XCHG(a,b,t)         {t = a; a = b; b = t;}                              //exchange a & b using temporary t
const int   checkE_bufSz=400;                                                   //

class cEmulator
   {public:                                                                     //
    int         m_errorCode;                                                    //
    private:                                                                    //
    uint8_t    *m_bramP;                                                        //
    int         m_bugLevel,    m_pgmSize,     m_curRow,     m_bramRows, m_dirn, //
                m_indxPerRow,  m_pagesPerRow, m_keySize,    m_clkCount,         //
                m_lineMapSize, m_grpsPerRow,  m_symbolSize, m_sourceSize,       //
                m_insPt,       m_holdLine,    m_holdCheckE, m_pc,       m_sp;   //
    CONDITION_CODES m_samStatus;                                                //
    sSYSTEM_STAT m_sysStat;                                                     //
    bool        m_stepR,   m_stepA,   m_singleStepB,                            //
                m_actualB, m_expectB, m_nuB[3];                                 //
    uint64_t    m_stak[32], samRegs[8], m_lastRegs[HOWMANY(samRegs)],m_holdReg0;//
    char       *m_microCodeP, *m_lineMapP, *m_symbolTblP, *m_sourceTextP,       //
                m_actual [checkE_bufSz], m_expect  [checkE_bufSz],              //
                m_hdr    [checkE_bufSz], m_body    [checkE_bufSz],              //buffers for CheckE formatting
                m_flags  [checkE_bufSz], m_tgtBuf  [checkE_bufSz],              //        "
                m_stakBuf[checkE_bufSz], m_printBuf[checkE_bufSz];              //        "
    const char *m_objNameOnlyP;                                                 //
    OPCODE     *m_pgmP;                                                         //
    cOpName    *m_opNameP;                                                      //
    public:                                                                     //
    int         m_repete;                                                       //
    cEmulator                  (int keySize, const char *objFileP,              //
                                             const char *srcfileP,int bugLevel);//
   ~cEmulator                  ();                                              //
    uint64_t    Arithmetic     (OPCODE op);                                     //
    void        Bugout         (CC fmtP,...);                                   //
    int         CheckE         (void);                                          //
    int         CompareKey     (const void *aV, const void *bV, int keySz);     //
    char       *ConditionNames (uint16_t cc);                                   //
    int         Configuration  (int pc, OPCODE op);                             //
    bool        DisplayableHex (CC keyP, char *outP);                           //
    int         ExecutePgm     (int state);                                     //
    void        FormatAsString (char *bufP, int bufSize, uint64_t u64);         //
    int         GetPc          (void) {return m_pc;}                            //
    int         GetSp          (void) {return m_sp;}                            //
    const char *GetTgtLabel    (int adr);                                       //
    const char *GetSourceLineP (int pc, int *lenP=NULL);                        //
    const char *GetLabelHere   (int pc);                                        //
    int         GetSourceLineNum(int pc, int *srcOffsetP=NULL);                 //
    void        InsertKey      (OPCODE op, int itemCnt, int itemSz, int locn, uint8_t *keyP);//
    int         OneOp          (int pc, OPCODE op);                             //
    int         OpBug          (int pc, OPCODE op);                             //
    int         OpPrint        (int pc, OPCODE op, int valu);                   //
    int         ReadTheFile    (CC nameOnlyP, CC extP, char **bufPP);           //
    int         RWfield        (int pc, OPCODE op);                             //
    bool        hSequencer     (void   *rowV,uint32_t rowCnt,   uint32_t itemSz,//array description inputs
                               uint8_t *keyP,uint32_t keyOffset,uint32_t keySz, //key description inputs
                               uint32_t *prevP,uint32_t *locnP);                //outputs
    int         SetBugLevel    (int lvl) {int old=m_bugLevel; if (lvl >= 0) m_bugLevel = lvl; return old;}
    int         ShowRow        (int style);                                     //
    int         UnhexMicrocode (char *codeP, int size);                         //
    public: //public access points                                              //
    uint64_t GetSamReg(int reg)                {return samRegs[reg & 7];}       //
    void     PutSamReg(int reg, uint64_t data) {samRegs[reg & 7] = data;}       //
                                                                                //
    uint64_t GetStack(int depth)               {return m_stak[depth];}          //
    void     PutStack(int depth, uint64_t data){m_stak[depth] = data;}          //
                                                                                //
    uint64_t GetSamStatus(void)                {return *(uint8_t*)&m_samStatus;}//
    void     PutSamStatus(uint8_t u8)          {*(uint8_t*)&m_samStatus = u8;}  //
    //Convert two hex digts to their binary value                               //
    uint32_t Hex2Bin        (CC pp)                                             //
       {char hex[3];                                                            //
        hex[0] = pp[0]; hex[1] = pp[1]; hex[2] = 0;                             //
        return (uint32_t)strtol(hex, NULL, 16);                                 //
       } //Hex2Bin(...                                                          //
   }; //cEmulator...

enum {EX_SS=0,          //0 Single step (opcode by opcode)
      EX_RUN,           //1 Run sam program at full speed
      EX_2BREAK,        //2 Run until next $bug opcode
      EX_2RET,          //3 Run up to next ret opcode
      EX_BY_LINE,       //4 Run until line number changes
     };
//end of file ...
