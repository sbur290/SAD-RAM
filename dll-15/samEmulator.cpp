#include "samEmulator.h"

#define TARGETBUS_SIZE 8                                                        //sim.cpp compatibility
#define ITEM_PAGE      2                                                        //        "
#define ITEM_INDX      1                                                        //        "
#define SNPRINTF(buf) buf[sizeof(buf)-1] = 0; snprintf(buf, sizeof(buf)-1
#define CAT_THIS(buf) len = istrlen(buf); buf[sizeof(buf)-1] = 0; snprintf(&buf[len], sizeof(buf)-1-len

FILE        *g_printFileP=NULL;                                                 //for Printf
int          g_memRowSize=256;                                                  //
cSamError    g_err;                                                             //

typedef enum {SS_OK=0, SS_ILLEGAL_OP=1, SS_ILLEGAL_RET=2, SS_SHUTDOWN=3} SS_CTRL;

int Error(int erC, const char *contextP, const char *paramP) 
   {printf("\x07**** Error %d: %s\n", erC < 0 ? -erC : erC, paramP); 
    return erC < 0 ? erC : -erC;
   }

cEmulator::cEmulator(int keySize, const char *objFileP, const char *srcFileP, int bugLevel)
   {int         erC, ii=0;                                                      //
    char       *simDirP=getenv("VerilogSimulationDir"), buf[_MAX_PATH], *messagesP;//
    const char *blokRam="blockRam.bin";                                         //
    m_singleStepB= false;                                                       // 
    m_bugLevel   = bugLevel;                                                    //
    m_bramP      = NULL;                                                        //
    m_microCodeP = NULL;                                                        //
    m_pgmP       = NULL;                                                        //
    m_keySize    = keySize;                                                     //
    m_curRow     = 0;                                                           //
    m_repete     = 0;                                                           //
    m_clkCount   = 0;                                                           //
    m_sp         = 0;                                                           //
    m_pc         = 0;                                                           //
    m_actualB    = m_expectB = false;                                           //
    *(uint8_t*)&m_samStatus = 0;                                                //
    samRegs[0]   = 10101; samRegs[1] = 0x19;samRegs[2] = 222; samRegs[3] = 333; //initialize $registers
    samRegs[4]   = 444;   samRegs[5] = 555; samRegs[6] = 666; samRegs[7] = 777; // (arbitrary values)
    memmove(m_lastRegs, samRegs, sizeof(m_lastRegs));                           //
    ii           = sizeof(hINDX) + m_keySize + TARGETBUS_SIZE-1;                //
    ii           = (ii / TARGETBUS_SIZE) * TARGETBUS_SIZE;                      //round to targetbus size
    m_indxPerRow = g_memRowSize / ii;                                           //calculate hINDX/row
    ii           = sizeof(hPAGE) + m_keySize + TARGETBUS_SIZE-1;                //
    ii           = (ii / TARGETBUS_SIZE) * TARGETBUS_SIZE;                      //round to targetbus size
    m_pagesPerRow= g_memRowSize / ii;                                           //calculate hPAGE/row
    if ((erC=ReadTheFile(blokRam, "",         (char**)&m_bramP)) < 0) goto err; //read blockRAM (sample blockRAM prepared by genVerilog)
    m_bramRows   = erC / g_memRowSize;                                          //size in rows
    m_grpsPerRow = g_memRowSize / TARGETBUS_SIZE;                               //groups per row
    m_objNameOnlyP = objFileP;                                                  //
    if ((erC=ReadTheFile(objFileP, ".microcode", &m_microCodeP)) < 0) goto err; //
    if ((erC=UnhexMicrocode(m_microCodeP, erC))                  < 0) goto err; //read microcode
    SNPRINTF(buf), "%s\\%s.capFile", simDirP, objFileP);                        //
    g_printFileP = fopen(buf, "wb");                                            //
                                                                                //
    if ((erC=ReadTheFile(objFileP, ".msgFile", &messagesP))      < 0) goto err; //read msgFile
    m_opNameP    = new cOpName(&g_err, messagesP, erC);                         //
    if ((erC=ReadTheFile(objFileP, ".lineMap",   &m_lineMapP))   < 0) goto err; //read lineMap
    m_lineMapSize = erC;                                                        //
    if ((erC=ReadTheFile(objFileP, ".symbolTbl", &m_symbolTblP)) < 0) goto err; //read lineMap
    m_symbolSize = erC;                                                         //
    if ((erC=ReadTheFile(srcFileP, NULL, &m_sourceTextP))        < 0) goto err; //read source
    m_sourceSize = erC;                                                         //
    Printf("##Start Emulation; rev=%d, program=%s\n", SAM_VERSION, m_objNameOnlyP);//
    Printf("##Params: keySize=%2d, rowSize=%3d\n", m_keySize, g_memRowSize);    //
    erC = 0;                                                                    //
err:m_errorCode = erC;                                                          //
   } //cEmulator::cEmulator...

cEmulator::~cEmulator() 
   {free(m_bramP);       m_bramP       = NULL;
    free(m_microCodeP);  m_microCodeP  = NULL;   
    free(m_pgmP);        m_pgmP        = NULL;
    delete m_opNameP;    m_opNameP     = NULL;
    free(m_lineMapP);    m_lineMapP    = NULL;
    free(m_symbolTblP);  m_symbolTblP  = NULL;
    free(m_sourceTextP); m_sourceTextP = NULL;
   } //cEmulator::~cEmulator...

