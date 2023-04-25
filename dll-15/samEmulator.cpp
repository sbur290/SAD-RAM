#include "samEmulator.h"
#include <malloc.h>

#define TARGETBUS_SIZE 8                                                        //sim.cpp compatibility
#define ITEM_PAGE      2                                                        //        "
#define ITEM_INDX      1                                                        //        "
#define SNPRINTF(buf) buf[sizeof(buf)-1] = 0; snprintf(buf, sizeof(buf)-1
#define CAT_THIS(buf) len = istrlen(buf); buf[sizeof(buf)-1] = 0; snprintf(&buf[len], sizeof(buf)-1-len

FILE        *g_printFileP=NULL;                                                 //for Printf
int          g_memRowSize=256;                                                  //
cSamError    g_err;                                                             //
extern cEmulator *g_emulatorP;                                                  //

typedef enum {SS_OK=0, SS_ILLEGAL_OP=1, SS_ILLEGAL_RET=2, SS_SHUTDOWN=3} SS_CTRL;

int Error(int erC, const char *contextP, const char *paramP) 
   {Printf("\x07**** Error %d: %s\n", erC < 0 ? -erC : erC, paramP); 
    return erC < 0 ? erC : -erC;
   } //Error...

uint16_t cEmulator::m_breaks[16];                                               //
int      cEmulator::m_maxBreaks=0;                                                //
cEmulator::cEmulator(int keySize, const char *objNameOnlyP, const char *srcFullNameP, 
                     int bugLevel, bool guiB)
   {int         erC, ii=0;                                                      //
    char       *simDirP=getenv("samObjectDir"), buf[_MAX_PATH], *messagesP;     //
    const char *blokRam="blockRam.bin";                                         //
    m_singleStepB    = false;                                                   // 
    m_guiB           = guiB;                                                    //
    m_bugLevel       = bugLevel;                                                //
    m_bramP          = NULL;                                                    //
    m_microCodeP     = NULL;                                                    //
    m_pgmP           = NULL;                                                    //
    m_samFilePrefixP = NULL;                                                    //
    m_keySize        = keySize;                                                 //
    m_curRow         = 0; m_curOvly   = 0;                                      //
    m_repete         = 0; m_clkCount  = 0;                                      //
    m_sp             = 0; m_pc        = 0;                                      //
    m_curLine        = 0; m_highlight = 0;                                      //
    m_lastExit       = 0; m_retLevel  = 0;                                      //
    m_expectedVersusActual = 0;                                                 //
    m_targetBusSz    = TARGETBUS_SIZE;                                          //
    m_actualB        = m_expectB   = false;                                     //not gatering $expect or $actual text
    m_expect[0]      = m_actual[0] = 0;                                         //
    m_printBuf[0]    = 0;                                                       //
    m_compilerMsgP   = NULL;                                                    //
    ComputeGeometry(keySize);                                                   //
    *(uint8_t*)&m_samStatus = 0;                                                // 
    //fill the registers and stack with arbitrary nonsense.                     //
    samRegs[0]   = 101010; samRegs[1] = 111; samRegs[2] = 222; samRegs[3] = 333;//initialize $registers
    samRegs[4]   = 444;    samRegs[5] = 555; samRegs[6] = 666; samRegs[7] = 777;// (arbitrary values)
    m_stak[0]    = 0xAAA;  m_stak[1]  = 0xBBB;m_stak[2] = 0xCCC;m_stak[3] = 0xDDD;
                                                                                //
    memmove(m_lastRegs, samRegs, sizeof(m_lastRegs));                           //
    ii           = sizeof(hINDX) + m_keySize + TARGETBUS_SIZE-1;                //
    ii           = (ii / TARGETBUS_SIZE) * TARGETBUS_SIZE;                      //round to targetbus size
    m_indxPerRow = g_memRowSize / ii;                                           //calculate hINDX/row
    ii           = sizeof(hPAGE) + m_keySize + TARGETBUS_SIZE-1;                //
    ii           = (ii / TARGETBUS_SIZE) * TARGETBUS_SIZE;                      //round to targetbus size
    m_pagesPerRow= g_memRowSize / ii;                                           //calculate hPAGE/row
                                                                                //
    if ((erC=ReadTheFile(blokRam, "", &m_bramP))                 < 0) goto err; //read blockRAM (sample blockRAM prepared by genVerilog)
    m_bramRows   = erC / g_memRowSize;                                          //size in rows
    m_grpsPerRow = g_memRowSize / TARGETBUS_SIZE;                               //groups per row
    m_objNameOnlyP = (char*)objNameOnlyP;                                       //
    if ((erC=ReadTheFile(objNameOnlyP, ".bin", (void**)&m_pgmP)) < 0) goto err; //
    m_pgmSize = erC / sizeof(OPCODE);                                           //
                                                                                //
    SNPRINTF(buf), "%s\\%s.capFile", simDirP, objNameOnlyP);                    //
    if (!m_guiB) g_printFileP = fopen(buf, "wb");                               //GUI opens if necessary
                                                                                //
    if ((erC=ReadTheFile(objNameOnlyP, ".msgFile", &messagesP))  < 0) goto err; //read msgFile
    m_opNameP    = new cOpName(&g_err, messagesP, erC);                         //
                                                                                //
    if ((erC = ReadTheFile(objNameOnlyP, ".lineMap", (char**) & m_lineMapP)) < 0) goto err; //read lineMap
    m_lineMapSize = erC / sizeof(m_lineMapP[0]);                                //
                                                                                //
    if ((erC=ReadTheFile(objNameOnlyP,".symbolTbl",&m_symbolTblP))<0) goto err; //read symbol table
    m_symbolSize = erC;                                                         //
                                                                                //
    if ((erC=ReadTheFile(srcFullNameP, NULL, &m_sourceTextP))    < 0) goto err; //read source
    m_sourceSize = erC;                                                         //
                                                                                //
    m_curLine    = XlatPC2Line(0);                                              //
    if (!m_guiB)                                                                //Gui does UI
       {Printf("##Start Emulation; rev=%d, program=%s\n", SAM_VERSION, m_objNameOnlyP);//
        Printf("##Params: keySize=%2d, rowSize=%3d\n", m_keySize, g_memRowSize);//
       }
#ifdef _DEBUG
    TestDujour("X=1; // why X==1 ?, eh\n");
#endif
    erC = 0;                                                                    //
err:m_errorCode = erC;                                                          //
   } //cEmulator::cEmulator...

void cEmulator::TestDujour(const char *srcP) {}

cEmulator::~cEmulator() 
   {free(m_bramP);               m_bramP          = NULL;
    free(m_microCodeP);          m_microCodeP     = NULL;   
    free(m_pgmP);                m_pgmP           = NULL;
    delete m_opNameP;            m_opNameP        = NULL;
    free(m_lineMapP);            m_lineMapP       = NULL;
    free(m_symbolTblP);          m_symbolTblP     = NULL;
    free(m_sourceTextP);         m_sourceTextP    = NULL;
    free(m_samFilePrefixP);      m_samFilePrefixP = NULL;
    free((void*)m_compilerMsgP); m_compilerMsgP   = NULL;
    if (g_printFileP) fclose(g_printFileP); g_printFileP = NULL;
   } //cEmulator::~cEmulator...

