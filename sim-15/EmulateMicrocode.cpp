#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <fcntl.h>
#include <crtdbg.h>
#include "sim-15.h"
#include "simBug.h"
#include "compile.h"
#include <hStructures.h>
#include "..\include-15\c3_errors.h"

extern char        *g_captureFileP; //capture program for simulator output

#define WOOLY_REGS                                                              \
            samRegs[0], samRegs[0] == m_lastRegs[0] ? ",  " : "*, ",            \
            samRegs[1], samRegs[1] == m_lastRegs[1] ? ",  " : "*, ",            \
            samRegs[2], samRegs[2] == m_lastRegs[2] ? ",  " : "*, ",            \
            samRegs[3], samRegs[3] == m_lastRegs[3] ? ",  " : "*, ",            \
            samRegs[4], samRegs[4] == m_lastRegs[4] ? ",  " : "*, ",            \
            samRegs[5], samRegs[5] == m_lastRegs[5] ? ",  " : "*, ",            \
            samRegs[6], samRegs[6] == m_lastRegs[6] ? ",  " : "*, ",            \
            samRegs[7], samRegs[7] == m_lastRegs[7] ? ",  " : "*, "

typedef struct
   {OPCODE op;
    char  *targetP;
   } PCODE;

typedef enum {SS_OK=0, SS_ILLEGAL_OP=1, SS_ILLEGAL_RET=2, SS_SHUTDOWN=3} SS_CTRL;

class cEmulateMicrocode
   {public:
    int         m_errorCode;
    private:
    cSimDriver *m_simP;
    uint8_t    *m_bramP;
    char       *m_microCodeP, *m_messagesP;
    int         m_keySize, m_opCount, m_bugLevel, m_currentRow;
    CONDITION_CODES m_samStatus;
    bool        m_stepR, m_stepA;
    uint64_t    m_stak[32];
    int         m_sp, m_dirn;
    uint64_t    samRegs[8], m_lastRegs[HOWMANY(samRegs)];
    PCODE      *m_pgmP;
    public:
    int         m_repeat;
    bool        m_singleStepB;
    cEmulateMicrocode   (cSimDriver *simP, int keySize, const char *objFileP, const char *msgFileP);
   ~cEmulateMicrocode   ();
    uint64_t Arithmetic (OPCODE op);
    void Bugout         (char const *fmtP,...);
    int  OneOp          (int pc);
    int  ExecutePgm     (void);
    const char *GetTgtLabel(int adr);
    void OpPrintf       (int count, uint64_t valu=0);
    int  UnhexMicrocode (char *codeP, int size);
    int  SingleStep     (int pc, SS_CTRL ctrl, const char *opNameP);
    void CheckArchitecture     (void);
    //Convert two hex digts to their binary value
    uint32_t Hex2Bin    (const char *pp)
       {char hex[3];
        hex[0] = pp[0]; hex[1] = pp[1]; hex[2] = 0;
        return (uint32_t)strtol(hex, NULL, 16);
       } //Hex2Bin(...
   }; //cEmulateMicrocode...