//Read all file into memory at *bufPP; return size or zero if unable to open file
int cEmulator::ReadTheFile(const char *fileOnlyP, const char *extensionP, char **bufPP)
   {size_t sz;                                                                  //
    char  *bufP, fn[_MAX_PATH];                                                 //
    FILE  *fileP;                                                               //
    if (extensionP != NULL)                                                     //
        snprintf(fn, sizeof(fn), "%s\\%s%s",                                    //
                        getenv("VerilogSimulationDir"), fileOnlyP, extensionP); //
    else strncpy(fn, fileOnlyP, sizeof(fn));                                    //
    fileP = fopen(fn, "rb");                                                    //
    if (!fileP) return Error(ERR_0003, "", fn);                                 //Failed to open
    fseek(fileP, 0, SEEK_END);                                                  //
    sz   = ftell(fileP);                                                        //
    bufP = (char*)malloc(sz+2);                                                 //room for \nand 0x00
    fseek(fileP, 0, SEEK_SET);                                                  //
    fread(bufP, sz,1, fileP);                                                   //read entire file into memory
    bufP[sz] = 0;                                                               //
    fclose(fileP);                                                              //
    *bufPP   = bufP;                                                            //
    return (int)sz;                                                             //
   } //cEmulator::ReadTheFile...

//Instruction format is:
//1D0C //008 wrBingo : OP_WRYT[29] ".4Vx...."   #10 source  //example format
//Isolate target and convert opcode (following '_') to binary in m_pgmP[]
int cEmulator::UnhexMicrocode(char *codeP, int size)
   {char *pp; uint8_t *dP=(uint8_t*)codeP; int lines, cnt, pc=0;                //
                                                                                //
    //count lines in file - overestimate of size of m_pgmP; don't use strpbrk etc.
    for (pp=codeP, lines=1, cnt=0; cnt < size; cnt++, pp++)                     //
        if (*pp == '\r' && *pp == '\n') {lines++; pp++; cnt++;} else            //
        if (*pp == '\r' || *pp == '\n')  lines++;                               //
    m_pgmP = (OPCODE*) calloc(lines, sizeof(OPCODE));                           //
    for (pp=codeP, pc=0; pp && *(pp+=strspn(pp, " \t\r\n")) != 0;)              //
        {if (pp[0] == '/' && pp[1] == '/') {pp = strpbrk(pp, "\r\n"); continue;}//
         m_pgmP[pc++].u16 = (uint16_t)strtol(pp, NULL, 16);                     //
         pp = strpbrk(pp, "\r\n");                                              //skip to end of line
         if (pc > lines) {Bugout("Error %d: pc > lines", ERR_9995); exit(1);}   //
        }                                                                       //
    return m_pgmSize = pc;                                                      //
   } //cEmulator::UnhexMicrocode...

const char *cEmulator::GetSourceLineP(int pc, int *lenP)   
    {int srcOffset, len, ii; bool quoteB; char *srcP, *pp;
     if (GetSourceLineNum(pc, &srcOffset) == 0) 
        {if (lenP) *lenP = 0; return "";}
     srcP = &m_sourceTextP[srcOffset];
     len  = (int)(strpbrk(srcP, "\r\n") - srcP);
     for (pp=srcP, ii=0, quoteB=false; len-->= 0; pp++, ii++)
         {if (*pp == '\"') quoteB = !quoteB; 
          if (*pp == ';' && !quoteB) {len = ii+1; break;}
         }
     if (lenP != NULL) *lenP = len;
     return srcP;
    } //cEmulator::GetSourceLineP...

const char *cEmulator::GetLabelHere(int adr)     
    {char *pp, *qq;
     for (pp=m_symbolTblP; (int)(pp-m_symbolTblP) < m_symbolSize; pp++)
         {pp += strlen(qq=pp);
          if (strtol(pp+3, &pp, 16) == adr) return qq;
         }
     return NULL;
    } //cEmulator::GetLabelHere.. 

int cEmulator::GetSourceLineNum(int pc, int *srcOffsetP) 
    {sSRC_REF *mapP; const char *lP, *endLP=&m_lineMapP[m_lineMapSize];
     for (lP=m_lineMapP; lP < endLP; lP += sizeof(sSRC_REF))
         {mapP = (sSRC_REF*)lP;
          if (mapP->pc >= pc) 
             {if (srcOffsetP) *srcOffsetP = mapP->srcOffset; return mapP->lineNum;}
         }
     return 0;
    } //cEmulator::GetSourceLineNum...