//Read all file into memory at *bufPP; return size or -ve if unable to open file
//File name is %samObjectDir%\fileOnlyP.extensionP 
//unless extensionP == NULL, in which case the filename == fileOnlyP
int cEmulator::ReadTheFile(const char *fileOnlyP, const char *extensionP, void **bufPP)
   {size_t sz;                                                                  //
    char  *bufP, fn[_MAX_PATH];                                                 //
    FILE  *fileP;                                                               //
    if (extensionP != NULL)                                                     //
        snprintf(fn, sizeof(fn), "%s\\%s%s",                                    //
                                getenv("SamObjectDir"), fileOnlyP, extensionP); //
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
//1D0C //008 (line ####) wrBingo : OP_WRYT[29] ".4Vx...."   #10 source          //example format
int cEmulator::GetCompilerMsg(char *bufP)                                       //
   {if (m_compilerMsgP[0])                                                      //
       snprintf(bufP, _MAX_PATH, "Compiled for %s\n", m_compilerMsgP);          //health of compiler
    else bufP[0] = 0;                                                           //  hmmm ?
    return 0;                                                                   //
   }//cEmulator::GetCompilerMsg...

//Return name of label at adr
const char *cEmulator::GetLabel(int adr)     
    {char *pp, *qq;
     for (pp=m_symbolTblP; (int)(pp-m_symbolTblP) < m_symbolSize; pp++)
         {pp += strlen(qq=pp);
          if (strtol(pp+3, &pp, 16) == adr) return qq;
         }
     return NULL;
    } //cEmulator::GetLabel.. 

//Adjust user specified line to the next line with valid SamCode
int cEmulator::AdjustLineNum(int line) 
    {sSRC_REF *mapP, *endMapP=&m_lineMapP[m_lineMapSize]; int ii;
     for (mapP=1+m_lineMapP, ii=1; mapP < endMapP; mapP++, ii++)
         {if (line > (mapP-1)->lineNum && line <= mapP->lineNum) return mapP->lineNum;
         }
     return line;
    } //cEmulator::AdjustLineNum...

bool cEmulator::Isalpha(char ch) 
   {return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_';}

int  cEmulator::SetBugLevel(int lvl)  {int old=m_bugLevel;  if (lvl >= 0) m_bugLevel  = lvl; return old;}
int  cEmulator::SetRowFormat(int fmt) {int old=m_rowFormat; m_rowFormat = fmt; return old;}
bool cEmulator::strBegins(const char *strP, const char *whatP) {return strncmp(strP, whatP, strlen(whatP)) == 0;}

//Save settings into %samBinaryDir%\settings: h=height, w=width, l=logging, p=platform, r=reg index
typedef struct {uint32_t ver, options;        //form height and width, location of form.Top on physical screen
                int      h, w, top;           //.01=logging, .02=shoKeysOnly, .04=00 software, .04=04 hardware
                int      regN; uint64_t regV; //register number and value for 'stop-when'
                uint16_t breaks[HOWMANY(cEmulator::m_breaks)];
               } SETTINGS;
     
//Parameters follow fields in SETTINGS.
//Write settings to file %samBinaryDir%\settings
int cEmulator::SaveSettings(int h, int w, int top, uint32_t options, int regN, uint64_t regV)  
    {SETTINGS    s   = {SAM_VERSION, options, h, w, top, regN, regV};
     char        fn[_MAX_PATH];
     const char *binP=getenv("samBinaryDir");
     uint16_t    breaks[HOWMANY(cEmulator::m_breaks)];
     FILE       *fP;
     if (!binP || g_emulatorP == NULL)          return 0;
     if (RestoreSettings(&h, &w, &top, &options, &regN, &regV, breaks) == 1)
        {if (s.ver     == SAM_VERSION && s.h    == h && s.w == w && s.top  == top &&
             s.options == options     && s.regN == regN          && s.regV == regV&&
             memcmp(m_breaks, breaks, sizeof(m_breaks)) == 0
            ) return 0;                                     //file == locals; nuthing to do
        }
     snprintf(fn, sizeof(fn), "%s\\settings", binP);        //build file name
     if ((fP=fopen(fn, "wb")) == NULL)          return 0;   //failed
     memmove(s.breaks, m_breaks, sizeof(s.breaks));         //
     fwrite(&s, 1,sizeof(s), fP); fclose(fP);   return 1;   //happy
    } //cEmulator::SaveSettings..

//Get settings from file %samBinaryDir%\settings
int cEmulator::RestoreSettings(int *h, int *w, int *top, uint32_t*options, int*regN, uint64_t*regV, uint16_t *breaksP)
    {SETTINGS    s;
     char        fn[_MAX_PATH];
     const char *binP=getenv("samBinaryDir");
     FILE       *fP;
     if (!binP)                                 return 0; //failed
     snprintf(fn, sizeof(fn), "%s\\settings", binP);      //build file name
     if ((fP=fopen(fn, "rb")) == NULL)          return 0; //failed
     fread(&s, 1,sizeof(s), fP); fclose(fP);
     if (s.ver != SAM_VERSION)                  return 0; //failed
     *h       = s.h;       *w    = s.w;    *top  = s.top; 
     *options = s.options; *regN = s.regN; *regV = s.regV; 
     if (!breaksP) breaksP = cEmulator::m_breaks;
     memmove(breaksP, s.breaks, sizeof(cEmulator::m_breaks));
     cEmulator::m_maxBreaks = 0;
     for (int ii=HOWMANY(s.breaks); --ii > 0 && s.breaks[ii] == 0;) 
         cEmulator::m_maxBreaks = ii+1;
     return 1;
    } //cEmulator::RestoreSettings...

int cEmulator::StartLogging(bool startB, char *fileNameP)
    {char fileName[_MAX_PATH];
     snprintf(fileName, sizeof(fileName)-1, "%s\\%s.capFile", 
                          getenv("samObjectDir"), m_samFilePrefixP);
     if (startB) g_printFileP = fopen(fileName, "wb"); else
     if (!g_printFileP) fileNameP[0] = 0;              else
        {fclose(g_printFileP); g_printFileP = NULL;}
     strncpy(fileNameP, fileName, _MAX_PATH);
     return startB ? 1 : 0;
    } //cEmulator::StartLogging...

//CheckE messages are generated at beginning of op: 
//This routine only used in command line version of emu.exe
int cEmulator::CheckE(void)
   {static int   stopPlz= 2, lastLine=-1, lastii=IDNO;                          //response if singleStep = false
    OPCODE       op, nxt;                                                       //current OP
    int          lineNum   = XlatPC2Line(m_pc),                            //
                 response  = MB_OK,                                             //
                 len, ii, adr, srcLen;                                          //
    uint8_t      samStat   = *(uint8_t*)&m_samStatus;                           //
    uint64_t     reg0      = samRegs[0];                                        //
    const char  *srcP, *hereP=NULL, *thereP=NULL, *qq;                          //
    const char  *adviceP="\n\n\t\tY=step, N=run, C=stop";                       //
    #define catHdr  CAT_THIS(m_hdr)                                             //append to hdr
    #define catStak CAT_THIS(m_stakBuf)                                         //append to stak

    if (m_pc == -7)
      m_pc = m_pc;
    Printf("\n");                         
    m_hdr[0] = m_stakBuf[0] = m_flags[0] = 0;                                   //zero the text buffers
    op       = m_pgmP[m_pc];                                                    //current OP
    nxt      = m_pgmP[m_pc+1];                                                  //next word following opcode
    if (op.shortOp == OP_BUG && op.bug.fnc == 4) m_bugLevel = op.bug.level;     //bug is coming up
    if (m_bugLevel <  3) goto printOnly;                                        //otherwise interpret opcode
    adr    = IsLongJmp(op) ? nxt.u16               :                            //long jump is abs adr in next op
             IsGoOp(op)    ? op.go.relAdr + m_pc+1 :                            //short jump is relAdr
             IsCall(op)    ? op.call.callAdr       : -1;                        //absolute address
    hereP  = GetLabel(m_pc);                                                    //
    thereP = GetLabel(adr);                                                     //
    if (lineNum != lastLine)                                                    //print source line
       {Printf("\n---- line %04d: \"", lastLine=lineNum);                       //
        srcP = GetSourceLineP(m_pc, NULL, &srcLen);                             //
        for (qq=&srcP[srcLen]; srcP < qq; srcP++)                               //
             Printf("%c", *srcP == '\"' ? '.' : *srcP);                         //output source one char at a time
        Printf("\"\n");                                                         //
       }                                                                        //
    if (IsLongJmp(op))                                                          //
       {catHdr, "%03d:0x%04X %04X ", m_pc, op.u16, nxt.u16);}                   //
    else                                                                        //
       {catHdr, "%03d:0x%04X ", m_pc, op.u16);}                                 //
    if (hereP) {catHdr, "%s:", hereP);}                                         //
    catHdr, " %s, line %d", m_opNameP->Show(m_pc,op,nxt, thereP, true), lineNum);//
    SNPRINTF(m_flags),                                                          //
              "samStat=(0x%X, %s), clkCount=%04d, curRow=%d, insPt=%d",         //
               samStat, m_opNameP->ConditionNames(samStat),                     //
               m_clkCount, m_curRow, m_insPt);                                  //
    if (m_sp != 0)                                                              //
       {catStak, "(stak, sp=%d) ", m_sp);                                       //
        for (ii=0; ii < min(m_sp, 4); ii++)                                     //
            {catStak, "%s0x%016llX, ", ii == 2 ? "\n  " : "", m_stak[ii]);      //
       }    }                                                                   //
                                                                                //
    //Echo above information to log file                                        //
    Printf("%s, %s\n", m_hdr, m_flags);                                         // 
                                                                                //
    //Output samRegs noting changed values                                      //
    for (ii=0; ii < MAX_REGS; ii++)                                             //
        {Printf("%s0x%016llX", (ii & 3) == 0 ? "  " : "", samRegs[ii]);         //     
         Printf("%s", samRegs[ii] == m_lastRegs[ii]  ? ",  " : "*, ");          //
         if ((ii & 3) == 3) Printf("\n");                                       //
         m_lastRegs[ii] = samRegs[ii];                                          //
        }                                                                       //
    if (m_sp != 0) Printf("     %s\n", m_stakBuf);                              //
                                                                                //
printOnly:                                                                      //
    if (op.g.act == OP_PRINT && op.g.breg != 0)                                 //
        {if ((ii=DoOpPrint(m_pc, op, m_bugLevel)) < 0)                          //-ve means m_expect != m_actual
           {if (m_bugLevel < 3)                                                 //
               {Printf("%03d: $0<=0x%llX", m_pc-m_holdCheckE, reg0);            //reconstruct display
                FormatAsString(m_hdr, sizeof(m_hdr)-3, samRegs[0]);             //decorative touch
                Printf("%s line number=%d", m_hdr, lineNum);                    //
               }                                                                //
            Printf("\x7");                                                      //
            m_singleStepB = true;                                               //force messagebox
           }                                                                    //
        else if (m_bugLevel < 3)                      return 0;                 //
       } //OP_PRINT...                                                          //
    return 0;                                                                   //