cEmulateMicrocode::cEmulateMicrocode(cSimDriver *simP, int keySize, const char *objFileP, const char *msgFileP)
   {int         erC;
    char       *simDirP=getenv("VerilogSimulationDir"), buf[75];
    const char *blokRam="blockRam.bin";                                         //
    cTIMEVALUE  tv;                                                             //
    m_simP       = simP;                                                        //
    m_bramP      = NULL;                                                        //
    m_microCodeP = NULL;                                                        //
    m_pgmP       = NULL;                                                        //
    m_messagesP  = NULL;                                                        //
    m_keySize    = keySize;                                                     //
    m_bugLevel   = 1; //outputs OP_PRINT messages                               //
    m_currentRow = 0;                                                           //
    m_repeat     = 0;                                                           //
    m_sp         = 0;                                                           //
    *(uint8_t*)&m_samStatus = 0;                                                //
    m_singleStepB= true;                                                        //
    samRegs[0]   = 10101;samRegs[1] = 0;    samRegs[2] = 222;  samRegs[3] = 333;//
    samRegs[4]   = 444;  samRegs[5] = 555;  samRegs[6] = 666;  samRegs[7] = 777;//
    memmove(m_lastRegs, samRegs, sizeof(m_lastRegs));                           //
    if ((erC=m_simP->ReadAllFile(blokRam, (char**)&m_bramP)) < 0) goto err;     //
    if ((erC=m_simP->ReadAllFile(msgFileP, &m_messagesP))    < 0) goto err;     //
    m_simP->m_messagesP = m_messagesP; m_simP->m_messageSize = erC;             //how unstructured can U get
    if ((erC=m_simP->ReadAllFile(objFileP, &m_microCodeP))   < 0) goto err;     //
    if ((erC=UnhexMicrocode(m_microCodeP, erC))              < 0) goto err;     //
    if (g_printFileP == NULL) g_printFileP = fopen(m_simP->m_captureFileP,"wb");//
    tv.GetGmtv(); tv.Format(buf, sizeof(buf), "%d/%m/%4y@%h:%m");               //
    fprintf(g_printFileP, "//File created %s.\n", buf);                         //
    erC = 0;                                                                    //
err:m_errorCode = erC;                                                          //
   } //cEmulateMicrocode::cEmulateMicrocode..

cEmulateMicrocode::~cEmulateMicrocode() 
   {free(m_bramP);      m_bramP      = NULL;
    free(m_microCodeP); m_microCodeP = NULL;   
    free(m_pgmP);       m_pgmP       = NULL;
    free(m_messagesP);  m_messagesP  = NULL; m_simP->m_messagesP = NULL;
   } //cEmulateMicrocode::~cEmulateMicrocode...

//Instruction format is:
//1D0C //008 wrBingo : OP_WRYT[29] ".4Vx...."   #10 source  //example format
//Isolate target and convert opcode (following '_') to binary in m_pgmP[]
int cEmulateMicrocode::UnhexMicrocode(char *codeP, int size)
   {char *pp; uint8_t *dP=(uint8_t*)codeP; int lines, cnt, pc=0;                //
                                                                                //
    //count lines in file - overestimate of size of m_pgmP; don't use strpbrk etc.
    for (pp=codeP, lines=1, cnt=0; cnt < size; cnt++, pp++)                     //
        if (*pp == '\r' && *pp == '\n') {lines++; pp++; cnt++;} else            //
        if (*pp == '\r' || *pp == '\n')  lines++;                               //
    m_pgmP = (PCODE*) calloc(lines, sizeof(PCODE));                             //
    for (pp=codeP, pc=0; pp && *(pp+=strspn(pp, " \t\r\n")) != 0;)              //
        {if (pp[0] == '/' && pp[1] == '/') {pp = strpbrk(pp, "\r\n"); continue;}//
         m_pgmP[pc  ].targetP = NULL;                                           //
         m_pgmP[pc++].op.u16 = (uint16_t)strtol(pp, NULL, 16);                  //
         pp = strpbrk(pp, "\r\n");                                              //skip to end of line
         if (pc > lines) {Bugout("Error %d: pc > lines", ERR_9995); exit(1);}   //
        }                                                                       //
    return m_opCount = pc;                                                      //
   } //cEmulateMicrocode::UnhexMicrocode...
    
const char *cEmulateMicrocode::GetTgtLabel(int adr)
   {return m_simP->m_compilerP->GetTgtLabel(adr);
   } //cEmulateMicrocode::GetTgtLabel...