//CheckE messages are generated at beginning of op: 
//Straightforward code, except for special handling of $0 = literal, LDI, LDI,...
//These are compacted down to a single $0 = long literal for user convenience.
int cEmulator::CheckE(void)
   {static int   stopPlz= 2, lastLine=-1, lastii=IDNO;                          //response if singleStep = false
    OPCODE       op, nxt;                                                       //current OP
    int          pc        = m_pc,                                              //program counter
                 sp        = m_sp,                                              //stack pointer
                 bugLvl    = m_bugLevel,                                        //
                 curRow    = m_curRow,                                          //
                 lineNum   = GetSourceLineNum(pc),                              //
                 response  = MB_OK, len, ii, adr,                               //
                 srcLen;                                                        //
    uint8_t      samStat   = *(uint8_t*)&m_samStatus;                           //
    uint64_t     reg0      = samRegs[0];                                        //
    const char  *srcP, *hereP=NULL, *thereP=NULL, *qq;                          //
    static const char *adviceP="\n\n\t\tY=step, N=run, C=stop";                 //
    #define catHdr  CAT_THIS(m_hdr)                                             //append to hdr
    #define catStak CAT_THIS(m_stakBuf)                                         //append to stak

    if (m_pc == -7)
      m_pc = m_pc;
    m_hdr[0] = m_stakBuf[0] = m_flags[0] = 0;                                   //zero the text buffers
    op       = m_pgmP[pc];                                                      //current OP
    nxt      = m_pgmP[pc+1];                                                    //next word following opcode
    if (op.shortOp == OP_BUG && op.bug.set == 1 && op.bug.sho == 0) bugLvl = op.bug.level;//bug is coming up
    if (bugLvl <  3) goto printOnly;                                            //otherwise interpret opcode
    adr    = IsLongJmp(op) ? nxt.u16             :                              //long jump is abs adr in next op
             IsGoOp(op)    ? op.go.relAdr + pc+1 :                              //short jump is relAdr
             IsCall(op)    ? op.call.callAdr     : -1;                          //absolute address
    hereP  = GetLabelHere(pc);                                                  //
    thereP = GetLabelHere(adr);                                                 //
//print source line                                                             //
    if (lineNum != lastLine)                                                    //
       {Printf("\n---- line %04d: \"", lastLine=lineNum);                       //
        srcP = GetSourceLineP(pc, &srcLen);                                     //
        for (qq=&srcP[srcLen]; srcP < qq; srcP++)                               //
            {Printf("%c", *srcP == '\"' ? '.' : *srcP);                         //output source one char at a time
             srcP += strspn(srcP, " ");                                         //compress out multiple spaces  
            }                                                                   //
        Printf("\"\n");                                                         //
       }                                                                        //
    catHdr, "%03d:0x%04X ", pc, op.u16);                                        //
    if (hereP) {catHdr, "%s:", hereP);}                                         //
    catHdr, " %s, line %d", m_opNameP->Show(pc, op, nxt, thereP, true), lineNum);//
                                                                                //
    SNPRINTF(m_flags),                                                          //
                "samStat=(0x%X, %s), clkCount=%04d, curRow=%d, insPt=%d",       //
                 samStat, ConditionNames(samStat), m_clkCount, curRow, m_insPt);//
    if (sp != 0)                                                                //
       {catStak, "(stak, sp=%d) ", sp);                                         //
        for (ii=0; ii < min(sp, 4); ii++)                                       //
            {catStak, "%s0x%016llX, ", ii == 2 ? "\n  " : "", m_stak[ii]);      //
       }    }                                                                   //
                                                                                //
    //Echo above information to log file                                        //
    Printf("%s, %s\n", m_hdr, m_flags);                                         // 
                                                                                //
    //Output samRegs noting changed values                                      //
    for (ii=0; ii < MAX_REGS; ii++)                                             //
        {Printf("%s0x%016llX", (ii & 3) == 0 ? "  " : "", samRegs[ii]);//     
         Printf("%s", samRegs[ii] == m_lastRegs[ii]  ? ",  " : "*, ");          //
         if ((ii & 3) == 3) Printf("\n");                                       //
         m_lastRegs[ii] = samRegs[ii];                                          //
        }                                                                       //
    if (sp != 0) Printf("     %s\n", m_stakBuf);                                //
                                                                                //
printOnly:                                                                      //
    if (op.g.act == OP_PRINT)                                                   //
        {if ((ii=OpPrint(pc, op, bugLvl)) < 0)                                  //-ve means m_expect != m_actual
           {                                                                    //
            if (bugLvl < 3)                                                     //
               {Printf("%03d: $0<=0x%llX", pc-m_holdCheckE, reg0);              //reconstruct display
                FormatAsString(m_hdr, sizeof(m_hdr)-3, samRegs[0]);             //decorative touch
                Printf("%s line number=%d", m_hdr, lineNum);                    //
               }                                                                //
            printf("\x7");                                                      //
            m_singleStepB = true;                                               //force messagebox
           }                                                                    //
        else if (bugLvl < 3)                      return 0;                     //
       } //OP_PRINT...                                                          //
    return 0;                                                                   //
#undef catHdr
#undef catStak
   } //cEmulator::CheckE...

char *cEmulator::ConditionNames(uint16_t cc)
   {static char buf[15];
    buf[0] = 0;
    if ((cc & COND_QRDY) != 0)        {strcpy(buf, "qrdy "); cc &= ~COND_QRDY;}	
    if ((cc & COND_FULL) == COND_FULL){strcat(buf, "full "); cc &= ~COND_FULL;}	//uses c & ~C simultaneously
    if ((cc & COND_CC)   != 0)         strcat(buf, "C ");   //condition bit c1= carry
    if ((cc & COND_NC)   != 0)         strcat(buf, "~C ");  //condition bit c2=~carry
    if ((cc & COND_ZZ)   != 0)         strcat(buf, "Z ");   //condition bit c3= zero
    if ((cc & COND_NZ)   != 0)         strcat(buf, "~Z ");  //condition bit c4=~zero
    if (buf[strlen(buf)-1] == ' ') buf[strlen(buf)-1] = 0;  //ridiculous
    return buf;                                             //
   } //cEmulator::ConditionNames...

//append regP=hex of target as string
void cEmulator::FormatAsString(char *bufP, int bufSize, uint64_t reg)
   {int len=istrlen(bufP); uint8_t u8; const char *qq;                          //
    char regP[64]; SNPRINTF(regP), "%016llX", reg);                             //
    bufP[len++] = '='; bufP[len++] = '\"';                                      //
    for (qq=&regP[strlen(regP)]; (qq-=2) >= regP;)                              //
        if (len >= bufSize-3) break;                                            //
        else bufP[len++] = (u8=Hex2Bin(qq)) >= 0x20 && u8 < 0x80 ? u8 : '.';    //
    bufP[len++] = '\"'; bufP[len] = 0;                                          //
   } //cEmulator::FormatAsString..

