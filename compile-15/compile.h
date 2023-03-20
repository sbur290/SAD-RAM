#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <fcntl.h>
#include <opName.h>
#include <C3_atomize.h>
#include <C3_errors.h>

#pragma pack(push, 1)
//External structure for opcode; represented as four hex digits in <program>.microcode.
typedef struct                                                                  //Basic 16-bit opcode
   {const char *nameP;                                                          //
    uint16_t    act;                                                            //
    int8_t      ctrl;                                                           //procesing routine#
   } OPCODE_TBL;

//Internal structure for emerging OPCODES
typedef struct {OPCODE   op;                                                    //samOp
                char    *tgtLabelP,                                             //target label of this op, eg goto fitz
                        *hereP;                                                 //label of this op,        eg william: $bug=3
                sSRC_REF ref;                                                   //file/line/srcOffset
                bool     isDwB,      isBreakB,   isContinueB,                   //
                         isLongJmpB, isCodeAdrB, isPcB;                         //labelP is located on this opcode
               } sCODE_BLOB;
#pragma pack(pop)

#define XCHG(a,b,t)         {t = a; a = b; b = t;}                          //exchange a & b using temporary t

inline char *LocalCopy(void *vP, IATOM aa) 
    {char *dP=(char*)vP; memmove(dP, aa.textP, aa.len); dP[aa.len] = 0; return dP;}
#define LOCAL_COPY(aa)  LocalCopy(alloca(aa.len+1), aa)                     //cleanup IATOM (watch side effects :)

#define PRECEDENCE_LEVELS  12                                               // levels of operators as defined in C++
#define MAX_LINE_LEN      512

#define CC const char*
#define CV const void*

#define Error(erC, context, param) _Error(erC,  context, param, __FILE__, __LINE__, __FUNCTION__)
#define ErrorN(erC, p1)            _ErrorN(erC, p1,             __FILE__, __LINE__, __FUNCTION__)

//Variables declared in allocate statement.
typedef struct
   {uint64_t wordAdr, row, nameLen;
    char    *nameP;
   } sVARIABLE;

#define CC const char*
#define CV const void*
#define TF(a) ((a) ? "true " : "false")
#define SNPRINTF(buf) buf[sizeof(buf)-1] = 0; snprintf(buf, sizeof(buf)-1
#define BRAM_ADR_BITS 5
const int TARGETBUS_SIZE=8, CELL_SZ=8;