int cEmulateMicrocode::OneOp(int pc)
   {PCODE       pgm = m_pgmP[pc], pgm1 = m_pgmP[pc+1];
    const char *goP  = "GO_FALSE";                                              //
    char       *tgtP = pgm.targetP;                                             //
    OPCODE      op   = pgm.op, nxt=pgm1.op;                                     //
    static int  random=0;                                                       //
    uint64_t   *u64P, tReg;                                                     //
    char       *opNameP, opName[125];                                           //
    int         adr  = op.s.act == OP_CALL ? op.call.adr : op.s.adr,            //
                sReg = op.r2r.sreg,                                             //
                dReg = op.r2r.dreg,                                             //
                num  = op.arith.nreg,                                           //
                cond = 0;                                                       //
    static const char *bug_sho[] = {"", "sho=PAGE", "sho=RAW", "sho=INDX"};     //
                                                                                //
    opNameP         = m_simP->OpName(pc, op, nxt, GetTgtLabel(adr), true);      //
    SNPRINTF(opName), "%03d: Executing %s (0x%04X)", pc, opNameP, op.u16);      //
    if (m_bugLevel >= 2)                                                        //
       {Bugout("%s", opName);                                                   //
        Bugout( ", stat=%s\n    [0] %016llX%s%016llX%s%016llX%s%016llX%s"       //
                         "\n    [4] %016llX%s%016llX%s%016llX%s%016llX%s\n",    //
                m_simP->ConditionNames(*(uint16_t*)&m_samStatus), WOOLY_REGS);  //
       }                                                                        //
     //Now emulate the opcode                                                   //
    if (m_singleStepB && op.u16 != OP_STOP)                                     //
       {if (SingleStep(pc, SS_OK, opName) < 0) return -1;}                      //
    //Now start interpretting the op -------------------------------------------//
    switch(op.s.act & 3)
       {case OP_LDI: samRegs[0]    = (samRegs[0] << 14) | op.ldi.imm;    return pc+1; //
        case OP_R2R: samRegs[dReg] = (sReg==dReg) ? adr : samRegs[sReg]; return pc+1;
       }                                                                        //
    switch (op.s.act)                                                           //
      {case OP_CALL: if (m_stak[m_sp-1] >= HOWMANY(m_stak)-1)                   //
                               return SingleStep(pc,SS_ILLEGAL_OP,opNameP);     //
                     m_stak[m_sp++] = pc;                                return adr;//
       case OP_RDF:  Bugout((op.s.adr & 1) ? "OP_WRF\n" : "OP_RDF\n");   return pc+1;//
       case 0x10:    switch(op.s.xreg)                                          //
                         {//se OP_CFG_C:                     return pc+1;       //
                          case OP_CFG_G/32:  if (op.u16 == OP_STOP)             //
                                                {if (m_bugLevel <= 1) return -1;//
                                                 return SingleStep(pc, SS_SHUTDOWN, opName);
                                                }                               //
                                             return pc+1;                       //
                          case OP_CROWI/32:  m_currentRow = op.s.adr; return pc+1;
                          case OP_RET/32:    if (m_stak[m_sp-1] >= HOWMANY(m_stak)) //
                                                return SingleStep(pc,SS_ILLEGAL_OP,opNameP);
                                             return (int)m_stak[--m_sp]+1;      //
                          case OP_REPEAT/32: m_stepA  = op.rpt.stepA != 0;      //
                                             m_stepR  = op.rpt.stepR != 0;      //
                                             m_dirn   = op.rpt.bkwd ? -1 : +1;  //
                                             m_repeat = op.rpt.count+2;         //
                                             return pc+1;                       //
                          case OP_BUG/32:    m_bugLevel      = op.bug.level & 7;//
                                             Bugout(bug_sho[op.bug.sho]);       //
                                             if (m_singleStepB)    return pc+1; //already single stepping
                                             if (op.bug.step == 0) return pc+1; //
                                             return SingleStep(pc,SS_OK,opName);//single step request
                          case OP_PRINT/32:  OpPrintf(op.s.adr); return pc+1;   //
                          case OP_CROW/32:   m_currentRow = (int)samRegs[op.ind.dataReg]; return pc+1;
                          default:           return SingleStep(pc, SS_ILLEGAL_OP, opName);
                         }                                                      //
       case OP_ARITH:if (IsExtendedOp(op))                                      //
                        {switch (op.arith.subOp)                                //
                           {case OPS_PUSH: m_stak[m_sp++]= samRegs[sReg]; break;//
                            case OPS_POP:  samRegs[sReg] = m_stak[--m_sp];break;//
                            case OPS_XCHG: XCHG(samRegs[sReg], samRegs[num], tReg); break;//
                            case OPS_XTOS: XCHG(m_stak[m_sp-1],samRegs[num], tReg); break;//
                            case OPS_STEP_SP:m_sp++;                      break;//
                            default: SingleStep(pc, SS_ILLEGAL_OP, opNameP);    // 
                        }  }                                                    //
                     else samRegs[op.arith.sreg] = Arithmetic(op);        break;//other arithmetic operations
       case OP_READ: u64P = (uint64_t*)&m_bramP[g_memRowSize * samRegs[2]];     //
                     samRegs[0] =  u64P[op.s.adr];                        break;//
       case OP_SCAN:                                                      break;//
       case OP_SCIN:                                                      break;//
       case OP_WRYT: u64P = (uint64_t*)&m_bramP[g_memRowSize * samRegs[2]];     //
                     u64P[op.s.adr] = samRegs[0];                         break;//
       case OP_GO_T: case OP_GO_T+8: case OP_GO_T+16: case OP_GO_T+24:          //go_t(0) is unconditional goto
       case OP_GO_F: case OP_GO_F+8: case OP_GO_F+16: case OP_GO_F+24:          //go_f(0) is noop
                     if (((*(uint8_t*)&m_samStatus & op.go.cond) == op.go.cond) //
                          == (op.go.act == OP_GO_T))                            //take  jump
                         return IsLongJmp(op) ? m_pgmP[pc+1].op.u16 : pc+adr+1; //
                     break;                                                     //spurn jump
       default: return SingleStep(pc, SS_ILLEGAL_OP, opNameP);                  // illegal opcode
      }                                                                         //
    return pc+1;                                                                //
   } //cEmulateMicrocode::OneOp...