//Assemble #expect: and #actual: messages up to the next '\n' then compare
//them against each other. The text of the OP_PRINTF message is stored in
//inputFileName.messages, juiced up by the values of various registers
//denoted by $digit in the message, eg., print "reg[0]=$0\n";
int cEmulator::OpPrint(int pc, OPCODE op, int bugLevel)
   {char *msgP, *pp, ch[256]; const char *ppc; int len=0, line=0;               //
    if (!(msgP=(char*)m_opNameP->FindMessage(op.g.adr))) return 0;              //bull shit: message must exist
    for (len=m_printBuf[0]=0; *msgP; msgP++)                                    //
        {if (*msgP == '$')                                                      //
             {if (strnicmp(msgP, "$line", 5) == 0)                              //
                 {snprintf(&m_printBuf[len], sizeof(m_printBuf)-len-1, "0x%X",  //
                                                   GetSourceLineNum(pc));       //
                  msgP += 4;                                                    //
                 }                                                              //
              else                                                              //
              if (strnicmp(msgP, "$pc", 3) == 0)                                //
                 {snprintf(&m_printBuf[len],sizeof(m_printBuf)-len-1,"0x%X",pc);//
                  msgP += 2;                                                    //
                 }                                                              //
              else                                                              //
                  snprintf(&m_printBuf[len], sizeof(m_printBuf)-len-1,          //
                                    "%llX", samRegs[(*(++msgP)&7)]);            //
              len += istrlen(&m_printBuf[len]);                                 //
             }                                                                  //
         else m_printBuf[len++] = *msgP;                                        //
         m_printBuf[len] = 0;                                                   //
         if (len > sizeof(m_printBuf)-10)                                       //
            {m_actualB = m_expectB = false;                                     //
             return Error(ERR_1566, m_printBuf, "");                            //1566 = Buffer is too small to perform requested operation.
        }   }                                                                   //
    if (m_bugLevel >= 3)                                                        //
       {//noisy environment: wrap the print message in >>>> <<<< and ignore LFs //
        Printf(">>>> "); OutputDebugStringA(">>>> ");                           //
        for (pp=m_printBuf; *pp; pp++)                                          //
           {DisplayChar(ch, *pp); Printf("%s", ch); OutputDebugStringA(ch);}    //
        Printf(" <<<<\n"); OutputDebugStringA(" <<<<\n");                       //isolate user string
       }                                                                        //
    else                                                                        //
       {Printf("%s", m_printBuf); OutputDebugStringA(m_printBuf);}              //bugLevel == 1 or 2
//#expect: message                                                              //
    if (msgP=strstr(m_printBuf, ppc="#expect:"))                                //
       {m_expect[0] = 0; m_expectB = !(m_actualB=false); msgP += strlen(ppc);}  //assert expectB, deassert actualB
    else msgP = m_printBuf;                                                     //
    if (m_expectB)                                                              //
       {CAT_THIS(m_expect), "%s", msgP);                                        //concatenate expected msg to m_expect
        if (pp=strchr(m_expect, '\n')) {m_expectB = false; *pp = 0;}            //
       }                                                                        //
//#actual: message                                                              //
    if (msgP=strstr(m_printBuf, ppc="#actual:"))                                //
       {m_actual[0] = 0; m_expectB = !(m_actualB=true); msgP += strlen(ppc);}   //assert actualB, deassert expectB
    else msgP = m_printBuf;                                                     //
    if (m_actualB)                                                              //
       {CAT_THIS(m_actual), "%s", msgP);                                        //concatenate actual msg to m_expect
        if ((msgP=strchr(m_actual, '\n')) == NULL) return 0;                    //
        m_actualB = false; *msgP = 0;                                           // 
        for (char *eP=m_expect, *aP=m_actual; ;)                                //compare expect and actual
           {while (*eP == ' ') eP++; while (*aP == ' ') aP++;                   //ignore white space
            if (*eP == 0 && *aP == 0) 
               {m_expect[0] = m_actual[0] = 0; return 0;}                       //
            if ((*eP++ | 0x20) == (*aP++ | 0x20)) continue;                     //ignore case
            return -ERR_2739;                                                   //2739 = #expect: <value> not equal to #actual: <value> in simulation.
       }   }                                                                    //
    return 0;                                                                   //
   } //cEmulator::OpPrint...
    
