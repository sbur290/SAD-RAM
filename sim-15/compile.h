#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <fcntl.h>
#include "sim-15.h"
#include "simBug.h"
#include <C3_atomize.h>
#include <C3_errors.h>

#pragma pack(push, 1)
typedef struct {OPCODE  op;                                                 //samOp
                char   *tgtLabelP,                                          //target label of this op, eg goto fitz
                       *hereP;                                              //label of this op,        eg william: $bug=3
                int     lineNum, fileNum;                                   //back linke to source
                bool    isDwB, isBreakB, isContinueB,                       //
                        isLongJmpB, isCodeAdrB, isPcB;                      //labelP is located on this opcode
               } sCODE_BLOB;
#pragma pack(pop)
typedef struct {uint32_t lo; uint32_t hi;} sPAIR;
extern uint16_t HiLo16(uint16_t u16);
extern uint32_t HiLo32(uint32_t u32);
extern uint64_t HiLo64(uint64_t u64);
#define XCHG(a,b,t)         {t = a; a = b; b = t;}                          //exchange a & b using temporary t
inline char *LocalCopy(void *vP, IATOM aa) 
    {char *dP=(char*)vP; memmove(dP, aa.textP, aa.len); dP[aa.len] = 0; return dP;}
#define LOCAL_COPY(aa)  LocalCopy(alloca(aa.len+1), aa)                     //cleanup IATOM (watch side effects :)
#define PRECEDENCE_LEVELS  12                                               // levels of operators as defined in C++
#define MAX_LINE_LEN      512
//#define CUR_ROW             1
#define CC const char*
#define CV const void*

typedef struct
   {uint64_t wordAdr, row, nameLen;
    char    *nameP;
   } sVARIABLE;

class cCompile
  {public:                                                                   //
   cPreProcessor *m_prepP;                                                   //
   int            m_pgmSize, m_pgmAvail, m_lineNumber, m_fileNumber, m_post, //
                  m_rowOvr,  m_keySize,  m_loAdr,      m_hiAdr,              //
                 *m_nastiesP,m_nastyCnt;                                     //
   bool           m_bugEmitB, m_rowOvrB, m_unconditionalP, m_stepableB,      //
                  m_badReg0B, m_safeReg0B, m_safeReg0defaultB, m_nu[1];      //
   char           m_cleanSource[MAX_LINE_LEN+1];                             //
   OPCODE         m_max;                                                     //
   cSimDriver    *m_simP;                                                    //
   cAtomize      *m_az;                                                      //
   cCompile(cSimDriver *simP, CC srcFileP,   CC includeP,                    //input  files
             CC captureFileP, CC m_objcodeP, CC msgFileP, CC lineMapP,       //output files
             CC symbolTblP);
   ~cCompile();                                                              //
   int           CompileProgram(void);                                       //
   const char   *GetTgtLabel(uint32_t adr);                                  //
   const char   *GetHereLabel(uint32_t adr);                                 //
   private:                                                                  //
   int           m_longestLabel, m_loopDepth, m_lastCurRow,                  //
                 m_breaks, *m_breaksP, m_continues, *m_continuesP;           //list of break/continues in for, while, or do loops
   sCODE_BLOB   *m_codeP;                                                    //code emitted at each location
   IATOM         m_a;                                                        //
   const char   *m_objFileP, *m_msgFileP, *m_lineMapP, *m_symbolTblP;        //
   sVARIABLE    *m_vblsP;
   int           m_vblCount;
   int           Address       (int pc);                                     //
   int           AllocateBram  (void);                                       //
   uint64_t      Anumber       (CC pp, char **ppP=NULL, int *bitsP=NULL);    //
   int           AssignMem     (int pc, int vbl);                            //
   int           AssignReg     (int pc, IATOM aa);                           //
   void          Backup        (IATOM aa);                                   //
   int           BreakStmt     (int pc, int isBreak, char **labelPP);        //
   void          BugEmit       (int startPc, int endPc, CC commentP=NULL,    //
                                CC labelP=NULL, bool knownB=false);          //
   void          Bugout        (CC fmtP,...);                                //
   void          CheckCodeSize(int pc);                                      //
   int           CollapsePops  (int startPc, int pc);                        //
   int           CompileExpression(int pc, bool stdForB);                    //
   int           BuildLiteral  (int pc, bool curRowB, int dReg);             //
   int           CompoundStatement(int pc);                                  //
   int           Conditional   (int pc, bool *unconditionalP);               //
   int           ConvertTarget (char *dstP, IATOM aa);                       //
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
   int           InsertLongLiteral(int pc, uint64_t);                       //
   int           LongStringOrLiteral(int pc, const IATOM aa);                //
   int           MultiOp       (int pc, OPCODE baseOp);                      //
   bool          NoJumpsToHere (int pc);                                     //
   #define OPNAME(pc, labelP, knownB)                                        \
     m_simP->m_opNameP->Show(pc, m_codeP[pc].op, m_codeP[(pc)+1].op, labelP, knownB)  //
   int           PatchSamDefines(CC samDefinesNameP);                        //
   int           Precedence    (uint32_t op);                                //
   int           PrintStmt     (int pc, int caze);                           //
   int           PrintProgram  (void) {return m_az->PrintProgram();}         //
   int           Qualified     (bool cfgB);                                  //
   int           StatementList (int pc,char stopper=';',bool oneShotB=false);//
   int           ResolveGotos  (void);                                       //
   int           ResolveNasties(void);                                       //
   int           SetEnvironment(void);                                       //
   int           ShiftStmt     (int pc, uint8_t subOp);                      //
   int           SignedOffset  (OPCODE *opP);                                // 
   int           SimpleCondition(IATOM aa);                                  //
   int           SimplifiedForz(sCODE_BLOB *, int, sCODE_BLOB *, int);       //
   int           RegisterAssignment(int pc, IATOM aa, int act);              //
   int           RegisterPostOp(int pc, IATOM reg, IATOM postOp);            //
   void          StretchOp     (int pc);                                     //
   int           WhereisLabel  (const char *labelP);                         //
   int           WhileStmt     (int pc);                                     //
   friend class cEmulateMicrocode;
   friend class cSimDriver;
  }; //class cCompile...

//end of file