#undef catHdr
#undef catStak
   } //cEmulator::CheckE...

#define whatClass cEmulator                                                     //
#define cINDX     hINDX                                                         //fake out computeGeometry.cpp
#define cPAGE     hPAGE                                                         //            "
#define cBOOK     hBOOK                                                         //            "
#include <computeGeometry.cpp>                                                  //

//if (samOp.bug.dump) $write("#checkF:%...\n", clkCount, samOp, pc, curRow, rowOut);
int cEmulator::CheckF(uint64_t row, int rowType)
   {int     grpsPerItem, grp, ii, jj, kk, pc, perLine, item;                    //
    uint8_t*rowP = &m_bramP[row*g_memRowSize];                                  //
    char    buf[99];                                                            //
    bool    dupB;                                                               //
    OPCODE  op;                                                                 //
                                                                                //
    op.u16   = m_pgmP[pc=m_pc].u16;                                             //
    Printf("\nRow[0x%X]\n", row);                                               //
    if (rowType == BUG_SHO_RAW  || rowType == BUG_SHO_INDX)                     //
                                               {item = ITEM_INDX; perLine = 32;}//
    if (rowType == BUG_SHO_PAGE || rowType == BUG_SHO_BOOK)                     //
                                               {item = ITEM_PAGE; perLine = 24;}//
    grpsPerItem = m_padSz[item] / TARGETBUS_SIZE;                               //groups per hITEM
    for (ii=kk=0, dupB=false; ii < (int)g_memRowSize; ii+=TARGETBUS_SIZE)       //
        {grp = ii / m_targetBusSz;                                              //
         if ((ii % perLine) == 0) Printf("   %03X: ", ii);                      //
         if ((grp % grpsPerItem) == (grpsPerItem-1) && rowType != BUG_SHO_RAW &&//
              DisplayableHex(rowP-2*m_keySize, buf))                            //
                 {Printf("\"%s\"         ", buf); rowP -= 2*TARGETBUS_SIZE;}    //
         else                                                                   //
             for (jj=0; jj < TARGETBUS_SIZE; jj++, rowP++)                      //
                Printf("%02X%s",*rowP, (jj&7)==7 ? "  " : (jj&3)==3 ? "_" : "");//
                                                                                //
         if ((kk+=TARGETBUS_SIZE) >= perLine)                                   //
            {Printf("\n"); kk=0;                                                //
             //suppress duplicate lines                                         //
             for (dupB=false; ii >= perLine && ii  < ((int)g_memRowSize-perLine)//not first and not last
                   && memcmp(rowP, rowP-2*perLine, 2*perLine) == 0;)            //
                 {dupB = true; ii += perLine; rowP -=2*perLine;}                //
             if (dupB) Printf("%13s%16s%16s\n", "\"\"", "\"\"", "\"\"");        //  
        }   }                                                                   //
    Printf("\n");                                                               //
    return 0;                                                                   //
   } //cEmulator::CheckF...

//We can default almost everything except the highlight colors.
char *cEmulator::RtfHeading(char *bufP)
   {strcpy(bufP,                                                                //
       "{\\rtf1\\ansi"                                                          //character set
       "{\\colortbl ;"                                         //0              // default color
                   "\\red150\\green250\\blue150;"              //1              //highlight1 color (pale green)
                   "\\red220\\green220\\blue220;"              //2              //highlight2 color (grey)
                   "\\red020\\green020\\blue220;"              //3              //highlight3 color (blue)
                   "\\red250\\green150\\blue200;"              //4              //highlight4 color (red)
                   "\\red255\\green128\\blue040;}\r\n");       //5              //highlight4 color (brown)
    return &bufP[strlen(bufP)];
   } //cEmulator::RtfHeading...
#define BK_WHITE   "\\highlight0 "
#define BK_GREEN   "\\highlight1 "
#define BK_GREY    "\\highlight2 "
#define BK_BLUE    "\\highlight3 "
#define BK_RED     "\\highlight4 "
#define BK_BROWN   "\\highlight5 "
#define FG_DEFAULT "\\cf0 "
#define FG_GREEN   "\\cf1 "
#define FG_GREY    "\\cf2 "
#define FG_BLUE    "\\cf3 "

//Format row with consideration for the type in m_rowFormat.
//Keys will be displayed in alpha, binary field in hex.
int cEmulator::GetFormattedRow(char *bufP, bool keysOnlyB)
   {int     grpsPerItem, grp, ii, jj, kk, perLine, item, lines=0, sz,           //
            itemsPerRow, rowFormat=m_rowFormat, keySize=8;                      //
    bool    badB, pageB, highlightedB=false, lineHeadB=true;                    //
    uint8_t*rowP = &m_bramP[m_curRow*g_memRowSize], *u8P;                       //
    char    buf[99], *pp;                                                       //
                                                                                //
    if (m_rowFormat == BUG_AUTODETECT)                                          //autodetect == 0
       {//Examine the row to determine what sort of data is there               //
        rowFormat = BUG_SHO_INDX;                                               //sho_indx = 3
        for (sz=keySize+sizeof(hINDX), pageB=false;; sz=keySize+sizeof(hPAGE), pageB=true)
            {itemsPerRow = g_memRowSize / sz;                                   //
             for (ii=0, badB=false; ii < itemsPerRow; ii++)                     //
                 {u8P = &rowP[ii*sz] + (pageB ? sizeof(hPAGE) : sizeof(hINDX)); //point to key field in hITEM structure
                  if (!DisplayableHex(u8P, buf)) {badB = true; break;}          //that's not a valid hINDX or hPAGE entry
                  if (*(u8P-1) & 0x80) break;                                   //end of row
                 }                                                              //
              if (!badB) goto go;                                               //nobody bitched; must be good
              rowFormat = BUG_SHO_PAGE;                                         //sho_page = 1
              if (pageB) break;                                                 //'for (sz=...' executed only twice.
             } //for (sz=...                                                    //
        rowFormat = BUG_SHO_RAW;                                                //
       }                                                                        //
 go:switch (rowFormat)                                                          //
       {case BUG_SHO_RAW:                                                       //raw  = 2
        case BUG_SHO_INDX: item = ITEM_INDX; perLine = 32; break;               //indx = 3
      //case BUG_SHO_BOOK:                                                      //
        case BUG_SHO_PAGE: item = ITEM_PAGE; perLine = 24; break;               //page = 1
       }                                                                        //
    grpsPerItem = m_padSz[item] / TARGETBUS_SIZE;                               //groups per hITEM
    RtfHeading(bufP);                                                           //
    pp = &bufP[strlen(bufP)];                                                   //
    if (rowFormat == BUG_SHO_RAW) keysOnlyB = false; else                       //
    if (keysOnlyB)                perLine  *= 2;                                //
    //Highlight each word for which m_highlight[word] is set.                   //
    for (ii=kk=0; ii < (int)g_memRowSize; ii+=TARGETBUS_SIZE)                   //
        {//Step thru row word by word                                           // 
         grp = ii / m_targetBusSz;                                              //
         if (lineHeadB)                                                         //
            {if( highlightedB) LOWLIGHT(pp); highlightedB = false;              //display word address of this line
             snprintf(pp, 32, "%03X: ", ii); pp += strlen(pp);                  //
             lineHeadB = false;                                                 //
            }                                                                   //
         if (m_rowFormat != BUG_SHO_RAW)                                        //no zero detect for raw format
         if (rowP[0] == 0 && memcmp(rowP, rowP+1, g_memRowSize-ii-1) == 0)      //
            {snprintf(pp, 32, " Zero to end"); pp += strlen(pp); break;}        //
         if (m_highlight & (1 << (ii / TARGETBUS_SIZE)))                        //should word be highlighted ?
              {if (!highlightedB) HIGHLIGHT(pp); highlightedB = true;}          //set   highlight (if needed) 
         else {if ( highlightedB) LOWLIGHT(pp);  highlightedB =false;}          //clear highlight (if needed)   
         pp += strlen(pp);                                                      //
         if (m_rowFormat == BUG_SHO_RAW)     goto hex;                          //
         if ((grp % grpsPerItem) == (grpsPerItem-1))                            //display keys in alpha
            {if (!DisplayableHex(rowP, buf)) goto hex;                          // well, um; maybe not
             snprintf(pp, 32, "\"%s\"%s" , buf, keysOnlyB ? "  " : "         ");//
             rowP += TARGETBUS_SIZE;                                            //
             pp   += strlen(pp);                                                //
            }                                                                   //
         else                                                                   //
            {if (keysOnlyB) {rowP += TARGETBUS_SIZE; continue;}                 //
 hex:        for (jj=0; jj < TARGETBUS_SIZE; jj++, rowP++, pp+=strlen(pp))      //display binary parts in hex
                 snprintf(pp, 32, "%02X%s",*rowP,                               //
                                       (jj&7)==7 ? "  " : (jj&3)==3 ? "_" : "");//spacing
            }                                                                   //
         //End of line processing                                               //
         if ((kk+=TARGETBUS_SIZE) >= perLine)                                   //
            {strcpy(pp, "\\line "); pp += 6;                                    //
             if ((++lines) >= 4 && perLine != 24)                               //!= 24 because textBox is limitted in size
                {strcpy(pp, "\\line "); pp += 6; lines = 0;}                    //
             *pp = 0; kk=0; lineHeadB = true;                                   //
        }   }                                                                   //
    *pp++ = '}'; *pp++ = '\r'; *pp++ = '\n'; *pp = 0;                           // } = RTF trailer
    m_highlight = 0;                                                            //
    return 0;                                                                   //
   } //cEmulator::GetFormattedRow...