inline int CarryAdd32(uint32_t a, uint32_t b, uint32_t c) 
   {return (((uint64_t)a + (uint64_t)b  + (uint64_t)c) & 0x100000000ULL) != 0;} //add with carry

inline int CarrySub32(uint32_t a, uint32_t b, uint32_t c) 
   {return (((uint64_t)a - (uint64_t)b  - (uint64_t)c) & 0x100000000ULL) != 0;} //subtract with borrow

uint64_t cEmulateMicrocode::Arithmetic(OPCODE op)
   {typedef struct {uint32_t lo, hi;} U64;                                      //
    uint32_t u0=0, num=op.arith.nreg;                                           //
    uint64_t mid64=0x100000000ULL, u64, hi64=mid64 << 31,                       //
             l64 = samRegs[0], r64 = samRegs[op.arith.sreg];                    //parameters to binary op
    U64      L64 = *(U64*)&l64, R64= *(U64*)&r64;                               //parameters as pairs of u32
    static   uint32_t ccc=0;                                                    //
                                                                                //
    m_samStatus.cc = 0; m_samStatus.nc = 1;                                     //
    switch (op.arith.subOp)                                                     //
        {case OPS_ADC: u0  = ccc;                                               //
         case OPS_ADD: u64 = l64 + r64 + u0;                                    //
                       ccc = CarryAdd32(L64.lo, R64.lo,u0+((u64 & mid64) != 0));//
                       ccc = CarryAdd32(L64.hi, R64.hi, ccc);                   //
                       m_samStatus.cc = ccc;        m_samStatus.cc = !ccc;      //
                       m_samStatus.zz = (u64 == 0); m_samStatus.nz = (u64 != 0);//
                       return u64;                                              //
         case OPS_CMP: ccc = 0;                     //fall thru                 //
         case OPS_SBB: u0  = ccc;                   //fall thru                 //
         case OPS_SUB: u64 = l64 - r64 - u0;                                    //
                       ccc = CarrySub32(L64.lo, R64.lo,u0+((u64 & mid64) != 0));//
                       ccc = CarrySub32(L64.hi, R64.hi, ccc);                   //
                       m_samStatus.cc = ccc;        m_samStatus.nc = !ccc;      //
                       m_samStatus.zz = (u64 == 0); m_samStatus.nz = (u64 != 0);//
                       return (op.arith.subOp == OPS_CMP) ? r64 : u64;          //ignores result of CMP
         case OPS_XOR: ccc = 0; u64 = l64 ^ r64;                 break;         //
         case OPS_OR:  ccc = 0; u64 = l64 | r64;                 break;         //
         case OPS_AND: ccc = 0; u64 = l64 & r64;                 break;         //
         case OPS_INC:          u64 = ++r64;                     break;         //
         case OPS_DEC:          u64 = --r64;                     break;         //
         case OPS_SHL: ccc =(l64 & hi64)!=0; l64 <<= 1; m_samStatus.cc = ccc; u64 = l64; break;
         case OPS_SHR: ccc = l64 & 1; l64 >>= 1;        m_samStatus.cc = ccc; u64 = l64; break;
         case OPS_RCL: u64 =(l64 & hi64)!=0; l64 = (l64 << 1) + ccc; ccc = (uint32_t)u64;
                                                        m_samStatus.cc = ccc; u64 = l64; break;
         case OPS_RCR: ccc = l64 &1; u64 = ccc ? hi64 : 0; l64 = (l64 >> 1) + u64;
                                                        m_samStatus.cc = ccc; u64 = l64; break;
         default:      Bugout("error %d: Illegal opcode", ERR_9995); exit(1);          //
        }                                                                       //
    m_samStatus.zz = (u64 == 0); m_samStatus.nz = (u64 != 0);                   //
    m_samStatus.nc = !m_samStatus.cc;                                           //
    return u64;                                                                 //
   } //cEmulateMicrocode::Arithmetic...