int cEmulator::OneOp(int pc, OPCODE op)
   {OPCODE      nxt  = m_pgmP[pc+1];                                            //
    static int  random=0, pcPlz=-2;                                             //
    uint8_t    *rowP = &m_bramP[g_memRowSize * m_curRow];                       //
    uint64_t   *u64P =(uint64_t*)rowP, tReg, wordOut=0, arithOut=0, target=0;   //
    bool        bb, scinB=false, fullB;                                         //
    uint32_t    prev, locn;                                                     //                                     
    int         adr  = op.g.act == OP_CALL ? op.call.callAdr : op.g.adr,        //
                breg = op.arith.breg,                                           //
                areg = op.arith.areg,                                           //
                subOp= op.arith.subOp,                                          // 
                cond = 0, itemCnt, itemSz, repete=op.rpt.count+1, erC;          //
    m_clkCount++;                                                               //
    if (pc == pcPlz)                                                            //
        pc = pc;                                                                // <<<--- software debug point ---<<<
    //Interpret the op ---------------------------------------------------------//
    switch (op.g.act)                                                           //
      {case OP_CALL: if (m_sp >= HOWMANY(m_stak)-1) goto stakOvflw;             //
                     m_stak[m_sp++] = pc + (m_bugLevel << 16);                  //
                     if (m_bugLevel == 3) m_bugLevel = 2;                       //
                     return adr;                                                //return adr of subroutine
       case OP_RDF: case OP_WRF: return RWfield(pc, op);                        //
       case OP_BUG:  return OpBug(pc, op);                                      //stop, $bug_raw, $bug_indx, or $bug_page
       case OP_CROWI:m_curRow = adr; return pc+1;                               //
       case OP_CFG_G: case OP_CFG_C:                                            //
       case OP_RET:  if (op.g.breg != 0) return Configuration(pc, op);          //
                     if (m_sp < 1 || m_sp >= HOWMANY(m_stak)) goto stakOvflw;   //
                     pc         = (m_stak[--m_sp]+1) & 0xFFFF;                  //
                     m_bugLevel = (m_stak[m_sp]     >> 16) & 31;                //restore bug level at call
                     if (pc >= m_pgmSize) goto pgmOvflw;                        //
                     return pc;                                                 //
       case OP_REPREG:repete  = (int)samRegs[breg];                             //case 3
       case OP_REPEAT:m_stepA = op.rpt.stepA != 0;                              //case 4
                     m_stepR = op.rpt.stepR != 0;                               //
                     m_dirn  = op.rpt.bkwd ? -1 : +1;                           //
                     m_repete= repete;                                          //
                     return pc+1;                                               //
       case OP_PRINT:if ((erC=OpPrint(pc, op, m_bugLevel)) < 0) return erC;     //-ve means m_expect != m_actual
                     return pc+1;                                               //complicit coding; host sees OP_PRINT and acts
       case OP_CROW: m_curRow = (int)samRegs[op.ind.areg];                      // 
                     if (m_curRow >= m_bramRows)goto badRow;                    //
                     return pc+1;                                               //
       case OP_ARITH:if (!IsExtendedOp(op))                                     //
                        {samRegs[areg] = Arithmetic(op);                 break;}//other arithmetic operations 
                     switch (subOp)                                             //
                       {case OPS_R2R:  samRegs[breg] = samRegs[areg];    break; //
                        case OPS_XCHG: XCHG(samRegs[breg], samRegs[areg],tReg); break;//
                        case OPS_XTOS: XCHG(m_stak[m_sp-1],samRegs[areg],tReg); break;//
                        case OPS_POP:  samRegs[breg] = m_stak[--m_sp];   break; //
                        case OPS_PUSH: m_stak[m_sp++]= samRegs[breg];    break; //
                        case OPS_PUSH_CURROW: m_stak[m_sp++]=m_curRow;   break; //
                        case OPS_STC: m_samStatus.cc = 1; m_samStatus.nc = 0; break;
                        case OPS_CLC: m_samStatus.cc = 0; m_samStatus.nc = 1; break;
                        case OPS_STZ: m_samStatus.zz = 1; m_samStatus.nz = 0; break;
                        case OPS_CLZ: m_samStatus.zz = 0; m_samStatus.nz = 1; break;
                        default:       goto illegalOp;                          //
                       }                                                        //
                     break;                                                     //other arithmetic operations
       case OP_RI:    samRegs[breg] = adr;                               break; //
       case OP_LDI+ 0: case OP_LDI+ 4: case OP_LDI+ 8: case OP_LDI+12:          //
       case OP_LDI+16: case OP_LDI+20: case OP_LDI+24: case OP_LDI+28:          //
                     samRegs[0] = (samRegs[0] << 14) | op.ldi.imm;       break; //
       case OP_SCIN: scinB = true;                                              //
       case OP_SCAN: itemCnt = (op.sc.rowType ? m_indxPerRow : m_pagesPerRow);  //max per row
                     itemSz  = (op.sc.rowType ? sizeof(hINDX) : sizeof(hPAGE))+ m_keySize;
                     itemSz  =((itemSz + TARGETBUS_SIZE-1) / TARGETBUS_SIZE) * TARGETBUS_SIZE;//
                     fullB   =((hINDX*)(rowP +(itemSz * (itemCnt-1))))->stop != 0;//stop bit of last item
                     if (fullB && scinB)                                        //
                        {m_samStatus.cc = m_samStatus.nc = 1; break;}           //
                     if (op.sc.rowType > 1) goto badType;                       //
                     bb      = hSequencer(rowP, itemCnt, itemSz,                //row, count(hITEM[]), sizeof(hITEM)
                                (uint8_t*)&samRegs[op.sc.breg],                 //target of scan
                                (op.sc.rowType ? sizeof(hINDX) : sizeof(hPAGE)),//offsetof(hITEM::key)
                                m_keySize, &prev, &locn);                       //sizeof(key), and outputs 
                     m_insPt        = locn * itemSz / TARGETBUS_SIZE; //is that right ???             
                     m_samStatus.cc = m_samStatus.nc = 0;                       //
                     if (bb && scinB)                                           //
                         InsertKey(op, itemCnt, itemSz, locn, (uint8_t*)&samRegs[op.sc.breg]);//
                     break;                                                     //
       case OP_WRYT:                                                            //       
       case OP_READ: if (m_curRow >= m_bramRows)   goto invalidBram;            //
                     if (op.g.adr >= m_grpsPerRow) goto badAdr;                 //
                     if (op.g.act == OP_WRYT)                                   //
                          u64P[op.g.adr]     = samRegs[breg];                   //
                     else samRegs[op.g.breg] = u64P[op.g.adr];                  //
                     break;                                                     //
       case OP_GO_T: case OP_GO_T+8: case OP_GO_T+16: case OP_GO_T+24:          //go_t(0) is unconditional goto
       case OP_GO_F: case OP_GO_F+8: case OP_GO_F+16: case OP_GO_F+24:          //go_f(0) is noop
                     m_samStatus.qrdy = ((random++) & 5) == 4;                  //'random' setting for qrdy
                     if (((*(uint8_t*)&m_samStatus & op.go.cond) == op.go.cond) //
                          == (op.go.act == OP_GO_T))                            //take  jump
                        {pc = IsLongJmp(op) ? m_pgmP[pc+1].u16             :    //
                                             (adr & 0x80) ? pc - (255-adr) :    //
                                              pc+adr+1;                         //
                         pc--;                                                  //fall out to address check
                        }                                                       //
                     break;                                                     //spurn jump
       default:      goto illegalOp;                                            //
      }                                                                         //
    if (++pc >= m_pgmSize) goto pgmOvflw;                                       //address check
    return pc;                                                                  //
badType:    Printf("##Abort(emu):Invalid rowType=$d\n",         op.sc.rowType); return -1;
badAdr:     Printf("##Abort(emu):Invalid memory address =$d\n", op.g.adr);      return -1;
badRow:     Printf("##Abort(emu):Invalid current Row=$%d\n",    m_curRow);      return -1;
illegalOp:  Printf("##Abort(emu):Invalid op pc=%d, op=0x%04X",  pc, op.u16);    return -1;
stakOvflw:  Printf("##Abort(emu):Stack overflow @ pc = %5d\n",  pc);            return -1;
pgmOvflw:   Printf("##Abort(emu):Invalid program address = %05d\n", pc);        return -1;
invalidBram:Printf("##Abort(emu):BRAM[%d] is current row=%d\n", m_curRow);      return -1;
    } //cEmulator::OneOp...

inline int CarryAdd32(uint32_t a, uint32_t b, uint32_t c) 
   {return (((uint64_t)a + (uint64_t)b  + (uint64_t)c) & 0x100000000ULL) != 0;} //add with carry

