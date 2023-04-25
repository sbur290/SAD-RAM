#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <fcntl.h>
#include <c3_errors.h>
#include <opName.h>
#include "..\dll-15\\samEmulator.h"

#define TARGETBUS_SIZE 8                                                        //sim.cpp compatibility
#define ITEM_PAGE      2                                                        //        "
#define ITEM_INDX      1                                                        //        "
#define SNPRINTF(buf) buf[sizeof(buf)-1] = 0; snprintf(buf, sizeof(buf)-1
#define CAT_THIS(buf) len = istrlen(buf); buf[sizeof(buf)-1] = 0; snprintf(&buf[len], sizeof(buf)-1-len

FILE        *g_printFileP=NULL;                                                 //for Printf
int          g_memRowSize=256;                                                  //

const char KEY_FNC1  = 0x3B, KEY_SFNC1  = 0x54;                                 //function key 1
const char KEY_FNC2  = 0x3C, KEY_SFNC2  = 0x55;                                 //      "      2
const char KEY_FNC3  = 0x3D, KEY_SFNC3  = 0x56;                                 //      "      3
const char KEY_FNC4  = 0x3E, KEY_SFNC4  = 0x57;                                 //      "      4
const char KEY_FNC5  = 0x3F, KEY_SFNC5  = 0x58;                                 //      "      5
const char KEY_FNC6  = 0x40, KEY_SFNC6  = 0x59;                                 //      "      6
const char KEY_FNC7  = 0x41, KEY_SFNC7  = 0x5A;                                 //      "      7
const char KEY_FNC8  = 0x42, KEY_SFNC8  = 0x5B;                                 //      "      8
const char KEY_FNC9  = 0x43, KEY_SFNC9  = 0x5C;                                 //      "      9
const char KEY_FNC10 = 0x44, KEY_SFNC10 = 0x5D;                                 //      "     10

int Error(int erC, CC meaningP, CC paramP) 
   {Printf("Error %d, %s: %s\n", erC, meaningP, paramP); return -erC;}

void Help() 
   {Printf("\nValid commands:");                                                //
    Printf("\n    h:     This help message.");                                  //help
    Printf("\n    q:     Quit simulation");                                     //quit
    Printf("\n    g:     Run at normal speed");                                 //go (run at full speed)
    Printf("\n    space  Single-Step one opcode at a time");                    //single step
    Printf("\n    b:     Run to Breakpoint ($bug opcode)");                     //run to break point ($bug)
    Printf("\n    t:     Run to Ret opcode");                                   //run to return
    Printf("\n    l:     Step line-by-line");                                   //run to next line
    Printf("\n    p:     Put value into Sam Register ");                        //
    Printf("\n    u:     Put value into stack");                                //
    Printf("\n    s:     Force Sam Status to value");                           //
    Printf("\n    digit: Set debug level to specified digit\n\n");              //
   } //Help...

    int (*OpenEmulatorP)   (const char *microcodeNameP);                        //Entry points to DLL
    int (*CloseEmulatorP)  (void);                                              //      "
    int (*ExecutePgmP)     (int mode, uint64_t p1);                             //      "
    int (*CheckEP)         (void);                                              //      "
    int (*CheckFP)         (uint64_t row, int rowType);                         //      "
    int (*GetPcP)          (void);                                              //      "
    int (*GetSpP)          (void);                                              //      "
    int (*GetCurLineP)     (void);                                              //      "
    int (*GetCurRowP)      (void);                                              //      "
    int (*GetSamRegP)      (int reg);                                           //      "
    int (*PutSamRegP)      (int reg, uint64_t data);                            //      "
    int (*PutStackP)       (int posn, uint64_t data);                           //      "
    int (*GetSamStatusP)   (void);                                              //      "
    int (*PutSamStatusP)   (uint8_t stat);                                      //      "
    int (*SetBugLevelP)    (int lvl);                                           //      "
    #define GetBugLevelP() SetBugLevelP(-1)

uint64_t GetValue(const char *msgP)
   {bool hexB=false;                                                            //
    char buf[32], ch; uint64_t val; int ii, radix=10;                           //
    Printf("%s", msgP);                                                         //
    while (true) {if (_kbhit() > 0) break; Sleep(10);}                          //wait for keystroke
    for (ii=(int)strlen(msgP); --ii >= 0;) Printf("\b  \b\b");                  //remove prompt
    for (val=0, ii=0; ii < sizeof(buf)-1; ii++)                                 //accumulate value
       {if ((ch=_getch()) == 0) {ch = getch(); continue;}                       //extended keybd functions
        if (ch == '\r' || ch == '\n') return (ii==0) ? -1 : val;                //
        Printf("%c", ch);                                                       //
        if (ch >= '0' && ch <= '9') {val = val * radix + (ch -'0'); continue;}  //decimal/hex digit
        if (ch == 'x' && ii == 1)   {hexB = true; radix = 16;       continue;}  // '0x' switch to hex
        if ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))               //hex value
                  {ch |= 0x20; val = val * radix + (ch -'a' + 10); continue;}   //
        if (ch == '=' || ch == '\n' || ch == '\r') break;                       //end of data
        Printf("\b \b");                                                        //wtf
       }                                                                        //
    return val;                                                                 //
   } //GetValue...