#if 0
void cEmulateMicrocode::OpPrintf(int count, uint64_t valu)
   {char buf[9], *bP=buf, buf1[32], *pp; int jj=(count+1) & ~1;                 //
    for (jj=(count+1) & ~1; count-- > 0; ) *bP++ = valu >> (7*(--jj)) & 0x7F;   //
    *bP = 0;                                                                    //
    //Is it $digit, eg. "f=$0 eh!". $digit guarranteed to be in one 64-bit word //
    if ((pp=strchr(buf, '$')) && pp[1] >= '0' && pp[1] < '0'+MAX_REGS)          //
       {*pp++ = 0; if (buf[0]) OutputDebugStringA(buf);                         //text up to %
        snprintf(buf1, sizeof(buf1)-10, "0x%llX", samRegs[*pp++ & 7]);          //
        strcat(buf1, pp);                                                       //text beyond $digit
        pp = buf1;                                                              //
       }                                                                        //
    else pp = buf;                                                              //
    OutputDebugStringA(pp);                                                     //
   } //cEmulateMicrocode::OpPrintf...
#else
void cEmulateMicrocode::OpPrintf(int msgNum, uint64_t nu)
   {char *msgP;                                                                 //
    for (msgP=m_simP->FindMessage(msgNum); msgP && *msgP; msgP++)               //
        {if (*msgP == '$') {Bugout("0x%llX", samRegs[7+(*(++msgP)&7)]);}        //
         else              {Bugout("%c", *msgP);}                               //
   }    } //cEmulateMicrocode::OpPrintf...
#endif

void cEmulateMicrocode::Bugout(char const *fmtP,...)
    {va_list arg;                                                               //
     char    buf[512];                                                          //
     if (m_bugLevel <= 2) return;                                               //sworn to silence
     va_start(arg, fmtP);                                                       //
     vsnprintf(buf, sizeof(buf)-1, fmtP, arg);                                  //
     va_end(arg);                                                               //
     buf[sizeof(buf)-1] = 0;                                                    //
     OutputDebugStringA(buf);
    } //cEmulateMicrocode::Bugout...