inline int CarrySub32(uint32_t a, uint32_t b, uint32_t c) 
   {return (((uint64_t)a - (uint64_t)b  - (uint64_t)c) & 0x100000000ULL) != 0;} //subtract with borrow

//OP_RDF and OP_WRF are read/write field operations: 
//    {areg, nu, fieldNum, breg, act}
// areg is the handle of the hITEM; points to the first key of the hITEM.
// breg is the source/target data register of the write/read.
// fieldNum >= 8 is a key field addressed forward from          the handle.
// fieldNum <  8 are binary fields in the hPAGE/hINDX preceding the handle.
int cEmulator::RWfield(int pc, OPCODE op)
   {uint64_t theAddr, fieldVal, *u64P;                                          //
    int      areg    =op.ind.areg, breg=op.ind.breg,                            //
             fieldNum=op.ind.fieldNum, stopPlz=-1;                              //
    hPAGE   *pageP=NULL;                                                        //
    hINDX   *indxP=NULL;                                                        //
                                                                                //
    if (pc == stopPlz)
       pc = pc;
#ifdef _DEBUG                                                                   //
    if (op.ind.act == OP_WRF)                                                   //
         Bugout("%03d: OP_WRF [$%d].%d = $%d\n", pc, areg, fieldNum, breg);     //
    else Bugout("%03d: OP_RDF $%d = [$%d].%d\n", pc, breg, areg,     fieldNum); //
#endif                                                                          //
    u64P    = (uint64_t*)&m_bramP[g_memRowSize * m_curRow];                     //point to row
    theAddr = samRegs[areg];                                                    //structure location in row
    if (fieldNum & 8)                                                           //bit[3] == 1 (key field)
       {//key field access is row[handle + fieldNum]                            //
        theAddr += (fieldNum & 7);                                              //
        if (op.ind.act == OP_WRF) u64P[theAddr]    = samRegs[breg];             //
        else                      samRegs[breg] = u64P[theAddr];                //
        return pc+1;                                                            //
       }                                                                        //
    //non-key is row[handle-1 or handle-2].fieldNum                             //bit[3] == 0 (binary field)
    if ((fieldNum & 7) <= FLD_STOP)                                             //
          theAddr -= sizeof(hINDX)/sizeof(uint64_t);                            //start of hINDX (2 uint64's)
    else  theAddr -= sizeof(hPAGE)/sizeof(uint64_t);                            //start of hPAGE (3 uint64's)
    pageP   = (hPAGE *)&u64P[theAddr];                                          //only one of these is correct
    indxP   = (hINDX *)&u64P[theAddr];                                          //            "
    if (op.ind.act == OP_RDF) //read field                                      //
      {switch (fieldNum & 7)                                                    //
         {case FLD_DATA:      fieldVal = indxP->data;    break;                 //hINDX field
          case FLD_STOP:      fieldVal = indxP->stop;    break;                 //hINDX/hPAGE/hBOOK field
          case FLD_P1:        fieldVal = GetP1(pageP);   break;                 //PAGE/hBOOK fields
          case FLD_P2:        fieldVal = GetP2(pageP);   break;                 //        "
          case FLD_COUNT:     fieldVal = pageP->kount;   break;                 //        "
          case FLD_TOTAL:     fieldVal = GetTotal(pageP);break;                 //        "
         }                                                                      //
       samRegs[breg] = fieldVal;                                                //
      } //(fieldGo && !wryt)...                                                 //
    if (op.ind.act == OP_WRF)                                                   //
      {fieldVal = samRegs[breg];                                                //
       switch (fieldNum & 7)                                                    //
         {case FLD_DATA:      indxP->data  = fieldVal;  break;                  //hINDX field
          case FLD_STOP:      indxP->stop  = fieldVal;  break;                  //hINDX/hPAGE/hBOOK field
          case FLD_P1:        PutP1(pageP,   fieldVal); break;                  //PAGE/hBOOK fields
          case FLD_P2:        PutP2(pageP,   fieldVal); break;                  //        "
          case FLD_COUNT:     pageP->kount = fieldVal;  break;                  //        "
          case FLD_TOTAL:     PutTotal(pageP,fieldVal); break;                  //        "
      }  }                                                                      //
     return pc+1;                                                               //
   } //cEmulator::RWfield..

int cEmulator::Configuration(int pc, OPCODE op) {return pc+1;}