int cEmulator::GetOpName(char *hexP, char *alphaP)
   {OPCODE op=m_pgmP[m_pc], nxt=m_pgmP[m_pc+1];                                 //
    const char *hereP, *thereP, *pp;                                            //
    int adr= IsLongJmp(op) ? nxt.u16               :                            //long jump to abs adr in next op
             IsGoOp(op)    ? op.go.relAdr + m_pc+1 :                            //short jump to relAdr
             IsCall(op)    ? op.call.callAdr       : -1;                        //call to absolute address in call opcode
    hereP  = GetLabel(m_pc);                                                    //
    thereP = GetLabel(adr);                                                     //
    pp     = m_opNameP->Show(m_pc, op, nxt, thereP, true, &m_pgmP[m_pc]);       //
    strncpy(alphaP, pp, _MAX_PATH);                                             //
    if (IsTwoWordOpCode(op, nxt, m_pgmP[m_pc+2]))
//  if (IsLongJmp(op) ||                                                        //
//     (op.ri.act == OP_RI && op.ri.breg == 0 &&                                //
//         (nxt.ldi.act == OP_LDI ||                                            //
//          nxt.rpt.act == OP_REPEAT && m_pgmP[m_pc+2].ldi.act == OP_LDI)))     //
       snprintf(hexP, _MAX_PATH, "%04X %04X", op.u16, nxt.u16);                 //
    else                                                                        //
       snprintf(hexP, _MAX_PATH, "%04X ____", op.u16);                          //
    return 0;                                                                   //
   } //cEmulator::GetOpName...

//Return source line number from pc; set offset of line within file
int cEmulator::XlatPC2Line(int pc, int *lineOffsetP) 
    {sSRC_REF *mapP, *endMapP=&m_lineMapP[m_lineMapSize]; int ii;
     if (pc < 0) pc = m_pc;
     for (mapP=m_lineMapP, ii=0; mapP < endMapP; mapP++, ii++)
        {if ((mapP+1) == endMapP || pc < (mapP+1)->pc)
             {if (lineOffsetP) *lineOffsetP = mapP->lineOffset; 
              return mapP->lineNum;
         }   }
     if (lineOffsetP) *lineOffsetP = -1;
     return 0;
    } //cEmulator::XlatPC2Line...

//Re-create source text with RTF header and various items highlighted.
//Value returned is the char-offset of a line five above the current line
int cEmulator::GetSourceAll(char *bufP, int line, int bufSize, bool noSelectB)  //
   {char *dP, *endDP=&bufP[bufSize]-32, *sP,*endSP=&m_sourceTextP[m_sourceSize];//
    const char*colP;                                                            //
    int        ll, cc, ii, kk, ww;                                              //
    bool       commentB, breakerB;                                              //
    static const char *wds[] =                                                  //
                          {"print", "for", "if", "else", "call", "ret", "goto"};//
                                                                                //
    dP = RtfHeading(bufP); dP += strlen(dP);                                    //
    for (sP=m_sourceTextP, ll=1; sP < endSP; sP++, ll++)                        //
        {for (ii=0, breakerB=false; ii < m_maxBreaks; ii++)                     //
             if (breakerB=(ll == m_breaks[ii])) break;                          //one of the break lines
         colP = (ll == line && !noSelectB && breakerB  ) ? BK_BROWN :           //highlight active & breakpoint line
                (ll == line && !noSelectB              ) ? BK_GREEN :           //          active
                (breakerB                              ) ? BK_RED   : "";       //          breakpoint or no color
         snprintf(dP, 32, "%s%04d: %s", colP, ll, colP != "" ? BK_WHITE : "");  //
         for (cc=0, commentB=false; *sP != '\n' && *sP != '\r' && *sP != 0 && sP < endSP;) //copy upto end of line
            {if ((dP+=strlen(dP)) >= endDP) goto bad;                           //
             if (strncmp(sP, "//", 2) == 0 && !commentB)                        //apply color to comments
                {strcpy(dP, BK_GREY ); dP += strlen(dP); commentB=true;}        //
             if ((cc == 0 || !Isalpha(*(sP-1))) && !commentB)                   //apply color to syntax words
                for (ww=0; ww < HOWMANY(wds); ww++)                             //
                   if (strnicmp(sP, wds[ww], kk=istrlen(wds[ww])) == 0 && !Isalpha(sP[kk]))
                      {strcpy(dP, FG_BLUE); strncat(dP, sP, kk); strcat(dP, FG_DEFAULT); //apply color
                       dP += strlen(dP); sP += kk; goto eLup;                   //
                      }                                                         //
             if (strchr("{}/\\", *sP)) *dP++ = '\\';                            //rtf ignores '\n' etc on display
             *dP++ = *sP++;                                                     //
        eLup:cc++;                                                              //
            } //for (... sP to end of line...                                   //
         if (commentB) strcpy(dP, BK_WHITE "\r\n"); else strcpy(dP, "\r\n");    //
         dP += strlen(dP);                                                      //
         if (sP[0] == '\r' && sP[1] == '\n') sP++;                              //double step over \r\n
         strcpy(dP, "\\line "); dP += strlen(dP);                               //append rtf notion of "\r\n"
        }                                                                       //
    *dP++ = '}'; *dP++ = 0; *dP = 0;                                            //
    return line;                                                                //
bad:Bugout("GetSourceAll ovflw\n"); return -ERR_1566;                           //1566 = buffer is too small for request
   } //cEmulator::GetSourceAll...

//Generate text for AssemblerBox when op-by-op mode is selected
int cEmulator::GetAssembler(char *bufP, int height, int width)
   {int         lines = height/20, pc, ii, tgt;                                 //
    OPCODE      op, nxt;                                                        //
    char       *dP;                                                             //
    const char *labelP;                                                         //
    pc = max(0, m_pc-lines/2);                                                  //start a bit before current OP
    if (pc > 0 && IsTwoWordOpCode(m_pgmP[pc-1], m_pgmP[pc], m_pgmP[pc+1])) pc--;//adjust for 64-bit opcode
    dP = RtfHeading(bufP); dP += strlen(dP);                                    //
    for (ii=0; ii < lines; ii++, pc++)                                          //
        {op = m_pgmP[pc];                                                       //
         if (pc == m_pc)                                                        //
              sprintf(dP, BK_GREEN "%03d: " BK_WHITE "%04X", pc, op.u16);       //highlight the current OP
         else sprintf(dP,          "%03d: %04X",             pc, op.u16);       //
         dP += strlen(dP);                                                      //
         if (IsTwoWordOpCode(op, nxt=m_pgmP[pc+1], m_pgmP[pc+2]))               //
              {sprintf(dP, "_%04X", nxt.u16); pc++;}                            //
         else {sprintf(dP, "_____");}                                           //  
         dP += strlen(dP);                                                      //
         if (IsLongJmp(op)) tgt = nxt.u16;                  else                //
         if (IsShortJmp(op))tgt = pc + ((int)op.go.relAdr); else                //
         if (IsCall(op))    tgt = op.call.callAdr;          else tgt = -1;      //
         labelP = (tgt >= 0) ? GetLabel(tgt) : NULL;                            //
         sprintf(dP, "  %s\\line ",                                             //RTF's notion of "\r\n"
                    m_opNameP->Show(pc, op, nxt, labelP, true));                //
         dP += strlen(dP);                                                      //
        }                                                                       //
    *dP++ = '}'; *dP = 0;                                                       //end RTF block
    return 0;                                                                   //
   } //cEmulator::GetAssembler...

//Get source text for current PC; prefix with "\\hightlight1 "
int cEmulator::GetSourceAtPc(char *bufP, int *lineP) 
   {int         len, line;                                                      //
    char       *pp;                                                             //
    const char *srcP=GetSourceLineP(m_pc, &line, &len);                         //
    bufP[0] = 0;                                                                //
    if (m_guiB) snprintf(bufP, 32, "\\hightlight1 %04d: ", line);               //
    memmove(pp=&bufP[istrlen(bufP)], srcP, len);                                //
    pp += len; *pp++ = '\r'; *pp++ = '\n'; *pp = 0;                             //RTF does not care
    *lineP = line;                                                              //
    return 0;                                                                   //
   } //cEmulator::GetSourceAtPc...