class cCompile
  {public:                                                                   //
   cPreProcessor *m_prepP;                                                   //
   int            m_pgmSize,   m_pgmAvail, m_loAdr,          m_hiAdr,        //
                  m_errCount, *m_nastiesP, m_nastyCnt,       m_rowOvr;       //
   bool           m_bugEmitB,  m_rowOvrB,  m_unconditionalP, m_stepRegB,     //
                  m_badReg0B,  m_safeRegB, m_safeRegDefaultB,m_patchDrvB,    //
                  m_alwaysMessageBoxB,     m_nuB[3];                         //
   char           m_cleanSource[MAX_LINE_LEN+1];                             //
   sSRC_REF       m_ref;
   OPCODE         m_max;                                                     //max field sizes
   cAtomize      *m_azP;                                                     //
   cCompile                    (CC srcNameP, CC incDirP, bool bugEmitB, bool patchDrvB);   //input file & dir
   ~cCompile                   ();                                           //
   int           CompileProgram(void);                                       //
   int           PrintProgram  (void) {return m_azP->PrintProgram();}        //
   const char   *GetTgtLabel   (uint32_t adr);                               //
   const char   *GetHereLabel  (uint32_t adr);                               //
   static int   _Error         (int erC, CC contextP, CC paramsP, CC fileNameP, int line, const char *fncP);
   static int   _ErrorN        (int erC, uint32_t p1,             CC fileNameP, int line, const char *fncP); //
   int           GetError      (void) {return m_errCount;}                   //
   private:                                                                  //
   eENVIRONMENT  m_environment;                                              //
   int           m_longestLabel, m_loopDepth, m_lastCurRow,                  //
                 m_breaks, *m_breaksP, m_continues, *m_continuesP;           //list of break/continues in for, while, or do loops
   sCODE_BLOB   *m_codeP;                                                    //code emitted at each location
   IATOM         m_a;                                                        //
   const char   *m_objFileP, *m_msgFileP, *m_lineMapP, *m_symbolTblP, *m_srcFileNameP;        //
   char          m_microcode[_MAX_PATH],   m_msgFile[_MAX_PATH],             //names of subordinate files
                 m_lineMap  [_MAX_PATH],   m_capFile[_MAX_PATH],             //            "
                 m_symbolTbl[_MAX_PATH];                                     //
   sVARIABLE    *m_vblsP;
   int           m_vblCount;
   class cOpName *m_opNameP;                                                 //
   int           Address       (int pc);                                     //
   int           AllocateBram  (void);                                       //
   uint64_t      Anumber       (CC pp, char **ppP=NULL, int *bitsP=NULL);    //
   int           Arithmetic    (int pc, OPCODE_TBL *otP);                    //
   int           AssignMem     (int pc, int vbl);                            //
   int           AssignReg     (int pc, IATOM aa);                           //
   void          Backup        (IATOM aa);                                   //
   int           BreakStmt     (int pc, int isBreak, char **labelPP);        //
   void          BugEmit       (int startPc, int endPc, CC commentP=NULL,    //
                                CC labelP=NULL, bool knownB=false);          //
   void          Bugout        (CC fmtP,...);                                //
   int           BuildFileNames(char *srcFileNameP);                         //
   void          CheckCodeSize(int pc);                                      //
   int           CollapsePops  (int startPc, int pc);                        //
   int           CompileExpression(int pc, bool stdForB);                    //
   int           BuildLiteral  (int pc, bool curRowB, int dReg);             //
   int           CompoundStatement(int pc);                                  //
   int           Conditional   (int pc, bool *unconditionalP);               //
   void          CopySource    (char *bufP, int bufSize);                    // 
   int           ConstantExpression(int64_t *resultP);                       //
   int           DoStmt        (int pc);                                     //
   int           DwStmt        (int pc);                                     //
   void          EmitOp        (int pc, uint16_t op, uint32_t tgtPc, bool knownB=false);//
   void          EmitOp        (int pc, OPCODE op,   uint32_t tgtPc, bool knownB=false);//
   int          _ErrorA        (int erC, IATOM aa, CC fileP, int line, CC fncP);
   int           EvaluateExpr  (IATOM *listP, int min, int max, int64_t *rP);//
   int           Evaluate      (int64_t *resultP, int op, int64_t left, int64_t rite);
   int           EvaluateString(int op, char *leftP, char *riteP);           //
   int           FieldName     (IATOM aa, int fieldNum=-1, CC *namePP=NULL); //
   const char   *FileNameOnly  (CC pp);                                      //
   const char   *FileNameOnly  (int fileNum);                                //
   void          FixGoto       (int pc, uint32_t target, CC commentP=NULL);  //
   int           FixThese      (int pc, bool isBreakB, int *prevP, int pCnt);//
   int           ForStmt       (int pc, IATOM forz);                         //
   int           GenerateOp    (int pc, int op, int subOp=0, int sreg=0, int dreg=0);
   int           GenerateMicrocodeFile (CC objNameP);                        //
   int           GenerateMsgFile(CC msgNameP);                               //
   int           GenerateLinemapFile(CC mapFileP);                           //
   int           GenerateSymbolTable(CC symbolTableFileP);                   //
   char         *GetContext    (void);                                       //
   const char   *GetSourceLineP(sCODE_BLOB *blobP);                          //
   const char   *GetSourceLineP(int pc);                                     //
   int           GetSourceLineNum(int pc);                                   //
   IATOM         Get           (bool probeB=false);                          //
   uint16_t HiLo16(uint16_t u16) {return ((u16 >> 8) & 0xFF) + ((u16 << 8) & 0xFF00);}
   uint32_t HiLo32(uint32_t u32) {return (u32 >> 24) + ((u32 >> 8) & 0xFF00) + ((u32 << 8) & 0xFF0000) + ((u32 <<24) & 0xFF000000);}
   uint64_t HiLo64(uint64_t u64) {return (((uint64_t)HiLo32((uint32_t)u64)) << 32) + HiLo32((uint32_t)(u64 >> 32));}
   int           iabs          (int a) {return (a < 0) ? -a : a;}            //
   void          InvertJmp     (int pc);                                     //
   bool          Is            (char ch);                                    //
   bool          Is            (CC wordP);                                   //
   bool          IsChar        (char ch);                                    //
   bool          IsNoCase      (CC wordP);                                   //
   bool          IsRegister    (IATOM aa, int *regP=NULL);                   //
   int           IfStmt        (int pc);                                     //
   bool          IsWord        (CC wordP);                                   //
   bool          IsWordNoCase  (const char *wordP);                          //
   int           IntegerLiteral(int pc, uint64_t u64, int regN);             //
   bool          IsNumber      (uint32_t *valP);                             //
   int           LookupVbl     (const char *nameP, int nameLen);             //
   int           InsertLongLiteral(int pc, uint64_t);                        //
   int           LongStringOrLiteral(int pc, const IATOM aa);                //
   int           MultiOp       (int pc, OPCODE baseOp);                      //
   bool          NoJumpsToHere (int pc);                                     //
   #define OPNAME(pc, labelP, knownB)                                        \
     m_opNameP->Show(pc, m_codeP[pc].op, m_codeP[(pc)+1].op, labelP, knownB) //
   int           OpCfg         (int pc);                                     //
   int           OpScan        (int pc);                                     //
   int           PatchSamDefines(CC samDefinesNameP);                        //
   int           Precedence    (uint32_t op);                                //
   int           PrintStmt     (int pc, int caze);                           //
   int           StatementList (int pc,char stopper=';',bool oneShotB=false);//
   int           ResolveGotos  (void);                                       //
   int           ResolveNasties(void);                                       //
   int           SafeReg       (int pc, int pushPop, int reg);               //
   int           SetEnvironment(void);                                       //
   int           ShiftStmt     (int pc, uint8_t subOp);                      //
   int           SignedOffset  (OPCODE *opP);                                // 
   int           SimpleCondition(IATOM aa);                                  //
   int           SimplifiedForz(sCODE_BLOB *, int, sCODE_BLOB *, int);       //
   char         *strdupl(const char *srcP, int len=-1);                      //
   int           RegAssign     (int pc, IATOM aa, int act);                  //
   int           RegisterPostOp(int pc, IATOM reg, IATOM postOp);            //
   void          StretchOp     (int pc);                                     //
   int           WhereisLabel  (const char *labelP);                         //
   int           WhileStmt     (int pc);                                     //
   friend class cEmulateMicrocode;
   friend class CompilerWrap;
  }; //class cCompile...

//end of file