uint64_t cEmulator::Arithmetic(OPCODE op)
   {typedef struct {uint32_t lo, hi;} U64;                                      //
    uint32_t u0=0, num=op.arith.areg;                                           //
    uint64_t mid64=0x100000000ULL, wordO, hi64=mid64 << 31, tmp,                //
             l64 = samRegs[op.arith.areg], r64 = samRegs[op.arith.breg];        //parameters to binary op
    U64      L64 = *(U64*)&l64,            R64 = *(U64*)&r64;                   //parameters as pairs of u32
    static   uint32_t ccc=0;                                                    //
                                                                                //
    m_samStatus.cc = 0; m_samStatus.nc = 1;                                     //
    switch (op.arith.subOp)                                                     //
        {case OPS_ADC: u0  = ccc;                                               //
         case OPS_ADD: wordO = l64 + r64 + u0;                                  //
                       ccc = CarryAdd32(L64.lo, R64.lo,u0+((wordO & mid64)!=0));//
                       ccc = CarryAdd32(L64.hi, R64.hi, ccc);                   //
                       m_samStatus.cc = ccc;        m_samStatus.cc = !ccc;      //
                       m_samStatus.zz = (wordO==0); m_samStatus.nz = (wordO!=0);//
                       return wordO;                                            //
         case OPS_XSBB: u0 = ccc;  goto case_SUB;                               //
         case OPS_XSUB: XCHG(l64, r64, tmp);                                    //
         case OPS_CMP: ccc = 0;                     //fall thru                 //
         case OPS_SBB: u0  = ccc;                   //fall thru                 //
         case_SUB:
         case OPS_SUB: wordO = l64 - r64 - u0;                                  //
                       ccc = CarrySub32(L64.lo, R64.lo,u0+((wordO & mid64)!=0));//
                       ccc = CarrySub32(L64.hi, R64.hi, ccc);                   //
                       m_samStatus.cc = ccc;        m_samStatus.nc = !ccc;      //
                       m_samStatus.zz = (wordO==0); m_samStatus.nz = (wordO!=0);//
                       return (op.arith.subOp == OPS_CMP) ? r64 : wordO;        //ignores result if CMP
         case OPS_XOR: ccc = 0; wordO = l64 ^ r64;                 break;       //
         case OPS_OR:  ccc = 0; wordO = l64 | r64;                 break;       //
         case OPS_AND: ccc = 0; wordO = l64 & r64;                 break;       //
         case OPS_INC:          wordO = ++r64;                     break;       //
         case OPS_DEC:          wordO = --r64;                     break;       //
         case OPS_SHL: ccc =(l64 & hi64)!=0; l64 <<= 1; m_samStatus.cc = ccc; wordO = l64; break;
         case OPS_SHR: ccc = l64 & 1; l64 >>= 1;        m_samStatus.cc = ccc; wordO = l64; break;
         case OPS_RCL: wordO =(l64 & hi64)!=0; l64 = (l64 << 1) + ccc;  ccc = (uint32_t)wordO;
                                                        m_samStatus.cc = ccc; wordO = l64; break;
         case OPS_RCR: wordO = ccc ? hi64 : 0; l64 = (l64 >> 1) + wordO;ccc = (uint32_t)l64;
                                                        m_samStatus.cc = ccc; wordO = l64; break;
         default:      Bugout("error %d: Illegal opcode", ERR_9995); exit(1);   //
        }                                                                       //
    m_samStatus.zz = (wordO == 0); m_samStatus.nz = (wordO != 0);               //
    m_samStatus.nc = !m_samStatus.cc;                                           //
    return wordO;                                                               //
   } //cEmulator::Arithmetic...

//The row of data is maintained in sorted order. Each element is compared with keyP; 
//the point at which the comparison shifts from < to >= is the insertion point. 
//Every element thereafter is also >= keyP; every element before is < keyP.
//
//if an insertion point is correctly found:
//      hSequencer returns true,
//      *locnP = insertion point
//      *prevP = preceding record
//if an insertion point is not found:
//      hSequencer returns false,
//      *locnP = 'eof', ie. insertion point past last record.
//      *prevP = rcdNum of last valid cINDX entry.
bool cEmulator::hSequencer(void     *rowV,  uint32_t rowCnt,    uint32_t itemSz,//array description inputs
                           uint8_t  *keyP,  uint32_t keyOffset, uint32_t keySz, //key description inputs
                           uint32_t *prevP, uint32_t *locnP)                    //outputs
   {uint8_t   *kP=((uint8_t*)rowV)+keyOffset;                                   //location of 1st key field
    uint32_t  locn;                                                             //
    //Check if key is larger than highest key in array                          //
//  if (rowCnt >= 2 &&                                                          //
//      CompareKey(keyP, kP + (rowCnt-1)*itemSz, m_keySize) >= 0)               //highest key in array
//     {*locnP = rowCnt; *prevP = rowCnt-1; return false;}                      //
                                                                                //
    for (*prevP=*locnP=locn=0; locn < rowCnt; kP+=itemSz, *locnP=++locn)        //for each item in this vector
        {if (CompareKey(keyP, kP, keySz) < 0) return true;                      //
         *prevP = locn;                                                         //
        }                                                                       //
    return false;                                                               //not found condition
   } //cEmulator::hSequencer...

inline uint32_t HiLo32(uint32_t u32)
    {return ((u32 << 24) & 0xFF000000) + ((u32 <<  8) & 0x00FF0000) +
            ((u32 >>  8) & 0x0000FF00) + ((u32 >> 24) & 0x000000FF);}

//aV < bV return -1, aV == bV return 0, aV > bV return +1
int cEmulator::CompareKey(const void *aV, const void *bV, int keySz)
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
   } //cEmulator::CompareKey...

void cEmulator::InsertKey(OPCODE op, int itemCnt, int itemSz, int locn, uint8_t *keyP)
   {uint8_t *ramP= &m_bramP[g_memRowSize * m_curRow] + (itemCnt-1) * itemCnt;   //
    while (--itemCnt > locn)                                                    //right shift all hITEMs
          {memmove(ramP, ramP-itemSz, itemSz); ramP -= itemSz;}                 //  to the right of [locn]
    memmove(ramP+=itemSz-m_keySize, keyP, m_keySize);                           //move key part
    for(uint8_t fill=0x22; (itemSz -= TARGETBUS_SIZE) > 0; fill <<= 1)          //fill binary part with:
           memset(ramP-=TARGETBUS_SIZE, fill, TARGETBUS_SIZE);                  //   222, 333, 444,...
   } //cEmulator::InsertKey...                                                  //   consistent with hardware

void cEmulator::Bugout(char const *fmtP,...)
    {va_list arg;                                                               //
     char    buf[512];                                                          //
     if (m_bugLevel <= 2) return;                                               //sworn to silence
     va_start(arg, fmtP);                                                       //
     vsnprintf(buf, sizeof(buf)-1, fmtP, arg);                                  //
     va_end(arg);                                                               //
     buf[sizeof(buf)-1] = 0;                                                    //
     OutputDebugStringA(buf);
    } //cEmulator::Bugout...

