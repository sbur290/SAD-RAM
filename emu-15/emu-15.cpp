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

int Error(int erC, CC meaningP, CC paramP) 
   {printf("Error %d, %s: %s\n", erC, meaningP, paramP); return -erC;}

void Help() 
   {printf("\nValid commands:");                                                //
    printf("\nh:     This help message.");                                      //help
    printf("\nq:     Quit simulation");                                         //quit
    printf("\ng:     Run at normal speed");                                     //go (run at full speed)
    printf("\nspace or \n");                                                    //single step
    printf("\ns      Single Step on opcode at a time");                         //single step
    printf("\nb:     Run to Breakpoint ($bug opcode)");                         //run to break point ($bug)
    printf("\nt:     Run to Ret opcode");                                       //run to return
    printf("\nl:     Step line-by-line");                                       //run to next line
    printf("\np:     Put Sam Register "  );                                     //
    printf("\nu:     Put value into stack");                                    //
    printf("\n>:     Force Sam Status to value");                               //
    printf("\ndigit: Set debug level to specified digit");                      //
    printf("\n");                                                               //
   } //Help...

    int (*OpenEmulatorP)   (const char *microcodeNameP);                        //Entry points to DLL
    int (*CloseEmulatorP)  (void);                                              //      "
    int (*ExecutePgmP)     (int mode);                                          //      "
    int (*CheckEP)         (void);                                              //      "
    int (*GetPcP)          (void);                                              //      "
    int (*GetSpP)          (void);                                              //      "
    int (*GetSamRegP)      (int reg);                                           //      "
    int (*PutSamRegP)      (int reg, uint64_t data);                            //      "
    int (*PutStackP)       (int posn, uint64_t data);                           //      "
    int (*GetSamStatusP)   (void);                                              //      "
    int (*PutSamStatusP)   (uint8_t stat);                                      //      "
    int (*SetBugLevelP)    (int lvl);                                           //      "

uint64_t GetValue(const char *msgP)
   {bool hexB=false;                                                            //
    char buf[32], ch; uint64_t val; int ii, radix=10;                           //
    printf("%s", msgP);                                                         //
    while (true) {if (_kbhit() > 0) break; Sleep(10);}                          //wait for keystroke
    for (ii=(int)strlen(msgP); --ii >= 0;) printf("\b  \b\b");                  //remove prompt
    for (val=0, ii=0; ii < sizeof(buf)-1; ii++)                                 //accumulate value
       {printf("%c", ch=_getch());                                              //
        if (ch >= '0' && ch <= '9') {val = val * radix + (ch & 7); continue;}   //decimal/hex digit
        if (ch == 'x' && ii == 1)   {hexB = true; radix = 16;      continue;}   // '0x' switch to hex
        if ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))               //hex value
                  {ch |= 0x20; val = val * radix + (ch -'a' + 10); continue;}   //
        if (ch == '=' || ch == '\n' || ch == '\r') break;                       //end of data
        printf("\b \b");                                                        //wtf
       }                                                                        //
    return val;                                                                 //
   } //GetValue...

//p command: Store value into samReg[reg]; expect reg=data
int PutSamReg(void)
   {int      reg=(int)GetValue("<reg>");
    uint64_t data=    GetValue("<data>");
    PutSamRegP(reg, data);
    return 0;
   } //PutSamReg...

//u command: Store value into stack[n]: expect n=data
int PutStack(void)
   {int      reg=(int)GetValue("<stack offset>");
    uint64_t data=    GetValue("<data>");
    PutStackP(reg, data);
    return 0;
   } //PutStack...

//> command: Store system status: expect u8
int PutSamStatus(void)
   {int8_t u8=(int)GetValue ("<status>");
    PutSamStatusP(u8);
    return 0;
   } //PutSamStatus...

int main(int argc, char **argv)
   {int         erC=0, keySize=8, bugLevel=-1, errors, ii;                      //
    const char *samFileNameP=NULL, *pp;                                         //
    char        ch, dllName[_MAX_PATH];                                         //
    bool        promptB=true, stoppedB=false;                                   //
    uint64_t    reg=0, stak=0;                                                  //
    uint8_t     stat = 0;                                                       //
    HMODULE     dllH;                                                           //handle to DLL
#define Check(name) if (name == NULL) {erC = Error(ERR_7291, "", pp); goto xit;}//
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
    Import1(ExecutePgm,      int mode);                                         //
//status display routines                                                       //
    Import1(CheckE,          void);                                             //Display samRegs, stack, and status
    Import1(SetBugLevel,     int);                                              //
//service routines                                                              //
    Import1(GetSamReg,       int);                                              //
    Import2(PutSamReg,       int, uint64_t data)                                //
    Import1(GetSp,           void);                                             //
    Import1(GetPc,           void);                                             //       
    Import2(PutStack,        int, uint64_t data);                               //
    Import1(GetSamStatus,    void);                                             //
    Import1(PutSamStatus,    uint8_t);                                          //

    if ((erC=OpenEmulatorP(samFileNameP)) < 0)                      goto xit;   //compile error
    if (bugLevel < 0) {erC = ExecutePgmP(EX_RUN);                   goto xit;}  //plain emu samProgramName; otherwise
    SetBugLevelP(bugLevel);
    for (errors=0;;)                                                            //      emu samProgramName /bugLevel
       {if (promptB)                                                            //
           {CheckEP(); printf("\n%03d: sam >", GetPcP()); promptB = false;}     //
        if (_kbhit() > 0)                                                       //
           {printf("%c", ch=_getch()); promptB = true;                          //
            if (ch == 0x03) {printf("\nbreak Detected\n");          goto xit;}  //
            switch(ch|0x20)                                                     //lower case eh?
                {case 'h':       Help();                            continue;   //help
                 case 'q': erC = 0;                                 goto xit;   //quit
                 case '1': case '2': case '3':                                  //
                       SetBugLevelP(bugLevel=ch & 7);                           //
                       if (bugLevel >= 3) {erC = ExecutePgmP(EX_SS);break;}     //single step
                     //else fall thru                                           //
                 case 'g': erC = ExecutePgmP(EX_RUN);               break;      //go (run at full speed)
                 case 's':                                                      //
                 case ' ': erC = ExecutePgmP(EX_SS);                break;      //single step
                 case 'b': erC = ExecutePgmP(EX_2BREAK);            break;      //run to break point ($bug)
                 case 't': erC = ExecutePgmP(EX_2RET);              break;      //run to return
                 case 'l': erC = ExecutePgmP(EX_BY_LINE);           break;      //run to next line
                 case 'p': erC = PutSamReg       (); printf("\n");  break;      //
                 case 'u': erC = PutStack        (); printf("\n");  break;      //
                 case '>': erC = PutSamStatus    (); printf("\n");  break;      //
                 default:                                                       //
                    if (ch > ' ') printf("Unknown cmd %c(0x%02X)\n", ch, ch);   //
                    continue;                                                   //
                }                                                               //
            if (erC == (-ERR_2704) && bugLevel >= 0)                            //ie, not /run option
               {snprintf(dllName, sizeof(dllName), "Stop Encountered at pc=%d", GetPcP());                                                               //
                MessageBoxA(NULL, dllName, "OP_STOP", MB_OK);                   //
               }                                                                //
            if (erC < 0) goto xit;                                              //
            continue;                                                           //
           }                                                                    //
        Sleep(100);                                                             //
       }                                                                        //
xit:FreeLibrary(dllH);                                                          //
    return erC;
   } //main...

//end of file

