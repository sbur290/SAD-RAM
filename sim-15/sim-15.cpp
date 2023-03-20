/* Program sim-<version>.cpp : Version 13, Jan 2023.
   See Help() for operational information.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <fcntl.h>
#include <crtdbg.h>
#ifdef _WIN32
    #include <direct.h>
    #define getcwd _getcwd
#elif
    #include <unistd.h>
#endif
#include <conio.h>
#include "sim-15.h"
#include "simBug.h"
#include "compile.h"
#include <C3_always.h>
#include <hStructures.h>
#include <C3_errors.h>

#define p m_params
#define ERR(a) {erC = a; goto err;}

cSamError g_err;
#ifdef _DEBG
   bool g_alwaysMessageBoxB; //true=always use messagebox for errors
#else
   bool g_alwaysMessageBoxB; //true=always use messagebox for errors
#endif

extern int EmulateMicrocode(cSimDriver *simP, int keySize, const char *objFileP, const char *msgFileP);

using namespace std;                                                            //
char             g_exeFileDir[_MAX_PATH];                                       //directory of sim.exe
FILE            *g_bugFileP,                                                    //print output file for debugging
                *g_printFileP=NULL;                                             //required for Printf
uint32_t         g_bugLevel=0, g_memRowSize=256;                                //default
sRECORD         *g_dataP;                                                       //
cBugIndex       *g_bugP;                                                        //
cBugSim         *g_bugSimP;                                                     //
uint8_t         *g_bramP;                                                       //

#define SHO_ERR() ShowNtError(__FUNCTION__, __LINE__)
#ifdef _DEBUG
   #define BUGOUT if (IsDebuggerPresent()) OutputDebugStringA
#else
   #define BUGOUT {}
#endif

//Convert two hex digts to their binary value
inline uint32_t Hex2Bin(const char *pp)
   {char hex[3];
    hex[0] = pp[0]; hex[1] = pp[1]; hex[2] = 0;
    return (uint32_t)strtol(hex, NULL, 16);
   } //Hex2Bin(...

//convert ch to displayable format at outP; return # chars in output

//append regP=hex of target as string
void cSimDriver::FormatAsString(char *bufP, int bufSize, const char *regP)
   {int len=istrlen(bufP); uint8_t u8; const char *qq;                          //
    bufP[len++] = '='; bufP[len++] = '\"';                                      //
    for (qq=Endof((char*)regP); (qq-=2) >= regP;)                               //
        if (len >= bufSize-3) break;                                            //
        else bufP[len++] = (u8=Hex2Bin(qq)) >= 0x20 && u8 < 0x80 ? u8 : '.';    //
    bufP[len++] = '\"'; bufP[len] = 0;                                          //
   } //cSimDriver::FormatAsString..

#define whatClass cSimDriver
#include <computeGeometry.cpp>

cSimDriver::cSimDriver()
   {OPCODE op; int ii=0;                                                        //
    m_environment         = ENV_NOT_SET;                                        //
    m_singleStepB         = true;                                               //
    m_expectingP          = NULL;                                               //
    for (ii=0; ii < MAX_REGS; ii++) m_lastRegsP[ii] = strdupl("");              //
    m_captureFileP        = NULL;                                               //
    m_expandOnlyB         = m_expectB = m_actualB = false;                      //
    m_errorB = m_doneB    = m_noReadCheckB = false;                             //
    m_showAllB            = false;                                              //
    m_compileB            = false;                                              //
    m_skipXsimB           = false,                                              //
    m_patchDrvB           = true;                                               //patch 'MICROCODE_SIZE' in samDefines.sv
    m_dryRunB             = false;                                              //
    m_bugMemB             = false;                                              //
    m_showLineB           = m_showAllB;                                         //
    m_bUsed               = 0;                                                  //
    m_countWrites         = 0;                                                  //
    m_holdCheckE          = 0;                                                  //
    m_holdReg0P           = NULL;                                               //
    m_testRowP            = NULL;                                               //
    m_itemName[ITEM_CFG]  = "CFG ";                                             //+0
    m_itemName[ITEM_INDX] = "INDX";                                             //+1
    m_itemName[ITEM_PAGE] = "PAGE";                                             //+2
    m_itemName[ITEM_BOOK] = "BOOK";                                             //+3
    m_testNamesP[0]       = "Sequencer one row";                                //
    m_testNamesP[1]       = "Show SAM geometry";                                //
    //Calculate bits in signed op.ind.offset field                              //
    op.u16                = -1;                                                 //
    m_opNameP             = new cOpName(&g_err, NULL, 0);                       //starting new messages block
   } //cSimDriver::cSimDriver...

cSimDriver::~cSimDriver()
   {//Tell command to quit - this will cause our read threads to stop           //
// if (g_echoBackB) SendCmd("exit");                                            //
    free(m_samDefinesSrcP); m_samDefinesSrcP = NULL;                            //
    free(m_testRowP);       m_testRowP       = NULL;                            //
    free(m_grpMaskBinP);    m_grpMaskBinP    = NULL;                            //
    free(m_holdReg0P);      m_holdReg0P      = NULL;                            //
    delete m_opNameP;       m_opNameP        = NULL;
//  delete m_compilerP;     m_compilerP      = NULL;                            //
                                                                                //
    for (int ii=0; ii < MAX_REGS; ii++) free(m_lastRegsP[ii]);                  //
    //Close our handles                                                         //
    if (!CloseHandle(m_hPipeParentRcv))      SHO_ERR();                         //
    if (!CloseHandle(m_hPipeChildRcv))       SHO_ERR();                         //
    if (!CloseHandle(m_hPipeParentRcvError)) SHO_ERR();                         //
   } //cSimDriver::~cSimDriver...

//build name of microcode, message, linemap, and capture files.
//NOTE: this fully qualified name is not what is stored in samControl.sv
//  since Vivado can't manage a fully qualified name but insists that the
//  file be in the 'current directory', ie., genenv("VerilogSimulationDir")
int cSimDriver::BuildFileNames(void)
   {const char *pp, *qq; char buf[_MAX_PATH];                                   //
    int         ii;                                                             //
    #define CreateSubordinateFileName(name, first)                              \
        if (!first) strncpy(m_##name, m_microcode, sizeof(m_##name) - 1);       \
        strncpy(&m_##name[ii], "." #name, sizeof(m_##name) - ii-1);             //
    for (pp=qq=m_srcFileNameP; pp=strpbrk(qq, "\\/");) qq=pp+1;                 //find last '\' or '/'
    SNPRINTF(m_microcode),"%s\\%s", getenv("VerilogSimulationDir"), qq);        //
    if (!(pp=strrchr(m_microcode, '.')))return Error(ERR_7296, "", m_microcode);//7296 = Invalid filename '%s'
    ii = (int)(pp - m_microcode);                                               //
    CreateSubordinateFileName(microcode,   true);                               //
    CreateSubordinateFileName(msgFile,     false);                              //
    CreateSubordinateFileName(lineMap,     false);                              //
    CreateSubordinateFileName(symbolTbl,   false);                              //
    if (m_captureFileP == NULL)                                                 //user has overriden cap file name
       {CreateSubordinateFileName(capFile, false); m_captureFileP = m_capFile;} //
#undef CreateSubordinateFileName
    if (!m_dryRunB) return 0;                                                   //
    Printf("Options:\n");                                                       //
    Printf("   - showAll  = %s (%s)\n", TF(m_showAllB),    "show all xsim.exe output in capture file");
    Printf("   - compile  = %s (%s)\n", TF(m_compileB),    "call compiler only");
    Printf("   - patchDrv = %s (%s ",   TF(m_patchDrvB),   "patch samDefines.sv lines containing:\n");
    Printf("                    "                          "     parameter MICROCODE_SIZE and\n"); 
    Printf("                    "                          "    `define    NAMEOF_MICROCODE in)\n");                
    Printf("   - skipXsim = %s (%s)\n", TF(m_skipXsimB),   "reprocess simulate.log (generated by xsim.exe)");
    Printf("   - capture  = %s (%s)\n", TF(m_captureFileP),"capture filtered and reformatted output in this file");
    Printf("   - bugLevel = %d\n",      g_bugLevel);                            //
    Printf("Files:\n");                                                         //
    Printf("    source    input file  = %s\n", m_srcFileNameP);                 //
    Printf("    include   directory   = %s\n", m_includeDirP);                  //
    Printf("    microCode output file = %s\n", m_microcode);                    //
    Printf("    messages  output File = %s\n", m_msgFile);                      //
    Printf("    lineMap   output file = %s\n", m_lineMap);                      //
    Printf("    capture   output File = %s\n", m_capFile);                      //
    Printf("    current directory     = %s\n", getcwd(buf, sizeof(buf)-1));     //
    return 0;
   } //cSimDriver::BuildFileNames...

//Create pipes to redirect output from xsim.exe and process each line. When
//g_skipXsimB is true the output is generated by simply typing simulate.log. 
//This avoids the expensive simulation when the problem is not with the FPGA itself.
//LaunchChild initiates compile.bat (which then calls xsim.exe) or 
//reprocess.bat (which just types the output file).
int cSimDriver::StartSimulator(int keySize)
   {HANDLE                hParentRcvTmp,      hParentSend,                      //
                          hChildRcvTmp,       hChildSend,                       //
                          hParentRcvErrorTmp, hChildErrorSend, cur;             //
    SECURITY_ATTRIBUTES   sa;                                                   //
    DWORD                 threadId;                                             //
    sHDW_PARAMS           hParams;                                              //
    int                   erC=0, environment;                                   //
    char                 *bufP, *fnP;                                           //
                                                                                //
    m_keySize = keySize;                                                        //
    if ((erC=BuildFileNames()) < 0)     return erC;                             //
    if (m_dryRunB)     {m_doneB = true; return 0;}                              //
    if ((erC=ReadParams(&hParams)) < 0) return erC;                             //
    if ((erC=ComputeGeometry(m_keySize)) < 0) {m_doneB = true; return erC;}     //
    free(m_samDefinesSrcP); m_samDefinesSrcP = NULL;                            //
    #ifdef _DEBUG                                                               //
        if ((erC=CheckArchitecture()) < 0) return erC;                          //
    #endif                                                                      //
    //Compile microcode to objFile                                              //
    bufP = (char*)malloc(erC=500);                                              //
    snprintf(bufP, erC-1, "cmd.exe /c samCompile %s /include \"%s\"\n",        //
                                       m_srcFileNameP, getenv("samSourceDir")); //
    if ((environment=system(bufP)) == 255)                                      //compiler error
       {m_doneB = true; free(bufP); return -ERR_0001;}                          //
    #ifdef _DEBUG                                                               //
        _CrtCheckMemory();                                                      //
    #endif                                                                      //
    fnP = StripFileName(m_srcFileNameP);                                        //  
    if ((erC=ReadAllFile(fnP, ".msgFile",   &m_opNameP->m_messagesP))  < 0) goto err;      //read msgFile
    m_msgSize = erC;                                                            //
    if ((erC=ReadAllFile(fnP, ".lineMap",   &m_lineMapP))   < 0) goto err;      //read lineMap
    m_lineMapSize = erC;                                                        //
    if ((erC=ReadAllFile(fnP, ".symbolTbl", &m_symbolTblP)) < 0)                //read symbol table
       {err: m_errorB = m_doneB = true; return erC;}                            //
    m_symbolSize = erC;                                                         //
    free(fnP);                                                                  //
                                                                                //
    //Separate out various $environment requests from microcode                 //
    if (m_compileB) environment = ENV_COMPILE; else                             //override pprogram settings
    if (m_emulateB) environment = ENV_SW;                                       // with command line arguments
    switch (environment)                                                        //
       {case ENV_SW:      m_emulateB = true;                                    //
        case ENV_XSIM:    if (erC >= 0) break;                                  //otherwise fall thru to compiler error
        case ENV_COMPILE:                                                       //compile only
            Printf("%s Compiled, error=%d\n", m_srcFileNameP, erC);             //
            m_doneB = true; return erC;                                         //
        case ENV_FPGA:                                                          //
        case ENV_HW:      m_doneB = true; return Error(ERR_9998, "", "");       //9998 = not yet coded
        default:          m_doneB = true; return Error(ERR_7176, "", "");       //7176 = Invalid/duplicate environment setting                                                      //
       } //m_environment...                                                     //
                                                                                //full xsim.exe simulation
    //Create pipes and launch cmd.exe                                           //
    // Set up the security attributes struct.                                   //
    sa.nLength              = sizeof(SECURITY_ATTRIBUTES);                      //
    sa.lpSecurityDescriptor = NULL;                                             //
    sa.bInheritHandle       = TRUE;                                             //
                                                                                //
    if (!CreatePipe(&hParentRcvTmp,      &hChildSend,      &sa, 0)) SHO_ERR();  //Create the child to parent output pipe
    if (!CreatePipe(&hParentRcvErrorTmp, &hChildErrorSend, &sa, 0)) SHO_ERR();  //Create the child to parent error pipe
    if (!CreatePipe(&hParentSend,        &hChildRcvTmp,    &sa, 0)) SHO_ERR();  //Create the parent to child pipe

    //Create new parent->child handle and child->parent handles.
    //Set the Properties to FALSE. Otherwise, the child inherits the
    //properties and, as a result, non-closeable handles to the pipes are created.
    cur = GetCurrentProcess();                                                  //
    if (!DuplicateHandle(cur, hParentRcvTmp,     cur, &m_hPipeParentRcv,      0, FALSE, DUPLICATE_SAME_ACCESS)) SHO_ERR();
    if (!DuplicateHandle(cur, hChildRcvTmp,      cur, &m_hPipeChildRcv,       0, FALSE, DUPLICATE_SAME_ACCESS)) SHO_ERR();
    if (!DuplicateHandle(cur, hParentRcvErrorTmp,cur, &m_hPipeParentRcvError, 0, FALSE, DUPLICATE_SAME_ACCESS)) SHO_ERR();

    //Close inheritable copies of the handles we do not want to be inherited
    if (!CloseHandle(hChildRcvTmp ))      SHO_ERR();
    if (!CloseHandle(hParentRcvTmp))      SHO_ERR();
    if (!CloseHandle(hParentRcvErrorTmp)) SHO_ERR();

    //LaunchChild... will create the child process with the specified pipe handles
    //replacing its standard pipe handles
    LaunchChild(hChildSend, hParentSend, hChildErrorSend);

    //Close pipe handles (do not continue to modify the parent).
    //You need to make sure that no handles to the write end of the
    //output pipe are maintained in this process or else the pipe will
    //not close when the child process exits and the ReadFile will hang.
    if (!CloseHandle(hChildSend     )) SHO_ERR();
    if (!CloseHandle(hParentSend    )) SHO_ERR();
    if (!CloseHandle(hChildErrorSend)) SHO_ERR();

    //Launch the thread that gets the input and sends it to the child.
    if ((CreateThread(NULL, 0, RcvFromChildThread,      (LPVOID)this, 0, &threadId)) == NULL) SHO_ERR();
    //Launch the thread that gets errors from the child
    if ((CreateThread(NULL, 0, RcvErrorFromChildThread, (LPVOID)this, 0, &threadId)) == NULL) SHO_ERR();
    m_errorB = false;
    return 0;
   } //cSimDriver::StartSimulator()

//Read all file into memory at *bufPP; return size or zero if unable to open file
int cSimDriver::ReadAllFile(const char *fileOnlyP, const char *suffixP, char **bufPP)
   {size_t sz; char  *bufP, *fnP, *tempP=NULL;                                  //
    if (suffixP != NULL)                                                        //
       {tempP = (char*)calloc(strlen(fileOnlyP) + strlen(suffixP)+1,1);         //
        strcpy(tempP, fileOnlyP);                                               //
        strcat(tempP, suffixP);                                                 //
        fileOnlyP = tempP;                                                      //
       }                                                                        //
    fnP = FullFileName(fileOnlyP);                                              //    
    FILE  *fileP=fopen(fnP, "rb");                                              //
    if (!fileP) return Error(ERR_0003, "", fnP);                                //Failed to open
    fseek(fileP, 0, SEEK_END);                                                  //
    sz   = ftell(fileP);                                                        //
    bufP = (char*)malloc(sz+2);                                                 //room for \nand 0x00
    fseek(fileP, 0, SEEK_SET);                                                  //
    fread(bufP, sz,1, fileP);                                                   //read entire file into memory
    bufP[sz] = 0;                                                               //
    fclose(fileP);                                                              //
    free(tempP);                                                                //
    *bufPP   = bufP;                                                            //
    return (int)sz;                                                             //
   } //cSimDriver::ReadAllFile...

void cSimDriver::PrintRowRaw(int row)
    {uint8_t *u8P=(uint8_t*)RowFromBlockramData(row);                           //
     for (int ii=0; ii < m_rowBytes; ii++)                                      //
         {if ((ii & 31) == 0) Printf("  %03X: ", ii);                           //
          Printf("%02X%s", u8P[ii] & 0xFF,                                      //
              (ii &  7) == 7 ? "  " :                                           //
              (ii &  3) == 3 ? " " : "");                                       //
          if ((ii & 31) ==31)                                                   //
             {Printf("  ");                                                     //
              for (int jj=ii-31; jj <= ii; jj++)                                //
                Printf("%c%s", (u8P[jj] >= ' ') ? u8P[jj] : '.',                //
                          (jj & 7) == 7 ? " " : "");                            //
              Printf("\n");                                                     //
         }   }                                                                  //
    } //cSimDriver::PrintRowRaw...

void cSimDriver::PrintRowBkwds(const char *titleP, const char *rowP)
   {const char *pp, *qq; char hex[3]; int ii, jj; uint8_t u8;                   //
    Printf("%s row\n", titleP);                                                 //
    for (pp=Endof((char*)rowP), ii=0; pp && (pp-=2) >= rowP; ii++)              //
        {if ((ii & 31) == 0) Printf("  %03X: ", ii);                            //
         Printf("%c%c%s", pp[0] & 0xFF, pp[1] & 0xFF,                           //
                   (ii &  7) ==  7 ? "  " :                                     //
                   (ii &  3) ==  3 ? " "  : "");                                //
         if ((ii & 31) == 31)                                                   //
            {Printf("  ");                                                      //
             for (jj=ii-31, qq=pp+2*31; jj <= ii; qq-=2, jj++)                  //
                {hex[0] = qq[0]; hex[1] = qq[1]; hex[2] = 0;                    //
                 u8     = (char)strtol(hex, NULL, 16);                          //
                 Printf("%c%s",(u8 >= ' ') ? u8 : '.',(jj & 7) == 7 ? " " : "");//
                }                                                               //
             Printf("\n");                                                      //
        }   }                                                                   //
   } //cSimDriver::PrintRowBkwds...

char *cSimDriver::strdupl(const char *srcP, int len) //== -1
   {char *destP;                                                                //
    if (len < 0) len = (int)strlen(srcP);                                       //
    destP = (char*)calloc(len+1, 1);                                            //
    if (destP == NULL) {Error(ERR_0005, "", srcP); exit(1);}                    //memory allocation failed
    memmove(destP, srcP, len);                                                  //
    destP[len] = 0;                                                             //
    return destP;                                                               //
   } //cSimDriver::strdupl..

//Get a number allowing:
// - Verilog format (eg 8'h55), 
// - C++ format (eg 0x123)
// - binary (eg 0b0100010), 
// - k/m/b suffixes (10^3, 10^6, and 10^9).
//For example: 123, 0x456, 19'h123_456, 7'b0000_0001, 100k, 100m, 100b
uint64_t cSimDriver::Anumber(const char *ppc, char **ppP, int *bitCountP)
   {uint64_t  value;
    int       bitCount;                                                         //
    char     *pp=(char*)ppc;                                                    //
    if (!bitCountP) bitCountP = &bitCount;                                      //someplace to put it
    *bitCountP = 32;                                                            //
    pp += strspn(pp, " ");                                                      //
    if (pp[0] == '0' && pp[1] == 'x') value = _strtoui64(pp+2, &pp, 16);        //
    else                              value = _strtoui64(pp,   &pp, 10);        //
    if (*pp == '\'')                                                            //eg 31'h913
       {*bitCountP = (int) value;                                               //
        if (*(++pp) == 'h') value = _strtoui64(pp+1, &pp, 16); else             //Verilog representation
        if (*(  pp) == 'b') value = _strtoui64(pp+1, &pp,  2); else             //
        if (*(  pp) == 'd') value = _strtoui64(pp+1, &pp, 10); else             //
                            value = _strtoui64(pp+0, &pp, 10);                  //
      }                                                                         //
    else
      {//bitcount not specified Verilog style. guess at bit count: 
       //value <= 255 bitCount 8, <= 65535 bitCount 16, >= 2**32 bitCount 64
       if (value <=   255)         *bitCountP = 8; else
       if (value <= 65535)         *bitCountP =16; else
       if (value > 0x100000000ull) *bitCountP =64; 
      }
    if ((*pp | 0x20) == 'k') {value *= 1000;          pp++;} else               // * 1000 kilo
    if ((*pp | 0x20) == 'm') {value *= 1000000;       pp++;} else               // * 1000,000 millions
    if ((*pp | 0x20) == 'b') {value *= 1000000000;    pp++;} //else             // * 1000,000,000 billions
//  if ((*pp | 0x20) == 't') {value *= 1000000000000; pp++;}                    // * 1000,000,000,000 trillions
    if (ppP) *ppP = pp;                                                         //
    return value;                                                               //
   } //cSimDriver::Anumber...

//Prefix the simulation/source directory on the front of fileOnlyP.
//If the file is type .sv choose the source directory, otherwise the simulation directory.
char *cSimDriver::FullFileName(const char *nameOnlyP)
   {static char name[_MAX_PATH];                                                //
    const char *dirP = (strstr(nameOnlyP, ".sv") == NULL)                       //
                        ? "VerilogSimulationDir" : "VerilogSourceDir";          //
    if (strchr(nameOnlyP, '\\') || strchr(nameOnlyP, '/')) return (char*)nameOnlyP; //nameOnly my arse
    SNPRINTF(name), "%s\\%s", getenv(dirP), nameOnlyP);    return name;         //
   } //cSimDriver::FullFileName...


char *cSimDriver::ConditionNames(uint16_t cc)
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
   } //cSimDriver::ConditionNames...

const char *cSimDriver::StateName(SC_CODES state)
    {return state == SC_START  ? "SC_START" :                                   //+0
            state == SC_IDLE   ? "SC_IDLE"  :                                   //+1
            state == SC_GETOP  ? "SC_GETOP" :                                   //+2
            state == SC_DO_OP  ? "SC_DO_OP" :                                   //+3
            state == SC_SCAN   ? "SC_SCAN"  :                                   //+4
            state == SC_READ   ? "SC_READ"  :                                   //+5
            state == SC_READ1  ? "SC_READ1" :                                   //+6
          //state == NU_XCHG   ? "SC_XCHG"  :                                   //+7
            state == SC_R2R    ? "SC_R2R"   :                                   //+8
            state == SC_WRYT   ? "SC_WRYT"  :                                   //+9
            state == SC_WRYT1  ? "SC_WRT1"  :                                   //10
            state == SC_SCIN   ? "SC_SCIN"  :                                   //11
            state == SC_SCIN1  ? "SC_SCIN1" :                                   //12
            state == SC_ARITH1 ? "SC_ARITH1":                                   //13
            state == SC_XTOS   ? "SC_XTOS"  :                                   //14
            state == SC_RDF1   ? "SC_RDF1"  :                                   //15
            state == SC_RDF2   ? "SC_RDF2"  : "SC_?**?";                        //16


    } //cSimDriver::StateName...

char *cSimDriver::AppendKey(char *bufP, int bufSize, const char *targetP, int keySize)
    {int ii, len; bool hexB;                                                    //
     if (targetP == NULL) {strncpy(bufP, "<null>", bufSize); return bufP;}      //
     for (int ii=keySize; --ii >= 0 && !(hexB=(targetP[ii] < 0x20));){}         //
     len = istrlen(bufP);                                                       //
     if (!hexB) snprintf(&bufP[len], sizeof(bufP)-len-1, " \"%s\"", targetP);   //
     else                                                                       //
        {snprintf(&bufP[len], bufSize-len-1, " 0x");                            //
         for (ii=m_keySize; --ii >= 0;)                                         //
             {len = istrlen(bufP);                                              //
              snprintf(&bufP[len], bufSize-len-1, "%02X",targetP[ii] & 0xFF);   //
        }    }                                                                  //
     return bufP;                                                               //
    } //cSimDriver::AppendKey...  

//Search for parameter <nameP> = value in samDefines.sv. paramB means look for 'parameter <nameP>'
bool IsAlpha(char ch) {return ch == '_' || ((ch|0x20) >= 'a' && (ch|0x20) <= 'z');}
bool IsDigit(char ch) {return ch >= '0' && ch <= '9';}
bool cSimDriver::VerilogParam(const char *paramNameP, int *valP, bool paramB)
   {const char *pp, *qq, *lineP=m_samDefinesSrcP;                               //
    *valP = 0;                                                                  //
    for (pp=lineP; (pp=strstr(pp, paramNameP)) != NULL; pp++)                   //
       {if (IsAlpha(*(pp-1)) || IsAlpha(pp[strlen(paramNameP)])) goto eLup;     //
        for (qq=pp-9; paramB && strncmp(qq, "parameter", 9) != 0; --qq)         //backup to 'parameter'
            {if (*qq == '\n') goto eLup;}                                       //that's not it, keep looking
        pp += strlen(paramNameP);                                               //step over name
        pp += strspn(pp, " =\t");                                               //step over whitespace and '='
        *valP = (int)Anumber((char*)pp);                                        //grab that value
        return true;                                                            //
  eLup: pp += istrlen(paramNameP);                                              //
       }                                                                        //
     return false;                                                              //
   } //cSimDriver::VerilogParam...

uint64_t cSimDriver::GetParam(char *lineP, const char *paramNameP)
   {const char *pp;                                                             //
    for (pp=lineP; (pp=strstr(pp, paramNameP)) != NULL; pp++)                   //
       if (!IsAlpha(*(pp-1)) && !IsAlpha(pp[strlen(paramNameP)]))               //
           {pp += strlen(paramNameP);                                           //step over name
            pp += strspn(pp, " =\t");                                           //step over whitespace and '='
            return Anumber((char*)pp, NULL);                                    //grab that value
           }                                                                    //
    return 0;
   } //cSimDriver::GetParam...

int cSimDriver::ShowNtError(const char *fncP, int line)
   {LPTSTR lpszTemp = NULL; int erC; char buf[512];
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                  NULL,
                  erC=GetLastError(),
                  LANG_NEUTRAL,
                  (LPTSTR)&lpszTemp,
                  0,
                  NULL);
    _snprintf(buf, sizeof(buf)-1, "Error %u, %s #%04d", erC, fncP, line);
    buf[sizeof(buf)-1] = 0;
//::MessageBoxA(NULL, (LPCSTR)lpszTemp, buf, MB_OK);
    if (lpszTemp) LocalFree((HLOCAL) lpszTemp);
    return 0;
   } //ShowNtError...

//Send command to captive cmd module.
int cSimDriver::SendCmd(const char *cmdP)
   {DWORD   nBytesWrote = 0;
    if (m_showAllB) printf("Sending Command : %s\n", cmdP);
    if ((!WriteFile(m_hPipeChildRcv, cmdP, (DWORD)strlen(cmdP), &nBytesWrote,NULL)) || (!nBytesWrote))
       {if (GetLastError() != ERROR_NO_DATA) SHO_ERR();} //if GetLastError() == ERROR_NO_DATA then the child exited properly
    return nBytesWrote;
   } //cSimDriver::SendCmd...

DWORD WINAPI cSimDriver::RcvFromChildThread(LPVOID lpvThreadParam)
   {return ((cSimDriver*)lpvThreadParam)->Receive(((cSimDriver*)lpvThreadParam)->m_hPipeParentRcv);}

#define uint8_t unsigned char
DWORD WINAPI cSimDriver::RcvErrorFromChildThread(LPVOID lpvThreadParam)
   {return ((cSimDriver*)lpvThreadParam)->ReceiveErr(((cSimDriver*)lpvThreadParam)->m_hPipeParentRcvError);}

DWORD WINAPI cSimDriver::ReceiveErr(HANDLE hPipe)
   {return 0xdeadC0de;}

//Capture stdout messages from xsim and reformat messages of interest.
//In particular text wrapped in '###Start Dump' ... '###End dump' is a raw hex of BRAM
//plus a few other critical parameters; capture and call cBugIndex::Bug().
//General message are bracketted with '##Start Simulation.' ... '##Endof Simulation.'
DWORD WINAPI cSimDriver::Receive(HANDLE hPipe)
   {DWORD    nBytesRead, osErr;                                                 //
    int      bufSize= 4096,                                                     //buffer size and amount reserved in the front
             front  = 1024,                                                     //  of buffer for split lines.
             len, lenBram=0, addr, lastAddr=0, bramRow=0;                       //
    CHAR    *bufP=(char*)calloc(bufSize, 1), *lineP, *pp, *qq, *eolP;           //
    uint8_t *bramP=NULL, *u8P;                                                  //
    bool     gatherB=false, errorB=false;                                       //
                                                                                //
    MilliSleep(1);                                                              //allow ReceiveErr to stop
    snprintf(bufP, bufSize, "---- #Start %s.exe\n", m_emulateB ?"emu" :"xsim"); //
    Printf("%s", bufP); BUGOUT(bufP);                                           //
    bufP[0] = 0;                                                                //
    if (g_printFileP) fclose(g_printFileP); g_printFileP = NULL;                //defensive - probably unnecessary
    if (pp=m_captureFileP)                                                      // sim /capture <fileName>
       {if (pp[1] == ':' || pp[0] == '\\')                                      //absolute file name ?
           snprintf(bufP, bufSize, "%s", m_captureFileP);                       //
        else                                                                    //
           snprintf(bufP, bufSize, "%s\\%s", getenv("VerilogSimulationDir"),pp);//otherwise file lives in sim directory
        g_printFileP = fopen(bufP, "wb");                                       //Printf will dupl everything to this file
        if (g_printFileP == NULL) return Error(ERR_7147, "", bufP);             //7147 = unable to create file
       }                                                                        //
    for (bufP[0]=m_errCount=0; !m_doneB;)                                       //
       {nBytesRead = 0; bufP[front] = 0;                                        //
//      memset(&over, 0, sizeof(over));                                         //
//      if (!ReadFile(hPipe,&bufP[front], bufSize-front-1, NULL, &over))        //
//      if ((nBytesRead=(int)over.InternalHigh) == 0) {MilliSleep(1); continue;}//
        if (!ReadFile(hPipe,&bufP[front], bufSize-front-1, &nBytesRead, NULL))  //
           {if ((osErr=GetLastError()) != ERROR_BROKEN_PIPE) SHO_ERR(); break;} //
        bufP[front+nBytesRead] = 0x00;                                          //
        strncat(lineP=bufP, &bufP[front], bufSize-1);                           //hook fragment from last readPipe
        while (pp=strchr(lineP=bufP, '\x0C')) *pp = ' ';                        //junk page feeds ?
        for (m_doneB=errorB=false; eolP=strpbrk(lineP, "\r\n"); lineP=eolP)     //break into lines
            {if (eolP[0] == '\r' && eolP[1] == '\n') *eolP++ = 0;               //clobber the \r
             *eolP++ = 0;                                                       //clobber the \n
             RemoveSourceDir(lineP);                                            //
             if (strBegins(lineP, "##Params:"))                                 //look for special messages
                {for (pp=qq=m_srcFileNameP; (pp=strpbrk(qq, "\\/")); qq=pp+1){} //find fileName 
                 Printf("%s, microcode=%s\n", lineP, qq);                       //
                 m_keySize        = (int) GetParam(lineP, "keySize=");          //Get operational parameters
                 m_rowBytes       = (int) GetParam(lineP, "rowSize=");          //   from ##param line.
                 if (ComputeGeometry(m_keySize) < 0) {m_doneB = true; break;}   //Calculate hINDX_size, etc.
               //Printf("\nRow[testAdr=%d]:\n", m_params.testAdr);              //
               //PrintRowRaw(m_params.testAdr);                                 //
               //Printf("\n");                                                  //
                 continue;                                                      //
                }                                                               //
             if (strBegins(lineP, "###Start Dump,"))                            //look for special messages
                {gatherB     = true;                                            //start gathering BRAM data
                 Printf("%s\n", lineP);                                         //
                 continue;                                                      //
                }                                                               //
             if (strBegins(lineP, "##Abort"))                                   //look for special messages
                {m_showLineB = m_showAllB; PrintLine(lineP);                    //revert to default display settings
                 sprintf(bufP, "##Abort, line=%04d", GetSourceLineNum(0));      //(pc)
                 BUGOUT("---- "); BUGOUT(lineP); BUGOUT("\n");                  //tell Visual Studio
                 MessageBoxA(NULL, lineP, "##Abort", MB_OK);                    //
                 m_doneB = m_errorB = true; break;                              //throw on the brakes
                }                                                               //
             if (gatherB && strcmp(lineP, "###End Dump") == 0)                  //
                {//Finally the end of BRAM data is signalled by '##End Dump'    //
                 //pass data gathered in bramP to cBugIndex::Bug()              //
                 if (m_doneB=!BugThis(bramP, lastAddr))                         //
                    {lineP = (char*)"**** Error in dump indexbase"; goto err;}  //
                 Printf("\n%s\n", lineP);                                       //
         xgather:gatherB = false;                                               //dat's dun
                 free(bramP); bramP = NULL; lenBram = lastAddr = 0;             //
                 continue;                                                      //
                }                                                               //
             if (gatherB)                                                       //gathering BRAM data
                {//Gather data bracketted by '###Start Dump ... '###end dump';  //
                 //Each line comprises 'bram[adr]: ' followed by the BRAM row.  //
                 if (strncmp(lineP, "bram[", 5) != 0) continue;                 //'bram[000]: '<hex word>
                 if ((addr=strtol(lineP+5, &lineP, 16)) != lastAddr++)          //
                   {Error(ERR_2708, "", lineP); goto xgather;}                  //Sequence error in BRAM dump
                 lineP  += 3;                                                   //']: ', point to <hex word>
                 len     = (int)strlen(lineP);                                  //
                 bramP   = (uint8_t*)realloc(bramP, lenBram += len/2);          //
                 for (u8P=&bramP[lenBram]; (len-=2) >= 0; lineP+=2)             //convert hex to binary
                      *(--u8P) = Hex2Bin(lineP);                                //
                 continue;                                                      //get next line
                }                                                               //
             if (strBegins(lineP, "##Start Simulation") ||                      //
                 strBegins(lineP, "##Start Emulation"))                         //
                {time_t now = time(NULL);                                       //
                 Printf("%s start=%s", lineP, Fixctime(&m_start));              //
                 Printf(", end=%s\n",         Fixctime(&now));                  //
                 m_showLineB = true;                                            //
                 continue;                                                      //
                }                                                               //
             pp = lineP + strspn(lineP, " ");                                   //
             if (strBegins(pp,"#Check" ))                                       //
                {if (Check(lineP)) continue; m_doneB = true; break;}            //
             if (strBegins(pp, "##Endof Simulation")        ||                  //
                 strBegins(pp, "##Endof Emulation")         ||                  //
                 strBegins(pp, "##Endof Compile.")          ||                  //
                 strstr(   pp, "] Exiting xsim at") != NULL ||                  //
                 strBegins(pp, "$stop called at time"))                         //
                {m_showLineB = m_showAllB; PrintLine(lineP);                    //revert to default display settings
                 BUGOUT("---- "); BUGOUT(lineP); BUGOUT("\n");                  //tell Visual Studio
                 m_doneB = true; break;                                         //throw on the brakes
                }                                                               //
             if ((pp=strstr(lineP, "#bram["))) bramRow = (int)Anumber(pp+6);    //anywhere in line will do
             //WARNING: [Simtcl 6-197] One or more HDL objects could not be logged because of object type or size limitations.
             if (strstr(lineP, "WARNING: [Simtcl 6-197]") != NULL) continue;    //ignore line; what does that warning mean ??
             if (strstr(lineP, "WARNING: ") != NULL) goto err;                  //
             if (strstr(lineP, "Abnormal program termination") != NULL)         //
                {m_doneB = true;                      goto warn;}               //
             if (strstr(lineP, "ERROR: [")   != NULL)                           //
                {m_showLineB = errorB = true;                                   //
                 Error(ERR_2724, "", lineP);                                    //
            err: if (m_errCount++ > 10) m_doneB = true;                         //
            warn:BUGOUT(lineP); BUGOUT("\n");                                   //
                 PrintLine(lineP);                                              //
                 continue;                                                      //
                }                                                               //
             if (m_showLineB) PrintLine(lineP);                                 //
            } //for (m_doneB=...                                                //
        if (m_doneB || errorB) break;                                           //
        strcpy(bufP, lineP);                                                    //save remnant for next readPipe
       } //for (bufP[0]=...                                                     //
    if (g_printFileP != NULL)                                                   //
       {fclose(g_printFileP); g_printFileP = NULL;                              //
        snprintf(bufP, bufSize, "---- #Output captured in %s\n",m_captureFileP);//
        Printf("\n%s", &bufP[6]); BUGOUT(bufP);                                 // "Output captured ....
       }                                                                        //
    free(bramP); free(bufP);                                                    //
    MilliSleep(1);                                                              //allow ReceiveErr to stop
    return 1;                                                                   //
   } //cSimDriver::Receive...

//aV < bV return -1, aV == bV return 0, aV > bV return +1
int cSimDriver::CompareKey(const void *aV, const void *bV, int keySz)
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
   } //cSimDriver::CompareKey...

//Reverse keyP in place
char *cSimDriver::ReverseKey(char *keyP)
   {char *revP=(char*)alloca(m_keySize);
    for (int ii=m_keySize, jj=0; --ii >= 0;) revP[jj++] = keyP[ii];             //
    memmove(keyP, revP, m_keySize);                                             //
    return keyP;                                                                //
   } //cSimDriver::ReverseKey...

//Convert and reverse keyP in place from hex. Ignore initial '0x'
char *cSimDriver::ReverseUnhex(char *keyP)
   {char *kP=keyP, *revP=(char*)alloca(m_keySize);                              //
    if (kP[0] == '0' && kP[1] == 'x') kP += 2;                                  //
    for (int ii=2*m_keySize, jj=0; (ii-=2) >= 0;)                               //
        {if (kP[ii] == 'z' && kP[ii+1] == 'z')                                  //
             revP[jj++] = 'z'; else           //                                //
             revP[jj++] = (char)strtol(&kP[ii], NULL, 16);                      //
         kP[ii] = 0;                                                            //
        }                                                                       //
    memmove(keyP, revP, m_keySize); keyP[m_keySize] = 0;                        //
    return keyP;                                                                //
   } //cSimDriver::ReverseKey...

//Convert and reverse keyP from hex to alpha in outP. Ignore initial '0x'.
//If keyP translates to displayable characters that is what will be copied into outP.
//If keyP cannot be so translated, then the hex will merely be reversed into outP.
//Return true if result is displayable
bool cSimDriver::DisplayableHex(const char *keyP, char *outP)
   {char       *dP=outP;                                                        //
    const char *sP;                                                             //
    int         ii, jj;                                                         //
    bool        displayableB=true, undefinedB=false;                            //
                                                                                //
    if (keyP[0] == '0' && keyP[1] == 'x') keyP += 2;                            //ignore leading 0x
    for (ii=2*m_keySize, jj=0, dP=outP, sP=keyP; (ii-=2) >= 0; jj++)            //
        {if (((sP[ii] | 0x20)== 'z' && (sP[ii+1] | 0x20) == 'z') ||             //
             ((sP[ii] | 0x20)== 'x' && (sP[ii+1] | 0x20) == 'x'))               //
             {displayableB = false; undefinedB = true;}                         //
         if ((dP[jj]=(char)Hex2Bin(&sP[ii])) < ' ') displayableB = false;       //
        }                                                                       //
    dP[jj] = 0;                                                                 //
    if (displayableB) return true;                                              //
    //not displayable characters                                                //
    if (undefinedB) strncpy(outP, keyP, jj=m_keySize);                          //
    else                                                                        //
       {strcpy(outP, "0x");                                                     //
        for (ii=2*m_keySize, jj=0, sP=keyP, dP=outP+2; (ii-=2) >= 0;)           //
            {dP[jj++] = sP[ii]; dP[jj++] = sP[ii+1];}                           //
       }                                                                        //
    dP[jj] = 0;                                                                 //
    return false;                                                               //
   } //cSimDriver::DisplayableHex...

//Generate 'string' or 0x<hex> format; hex input is in backwards order
char *cSimDriver::FormatKey(char *keyP)
   {char       *kP=keyP, *copyP=(char*)alloca(2*m_keySize+4);                   //
    bool        isAlphaB=true;                                                  //
    static char rev[69]; //max key size+5                                       //
    int         ii, jj;                                                         //
    if (kP[0] == '0' && kP[1] == 'x') kP += 2;                                  //
    copyP[0]   = '0'; copyP[1] = 'x';                                           //
    for (ii=0; ii < (int)m_keySize*2; ii+=2)                                    //
       {copyP[ii+3] = kP[2*m_keySize-ii-1]; copyP[ii+2] = kP[2*m_keySize-ii-2];}//
    copyP[2*m_keySize+2] = 0;                                                   //
    rev[0] = '\'';                                                              //
    for (ii=2*m_keySize, jj=1; (ii-=2) >= 0; )                                  //
        {if (kP[ii] == 'z' && kP[ii+1] == 'z') rev[jj] = 'z';                   //
         else rev[jj] = (char)strtol(&kP[ii], NULL, 16);                        //
         if (rev[jj] < 0x20) {strncpy(rev, copyP, 2*m_keySize+3); break;}       //
         kP[ii] = 0; rev[++jj] = '\''; rev[jj+1] = 0;                           //
        }                                                                       //
    return rev;                                                                 //
   } //cSimDriver::FormatKey...

char *cSimDriver::FormatBin32(uint32_t bin)
   {static char buf[65];
    memset(buf, '_', sizeof(buf)); buf[(32/4)*5-1] = 0;
    for (int ii=32; --ii >= 0; bin >>= 1)
        buf[(ii/4)*5 + (ii&3)] = (bin & 1) | 0x30;
    return buf;
   } //cSimDriver::FormatBin32...

//translate `COMPARE code to ascii. The firmware encode a comparison using a
//two bit COMPARE structurer. COMPARE[0] = gt, COMPARE[1] = eq.
const char cSimDriver::CmpName(const char *codeP)
   {if (*codeP == '0') return '<'; //gt = 0, eq = 0
    if (*codeP == '1') return '>'; //gt = 1, eq = 0
    if (*codeP == '2') return '='; //gt = 0, eq = 1
    if (*codeP == '3') return 'S'; //gt = 1, eq = 1
    return '?';
   } //cSimDriver::CmpName...

//break up line with fields separated by comma (,) and store each field in ff[ii]
//NOTE: first field is 'check<letter> is switched to ascii of letter
//All other fields are in hex. Fields containing xXzZ are noted in invalidP[ii]
//but the numeric value is converted as though the unknown fields were zero,
//eg, 12345XXX -> 0x12345000. Values so converted are stored in vP[ii].
char cSimDriver::BreakoutLine(char *lineP, char **ff, uint64_t *vP, bool *invalidP, int max)
   {char *eolP, caze; int ii, jj; uint64_t val; char ch;                        //
    if ((caze=lineP[ii=istrlen("#check")]) == 'T')                              //#check<letter> or
       if (!IsDigit(caze=lineP[ii+1])) caze = lineP[ii];                        //    #checkT<digit>
    ff[0] = NULL; vP[0] = caze;                                                 //
    if (!(lineP=strchr(lineP, ':'))) return -1;                                 //invalid format
    for (ii=1, eolP=(++lineP); eolP=strpbrk(lineP, ",:;"); lineP=eolP)          //
        {ff[ii]  = lineP+strspn(lineP, " \t"); *eolP++ = 0;                     //
         invalidP[ii] = strpbrk(ff[ii], "xXzZ") != NULL;                        //
         for (jj=0, val=0; (ch=ff[ii][jj]) != 0; jj++, val = (val << 4) + ch)   //
            ch = strchr("xXzZ", ch) ? 0 :ch <= '9' ? ch-'0' :(ch | 0x20)-'a'+10;//treat unknowns as '0'
         vP[ii] = val;                                                          //
         if (++ii >= max) break;                                                //wow! overflow
        }                                                                       //
    ff[ii] = lineP+strspn(lineP, " \t");                                        //
    vP[ii] = (invalidP[ii]=strspn(ff[ii], "xXzZ") == strlen(ff[ii]))            //
                             ? 0 : _strtoui64(ff[ii], NULL, 16);                //
    while (--max > ii) {ff[max] = NULL; vP[max] = 0; invalidP[max] = false;}    //
    return caze;                                                                //return field field id
   } //cSimDriver::BreakoutLine...

//Perform specified checks on messages generated by Verilog code.
//Lines beginning with '#check<letter>' are verified against their expected values.
bool cSimDriver::Check(char *lineP)
   {char    *ff[30];                                                            //
    uint64_t vv[HOWMANY(ff)];                                                   //numeric values of ff[i]
    bool     invalid[HOWMANY(ff)];                                              //numerics are xxxx's or zzzz's
    int      erC=(int)strlen(lineP), item, letter;                              //erC just for sport
    OPCODE   op;                                                                //
    letter = BreakoutLine(lineP, ff, vv, invalid, HOWMANY(ff));                 //
    op     = *(OPCODE*)(uint16_t*)&vv[2];                                       //
    item   = op.sc.rowType == 1 ? ITEM_INDX : ITEM_PAGE;                        //commonly used
    switch(letter)                                                              //switch on #check<letter>
        {CAZE_ILLEGAL_OP: erC = CheckA(lineP, ff, vv, invalid, item); break;    //A: abort samControl.sv adr/op illegal 
         CAZE_TOP_OF_OP : erC = CheckE(lineP, ff, vv, invalid, item); break;    //E: samControl.sv       Top of instruction loop.
         CAZE_DUMP_ROW  : erC = CheckF(lineP, ff, vv, invalid, item); break;    //F: sequencerGroup.sv   when bugLevel >= 1.
         CAZE_SEQ_GROUP : erC = CheckG(lineP, ff, vv, invalid, item); break;    //G: sequencerGroup.sv   when bugLevel >= 6.
         default:         erC = Error(ERR_2710, "", ff[0]);           break;    //2710 = Unknown #check number: %s
        } //switch...                                                           //
    m_errorB = erC < 0; return erC >= 0;                                        //
   } //cSimDriver::Check...

//Print leading text for #check<letter> message:
//vP[0] = <letter>
//vP[1] = opcode
//vP[3] = cellNo or grpNo or uninteresting nonsense.
void cSimDriver::ShowCheckHdr(int pc, const char*nameP, uint64_t *vP)
  {Printf("#check%c@%03d", vP[0], vP[1]);                                       //'check<letter> clkCount
   if (nameP[0] != 0) {Printf(": "); Printf(nameP, vP[3]);}                     //
   Printf(": %s(%04X)", m_opNameP->Show(pc, *(OPCODE*)&vP[2], *(OPCODE*)&vP[2]), vP[2]); //opcode
  } //cSimDriver::ShowCheckHdr...

int cSimDriver::CheckA(char *lineP, char *ff[20], uint64_t *vP, bool *invalidNuP, int itemNu)
   {char buf[100];
    SNPRINTF(buf), "%s %s %s", ff[1], ff[2], ff[3]);
    MessageBoxA(NULL, buf, "Illegal Opcode", MB_OK); 
    exit(1);
   } //cSimDriver::CheckA...

//CheckE messages are generated at beginning of op: 
// $write("#checkE:", clkCount,   nxtOp,      systemStatus,                     //0-3
//                    target,     wordOut,    arithOut,                         //4-6
//                    samRegs[0 thru 7],                                        //7-14
//                    stak[sp-1 thru sp-3]                                      //15-18
//Straightforward code, except for special handling of $0 = literal, LDI, LID,...
//These are compacted down to a single $0 = long literal for user convenience.
int cSimDriver::CheckE(char *nuP, char *ff[20], uint64_t *vP, bool *invalidNuP, int itemNu)
   {static int   stopPlz=+39, lastLine=-1, lastii=IDNO;                         //response if singleStep = false
    OPCODE       op, nxt;                                                       //current OP
    sDEBUG_STAT  bugStat=*(sDEBUG_STAT*)&vP[2];                                 //
    sSYSTEM_STAT sysStat  = *(sSYSTEM_STAT*)&vP[3];                             //
    int          pc        = sysStat.pc,                                        //program counter
                 sp        = sysStat.sp,                                        //stack pointer
                 samStat   = sysStat.stat,                                      //sam Status
                 insPt     = sysStat.insPt,                                     //insertionPoint
                 bugLvl    = bugStat.bugLevel,                                  //
                 curRow    = bugStat.curRow,                                    //
                 response  = MB_OK, len, ii, adr;                               //
    uint64_t     reg0      = vP[7], clkCount=vP[1];                             //
    char       **regsP     =&ff[7], *tgtP=ff[4], **stakP=&ff[15], *pp;          //samRegs[], target, stakBuf[]
    const char  *srcP,*labelP=NULL, *qq;                                        //
    static const char *adviceP="\n\n\t\tY=step, N=run, C=stop";                 //
    #define catBody CAT_THIS(m_body)                                            //append to body
    #define catHdr  CAT_THIS(m_hdr)                                             //append to hdr
    #define catStak CAT_THIS(m_stakBuf)                                         //append to stak
    for (ii=0; ii < MAX_REGS; ii++) strupr(regsP[ii]);                          //lower case hex screws up MessageBox
top:if (pc == stopPlz)                                                          //
        pc = pc;                                                                //debugging break point
    m_body[0]= m_hdr[0] = m_stakBuf[0] = m_flags[0] =0;                         //zero the text buffers
    op.u16   = sysStat.op;                                                      //current OP
    nxt.u16  = bugStat.nxtOp;                                                   //next word following opcode
    if (op.shortOp == OP_BUG) bugLvl = op.bug.level;                            //bug is coming up
    if (bugLvl == 0) return 0;                                                  //ignore even OP_PRINT
    if (bugLvl <  3) goto printOnly;                                            //otherwise interpret opcode
    //if seq is: $0 = literal, followed by OP_LDIs, then coalesce upto 5 LDIs   //
    if (bugLvl == 3)                                                            //
       {if (IsRegImmOp(op) && op.ri.breg == 0   &&                              // $0 = literal
            m_holdCheckE == 0                   && nxt.ldi.act == OP_LDI)       //     followed by OP_LDI
                    {m_holdLine  = GetSourceLineNum(pc);           //
                     m_holdReg0P = strdupl(regsP[0]); m_holdCheckE++; return 0;}//keep $0 for patched up display
        if (m_holdCheckE > 0 && m_holdCheckE < 6 && op. ldi.act == OP_LDI )     //second+ OP_LDI following holdoff
                    {m_holdCheckE++;                                  return 0;}//defer display
       }                                                                        //
    adr    = IsLongJmp(op) ? nxt.u16             :                              //long jump is abs adr in next op
             IsGoOp(op)    ? op.go.relAdr + pc+1 :                              //short jump is relAdr
             IsCall(op)    ? op.call.callAdr     : -1;                          //absolute address
    labelP = (adr >= 0) ? GetHereLabel(adr) : "";                               //
//print source line                                                             //
    if ((ii=m_holdCheckE > 0 ? m_holdLine : GetSourceLineNum(pc)) > 0)          //
       {if (ii != lastLine)                                                     //
           {if (ii > 0) Printf("\n---- line %04d: ", lastLine=ii);              //
            else        Printf("\n---- ");                                      //
            srcP = GetSourceLineP(pc);                                          //
            if (!(qq=strpbrk(srcP, "\r\n"))) qq = Endof((char*)srcP);           //scan for \r\n
            while (*(qq-1) == ' ') qq--;                                        //back out trailing spaces
            for (; srcP < qq; srcP++) Printf("%c", *srcP);                      //output source one char at a time
            Printf("\n");                                                       //
       }   }                                                                    //
 //Prepare hdr (for MessageBox)                                                 //
    if (m_holdCheckE > 0)                                                       //
       {catHdr,"%03d: $0<=0x%llX", pc-m_holdCheckE, reg0);                      //reconstruct display
        FormatAsString(m_hdr, sizeof(m_hdr)-3, regsP[0]);                       //decorative touch
        catHdr, ", line %d", GetSourceLineNum(pc));                             //
       }                                                                        //
    else                                                                        //
       {catHdr, "%03d:0x%04X %s, line %d",                                      //
                         pc, op.u16, m_opNameP->Show(pc, op, nxt, labelP, true),//
                         GetSourceLineNum(pc));                                 //
       }                                                                        //
    //Prepare flags (samStatus), and stakBuf                                    //
    SNPRINTF(m_flags),                                                          //
                   "samStat=(0x%X, %s), clkCount=%05lld, curRow=%d, insPt=%d",  //
                    samStat, ConditionNames(samStat), clkCount, curRow, insPt); //
    if (sp != 0)                                                                //
       {catStak, "(stak, sp=%d) ", sp);                                         //
        for (ii=0; ii < min(sp, 4); ii++)                                       //
            {if (*stakP[ii] == 'x') break;                                      //no good ones after this
             catStak, "%s%s, ", ii == 2 ? "\n  " : "", stakP[ii]);              //
       }    }                                                                   //
                                                                                //
    //Prepare samRegs for MessageBox noting changed registers                   //
    for (m_body[0]=0, ii=0; ii < MAX_REGS; ii++)                                //
        {catBody, "[%d] %s%s%s", ii,                                            //
                 ii == 0 && m_holdReg0P != NULL ? m_holdReg0P : regsP[ii],      //patch up holdE
                 strcmp(regsP[ii], m_lastRegsP[ii]) == 0 ? ", " : "*,",         //
                 (ii & 1) ? "\n" : "\t");                                       //two per line
        }                                                                       //
    catBody,  "  %s\n%s%s", m_stakBuf, m_flags, adviceP);                       //
                                                                                //
 //Echo above information to log file                                           //
    Printf("%s, %s\n", m_hdr, m_flags);                                         // 
                                                                                //
    //Output samRegs noting changed values                                      //
    for (ii=0; ii < MAX_REGS; ii++)                                             //
        {Printf("%s%s", (ii & 3) == 0 ? "  " : "",                              //
                      ii == 0 && m_holdReg0P != NULL ? m_holdReg0P : regsP[ii]);//     
         Printf("%s", strcmp(regsP[ii], m_lastRegsP[ii]) == 0  ? ",  " : "*, ");//
         if ((ii & 3) == 3) Printf("\n");                                       //
         free(m_lastRegsP[ii]); m_lastRegsP[ii] = strdupl(regsP[ii]);           //
        }                                                                       //
    free(m_holdReg0P); m_holdReg0P = NULL;                                      //
    if (sp != 0) Printf("     %s\n", m_stakBuf);                                //
                                                                                //
printOnly:                                                                      //
    if (op.g.act == OP_PRINT)                                                   //
       {if ((ii=OpPrint(pc, op, &vP[7], bugLvl)) < 0)                           //-ve means m_expect != m_actual
           {//Expectations have not been fulfilled. Cough up messsage           //
            if (!(pp=strstr(m_body, adviceP))) pp = m_body; *pp = 0;            //wipe out advice text
            if (bugLvl < 3 && m_body[0] == 0)                                   //
               {catHdr,"%03d: $0<=0x%llX", pc-m_holdCheckE, reg0);              //reconstruct display
                FormatAsString(m_hdr, sizeof(m_hdr)-3, regsP[0]);               //decorative touch
                catBody, "%s", m_srcFileNameP);                                 //
                catBody,"#%04d", GetSourceLineNum(pc));                         //
               }                                                                //
            catBody,"\n >>>> expect=%s\n >>>> actual=%s%s",                     //re-append expect, actual, & advice
                                              m_expect, m_actual, adviceP);     //
            printf("\x7");                                                      //bell
            m_singleStepB = true;                                               //force messagebox
           }                                                                    //
        else if (bugLvl < 3)                      return 0;                     //
       } //OP_PRINT...                                                          //
    if (op.u16 == OP_STOP && m_holdCheckE == 0)                                 //op_stop - no choices dumbo
        {if (pp=strstr(m_body, adviceP)) *pp = 0;                               //wipe out advice text
         response = MB_OK; m_singleStepB = true;                                //
        }                                                                       //
    else response = MB_YESNOCANCEL;                                             //
    if ((m_singleStepB          ||                                              //user requested singleStep
        (op.u16      == OP_STOP ||                                              //outright stop
         (op.shortOp == OP_BUG  && op.bug.level >= 3)                           //bug set to 3 or more
                                                      && m_holdCheckE == 0)) && //not holding off 
        m_body[0] != 0)                                                         //no MessageBox if text is empty
       lastii = MessageBoxA(NULL, m_body, m_hdr, response);                     //
    switch(lastii)                                                              //
        {case IDYES:    m_singleStepB = true;  response = 0; break;             //continue single stepping
         case IDNO:     m_singleStepB = false; response = 0; break;             //full speed mode
         default:       return -1;                                              //terminate emulation
        }                                                                       //
    if (m_holdCheckE > 0) {m_holdCheckE = 0; goto top;}                         //held off while LDI's were interpretted
    return response;                                                            //
#undef catBody
#undef catHdr
#undef catStak
   } //cSimDriver::CheckE...

//Assemble #expect: and #actual: messages up to the next '\n' then compare
//them against each other. The text of the OP_PRINTF message is stored in
//inputFileName.messages, juiced up by the values of various registers
//denoted by $digit in the message, eg., print "reg[0]=$0\n";
int cSimDriver::OpPrint(int pc, OPCODE op, uint64_t *regsP, int bugLevel)
   {char *msgP, *pp, ch[256]; const char *ppc; int len=0;                       //
    if (!(msgP=(char*)m_opNameP->FindMessage(op.g.adr))) return 0;              //bull shit: message is always found
    memset(m_printBuf, 0, sizeof(m_printBuf));                                  //
    for (len=m_printBuf[0]=0; *msgP; msgP++)                                    //
        {if (*msgP == '$')                                                      //
             {if (strnicmp(msgP, "$line", 5) == 0)                              //
                 {snprintf(&m_printBuf[len], sizeof(m_printBuf)-len-1, "0x%X",  //
                                      GetSourceLineNum(pc));                    //
                  msgP += 4;                                                    //
                 }                                                              //
              else                                                              //
              if (strnicmp(msgP, "$pc", 3) == 0)                                //
                 {snprintf(&m_printBuf[len],sizeof(m_printBuf)-len-1,"0x%X",pc);//
                  msgP += 2;                                                    //
                 }                                                              //
              else                                                              //
              snprintf(&m_printBuf[len], sizeof(m_printBuf)-len-1,              //
                                    "%llX", regsP[(*(++msgP)&7)]);              //
              len += istrlen(&m_printBuf[len]);                                 //
             }                                                                  //
         else m_printBuf[len++] = *msgP;                                        //
         if (len > sizeof(m_printBuf)-10)                                       //
            {m_actualB = m_expectB = false; m_errorB = true;                    //
             return Error(ERR_1566, m_printBuf, "");                            //1566 = Buffer is too small to perform requested operation.
        }   }                                                                   //
    if (bugLevel >= 3)                                                          //
       {Printf(">>>> "); OutputDebugStringA(">>>> ");                           //noisy environment
        for (pp=m_printBuf; *pp; pp++)                                          //
           {DisplayChar(ch, *pp); Printf("%s", ch); OutputDebugStringA(ch);}    //
        Printf(" <<<<\n"); OutputDebugStringA(" <<<<\n");                       //isolate user string
       }                                                                        //
    else {Printf("%s", m_printBuf); OutputDebugStringA(m_printBuf);}            //
//#expect: message. capture message up to \n in m_expect                        //
    if (msgP=strstr(m_printBuf, ppc="#expect:"))                                //
       {m_expect[0] = 0; m_expectB = !(m_actualB=false); msgP += strlen(ppc);}  //assert expectB, deassert actualB
    else msgP = m_printBuf;                                                     //
    if (m_expectB)                                                              //
       {CAT_THIS(m_expect), "%s", msgP);                                        //concatenate expected msg to m_expect
        if (pp=strchr(m_expect, '\n')) {m_expectB = false; *pp = 0;}            //end of expectations
       }                                                                        //
//#actual: message. capture message up to \n in m_actual                        //
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
            m_errorB = true; return m_expectErr=-ERR_2739;                      //2739 = #expect: <value> not equal to #actual: <value> in simulation.
       }   }                                                                    //
    return 0;                                                                   //
   } //cSimDriver::OpPrint...

//if (samOp.bug.dump) $write("#checkF:%...\n", clkCount, samOp, pc, curRow, rowOut);
int cSimDriver::CheckF(char *lineP, char *ff[20], uint64_t *vP, bool *invalidP, int item)
   {int    grpsPerItem, grp, ii, jj, kk, pc, row, perLine;                      //
    char  *rowP = ff[5] + 2*g_memRowSize, buf[99];                              //point to endof row
    bool   dupB;                                                                //
    OPCODE op=*(OPCODE*)&vP[2];                                                 //must be OP_BUG
                                                                                //
    pc       = (int)vP[3];                                                      //
    row      = (int)vP[4];                                                      //curRow
    Printf("Row[0x%X], op=%s(0x%04X)\n", row, m_opNameP->Show(pc,op,op),op.u16);//
    if (op.bug.sho >= 2) {item = ITEM_INDX; perLine = 32;}                      //BUG_SHO_RAW or BUG_SHO_INDX
    else                 {item = ITEM_PAGE; perLine = 24;}                      //
    grpsPerItem = m_padSz[item] / TARGETBUS_SIZE;                               //groups per hITEM
    for (ii=kk=0, dupB=false; ii < (int)g_memRowSize; ii+=TARGETBUS_SIZE)       //
        {grp = ii / m_targetBusSz;                                              //
         if ((ii % perLine) == 0) Printf("   %03X: ", ii);                      //
         if ((grp % grpsPerItem) == (grpsPerItem-1) &&                          //
              DisplayableHex(rowP-2*m_keySize, buf))                            //
                 {Printf("\"%s\"         ", buf); rowP -= 2*TARGETBUS_SIZE;}    //
         else                                                                   //
            {for (jj=0; jj < TARGETBUS_SIZE; jj++, rowP-=2)                     //
                   Printf("%c%c%s", *(rowP-2), *(rowP-1),                       //
                          (jj & 7) ==  7 ? "  "  : (jj &  3) ==  3 ? "_" : ""); //
            }                                                                   //
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
   } //cSimDriver::CheckF...

//#checkG: messages created by sequencerGroup.sv when bugLevel >= 6.
//      0      1       2       3       4        5        6         7         8
//#checkG: clkCount, lastOp, target, dramI, GROUP_NO, grpRsltI, grpRsltO, stopBit
//NOTE: CheckG is called multiple times, once for each bank in the sequencerArray. Each
//      bank comprises one or more groups of which only a portion will be situated over a key.
//      These are aggregated to display a single set of ducks which is verified against
//      m_grpMaskBinP which is the mask of cells not expected to right shift their data
//      (read from LS bit, ie., bit[0] == group[0]).
int cSimDriver::CheckG(char *lineP, char *ff[20], uint64_t *vP, bool *invalidP, int item)
   {char     erBuf[100], *dTgtP, *rowP, *keyP, *tgtP=ff[3], *dramP=ff[4];       //
    int      grpN=(int)vP[5], grpRsltI=(int)vP[6], grpRsltO=(int)vP[7], stop=(int)vP[8];//
    int      bankNum, grpsPerItem, grpInx, row=m_curRow, ii, lastBit,           //
             cmp,bit,offset;                                                    //
                                                                                //
    grpsPerItem = m_padSz[item] / m_targetBusSz;                                //groups per hITEM
    grpInx      = grpN % grpsPerItem;                                           //group posn within hITEM
    bankNum     = grpN / grpsPerItem;                                           //
    dTgtP       = (char*)alloca(2*m_keySize+10);                                //
                                                                                //
    if (bankNum == 0)                                                           //first group in bank of groups
       {//first group in group[] array                                          //                "
        ShowCheckHdr(0, "SeqGrup", vP);                                         //(0 - never GO_T or GO_F)
        DisplayableHex(tgtP, dTgtP);                                            //                "
        Printf(", bram[%d], tgt=\"%s\"\n", row, dTgtP);                         //                "
        m_grpStoppedB = false;                                                  //                "
        return m_insertPt=0;                                                    //                "
       }                                                                        //                "
                                                                                //
    if (m_grpStoppedB)                                                return 0; //middle groups in group[] 
    m_grpStoppedB     = stop != 0;                                              //stop on next call to CheckG
    ReverseUnhex(tgtP);                                                         //
    ReverseUnhex(dramP);                                                        //
    if (bankNum != (m_maxGroups-1) && stop == 0)                      return 0; //                "
                                                                                //
    //last group in group[]; check everything                                   //
    rowP                = RowFromBlockramData((int)row);                        //read from file  "
    if (m_grpMaskBinP)
       {Printf(", [");                                                          //[] to be consistent with documentation
        for (ii=0; ii < bankNum; ii++) Printf("%s", BIT_VECTOR(m_grpMaskBinP, ii*2) ? ">=" : "< ");
        Printf("]");                                                            //
        //Verify the ducks against the actual data.                             //
        offset = m_align[item]+m_baseSz[item];                      //offset of key in cITEM record
        for (ii=0, lastBit=1, m_insertPt=-1; ii < bankNum; lastBit=bit, ii++)   //
            {keyP = &rowP[ii*m_padSz[item]+offset];                       //location of key in hINDX/hBOOK
             cmp  = CompareKey(tgtP, keyP, m_keySize);                          // (-1) means tgtP < keyP. 0= tgtP == keyP
             bit  = BIT_VECTOR(m_grpMaskBinP, 2*ii);                            //
             if (lastBit == 1 && bit == 0) m_insertPt = ii;                     //
             if ((cmp <= 0) != bit == 0)                                        //is relop consistent with setting of grpMask
               {SNPRINTF(erBuf), "\"%s\" !%c \"%s\"", tgtP,                     //
                                  cmp < 0 ? '<' : cmp == 0 ? '=' : '>', keyP);  //
                Printf("\n"); return Error(ERR_2713, "", erBuf);}               //ducks do not correspond to actual data
            } //for (ii=... if (grpMask                                         //
        Printf(", insertPt=%d (ok)\n", m_insertPt);                             //
       }                                                                        //
    return 0;                                                                   //
   } //cSimDriver::CheckG...

char *cSimDriver::FmtGroupMask(const char *grpMaskP, int item)
   {static char buf[256];                                                       //
    int         ii, jj, kk, bufSize=sizeof(buf)-1,                              //
                m_grpsPerItem = m_padSz[item] / m_targetBusSz;            //
    for (jj=0, ii=m_maxGroups, grpMaskP+=m_maxGroups, buf[0]=0; --ii >= 0; jj++)//
       snprintf(&buf[kk=istrlen(buf)], bufSize-kk-1,                            //
              "%c%s", *--grpMaskP, (jj % m_grpsPerItem) == (m_grpsPerItem-1) ? " " : ""); //
    return buf;
   } //cSimDriver::FmtGroupMask...

void cSimDriver::PrintGrpMask(const char *titleP)
   {Printf(titleP);
    for (int ii=(m_maxGroups+7)/8; (ii-=2) >= 0;) Printf("%02X", m_grpMaskBinP[ii]);
   } //cSimDriver::PrintGrpMask...

//Search for file name ignoring \ and / differences.
//Replace directoryName from the front of files and replaces with '.'
void cSimDriver::RemoveSourceDir(char *lineP)
  {const char *dirP=getenv("verilogSourceDir");
   for (; *lineP; lineP++)
     for (int ii=0; lineP[ii] != 0; ii++)
         {if (dirP[ii] == 0)
             {strcpy(lineP++, ".");
              strcpy(lineP, lineP+strlen(dirP)-1);
              return;
             }
          if (!((lineP[ii] == '\\' && dirP[ii] == '/' ) ||
                (lineP[ii] == '/'  && dirP[ii] == '\\') ||
                (lineP[ii] == dirP[ii]              ))) break;
         }
  } //cSimDriver::RemoveSourceDir...

//Read row from blockRam.data; this requires converting to binary and reversing bytes.
char *cSimDriver::RowFromBlockramData(int rowNum)
   {char         name[_MAX_PATH];
    char        *sP, *dP;
    int          ii, sz2; //initially *2 bytes/hex char
    uint8_t      u8;
    FILE        *fileP;
    if (rowNum != m_lastRowNum)
       {SNPRINTF(name), "%s\\blockRam.data", getenv("VerilogSimulationDir"));
        if (!(fileP=fopen(name, "rb"))) return NULL;
        sz2 = m_rowBytes*2;
        if (m_testRowP == NULL) m_testRowP = (char*)malloc(sz2);
        fseek(fileP, (m_lastRowNum=rowNum)*(sz2+1), SEEK_SET); //row + '\n'
        fread(m_testRowP, sz2,1, fileP);
        fclose(fileP);
        //Convert row to binary; rowSz = actual size
        for (ii=0, sP=dP=m_testRowP, sz2/=2; ii < sz2; ii++, sP+=2) *dP++ = Hex2Bin(sP);
        //Reverse order of bytes
        for (ii=0, dP=&(sP=m_testRowP)[sz2]; ii < sz2/2; ii++) {u8 = *sP; *sP++ = *--dP; *dP = u8;}
       }
    return m_testRowP;
   } //cSimDriver::RowFromBlockramData...

void cSimDriver::PrintLine(const char *lineP, const char *msgP)
    {while (*lineP != 0)
       {if (*lineP < 0x20) Printf("\\%02X ", *lineP++ & 0xFF);
        else               Printf("%c",      *lineP++);}
     Printf("%s", msgP);
    } //cSimDriver::PrintLine...

void cSimDriver::LaunchChild(HANDLE hChildStdOut, HANDLE hChildStdIn, HANDLE hChildStdErr)
   {PROCESS_INFORMATION pi;
    STARTUPINFO         si;
    LPTSTR              cmdExe =_tcsdup(TEXT("c:\\windows\\sysWOW64\\cmd.exe"));
    LPTSTR              commandP;
    char                emuCmd[_MAX_PATH]={0}, *fnP;
    int                 ii;
    TCHAR               emuCmdw[_MAX_PATH]={0};

    // Set up the start up info struct.
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb          = sizeof(STARTUPINFO);
    si.dwFlags     = STARTF_USESTDHANDLES |
                     STARTF_USESHOWWINDOW;          //needed if you want to use the wShowWindow flags.
    si.hStdOutput  = hChildStdOut;                  //replace std handle with hoked up versions
    si.hStdInput   = hChildStdIn;                   //replace std handle with hoked up versions
    si.hStdError   = hChildStdErr;                  //replace std handle with hoked up versions
    si.wShowWindow = m_showAllB ? SW_SHOWNORMAL : SW_HIDE;//
    if (m_environment == ENV_SW) m_emulateB = true; //cmd /emulate or $environment select software emulation
    // Launch the process (reprocess.bat, emu.exe, compile.bat, or runSim.bat)
    SNPRINTF(emuCmd), "/k c:\\svn\\bin\\emu.bat %s", fnP=StripFileName(m_srcFileNameP)); free(fnP);
    if (stricmp(&emuCmd[(ii=istrlen(emuCmd))-4], ".sam") == 0) emuCmd[ii-=4] = 0;    //stip[ off '.sam'
    for (ii++; ii >= 0;) {emuCmdw[ii] = emuCmd[ii]; ii--;}
    commandP = _tcsdup(m_skipXsimB ? TEXT("/k c:\\svn\\sam\\ver-15\\sadram\\reprocess.bat") :
                       m_compileB  ? TEXT("/k c:\\svn\\sam\\ver-15\\sadram\\compile.bat")   :
                       m_emulateB  ? emuCmdw                                                :
                                     TEXT("/k c:\\svn\\sam\\ver-15\\sadram\\runsim.bat fromSim.exe"));
    m_start = time(NULL);                          //
    if (CreateProcess(cmdExe, commandP,            //LPCSTR lpApplicationName, lpCommandLine,
                       NULL,NULL,                  //LPSECURITY_ATTRIBUTES lpProcessAttributes, lpThreadAttributes
                       TRUE,                       //BOOL bInheritHandles
                       CREATE_NEW_CONSOLE,         //DWORD dwCreationFlags
                       NULL,NULL,                  //LPVOID lpEnvironment, LPCSTR lpCurrentDirectory
                       &si, &pi))                  //LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation
        m_childProcessHandle = pi.hProcess;
    else SHO_ERR();
    if (!(WaitForInputIdle(pi.hProcess, INFINITE))) SHO_ERR();
    // Close any unnecessary handles.
    if (!CloseHandle(pi.hThread)) SHO_ERR();
   } //cSimDriver::LaunchChild...

//Patch up cSadram class and call g_bugP->BugAll()
#if 0
bool cSimDriver::BugThis(uint8_t *bramP, int bramRows)
   {bool      bb;                                                               //
    int       bugLevel= g_bugLevel;                                             //save for exit
    char     *userFileNameP;                                                    //
    cBugSim   bugSim(m_params);                                                 //
    g_bugSimP         =             &bugSim;                                    //
    g_bugP            = (cBugIndex*)&bugSim;                                    //
    g_bramP           = bramP;                                                  //for ShowAdr()
    g_bugLevel        = m_params.bugLevel;                                      //
    g_doubleBookB     = true;                                                   //
    g_sP->PushBook((cBOOK*)&bramP[m_params.b1st * g_memRowSize], m_bUsed);      //preset indexbase for cSadram
    BUGOUT("---- #Starting bugSim ----\n");                                     //
    userFileNameP     = FullFileName("user.data");                              //copy of original source data
    if (bb=bugSim.ValidateBram(bramP, m_bUsed, bramRows))                       //
        bb            = bugSim.Bug(NULL, cBugSim::ShowAdr, userFileNameP);      //special adr display routine
    g_sP->PopBook();                                                            //clear m_bookP in cSadram
    g_bugLevel        = bugLevel;                                               //
    g_bramP           = NULL;                                                   //
    g_bugSimP         = NULL;                                                   //
    g_bugP            = NULL;                                                   //
    return bb;
   } //cSimDriver::BugThis...
#else
bool cSimDriver::BugThis(uint8_t *bramP, int bramRows) {return false;}
#endif

//Read params.data and convert from ascii to hex in m_params.
//Read samDefines.sv, check some variables, and populate hParams
//if g_skipXsimB then check bugLevel is the same as the value in simulate.log
int cSimDriver::ReadParams(sHDW_PARAMS *hParamsP)
   {typedef struct
         {const char *nameP; int *valuP; int expected; bool paramB;} hVARIABLES;//
    int         item_indx, item_page, item_book, dd=0, erC=0;                   //
    char        *bufP=NULL, *lineP;                                             //
    const char *ccP="";                                                         //
    int         ii=0, userCmds=0, *adrP, ignore;                                //
    hVARIABLES  vbls[] =                                                        //
       {{""},                                      //true for 'parameter foo='  //+0 for miscellaneous uses of err:
        {"BRAM_ROWS",       NULL,           0,                 true},           //+1 
        {"TARGETBUS_SIZE",  &m_targetBusSz, m_targetBusSz,     true},           //+2 
        {"ROW_BYTES",       &m_rowBytes,    (int)g_memRowSize, true},           //+3 number of bytes in BRAM row
        {"ITEM_INDX",       &item_indx,     ITEM_INDX,         true},           //+4 
        {"ITEM_PAGE",       &item_page,     ITEM_PAGE,         true},           //+5 
        {"ITEM_BOOK",       &item_book,     ITEM_BOOK,         true},           //+6 
        {"MICROCODE_SIZE",  &m_microCodeSz, 0,                 true},           //+7 space for SAM program

        {"SC_START",        NULL,           SC_START,         false},           // 0 initialization
        {"SC_IDLE",         NULL,           SC_IDLE,          false},           // 1 
        {"SC_GETOP",        NULL,           SC_GETOP,         false},           // 2 
        {"SC_DO_OP",        NULL,           SC_DO_OP,         false},           // 3 
        {"SC_SCAN",         NULL,           SC_SCAN,          false},           // 4 
        {"SC_READ",         NULL,           SC_READ,          false},           // 5 
        {"SC_READ1",        NULL,           SC_READ1,         false},           // 6 
      //{"NU_XCHG",         NULL,           NU_XCHG,          false},           // 7
        {"SC_R2R",          NULL,           SC_R2R,           false},           // 8 
        {"SC_WRYT",         NULL,           SC_WRYT,          false},           // 9 write target to dram
        {"SC_WRYT1",        NULL,           SC_WRYT1,         false},           //10 
        {"SC_SCIN",         NULL,           SC_SCIN,          false},           //11 scan/insert
        {"SC_SCIN1",        NULL,           SC_SCIN1,         false},           //12 
        {"SC_ARITH1",       NULL,           SC_ARITH1,        false},           //13 
        {"SC_XTOS",         NULL,           SC_XTOS,          false},           //14
        {"SC_RDF1",         NULL,           SC_RDF1,          false},           //15,
        {"SC_RDF2",         NULL,           SC_RDF2,          false},           //16, 

        {"OP_CALL",         NULL,           OP_CALL,           true},           //0x00 
        {"OP_BUG",          NULL,           OP_BUG,            true},           //0x18
        {"OP_STOP",         NULL,           OP_STOP,           true},           //0x0018
        {"OP_RDF",          NULL,           OP_RDF,            true},           //0x008 
        {"OP_WRF",          NULL,           OP_WRF,            true},           //0x108 
        {"OP_ARITH",        NULL,           OP_ARITH,          true},           //0x02
        {"OP_RI",           NULL,           OP_RI,             true},           //0x06 
        {"OP_RET",          NULL,           OP_RET,            true},           //0x0A
        {"OP_CFG_G",        NULL,           OP_CFG_G,          true},           //0x8A 
        {"OP_CFG_C",        NULL,           OP_CFG_C,          true},           //0xAA 
        {"OP_REPREG",       NULL,           OP_REPREG,         true},           //0x0E
        {"OP_REPEAT",       NULL,           OP_REPEAT,         true},           //0x12
        {"OP_CROWI",        NULL,           OP_CROWI,          true},           //0x16 
        {"OP_CROW",         NULL,           OP_CROW,           true},           //0x1A
        {"OP_PRINT",        NULL,           OP_PRINT,          true},           //0x1E
        {"OP_GO_T",         NULL,           OP_GO_T,           true},           //0x01 
        {"OP_GO_F",         NULL,           OP_GO_F,           true},           //0x05 
        {"OP_NOOP",         NULL,           OP_NOOP,           true},           //0x05 
        {"OP_LDI",          NULL,           OP_LDI,            true},           //0x03 
        {"OP_READ",         NULL,           OP_READ,           true},           //0x04 
        {"OP_WRYT",         NULL,           OP_WRYT,           true},           //0x0C 
        {"OP_SCAN",         NULL,           OP_SCAN,           true},           //0x14 
        {"OP_SCIN",         NULL,           OP_SCIN,           true},           //0x1C 

        {"OPS_ADD",         NULL,           OPS_ADD,           true},           //0x00
        {"OPS_ADC",         NULL,           OPS_ADC,           true},           //0x01
        {"OPS_SUB",         NULL,           OPS_SUB,           true},           //0x02
        {"OPS_SBB",         NULL,           OPS_SBB,           true},           //0x03
        {"OPS_CMP",         NULL,           OPS_CMP,           true},           //0x04
        {"OPS_XOR",         NULL,           OPS_XOR,           true},           //0x05
        {"OPS_OR",          NULL,           OPS_OR,            true},           //0x06
        {"OPS_AND",         NULL,           OPS_AND,           true},           //0x07
        {"OPS_XSUB",        NULL,           OPS_XSUB,          true},           //0x08 
        {"OPS_XSBB",        NULL,           OPS_XSBB,          true},           //0x09 
        {"OPS_INC",         NULL,           OPS_INC,           true},           //0x0A
        {"OPS_DEC",         NULL,           OPS_DEC,           true},           //0x0B
        {"OPS_SHL",         NULL,           OPS_SHL,           true},           //0x0C
        {"OPS_SHR",         NULL,           OPS_SHR,           true},           //0x0D
        {"OPS_RCL",         NULL,           OPS_RCL,           true},           //0x0E
        {"OPS_RCR",         NULL,           OPS_RCR,           true},           //0x0F
        {"OPS_R2R",         NULL,           OPS_R2R,           true},           //0x10
        {"OPS_XCHG",        NULL,           OPS_XCHG,          true},           //0x11
        {"OPS_XTOS",        NULL,           OPS_XTOS,          true},           //0x12
        {"OPS_POP",         NULL,           OPS_POP,           true},           //0x13
        {"OPS_PUSH",        NULL,           OPS_PUSH,          true},           //0x14
        {"OPS_STC",         NULL,           OPS_STC,           true},           //0x16
        {"OPS_CLC",         NULL,           OPS_CLC,           true},           //0x17
        {"OPS_STZ",         NULL,           OPS_STZ,           true},           //0x18
        {"OPS_CLZ",         NULL,           OPS_CLZ,           true},           //0x19
       };                                                                       //
                                                                                //
    memset(hParamsP, -1, sizeof(sHDW_PARAMS));                                  //
    memset(&m_params,-1, sizeof(m_params));                                     //
    m_samDefinesSrcP = NULL;                                                    //
                                                                                //
   //Update table values by reading the verilog file 'samDefines.sv'            //
    if (ReadAllFile(ccP="samDefines.sv", NULL, &m_samDefinesSrcP) < 0) ERR(ERR_0003); //
    for (ii=1; ii < HOWMANY(vbls); ii++)                                        //
        {ccP = vbls[ii].nameP;                                                  //
         if ((adrP=vbls[ii].valuP) == NULL) adrP = &ignore;                     //
         if (VerilogParam(ccP, adrP, vbls[ii].paramB) == NULL)   ERR(ERR_2720); //2720 = Unable to find %s
         if (*adrP != vbls[ii].expected && vbls[ii].expected !=0)ERR(ERR_2712); //2721 = Incorrect value for *ccP
        }                                                                       //
    hParamsP->targetBusSz = m_targetBusSz;                                      //
    hParamsP->rowSize     = g_memRowSize = m_rowBytes;                          //
    m_maxGroups           = m_rowBytes / m_targetBusSz;                         //number of groups across DRAM row
    if (!m_skipXsimB) return 0;                                                 //
//---- Reprocessing existing simulate.log file -------------------------------- //
    //Check bugLevel against simulate.log. Can't support different values       //
    if (ReadAllFile(ccP="simulate.log", NULL, &bufP) < 0)        ERR(ERR_0003); //0003 = file not found
    if (!(lineP=strstr(bufP,ccP="##Params:")))                   ERR(ERR_2720); //2720 = Unable to find "##Params:"
    if (strpbrk(lineP, "\r\n")) *strpbrk(lineP, "\r\n") = 0;                    //Get operational parameters
    if (m_keySize         != (int)GetParam(lineP, ccP="keySize="))ERR(ERR_2721);//2721 = %s is not consistent with value from previous simulation
    if (m_rowBytes        != (int)GetParam(lineP, ccP="rowSize="))ERR(ERR_2721);//Current RowSize is invalid
    free(bufP); return 0;                                                       //
err:free(bufP); return Error(erC, "", ccP);                                     //
   } //cSimDriver::ReadParams...

#if 0
//open params.txt and suck out numeric values for various parameters.
//Create params.data in a format that is acceptable to Vivado.
int cSimDriver::WriteParamsData(void)
   {int       ii;                                                               //
    uint16_t *u16P;                                                             //
    FILE     *fileP;                                                            //
    if (!(fileP=fopen(FullFileName("params.data"), "wb")))                      //
        return Error(ERR_7147, "", "params.data");                              //7147 Unable to create file
    for (u16P=(uint16_t*)&m_params, ii=0; ii < sizeof(m_params)/2; )            //
         fprintf(fileP, "%04X\n", u16P[ii++]);                                  //output to params.data
    fclose(fileP); return 0;                                                    //
   } //cSimDriver::WriteParamsData..
#endif

bool cSimDriver::strBegins(const char *lineP, const char *whatP)
   {return strnicmp(lineP, whatP, strlen(whatP)) == 0;}

char const *cSimDriver::GetHereLabel(uint16_t)     {return "here";}

//Get filename from fileNum returns in IATOM
const char *cSimDriver::FileNameOnly(int fileNum)
   {const char *pp="abc.sam", *qq=pp;
    while (pp=strpbrk(qq, "\\/")) qq=pp+1; //find last '\' or '/'
    return qq;
   } //cSimDriver::StripFileName...

//Strip off directory and suffix
char *cSimDriver::StripFileName(char *fileNameP)
   {char *pp; 
    for(pp=&fileNameP[strlen(fileNameP)]-1; *pp != '/' && *pp != '\\'; pp--){};
    pp = strdupl(pp+1);
    if (strchr(pp, '.')) *strchr(pp, '.') = 0;
    return pp;
   } //cSimDriver::StripFileName...

char const *cSimDriver::GetSourceLineP(int)        {return "a longline";}
int cSimDriver::GetSourceLineNum(int pc)      
   {sSRC_REF *mapP; const char *lP, *endLP=&m_lineMapP[m_lineMapSize];
    for (lP=m_lineMapP; lP < endLP; lP += sizeof(sSRC_REF))
         {mapP = (sSRC_REF*)lP;
          if (mapP->pc >= pc) return mapP->lineNum;
         }
     return 0;
   } //cSimDriver::GetSourceLineNum...

//What a monumental cockup:
// - date and time are intermixed (standard format for Martians i suppose).
// - the text contains \n (great convenience when embedding in a larger message)!
//   Any dimwit can add '\n' but taking it off is a hassle.
//This routine straightens out this mess.
char *cSimDriver::Fixctime(time_t *nP)
   {static char buf[25]; char b1[32];                                           //
    SNPRINTF(b1), "%s", ctime(nP));                                             //
    memset(buf, ' ', sizeof(buf));                                              //
    memmove(&buf[0],  &b1[0], 10); //"Thu Dec 15"                               //
    memmove(&buf[11], &b1[20], 4); //"2022"                                     //
    memmove(&buf[16], &b1[11], 9); //"17:38.20"                                 //
    buf[24] = 0;                                                                //
    return buf;                                                                 //
   } //cSimDriver::Fixctime...

int cSimDriver::_ErrorN(int erC, uint32_t p1, CC fileNameP, int line, const char *fncP)
   {char b1[20];
    SNPRINTF(b1), "%d", p1);
    return _Error(erC, "", b1, fileNameP, line, fncP);
   } //cSimDriver::_ErrorN..

//erC == 0 is a call to just publish an error that has already been logged.
int cSimDriver::_Error(int erC, CC contextP, CC paramsP, CC fileNameP, int line, const char *fncP)
   {char erBuf[MAX_ERROR_MESSAGE_SIZE+256], fn[_MAX_PATH+10]; int len;          //
    if (erC == 0) erC = g_err.GetLastError();                                   //
    else          erC = g_err.LogError(erC, contextP, paramsP);                 //
    SNPRINTF(fn), "\n%s #%04d, function=%s", (char*)fileNameP, line, fncP);     //
    if (g_alwaysMessageBoxB || g_err.Severity(erC) == XSEVERITY_MESSAGEBOX)     //
       {g_err.ShortError(erC, erBuf, sizeof(erBuf)); erBuf[sizeof(erBuf)-1] = 0;//
        len = istrlen(erBuf);                                                   //
        Printf("%s\x07", erBuf);                                                //beep :(
        strncat(erBuf,"\n\tPress Yes for more information\n"                    //
                        "\tPress No  to ignore error and continue\n"            //
                        "\tPress Cancel to abort further processing",           //
               sizeof(erBuf)-len-1);                                            //
        switch (MessageBoxA(NULL, erBuf, fn,MB_YESNOCANCEL))                    //
             {case IDNO:     return 0;                                          //
              case IDCANCEL: return erC;                                        //
             }                                                                  //
        erBuf[len] = 0;                                                         //strip 'Press yes...'
        g_err.AddContext(fn);                                                   //
        g_err.FullError(erC, erBuf, sizeof(erBuf));                             //
        Printf("\n%s\n", erBuf);                                                //
   //?  if (strpbrk(erBuf, "\r\n")) *strpbrk(erBuf, "\r\n") = 0;                //vas is das ?
        MessageBoxA(NULL, g_err.LocateErrorP(erC), erBuf, MB_OK);               //
        return erC;                                                             //
       }                                                                        //
    g_err.AddContext(fn);                                                       //
    g_err.ShortError(erC, erBuf, sizeof(erBuf));                                //
    Printf("\n%s\n", erBuf);                                                    //
    return erC;                                                                 //
   } //_Error...

int main(int argc, char **argv)
   {cSimDriver  sim;                                                            //
    int         ii, errors, erC=0, keySize=8;                                   //
    DWORD       wd=0;
    char        ch;                                                             //
    const char *msgP;                                                           //
    #define IF_ARG(what) if(argv[ii][0] == '/' && stricmp(argv[ii]+1,what) == 0)//
                                                                                //
    g_alwaysMessageBoxB = true;                                                 //for errors
    sim.m_emulateB      = true;                                                 //
    if (false)                                                                  //
        sim.m_skipXsimB = true;                                                 //analyze simulate.log but dont run xsim.exe
    if (false)                                                                  //
        sim.m_emulateB  =false;                                                 //
    if (false)                                                                  //
        sim.m_patchDrvB = true;                                                 //patch SimulationDriver.sv
    if (false)                                                                  //
        sim.m_expandOnlyB = true;                                               //
    strncpy(g_exeFileDir, argv[0], sizeof(g_exeFileDir)-1);                     //
    if (strstr(g_exeFileDir, "samCompile") != NULL) sim.m_compileB = true;      //program is called samCompiler.exe
    for (ii=istrlen(g_exeFileDir); --ii > 0 && g_exeFileDir[ii] != '\\';) {}    //
    g_exeFileDir[ii+1] = 0;                                                     //directory name of executable
    for (ii=1; ii < argc; ii++)                                                 //
      {IF_ARG("bugLevel") g_bugLevel         = strtol(argv[++ii],NULL,10); else //
       IF_ARG("capture")  sim.m_captureFileP = argv[++ii];                 else //specify capture file
       IF_ARG("compile")  sim.m_compileB     = sim.m_showAllB = true;      else //call compiler only
       IF_ARG("dryRun")   sim.m_dryRunB      = true;                       else //specify capture file
       IF_ARG("emulate")  sim.m_emulateB     = true;                       else //specify software emulation (emu.ex)
       IF_ARG("xilinx")   sim.m_emulateB     = false;                      else //specify Xilinx simulation(xsim.exe)
       IF_ARG("include")  sim.m_includeDirP  = argv[++ii];                 else //specify include directory
       IF_ARG("macro")    sim.m_expandOnlyB  = true;                       else //patch 'parameter MICROCODE_SIZE' in SimulationDriver.sv
       IF_ARG("patchDrv") sim.m_patchDrvB    = true;                       else //patch 'parameter MICROCODE_SIZE' in SimulationDriver.sv
       IF_ARG("showAll")  sim.m_showAllB     = true;                       else //show all piped input
       IF_ARG("xlatOnly") sim.m_skipXsimB    = true;                       else //pass simulate.log thru ReceiveFilter again.
       if (strcmp(argv[ii], "/h") == 0 || strcmp(argv[ii], "/?") == 0)          //help
                                     return sim.Help(argv[ii], argv[ii+1]);else //
       if (sim.m_srcFileNameP == NULL) sim.m_srcFileNameP = argv[ii];      else //
          {sim.Error(ERR_2005, NULL, argv[ii]); return 1;}                      //2005 = Missing or unknown command line argument'%s'
      }                                                                         //
                                                                                //
    if (!sim.m_srcFileNameP) {sim.Error(ERR_2741, "", ""); return 1;}           //2741 missing source file name
                                                                                //
    if ((erC=sim.StartSimulator(keySize)) >= 0)                                 //Start xsim and monitor outputs
       {for (errors=0;;)                                                        //
           {while (_kbhit() > 0)                                                //
               {switch(ch=_getch())                                             //
                  {case 'h': case 'H': msgP = "help\n";    break;               //
                   case 'q': case 'Q': msgP = "quit\n";    break;               //
                   case 'r': case 'R': msgP = "run 1ns\n"; break;               //
                   default:                                                     //
                       if (ch >' ') printf("Unknown cmd %c(0x%02X)\n", ch,ch);  //
                       continue;                                                //
                  }                                                             //
                sim.SendCmd(msgP); printf("%s\nRead for command: ", msgP);      //
               }                                                                //
            if (sim.m_doneB) break;                                             //
            if (sim.GetError() > 0 && errors++ > 3) break;                      //
            Sleep(1000);                                                        //
       }   }                                                                    //
    return erC < 0 ? 1 : 0;                                                     //
    #undef ARG_IS                                                               //
   } //main...

//end of file