//Convert keyP from alpha to hex unless the key is all displayale characters.
//If keyP translates to displayable characters that is what will be copied into outP.
//If keyP cannot be so translated, then it will be displayed in hex.
//Return true if result is displayable.
bool cEmulator::DisplayableHex(const char *keyP, char *outP)
   {char       *dP=outP;                                                        //
    const char *sP;                                                             //
    int         ii, jj;                                                         //
    bool        displayableB=true, undefinedB=false;                            //
                                                                                //
    if (keyP[0] == '0' && keyP[1] == 'x') keyP += 2;                            //ignore leading 0x
    for (ii=0, jj=0, dP=outP, sP=keyP; ii < m_keySize; ii++, jj++)              //
        {if ((dP[jj]=sP[ii]) < ' ') displayableB = false;}                       //
    dP[jj] = 0;                                                                 //
    if (displayableB) return true;                                              //
    //not displayable characters                                                //
    if (undefinedB) strncpy(outP, keyP, jj=m_keySize);                          //
    else                                                                        //
       {for (ii=0, jj=0, sP=keyP, dP=outP; ii < m_keySize; ii++, jj+=2)         //
             sprintf(&dP[jj], "%02X", sP[ii]&0xFF);                             //
       }                                                                        //
    return false;                                                               //
   } //cEmulator::DisplayableHex...

int cEmulator::OpBug(int pc, OPCODE op)
   {int    grpsPerItem, grp, ii, jj, kk, row=m_curRow, perLine, item;           //
    char   buf[99], *startRowP, *rowP=(char*)&m_bramP[m_curRow*g_memRowSize];   //point to row
    bool   dupB;                                                                //
    static const char *rowType[]={"", "$BUG_PAGE", "$BUG_RAW", "$BUG_INDX"};    //
    static int padSz[] = {16, 2*TARGETBUS_SIZE,3*TARGETBUS_SIZE};               //
    static int perLyne[] = {32, 24, 32, 32};                                    //
    static int eyetem[]  = {0, ITEM_PAGE, 0, ITEM_INDX};                        //
                                                                                //
    pc++;                                                                       //  
    if (op.bug.set && op.bug.sho == 3)                       return pc;         //$bug; or $bug break;
    if (op.bug.set)        {m_bugLevel = op.bug.level & 7;   return pc;}        //$bug = constant
    if (op.u16 == OP_STOP) {Printf("\n##Endof Emulation\n"); return -ERR_2704;} //2704 = stop encountered
    Printf("Row[0x%X], op=", row);                                              //
    if (op.bug.set) Printf("$bug = %d", op.bug.level);                          //
    else            Printf("%s",        rowType[op.bug.sho]);                   //
    Printf("(0x%04X)\n", op.u16);                                               //
    perLine     = perLyne[op.bug.sho];                                          //
    item        = eyetem [op.bug.sho];                                          //
    grpsPerItem = padSz[item] / TARGETBUS_SIZE;                                 //groups per hITEM
    for (ii=kk=0, startRowP=rowP; ii < (int)g_memRowSize;)                      //
        {grp = ii / TARGETBUS_SIZE;                                             //
         if ((ii % perLine) == 0) Printf("%03X: ", ii);                         //
         if ((grp % grpsPerItem) == (grpsPerItem-1) && item != 0                //
                           && DisplayableHex(rowP, buf))                        //
             {Printf("\"%s\"         ", buf); rowP += TARGETBUS_SIZE;}          //
         else                                                                   //
            {for (jj=0; jj < TARGETBUS_SIZE; jj++, rowP++)                      //
                   Printf("%02x%s", *rowP & 0xFF,                               //
                          (jj & 7) ==  7 ? "  "  : (jj &  3) ==  3 ? "_" : ""); //
            }                                                                   //
         ii += TARGETBUS_SIZE;                                                  //     
         if ((kk+=TARGETBUS_SIZE) >= perLine)                                   //end of line
            {Printf("\n"); kk = 0;                                              //
             //suppress duplicate lines                                         //
             for(dupB=false; ii > perLine && ii < ((int)g_memRowSize-perLine)   //not first and not last
                 && memcmp(startRowP, rowP, perLine) == 0;)                     //compare this with upcoming row
                 {dupB = true; ii += perLine; rowP += perLine;}                 //
             startRowP = rowP;                                                  //
             if (dupB) Printf("%15s%19s%19s\n", "\"\"", "\"\"", "\"\"");        //
        }   }                                                                   //
    Printf("\n");                                                               //
    return pc;                                                                  //
   } //cEmulator::OpBug...

int cEmulator::ExecutePgm(int mode)
   {int nxt, act, breg; OPCODE op;                                              //
    for (;;m_pc=nxt)                                                            //
        {if ((nxt=OneOp(m_pc, op=m_pgmP[m_pc])) < 0) return nxt;                //
         if (op.g.act == OP_RI && op.g.breg==0 && m_pgmP[nxt].ldi.act == OP_LDI)//$0 = lit; followed by OP_LDI
            {for (m_pc=nxt; (op=m_pgmP[m_pc]).ldi.act == OP_LDI; nxt=++m_pc)    //
                 samRegs[0] = (samRegs[0] << 14) | op.ldi.imm;                  //
            }                                                                   //
         else                                                                   //
            {for (op=m_pgmP[nxt]; m_repete != 0; m_repete--)                    //op_repeat encountered
                 {if ((nxt=OneOp(m_pc+1, op)) < 0) return 0;                    //execute next opcode m_repete times
                  if (m_stepA) op.g.adr += m_dirn;                              //      with increment of address
                  if (m_stepR) op.g.breg+= m_dirn;                              //                    and register
            }    }                                                              //
         act = op.g.act; breg = op.g.breg; m_pc = nxt;                          //
         switch (mode)                                                          //
             {case EX_RUN:                                             continue;//continue unabashed
              case EX_BY_LINE:                                                  //Run until line number changes
              case EX_SS:                                    return 0;          //single step; return and wait for user
              case EX_2BREAK:if (act == OP_BUG)              return 0; continue;//Run until next $bug opcode
              case EX_2RET:  if (act == OP_RET && breg == 0) return 0; continue;//Run up to next ret opcode
              default:                                       return 0;          //exit and wait for user
             }                                                                  //
         return 0;                                                              //return to user and await instructions
        }                                                                       //
    return nxt;                                                                 //exit from ExecutePgm
   } //cEmulator::ExecutePgm...

//end of file