//pc == 0 is taken to mean the first lick through SingleStep and profers more advice.
//ctrl != SS_OK signals various shades of failure; only allows OK response and stops simulation.
int cEmulateMicrocode::SingleStep(int pc, SS_CTRL ctrl, const char *opNameP)
   {char        hdr[99], buf2[256], whatNow[_MAX_PATH+50];                      //
    int         response;                                                       //
    const char *fullHelpP= "Press:\tYes\tto single step mode hereafter\n"       //
                           "\tNo\tto continue at full speed\n"                  //
                           "\tCancel\tto terminate program\n";                  //
    if (ctrl != SS_OK)                                                          //
       {SNPRINTF(whatNow), "\nEmulation saved in %s", m_simP->m_captureFileP);} //
    else whatNow[0] = 0;                                                        //
                                                                                //
    snprintf(hdr, sizeof(hdr), "%s, sp=%d", opNameP, m_sp);                     //
    if (ctrl == SS_ILLEGAL_RET)                                                 //
       {SNPRINTF(buf2), "\tIllegal return address = 0x%llX", m_stak[m_sp-1]);   //
        response = MB_OK;                                                       //
       }                                                                        //
    else                                                                        //
    if (ctrl == SS_ILLEGAL_OP)                                                  //
       {SNPRINTF(buf2), "\tIllegal opcode: 0x%04X. Emulation terminated\n",     //
                  m_pgmP[pc].op.u16);                                           //
        response = MB_OK;                                                       //
       }                                                                        //
    else                                                                        //
    if (pc == 0)                                                                //
      {snprintf(buf2, sizeof(buf2), "%s", fullHelpP);                           //full help on first execution
        response = MB_YESNOCANCEL;                                              //
      }                                                                         //
    else                                                                        //
       {SNPRINTF(buf2), "\t\t\tregisters\n"                                     //
                        "[0] %016llX%s%016llX%s%016llX%s\n"                     //
                        "[3] %016llX%s%016llX%s%016llX%s\n"                     //
                        "[6] %016llX%s%016llX%s"                                //
                        "flags=%s\n%s%s\n", WOOLY_REGS,                         //
            m_simP->ConditionNames(*(uint8_t*)&m_samStatus),                    //
            ctrl == SS_OK ? "\n\t\tY=step, N=run, C=stop" : "",                 //ctrl != SS_OK means shutdown 
            whatNow);                                                           //
        response = (ctrl == SS_OK) ? MB_YESNOCANCEL : MB_OK;                    //	
        for (int ii=0; ii < MAX_REGS; ii++)  m_lastRegs[ii] = samRegs[ii];      //have seen registers - pay attention dumbo
       }                                                                        //
    hdr[sizeof(hdr)-1] = buf2[sizeof(buf2)-1] = 0;                              //safety belt
    switch(MessageBoxA(NULL, buf2, hdr, response))                              //
        {case IDNO:     m_singleStepB =false; return pc+1;                      //full speed mode
         case IDYES:    m_singleStepB = true; return pc+1;                      //continue single stepping
         default:       return -1;                                              //terminate emulation
        }                                                                       //
   } //cEmulateMicrocode::SingleStep...