//Return source line at pc, line number and length of line
const char *cEmulator::GetSourceLineP(int pc, int *lineNumP, int *lenP)   
    {int srcOffset, len=0, ii, line; bool quoteB; char *srcP=NULL, *pp;         //
     if ((line=XlatPC2Line(pc, &srcOffset)) != 0)                               //get line number and offset in sourceBuf
        {srcP = &m_sourceTextP[srcOffset];                                      //beginning of line
         len  = (int)(strpbrk(srcP, "\r\n") - srcP);                            //
         for (pp=srcP, ii=0, quoteB=false; len-->= 0; pp++, ii++)               //
             {if (*pp == '\"') quoteB = !quoteB;                                //
              if (*pp == ';' && !quoteB) {len = ii+1; break;}                   //
        }    }                                                                  //
     if (lenP != NULL) *lenP = len;                                             //
     if (lineNumP) *lineNumP = line;                                            //
     return srcP;                                                               //
    } //cEmulator::GetSourceLineP...

//Return last message generated by print statement.
int cEmulator::GetMessages(char *bufP) 
   {int len= istrlen(m_printBuf);                                               //
    strncpy(bufP, m_printBuf, _MAX_PATH);                                       //copy to output buffer
    m_printBuf[0] = 0;                                                          //clear printBuf
    return len;                                                                 //
   } //cEmulator::GetMessages...

int cEmulator::GetSamReg    (int reg, char *bufP) 
   {snprintf(bufP, 22, "0x%08X_%08X",                                           //
                   (uint32_t)(samRegs[reg & 7] >> 32) & 0xFFFFFFFF,             //
                  ((uint32_t) samRegs[reg & 7])       & 0xFFFFFFFF);            //
    return 0;                                                                   //
   } //cEmulator::GetSamReg...
int cEmulator::SetSamReg(int reg, uint64_t u64) {samRegs[reg & 7] = u64; return 0;}
int cEmulator::SetPc(int pc)                    {return m_pc      = pc;}        //
int cEmulator::SetSp(int sp)                    {return m_sp      = sp;}        //
int cEmulator::SetCurRow(int row)               {return m_curRow  = row;}       //

int cEmulator::GetStack(int depth, char *bufP) 
   {snprintf(bufP, 22, "0x%08X_%08X", (uint32_t)(m_stak[depth] >> 32) & 0xFFFFFFFF, 
                                     ((uint32_t) m_stak[depth])       & 0xFFFFFFFF); 
    return 0;
   }

int cEmulator::GetCurOvly(char *bufP) 
   {if (m_curOvly == 0) strcpy(bufP, "none"); else snprintf(bufP, 20, "%d", m_curOvly); 
    return m_curOvly;
   } //cEmulator::GetCurOvly...

//Hardcore way to get GUI output to output window of VS
int cEmulator::DebugGui   (int locn, int line, const char *sP) {Bugout("locn=%d, line=%d, tgt=%s\n", locn, line, sP); return 0;}
int cEmulator::GetPc      (char *bufP) {if (bufP) snprintf(bufP, 22, "%05d", m_pc);       return m_pc;}
int cEmulator::GetSp      (char *bufP) {if (bufP) snprintf(bufP, 22, "%3d",  m_sp);       return m_sp;}
int cEmulator::GetCurRow  (char *bufP) {if (bufP) snprintf(bufP, 22, "%3d",  m_curRow);   return m_curRow;}
int cEmulator::GetClkCount(char *bufP) {if (bufP) snprintf(bufP, 22, "%d",   m_clkCount); return m_clkCount;}
int cEmulator::GetInsPoint(char *bufP) {if (bufP) snprintf(bufP, 22, "%d",   m_insPt);    return m_insPt;}
int cEmulator::GetFileName(char *bufP) 
   {if (!bufP)                    return -ERR_0001;
    if (this == NULL)             MessageBoxA(NULL, "this=NULL", "*", MB_OK); else
    if (m_samFilePrefixP == NULL) MessageBoxA(NULL, "no file",   "*", MB_OK); else
       {snprintf(bufP, _MAX_PATH, "%s.bin", m_samFilePrefixP); return 0;} 
    strcpy(bufP, ""); return -ERR_0003;
   } //cEmulator::GetFileName...

//return English and binary of SamPU status register
int cEmulator::GetSamStatus(char *binP, char *textP)
   {uint8_t samStat = *(uint8_t*)&m_samStatus;
    snprintf(binP, 64, "%d%d%d%d%d", (samStat & 16) != 0, (samStat & 8) != 0,
                  (samStat & 4) != 0, (samStat &  2) != 0,  samStat & 1);
    snprintf(textP, 64, "%s", m_opNameP->ConditionNames(samStat, false));
    return 0;  
   } //cEmulator::GetSamStatus...

int cEmulator::PutSamReg   (int reg, uint64_t data)  {samRegs[reg & 7] = data;      return 0;}//
int cEmulator::PutStack    (int depth, uint64_t data){m_stak[depth]    = data;      return 0;}//
int cEmulator::PutSamStatus(uint8_t u8)              {*(uint8_t*)&m_samStatus = u8; return 0;}//

//Set/clear break point. 
//pgmP = RTF text of program, offset = +/- beginning of line RTF text.
//Get line number from RTF text and add/subtract said line from m_breaks[]
//pgmP == NULL clear all breaks.
int cEmulator::SetBreakPoint(const char *pgmP, int offset)
   {char *pp; int line, ii;                                                 //
    if (pgmP == NULL)                                                       //
       {memset(m_breaks, m_maxBreaks=0,sizeof(m_breaks)); return 0;}        //
    line = strtol(&pgmP[offset >= 0 ? offset : -offset], &pp, 10);          //read line number
    for (ii=0; ii < m_maxBreaks; ii++)                                      //
        if (m_breaks[ii] == line) {m_breaks[ii] = 0; return line;}          //break already exist; clear and return line #
    //Find an unused slot in m_breaks[]                                     //
    m_maxBreaks = min(m_maxBreaks+1, HOWMANY(m_breaks));                    //
    for (ii=0; ii < m_maxBreaks; ii++)                                      //
        if (m_breaks[ii] == 0)                                              //break already exist
           {m_maxBreaks = max(m_maxBreaks, ii+1);                           //
            return m_breaks[ii] = line;                                     //set break, return line num
           }                                                                //
    return -1;                                                              //too many breaks
   } //cEmulator::SetBreakPoint...

//Convert two hex digts to their binary value                               //
uint32_t cEmulator::Hex2Bin(CC pp)                                          //
   {char hex[3];                                                            //
    hex[0] = pp[0]; hex[1] = pp[1]; hex[2] = 0;                             //
    return (uint32_t)strtol(hex, NULL, 16);                                 //
   } //cEmulator::Hex2Bin(...

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