//p command: Store value into samReg[reg]; expect reg=data
int PutSamReg(void)
   {int reg=(int)GetValue("<reg>"); return PutSamRegP(reg, GetValue("<data>"));
   } //PutSamReg...

//u command: Store value into stack[n]: expect n=data
int PutStack(void)
   {int reg=(int)GetValue("<stack offset>"); return PutStackP(reg, GetValue("<data>"));
   } //PutStack...

//> command: Store system status: expect u8
int PutSamStatus(void)
   {return PutSamStatusP((uint8_t)GetValue("<status>"));
   } //PutSamStatus...

int ShowRow(void)
   {uint64_t row = GetValue("<row#>");
    if (row == (-1)) row = GetCurRowP();               
    return CheckFP(row, BUG_SHO_RAW);
   } //PutSamStatus...

int ByLine(void)
   {int line = (int)GetValue("<line#>"), erC;                                   //
    if (line == (-1)) return ExecutePgmP(EX_BYLINE, line);                      //plain 'l' command: line-by-line debugging
    erC = ExecutePgmP(EX_TOLINE, line);                                         //lnumber: break at specified line number
    SetBugLevelP(3);                                                            //
    return erC;                                                                 //
   } //ByLine...
 
//-------------------------------------------------------------------------------
int main(int argc, char **argv)
   {int         erC=0, keySize=8, bugLevel=-1, ii;                              //
    const char *samFileNameP=NULL, *pp;                                         //
    char        ch, dllName[_MAX_PATH];                                         //
    bool        promptB=true, checkB=true, stoppedB=false;                      //
    uint64_t    reg=0, stak=0;                                                  //
    uint8_t     stat = 0;                                                       //
    HMODULE     dllH;                                                           //handle to DLL
    #define Check(name) if (name == NULL){erC=Error(ERR_7291, "", pp);goto xit;}//7291 = Unable to locate entry point
    #define Import1(name, p1)    name##P = (int(*)(p1)    )GetProcAddress(dllH, pp=#name); Check(name##P);
    #define Import2(name, p1,p2) name##P = (int(*)(p1, p2))GetProcAddress(dllH, pp=#name); Check(name##P);
                                                                                //
    //Check command line parameters 
    for (int ii=1; ii < argc; ii++)                                             //
        {if ((pp=argv[ii])[0] == '/' || pp[0] == '-')                           //
            {if (stricmp(++pp, "bugLevel") == 0 && (ii+1) < argc)               // /bugLevel #
                {bugLevel = strtol(argv[++ii], NULL, 10);    continue;}         //
             if (stricmp( pp, "run") == 0)   {bugLevel = -1; continue;}         //runs in non debug mode
             if (strnicmp(pp, "h", 1)  == 0) {Help();        continue;}         // /h or /help
             Error(ERR_2714, "Invalid command", argv[ii]);   return 1;          //2714 = invalid command
            }                                                                   //
         else                                                                   //
         if (samFileNameP != NULL)Error(ERR_2590,"Duplicate filename",argv[ii]);//2590 = duplicate filename on command line
         else samFileNameP = argv[1];                                           //
        }                                                                       //
    if (samFileNameP == NULL)                                                   //
       {Error(ERR_2741, "Missing program name", ""); return 1;}                 //2741 = missing source file name
                                                                                //
    //load dll from same directory as executable                                //
    strncpy(dllName, argv[0], sizeof(dllName)-1);                               //
    for (ii=istrlen(dllName); --ii >= 0 && !strchr("\\//:",dllName[ii]);) {}    //
    snprintf(&dllName[ii+1],  sizeof(dllName)-ii-2, "samDll.dll");              //directory name of executable
    dllName[sizeof(dllName)-1] = 0;                                             //
    if ((dllH=LoadLibraryA(dllName)) == NULL)                                   //
       {Error(ERR_0003, "", dllName); return 1;}                                //0003 = file/directory not found
    //Get entry points within the DLL and type cast them to proper form:        //
    Import1(OpenEmulator,    const char*);                                      //
    Import1(CloseEmulator,   void);                                             //
//flow control commands                                                         //
    Import2(ExecutePgm,      int, uint64_t);                                    //
//status display routines                                                       //
    Import1(CheckE,          void);                                             //Display samRegs, stack, and status
    Import2(CheckF,          uint64_t, int);                                    //
    Import1(SetBugLevel,     int);                                              //
//service routines                                                              //
    Import1(GetSamReg,       int);                                              //
    Import2(PutSamReg,       int, uint64_t data)                                //
    Import1(GetSp,           void);                                             //
    Import1(GetPc,           void);                                             //       
    Import1(GetCurLine,      void);
    Import1(GetCurRow,       void);
    Import2(PutStack,        int, uint64_t data);                               //
    Import1(GetSamStatus,    void);                                             //
    Import1(PutSamStatus,    uint8_t);                                          //
                                                                                //
    if ((erC=OpenEmulatorP(samFileNameP)) < 0)                      goto xit;   //compile error
    if (bugLevel < 0) {erC = ExecutePgmP(EX_RUN, -1);               goto xit;}  //plain emu samProgramName; run at speed
    SetBugLevelP(bugLevel);                                                     //  otherwise
    while(true)                                                                 //      emu samProgramName /bugLevel
       {if (promptB)                                                            //
           {if (checkB) CheckEP();                                              //
            if (GetBugLevelP() < 3) Printf("\n%03d:", GetPcP());                //
            Printf(" sam >");                                                   //
            promptB = checkB = false;                                           //
           }                                                                    //
        if (_kbhit() == 0) {Sleep(100); continue;}                              //
        ch = _getch(); promptB = checkB = true;                                 //
        if (ch == 0x03) {Printf("\nbreak Detected\n");              goto xit;}  //
        if (ch == 0x00)                                                         //extended character
           {switch(ii=_getch())                                                 //
                {case KEY_FNC1: ch = 'h'; break; case KEY_SFNC1:         break; //fnc 1: help (traditional usage)
                 case KEY_FNC2:           break; case KEY_SFNC2:         break; //
                 case KEY_FNC3:           break; case KEY_SFNC3:         break; //
                 case KEY_FNC4:           break; case KEY_SFNC4:         break; //
                 case KEY_FNC5: ch = 'g'; break; case KEY_SFNC5:         break; //fnc 5: run at normal speed
                 case KEY_FNC6:           break; case KEY_SFNC6:         break; //
                 case KEY_FNC7:           break; case KEY_SFNC7:         break; //
                 case KEY_FNC8:           break; case KEY_SFNC8:         break; //
                 case KEY_FNC9:           break; case KEY_SFNC9:         break; //
                 case KEY_FNC10:ch = ' '; break; case KEY_SFNC10:        break; //
                 default:       ch = 0;   break;                                //
                }                                                               //fnc 10: singleStep
            if (ch == 0)             continue;                                  //ignore extended character (fnc, etc)
           }                                                                    //
        Printf("%c", ch);                                                       //
        switch(ch|0x20)                                                         //ower case eh?
            {case 'h':       Help();                                   continue;//help
             case 'q': if (MessageBoxA(NULL, "Terminate simulation",            //quit ?
                                    "Terminate", MB_YESNO) == IDNO)    continue;//
                       Printf("\n"); erC = 0;                          goto xit;//quit
             case '1': case '2': case '3':                                      //
                   SetBugLevelP(bugLevel=ch & 7);                               //
                   if (bugLevel >= 3) {erC = ExecutePgmP(EX_SS, -1);   break;}  //single step
                 //else fall thru                                               //
             case 'g': if ((erC=ExecutePgmP(EX_RUN, -1)) == 0)                  //
                                                      erC = -ERR_2704; break;   //go (run at full speed)
             case ' ': erC = ExecutePgmP (EX_SS, -1);                  break;   //
             case 'b': erC = ExecutePgmP (EX_2BREAK, -1);              break;   //run to break point ($bug)
             case 't': erC = ExecutePgmP (EX_2RET, -1);                break;   //run to return
             case 'l': erC = ByLine();                                 break;   //run to next line
             case 'm': erC = ShowRow     (); checkB = false;           continue;//
             case 'p': erC = PutSamReg   (); Printf("\n");             break;   //
             case 'u': erC = PutStack    (); Printf("\n");             break;   //
             case 's': erC = PutSamStatus(); Printf("\n");             break;   //
             default:                                                  continue;//
            }                                                                   //
        if (erC == (-ERR_2704) && bugLevel >= 0)                                //2704 = stop encountered; bug >= 0 means not /run option
           {snprintf(dllName, sizeof(dllName),                                  //
                "OP_STOP Encountered at pc=%d, line=%d",GetPcP(),GetCurLineP());//
            MessageBoxA(NULL, dllName, "Stopped", MB_OK);                       //
           }                                                                     //
        if (erC == (-ERR_2739))                                                 //2739 = $expect != $actual
           {snprintf(dllName, sizeof(dllName),                                  //
                     "pc = %d, line=%d\n"                                       //
                     "\tPress Yes to continue\n\tPress No to terminate",        //
                     GetPcP(), GetCurLineP());                                  //
            if (MessageBoxA(NULL, dllName, "$expected value != $actual value",  //
                                                  MB_YESNO) == IDYES) continue; //
            erC = 1; goto xit;                                                  //
           }                                                                    //
        if (erC < 0) goto xit;                                                  //
       }                                                                        //
xit:FreeLibrary(dllH);                                                          //
    return erC;
   } //main...

//end of file