void cEmulateMicrocode::CheckArchitecture(void)
   {OPCODE   op;
    uint64_t u64;
    struct {uint64_t l,r,a; uint8_t n, c, o, s; const char *t;} tbl[] =
        {{0x8000000000000000, 0x8000000000000000, 0x0000000000000000, 0, 1, OPS_ADD, 6, "+"}, //+0
         {0xFFFFFFFF80000000, 0x0000000080000000, 0x0000000000000000, 0, 1, OPS_ADD, 6, "+"}, //+1
         {0xFFFFFFFFFFFFFFFF, 0x0000000000000001, 0x0000000000000000, 0, 1, OPS_ADD, 6, "+"}, //+2
         {0xFFFFFFFFFFFFFFFF, 0x0000000000000055, 0x0000000000000054, 0, 1, OPS_ADD, 2, "+"}, //+3
         {0xFFFFFFFFFFFFFFFF, 0x0000000000000055, 0xFFFFFFFFFFFFFFAA, 0, 1, OPS_SUB, 0, "-"}, //+4
         {0x0000000000000055, 0x0000000000000055, 0x0000000000000000, 0, 1, OPS_SUB, 4, "-"}, //+5
         {0x0000000000000054, 0x0000000000000055, 0xFFFFFFFFFFFFFFFF, 0, 1, OPS_SUB, 2, "-"}, //+6
         {0x0000000000000055, 0x0000000000000055, 0x0000000000000055, 0, 1, OPS_CMP, 4, ">"}, //+7
         {0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0, 1, OPS_CMP, 4, ">"}, //+8
         {0xFFFFFFFFFFFFFFAA, 0x0000000000000076, 0xFFFFFFFFFFFFFFDC, 0, 1, OPS_XOR, 0, "^"}, //+9
         {0xFFFFFFFFFFFFFFAA, 0x0000000000000076, 0xFFFFFFFFFFFFFFFE, 0, 1, OPS_OR , 0, "|"}, //10
         {0xFFFFFFFFFFFFFFAA, 0x0000000000000076, 0x0000000000000022, 0, 1, OPS_AND, 0, "&"}, //11
         {0x5555555555555555, 0xAAAAAAAAAAAAAAAA, 0x0000000000000000, 0, 1, OPS_AND, 4, "&"}, //12
         {0xD555555555555555, 0x0000000000000001, 0xAAAAAAAAAAAAAAAA, 1, 1, OPS_SHL, 2,"<<"}, //13
         {0x5555555555555555, 0x0000000000000001, 0x2AAAAAAAAAAAAAAA, 1, 1, OPS_SHR, 2,">>"}, //14
         {0xD555555555555555, 0x0000000000000001, 0xAAAAAAAAAAAAAAAB, 1, 1, OPS_RCL, 2,"<["}, //15 carryIn = 1
         {0x5555555555555555, 0x0000000000000001, 0xAAAAAAAAAAAAAAAA, 1, 1, OPS_RCR, 2,">]"}};//16 carryIn = 1

    op.arith.sreg   = 1;
    op.arith.act    = OP_ARITH;

    for (int ii=0; ii < HOWMANY(tbl); ii++)
        {op.arith.subOp = tbl[ii].o;
         samRegs[0]     = tbl[ii].l;
         samRegs[1]     = tbl[ii].r;
         op.arith.nreg  = tbl[ii].n;
         u64            = Arithmetic(op); 
         if (u64 != tbl[ii].a || *(uint8_t*)&m_samStatus != tbl[ii].s)
             Bugout("%llX %s %llX = %llX, stat=%X\n", tbl[ii].l, tbl[ii].t, tbl[ii].r, u64, *(uint8_t*)&m_samStatus);
        }             
   } //cEmulateMicrocode::CheckArchitecture...

int cEmulateMicrocode::ExecutePgm(void)
   {int pc, nxt; PCODE pgm; bool singleB;                                       //
    m_samStatus.qrdy = 1;                                                       //have to do something <<<<
    for (pc=0; (nxt=OneOp(pc)) >= 0;)                                           //execute one opcode
        {singleB = m_singleStepB; m_singleStepB = false;                        //
         for (; m_repeat != 0; m_repeat--)                                      //op_repeat encountered
             {if ((nxt=OneOp(pc+1)) < 0) return 0;                              //execute next opcode m_repeat times
              if (m_stepA) pgm.op.s.adr += m_dirn;                              //      with increment of address
              if (m_stepR) pgm.op.s.xreg+= m_dirn;                              //                    and register
             }                                                                  //
         pc = nxt; m_singleStepB = singleB;                                     //back to single shot execution
        }                                                                       //
    return 0;                                                                   //
   } //EmulateMicrocode::ExecutePgm...

//Entry point
int EmulateMicrocode(cSimDriver *simP, int keySize, const char *codeFileP, const char *msgFileP)
   {cEmulateMicrocode em(simP, keySize, codeFileP, msgFileP);
    if (em.m_errorCode < 0) return em.m_errorCode;
  //em.CheckArchitecture();
    return em.ExecutePgm();
   } //EmulateMicrocode...

//end of file...