#if 0
//Assemble $expect: and $actual: messages up to the next '\n' then compare
//them against each other. The text of the OP_PRINT message is stored in
//inputFileName.messages, juiced up by the values of various registers
//denoted by $digit in the message, eg., print "reg[0]=$0\n";
int cEmulator::OldPrint(int pc, OPCODE op, int bugLevel) 
   {char *msgP, *pp, ch[256]; const char *ppc; int len=0, line=0;               //
    if (!(msgP=(char*)m_opNameP->FindMessage(op.g.adr))) return 0;              //bull shit: message must exist
    for (len=m_printBuf[0]=0; *msgP; msgP++)                                    //
        {if (*msgP == '$')                                                      //
             {if (strncmp(msgP, "$$",     2) == 0)                              //
                 {m_printBuf[len++] = *msgP++; *msgP = 0;}                      //
             else                                                               //
              if (strnicmp(msgP, "$line", 5) == 0)                              //
                 {snprintf(&m_printBuf[len], sizeof(m_printBuf)-len-1, "0x%X",  //
                                                   XlatPC2Line(pc));       //
                  msgP += 4;                                                    //
                 }                                                              //
             else                                                               //
              if (strnicmp(msgP, "$pc", 3) == 0)                                //
                 {snprintf(&m_printBuf[len],sizeof(m_printBuf)-len-1,"0x%X",pc);//
                  msgP += 2;                                                    //
                 }                                                              //
             else                                                               //
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
                m_actualB = false; *msgP = 0;                                   // 
                for (char *eP=m_expect, *aP=m_actual; ;)                        //compare expect and actual
                   {while (*eP == ' ') eP++; while (*aP == ' ') aP++;           //ignore white space
                    if (*eP == 0 && *aP == 0) 
                       {m_expect[0] = m_actual[0] = 0; return 0;}               //
                    if ((*eP++ | 0x20) == (*aP++ | 0x20)) continue;             //ignore case
                    return -ERR_2739;                                           //2739 = #expect: <value> not equal to #actual: <value> in simulation.
       }        }                                                               //
    return 0;                                                                   //
   } //cEmulator::OldPrint...
#endif

//Execute user print, $expect, or $actual opcodes as specified by 
//op.g.breg = OPV_PRIT, OPV_EXPECT, OPV_ACTUAL, OPV_EXPECT/ACTUAL_END.
//Index into string pool = pop.g.adr; 
//$<digit>', '$pc', and '$line' are replaced by their current values. 
//Output is stored in m_printBuf, m_expect, or m_actual.
//Op==OPV_ACTUAL_END, m_expect and m_actual are compared and may cause error 2739.
int cEmulator::DoOpPrint(int pc, OPCODE op, int bugLevel)
   {char       *dP, *d1P;                                                       //
    const char *pp, *msgP;                                                      //
    int         len=0, line=0, sz, ii, breg;                                    //
    if (bugLevel <= 1) return 0;                                                //level = 0, no output
    sz       = (int)sizeof(m_printBuf)-3;                                       //
    dP = d1P = m_printBuf;                                                      //print
    switch (breg=op.g.breg)                                                     //
       {case OPV_EXPECT:                                                        //
            dP = d1P = m_expect; if (m_expectB=(bugLevel >= 2)) goto ae; break; //ignore is buglevel < 2
        case OPV_ACTUAL:                                                        //
            dP = d1P = m_actual; if (m_actualB=(bugLevel >= 2)) goto ae; break; //ignore is buglevel < 2
        ae: dP += strlen(dP); //fall thru                                       //concatenate to m_expect/m_actual
        case OPV_PRINT:                                                         //
            if (!(msgP=m_opNameP->FindMessage(op.g.adr)))          break;       //bull shit: message must exist
            for (len=0; *msgP; )                                                //
                {if (*msgP == '$')                                              //
                    {if (strBegins(msgP, pp="$$")) {*dP++ = '$'; *dP = 0;}      //$$ is single $ in output
                     else                                                       //
                     if (strBegins(msgP, pp="$line"))                           //$line outputs current line
                         snprintf(dP, sz, "0x%X", XlatPC2Line(pc));        //
                     else                                                       //
                     if (strBegins(msgP, pp="$pc")) snprintf(dP, sz, "0x%X",pc);//$pc output current pc
                     else                                                       //
                     if (msgP[1] >= '0' && msgP[1] <= '0'+MAX_REGS-1)           //$<digit> outputs register
                        {snprintf(dP, sz, "%llX", samRegs[(msgP[1] & 7)]); pp = "$0";}//
                     dP   += (ii=istrlen(dP)); len+=ii; sz -= ii;               //
                     msgP +=strlen(pp);                                         //
                    } // '$'...                                                 //
                 else {*dP++ = *msgP++; len++; sz--;}                           //not $
                 *dP = 0;                                                       //
                 if (len > sizeof(m_printBuf)-10)                               //== sizeof(m_expect/actual)
                    {m_actualB=m_expectB=false; return Error(ERR_1566,d1P,"");} //1566 = Buffer is too small to perform requested operation.
                } //for (len=                                                   //
            break; //breg == OPV_PRINT, EXPECT or ACTUAL...                     //
        case OPV_EXPECT_END: m_expectB = false; break;                          //OP_END_EXPECT; stop collecting strings
        case OPV_ACTUAL_END: m_actualB = false;                                 //OP_END_ACTUAL; now the fun starts
            for (char *eP=m_expect, *aP=m_actual; ;)                            //compare expect and actual
               {while (*eP == ' ' || *eP == '\n') eP++;                         //ignore white space
                while (*aP == ' ' || *aP == '\n') aP++;                         //ignore white space
                if (*eP == 0 && *aP == 0) break;                                //compared successfully
                if ((*eP++ | 0x20) != (*aP++ | 0x20))                           //compare ignoring case
                    return m_expectedVersusActual = -ERR_2739;                  //2739 = $expect: <value> not equal to $actual: <value> in simulation.
               } //for (char ...                                                //
            m_expect[0] = m_actual[0] = m_expectedVersusActual = 0; break;      //clear for next $expect/$actual
       } //switch (breg...                                                      //
    return 0;                                                                   //
   } //cEmulator::DoOpPrint...

//$expect versus $actual has failed; prepare message
int cEmulator::GetExpectedVersusActual(char *bufP)
   {if (m_expectedVersusActual == 0) return bufP[0] = 0;                        //
    snprintf(bufP, _MAX_PATH, "Expect: %s\nActual: %s\n", m_expect, m_actual);  //
    m_expectedVersusActual = 0; m_expect[0] = m_actual[0] = 0;                  //
    return -ERR_2739;                                                           //2739 = $expect: value not equal to $actual: value.
   } //cEmulator::GetExpectedVersusActual...

int cEmulator::Execute1(int pc, OPCODE op)
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
                cond = 0, itemCnt, itemSz, repete=op.rpt.count+1, cc, erC;      //
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
       case OP_BUG:  if (op.g.breg == 0) return OpBug(pc, op);                  //stop, $bug_raw, $bug_indx, or $bug_page
     //case OP_PRINT:                                                           //
                     if ((erC=DoOpPrint(pc, op, m_bugLevel)) < 0) return erC;   //-ve means m_expect != m_actual
                     return pc+1;                                               //complicit coding; host sees OP_PRINT and acts
       case OP_CFG_G: case OP_CFG_C:                                            //
       case OP_RET:  if (op.g.breg != 0) return Configuration(pc, op);          //
                     if (m_sp < 1 || m_sp >= HOWMANY(m_stak)) goto stakOvflw;   //
                     pc         = (m_stak[--m_sp]+1) & 0xFFFF;                  //
                     m_bugLevel = (m_stak[m_sp]     >> 16) & 31;                //restore bug level at call
                     if (pc >= m_pgmSize) goto pgmOvflw;                        //
                     return pc;                                                 //
       case OP_REPREG:repete  = (int)samRegs[breg];                             //
       case OP_REPEAT:m_stepA = op.rpt.stepA != 0;                              //
                     m_stepR  = op.rpt.stepR != 0;                              //
                     m_dirn   = op.rpt.bkwd ? -1 : +1;                          //
                     m_repete = repete;                                         //
                     return pc+1;                                               //
       case OP_CROWI:if (m_curRow != adr) m_highlight = 0; m_curRow = adr;break;//
       case OP_CROW: if (m_curRow != (int)samRegs[op.ind.areg]) m_highlight = 0;// 
                     if (m_curRow >= m_bramRows) goto badRow;                   //
                     m_curRow      = (int)samRegs[op.ind.areg];                 // 
                     return pc+1;                                               //
       case OP_ARITH:if (!IsExtendedOp(op) || subOp == OPS_CMPS)                //
                        {tReg = Arithmetic(op);                                 //
                         if (subOp != OPS_CMP && subOp != OPS_CMPS)             //
                                                   samRegs[areg] = tReg; break; //other arithmetic operations 
                        }                                                       //
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
                     itemSz  =((itemSz + TARGETBUS_SIZE-1) / TARGETBUS_SIZE) * TARGETBUS_SIZE;//round to targetbus_size
                     fullB   =((hINDX*)(rowP +(itemSz * (itemCnt-1))))->stop != 0;//stop bit of last item
                     if (fullB && scinB)                                        //
                        {m_samStatus.cc = m_samStatus.nc = 1; break;}           //
                     if (op.sc.rowType > 1) goto badType;                       //
                     bb      = hSequencer(rowP, itemCnt, itemSz,                //row, count(hITEM[]), sizeof(hITEM)
                                (uint8_t*)&samRegs[op.sc.breg],                 //target of scan
                                (op.sc.rowType ? sizeof(hINDX) : sizeof(hPAGE)),//offsetof(hITEM::key)
                                m_keySize, &prev, &locn);                       //sizeof(key), and outputs 
                     m_insPt        = locn * (cc=itemSz / TARGETBUS_SIZE);      //
                     m_samStatus.cc = m_samStatus.nc = 0;                       //
                     if (bb && scinB)                                           //
                        {m_highlight |= ((1 << cc)-1) << m_insPt;               //
                         InsertKey(op, itemCnt, itemSz, locn, (uint8_t*)&samRegs[op.sc.breg]);//
                        }                                                       //
                     break;                                                     //
       case OP_WRYT:                                                            //       
       case OP_READ: if (m_curRow >= m_bramRows)   goto invalidBram;            //
                     if (op.g.adr >= m_grpsPerRow) goto badAdr;                 //
                     if (op.g.act == OP_WRYT)                                   //
                         {m_highlight       |= 1 << op.g.adr;                   //
                          u64P[op.g.adr]     = samRegs[breg];                   //
                         }                                                      //
                     else samRegs[op.g.breg] = u64P[op.g.adr];                  //
                     break;                                                     //
       case OP_GOVLY:if (m_pgmP[pc+1].g.adr != 0) goto badOvly;                 //== 0 if ordinary unconditional jump
                     pc = m_pgmP[pc+1].u16-1; break;                            //-1 to keep addressCheck happy
       case OP_GO_T: case OP_GO_T+8: case OP_GO_T+16: case OP_GO_T+24:          //go_t(0) is unconditional goto
       case OP_GO_F: case OP_GO_F+8: case OP_GO_F+16: case OP_GO_F+24:          //go_f(0) is noop
                     m_samStatus.qrdy = ((random++) & 5) == 4;                  //'random' setting for qrdy
                     if (((*(uint8_t*)&m_samStatus & op.go.cond) == op.go.cond) //
                          == (op.go.act == OP_GO_T))                            //take  jump
                         pc = ((adr & 0x80) ? pc - (255-adr) : pc+adr+1)-1;     //-1 to keep addressCheck happy
                     break;                                                     //spurn jump
       default:      goto illegalOp;                                            //
      }                                                                         //
    goto addressCheck;                                                          //
addressCheck: if (++pc >= m_pgmSize) goto pgmOvflw; return pc;                  //

badOvly:    Printf("##Abort(emu):Invalid overlay number=$d\n",  op.g.adr);      return -1;
badType:    Printf("##Abort(emu):Invalid rowType=$d\n",         op.sc.rowType); return -1;
badAdr:     Printf("##Abort(emu):Invalid memory address =$d\n", op.g.adr);      return -1;
badRow:     Printf("##Abort(emu):Invalid current Row=$%d\n",    m_curRow);      return -1;
illegalOp:  Printf("##Abort(emu):Invalid op pc=%d, op=0x%04X",  pc, op.u16);    return -1;
stakOvflw:  Printf("##Abort(emu):Stack overflow @ pc = %5d\n",  pc);            return -1;
pgmOvflw:   Printf("##Abort(emu):Invalid program address = %05d\n", pc);        return -1;
invalidBram:Printf("##Abort(emu):BRAM[%d] is current row=%d\n", m_curRow);      return -1;
    } //cEmulator::Execute1...

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
    if (pc == stopPlz)                                                          //
       pc = pc;                                                                 //
#if 0//def _DEBUG                                                               //
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
        m_highlight |= 1 << theAddr;                                            //
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
       m_highlight |= 3 << theAddr;
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
    int64_t  sl64= l64, sr64=r64;                                               //
    U64      L64 = *(U64*)&l64,            R64 = *(U64*)&r64;                   //parameters as pairs of u32
    static   uint32_t ccc=0;                                                    //
                                                                                //
    m_samStatus.cc = 0; m_samStatus.nc = 1;                                     //maybe set later on
    switch (op.arith.subOp)                                                     //
        {case OPS_ADC: u0  = ccc;                                               //
         case OPS_ADD: wordO = l64 + r64 + u0;                                  //
                       ccc = CarryAdd32(L64.lo, R64.lo,u0+((wordO & mid64)!=0));//
                       ccc = CarryAdd32(L64.hi, R64.hi, ccc);                   //
                       m_samStatus.cc = ccc;        m_samStatus.nc = 1-ccc;     //
                       m_samStatus.zz = (wordO==0); m_samStatus.nz = (wordO!=0);//
                       return wordO;                                            //
         case OPS_CMPS:m_samStatus.cc = sr64 > sl64; wordO = l64 - r64; break;  //
         case OPS_XSBB:XCHG(l64, r64, tmp); u0 = ccc;            goto case_SUB; //
         case OPS_XSUB:XCHG(l64, r64, tmp);         //fall thru                 //
         case OPS_CMP: ccc = 0;                     //fall thru                 //
         case OPS_SBB: u0  = ccc;                   //fall thru                 //
         case_SUB:                                                              //
         case OPS_SUB: wordO = l64 - r64 - u0;                                  //
                       ccc = CarrySub32(L64.lo, R64.lo,u0+((wordO & mid64)!=0));//
                       ccc = CarrySub32(L64.hi, R64.hi, ccc);                   //
                       m_samStatus.cc = ccc;        m_samStatus.nc = 1-ccc;     //
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
    m_samStatus.zz = (wordO == 0); m_samStatus.nz = 1-m_samStatus.zz;           //
    m_samStatus.nc = 1-m_samStatus.cc;                                          //
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

//Get filename from fileNum returns in IATOM
const char *cEmulator::FileNameOnly(const char *fullNameP)
  {const char *pp=fullNameP, *qq=pp;
    while (pp=strpbrk(qq, "\\/")) qq=pp+1; //find last '\' or '/'
    return qq;
  } //cEmulator::StripFileName...

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

#ifdef _DEBUG
void cEmulator::Bugout(char const *fmtP,...)
    {va_list arg;                                                               //
     char    buf[512];                                                          //
   //if (m_bugLevel <= 2) return;                                               //sworn to silence
     va_start(arg, fmtP);                                                       //
     vsnprintf(buf, sizeof(buf)-1, fmtP, arg);                                  //
     va_end(arg);                                                               //
     buf[sizeof(buf)-1] = 0;                                                    //
     OutputDebugStringA(buf);
    } //cEmulator::Bugout...

#else
void cEmulator::Bugout(char const *fmtP,...) {}
#endif

int cEmulator::Bugger(const char *fmtP, int p1, int p2, int p3)
    {Printf(fmtP, p1, p2, p3); return 0; } //cEmulator::Bugger..

//Convert keyP from alpha to hex unless the key is all displayale characters.
//If keyP translates to displayable characters that is what will be copied into outP.
//If keyP cannot be so translated, then it will be displayed in hex.
//Return true if result is displayable.
bool cEmulator::DisplayableHex(const uint8_t *key8P, char *outP)
   {char       *dP=outP, *keyP = (char*)key8P;                                  //
    const char *sP;                                                             //
    int         ii, jj;                                                         //
    bool        displayableB=true, undefinedB=false;                            //
                                                                                //
    if (keyP[0] == '0' && keyP[1] == 'x') keyP += 2;                            //ignore leading 0x
    for (ii=0, jj=0, dP=outP, sP=keyP; ii < m_keySize; ii++, jj++)              //
        {if ((dP[jj]=sP[ii]) < ' ') displayableB = false;}                      //
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
   {int    grpsPerItem, grp, ii, jj, kk, row=m_curRow, perLine, item, fnc=op.bug.fnc;//
    char   buf[99];                                                             //
    uint8_t *startRowP, *rowP=(uint8_t*)&m_bramP[m_curRow*g_memRowSize];        //point to row
    bool   dupB;                                                                //
    static const char *rowType[]={"", "$BUG_PAGE", "$BUG_RAW", "$BUG_INDX"};    //
    static int padSz[] = {16, 2*TARGETBUS_SIZE,3*TARGETBUS_SIZE};               //
    static int perLyne[] = {32, 24, 32, 32};                                    //raw, hPAGE, hINDX, hINDX
    static int eyetem[]  = {0, ITEM_PAGE, 0, ITEM_INDX};                        //
                                                                                //
    pc++;                                                                       // 
    if (m_guiB)                                                                 //
       {if (fnc == 0)                                          return -ERR_2704;//2704 = stop encountered
        if (fnc == OP_SET_BUG) {m_bugLevel = op.bug.level;     return pc;}      //$bug = constant; set buglevel from opcode
       } //op.bug.fncs...                                                       //
    if (m_guiB) return pc;                                                      //
    //Non gui case; display curRow in requested format by fnc:                  //
    if (fnc == 0) {Printf("##Endof Simulation @pc=%d", pc); return -ERR_2704;}  //
    Printf("Row[0x%X], op=", row);                                              //
    if (fnc == 4) Printf("$bug = %d", op.bug.level);                            //
    else          Printf("%s",        rowType[fnc & 3]);                        //
    Printf("(0x%04X)\n", op.u16);                                               //
    perLine     = perLyne[fnc & 3];                                             //
    item        = eyetem [fnc & 3];                                             //
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

//return true if opcode is $expect or $actual
bool cEmulator::IsExpectOp(OPCODE op)
   {int breg = op.g.breg;
    return op.g.act == OP_PRINT &&                                              //never displayed
              (breg == OPV_EXPECT || breg == OPV_EXPECT_END ||                  //      "
               breg == OPV_ACTUAL || breg == OPV_ACTUAL_END);                   //
   } //cEmulator::IsExpectOp...

/*This routines calls Execute1() one or more times to interpet 'one OPCODE'. 
 Loops may be puntuated by returns to the GUI to under various conditions. 
 mode = EX_RUN:    Continue uninterrupted until OP_STOP or some other situation
                   requires the attention of the GUI.
        EX_2PC:    Run until $pc  == tgt
        EX_2REG<i>:Run until $<i> == tgt
        EX_2LINE:  Run until $line== tgt
        EX_BYLINE: Run line-by-line, ie., until line number changes. However,
                   OP_CALL does m_retLevel++ and swithces to EX_RUN (auto-run).
                   OP_RET  does m_retLevel--; if m_retLevel == 0 is encountered 
                           mode reverts to the mode setting prevailing at OP_CALL.
        EX_2BREAK: Run until next $bug opcode (no longer used).
        EX_2RET:   Run up to the next ret  opcode.
        EX_2CALL:  Run up to the next call opcode.
        EX_BYOP:   Execute opcode-by-opcode; return each opcode (Single step).
 'one opcode' is a figure of speech embracing:
    OP_REPEAT  OP_WHATEVER                 is  treated as single op.  
    OP_RI $0;  multiple OP_LDIs            are treated as single op (long literal).
    OP_RI $0;  OP_REPEAT; OP_LDI           variant of the above     (long literal).
    OP_EXPECT, OP_ACTUAL, OP_EXPECT_END, and OP_ACTUAL_END are executed silently.
       They function like OP_PRINT but capture their output in private buffers.
       On OP_ACTUAL_END these buffers are compared for equality.
    OP_PRINT exits under any mode to allow the GUI to output its message. 
    This means that the state of the execution loop must be reconstructed when
    the GUI re-enters ExecutePgm:
       m_lastExit, m_atLeastOneLineB, and m_oldLine are used for this purpose.
    m_lastExit ==
      == (-2700) simple print-pause to allow the GUI to output message.
      == (-2701) one of the EX_2... family has found its target.
      == (-2702) a print-pause inside an auto-run.
 Special care must be taken for the OP_print code:
    If it is followed by $expect/$actual these must be flushed out before returning.
    (Otherwise the GUI will throw up a tasteless OP_EXPECT or OP_ACTUAL on restart.)*/
int cEmulator::ExecutePgm(EX_MODE mode, uint64_t tgt)
   {int        nxt, breg, ii;                                                   //
    static int linePlz=99999999;                                                //debugging break at line
    uint64_t   longLit;                                                         //
    OPCODE     op;                                                              //
    bool       skipB;                                                           //
                                                                                //
    //reset state of execution-loop unless restarting inside auto-run           //
    if (m_lastExit != (-ERR_2702))                                              //last was a not print-pause; must
       {m_atLeastOneLineB = false; m_oldLine = m_curLine;}                      //   reinitialize state variables.
    //Condition input tgt                                                       //
    if (mode == EX_2LINE)                                                       //make sure line has valid SamCode 
	    tgt = AdjustLineNum((int)tgt);                                          // otherwise stop will miss
    //Execution loop                                                            //
    for (m_lastExit=0, nxt=m_pc;;m_pc=nxt)                                      //
        {op = m_pgmP[m_pc]; breg = op.g.breg; m_curLine = XlatPC2Line(m_pc);    //
         if (m_curLine == linePlz)                                              //
            m_pc = m_pc;                                                        //software break point
         m_atLeastOneLineB |= m_curLine != m_oldLine;                           //
    //check stop triggers                                                       //
         if (skipB=IsExpectOp(op))                        {}            else    //$expect or $actual
         if (mode == EX_2PC && m_pc == tgt)              goto x2701;    else    //
         if (mode >= EX_2REG0 && mode <= EX_2REG7 &&                            //
                         samRegs[mode-EX_2REG0] == tgt)  goto x2701;    else    //
         if (mode == EX_2LINE && m_curLine   == tgt &&                          //mode=11   run to line
                           m_atLeastOneLineB)            goto x2701;    else    //   (dont get stuck on same line)
         if (mode == EX_2RET  && op.u16      == OP_RET)  goto x2701;    else    //mode=12   run to RET
         if (mode == EX_2CALL && op.call.act == OP_CALL) goto x2701;    else    //mode=15   run to CALL
         if (mode == EX_BYLINE && m_oldLine != m_curLine &&                     //mode=13   run line by line
                           m_atLeastOneLineB)            goto x2701;            //2701=requested stop condition succeeded
         if ((mode == EX_RUN || mode == EX_2LINE || mode == EX_2PC) && m_atLeastOneLineB)
            for (ii=0; ii < m_maxBreaks; ii++)                                  //check for any breaks specified
                if (m_curLine == m_breaks[ii])           goto x2701;            //
    //execute the opcode                                                        //
         if ((ii=LongLiteral(m_pc, &longLit)) > 0)                              //returns count of opcodes in longLit
            {if (mode == EX_2PC && tgt >= m_pc && tgt < (m_pc+ii))goto x2701;   //stop in the middle of longLit !)
             samRegs[0] = longLit; nxt = m_pc + ii;                             //
            }                                                                   //
         else                                                                   //
         if (IsCall(op) && (mode == EX_BYLINE || mode == EX_BYOP))              //Supress debugging until return encountered
            {m_saveMode = mode; mode = EX_RUN; m_retLevel++; continue;}         //   save mode, switch to run mode
         else                                                                   //NOTE: GUI must help out this supression
         if (op.u16 == OP_RET && mode == EX_RUN && (--m_retLevel) == 0)         //returned to level of call   
            {mode = m_saveMode; goto doit;}                                     //   restore mode and restart from the top
         else                                                                   //
doit:    if ((nxt=Execute1(m_pc, op)) < 0)                   return nxt;        //return if error :(
         if (mode == EX_STEP_INTO)                                              //Called when poised over call to step into
            {mode = (EX_MODE)tgt;                                               //subroutine. Switch gears from step into
             if (mode == EX_2LINE) mode = EX_BYLINE; else                       // back to previous stepping mode
             if (mode == EX_2PC  ) mode = EX_BYOP;                              //
            }                                                                   //
    //post processing                                                           //
         if (IsPrintOp(op) && m_bugLevel >= 1)                                  //
            {//Before we yeild to the GUI, flush out any $expect/$actual.       //
             //Otherwise the GUI calls OpName with an $expect/$actual opcode.   //
             memmove(m_savePrint, m_printBuf, sizeof(m_savePrint));             //$expect/$actual clears printBuf
             while (IsExpectOp(op=m_pgmP[m_pc=nxt])) nxt = Execute1(m_pc++, op);//
             memmove(m_printBuf, m_savePrint, sizeof(m_printBuf));              //  
             return m_lastExit = -(m_retLevel <= 0 ? ERR_2700 : ERR_2702);      //2700/2702 = pause for GUI to display
            }                                                                   //
    //Interpret OP_REPEAT                                                       //
         for (op=m_pgmP[nxt]; m_repete != 0; m_repete--)                        //OP_REPEAT
             {if ((nxt=Execute1(m_pc+1, op)) < 0)     return nxt;               //ex opcode m_repete times, return if error
              if (m_stepA) op.g.adr += m_dirn;                                  //      increment of address
              if (m_stepR) op.g.breg+= m_dirn;                                  //            and/or register
             }                                                                  //
         if (skipB)                                       continue;             //
         if (IsExpectOp(op))                              continue;             //eat up $expect/$actual
         if (mode == EX_BYLINE && m_oldLine != m_curLine) return m_pc = nxt;    //mode = 13 line by line
         if (mode == EX_BYOP)                             return m_pc = nxt;    //mode = 1  opcode by opcode
        } //for (m_lastExit=0;...;m_pc=nxt)...                                  //
    return nxt;                                                                 //exit from ExecutePgm
x2701:m_retLevel = 0; return m_lastExit = -ERR_2701;                            //reg == tgt inside auto-run; cancel auto-run
   } //cEmulator::ExecutePgm...

//OP_RI $0; multiple OP_LDIs   are treated as single ops.
//OP_RI $0; OP_REPEAT; OP_LDI  is a slightly more compact variant of the same.
int cEmulator::LongLiteral(int pc, uint64_t *resultP)
   {OPCODE op=m_pgmP[pc]; int repete;
    if (op.g.act != OP_RI || op.g.breg != 0) return 0;                          //not a long literal
    *resultP = op.g.adr;                                                        //load and clear top end
    op       = m_pgmP[++pc];                                                    //
    if (op.ldi.act == OP_LDI)                                                   //$0=lit; op_ldi *
       {for (; (op=m_pgmP[pc]).ldi.act == OP_LDI; pc++)                         //assemble all the OP_LDIs
             *resultP  = (*resultP  << 14) | op.ldi.imm;                        //
        return pc - m_pc;                                                       //howmany did we chew up
       }                                                                        //
    if (op.rpt.act == OP_REPEAT && m_pgmP[++pc].ldi.act == OP_LDI)              //$0=lit; repeat(op_ldi)
       {for (repete=op.rpt.count; repete-- >= 0;)                               //
             *resultP  = (*resultP  << 14) | m_pgmP[pc].ldi.imm;                //
        return 3;                                                               //ri, rpt, and ldi - that's 3 opcodes
       }                                                                        //
    return 0;                                                                   //not a long literal
   } //cEmulator::LongLiteral...

//Static entry point
int cEmulator::SamCommandLine(const char **paramsPP, int countParams, char *fNameOutP)
   {int erC=0;
    snprintf(fNameOutP, _MAX_PATH, "%s\\%s.capFile", getenv("samObjectDir"), paramsPP[1]);
    if (countParams < 0) return 0;                                              //Useful to rturn file name 
    if (!(g_printFileP=fopen(fNameOutP, "wb")))                                 //
       {MessageBoxA(NULL, fNameOutP, "Error 7132 Creating log file", MB_OK); erC = ERR_7132;} //7132 = error opening file
    fclose(g_printFileP);                                                       //
    exit(erC);                                                                  //terminate GUI
   } //cEmulator::SamCommandLine...

//end of file

