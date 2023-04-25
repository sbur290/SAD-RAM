//Program smaDll.dll: dll interface to emulator class
#include "dllXface.h"
#include <samEmulator.h>
#include <direct.h>

cEmulator *g_emulatorP;

#define XPORT(type) __declspec(dllexport) type __stdcall
extern "C"
{
XPORT(int)  DllVersion(void) {return (SAM_VERSION << 24) + 0x0204;}             //return version: maj.min.rev.tweak

//Initial call to setup cEmulator class; samFilePrefixP = program prefix only.
//The full source file name is getenv("samSourceDir") + filePrefixP + ".sam"
//Compiler output files reside in getenv('VerilogSimulationDir') - this to 
//conform to the directory structure used by Xilinx simulator.
//OpenEmulator:
// - Compiles the specified code (if needed), 
// - Creates the cEmulator class, and 
// - Returns the value of $environment from the sam program.
//Remainder of program XPORTs the entry points for use by GUI/cmd-line program.
XPORT(int) OpenEmulator(const char *filePrefixP, bool guiB, bool mustCompileB)
    {char         *bufP, *sourceDirP =getenv("samSourceDir"),                   //
                         *includeDirP=getenv("samIncludeDir"), *nameOnlyP;      //
     const char  *msgP="";                                                      //
     int          erC, bufSize=2*_MAX_PATH+50, keySize=8;                       //keySize is arbitrary
     HANDLE       hh;                                                           //
     WIN32_FIND_DATAA ffData;                                                   //
     FILETIME     srcTime, objTime;                                             //
                                                                                //
     g_emulatorP = NULL;                                                        //
     if (filePrefixP == NULL || *filePrefixP == 0) return 0;                    //
     nameOnlyP   = strdup(cEmulator::FileNameOnly(filePrefixP));                //
     if (strchr(nameOnlyP, '.')) *strchr(nameOnlyP, '.') = 0;                   //
     bufP        = (char*)calloc(bufSize, 1);                                   //
                                                                                //
     snprintf(bufP, bufSize-1, "%s\\%s.bin",                                    //
                            getenv("verilogSimulationDir"), nameOnlyP);         //
     if ((hh=FindFirstFileA(bufP, &ffData)) == INVALID_HANDLE_VALUE)            //
          mustCompileB = true;                                                  //
     else objTime   = ffData.ftLastWriteTime;                                   //
     FindClose(hh);                                                             //
                                                                                //
     snprintf(bufP, bufSize-1, "%s\\%s.sam", sourceDirP, nameOnlyP);            //
     if ((hh=FindFirstFileA(bufP, &ffData)) == INVALID_HANDLE_VALUE) return -1; //
     srcTime   = ffData.ftLastWriteTime;                                        //
     FindClose(hh);                                                             //
                                                                                //
     if (mustCompileB || *(uint64_t*)&srcTime > *(uint64_t*)&objTime)           //source is more recent that microcode
        {if (!guiB) Printf("Compiling %s.sam\n", nameOnlyP);                    //
         snprintf(bufP, bufSize-1,                                              //
                          "cmd.exe /c samCompile %s\\%s.sam /include \"%s\"\n", //
                           sourceDirP, nameOnlyP, includeDirP);                 //
         erC = system(bufP);                                                    //Compile target program
         if (!guiB) Printf("Compile complete, return code=0x%X\n", erC); else   //
             switch(erC)                                                        //
                {case ENV_SW:      msgP = "software emulation"; break;          //0  emulated in software only
                 case ENV_XSIM:    msgP = "Xilinx simulation";  break;          //1  calls xsim.exe; sim.exe monitors results
                 case ENV_FPGA:    msgP = "FPGA";               break;          //2  load FPGA and monitor execution
                 case ENV_HWLOAD:  msgP = "hardware";           break;          //3  load hardware only
                 case ENV_COMPILE: msgP = "compile only";       break;          //4  emulated in software only
                 default:          msgP = "";                   break;          //5  expand macros only
                }                                                               //
         if (erC < 0 || erC > 10) {free(bufP); return -1;}                      //compiler error, otherwise
        }                                                                       //  erC == environment (ENV_SW, etc)
     snprintf(bufP, bufSize-1,"%s\\%s.sam", sourceDirP, nameOnlyP);             //
     g_emulatorP = new cEmulator(keySize, nameOnlyP, bufP, 0, guiB);            //create emulator
     if ((erC=g_emulatorP->m_errorCode) < 0)                                    //or perhaps not
        {delete g_emulatorP; g_emulatorP = NULL; free(bufP); return erC;}       //
     free(bufP);                                                                //
     g_emulatorP->m_samFilePrefixP = nameOnlyP;                                 //
     g_emulatorP->m_compilerMsgP   = strdup(msgP);                              //
     return erC;                                                                //return environment setting or error code
    } //OpenEmulator...

//other redirected calls to cEmulator class	
//Most of these calls return a formatted buffer to c#. 
//This because c# formatting is collosally overdesigned
#define CALL_EMU if (g_emulatorP == NULL) return -ERR_0001; else return g_emulatorP->
XPORT(void)     CloseEmulator (void)                  {delete g_emulatorP; g_emulatorP = NULL; }
XPORT(int)      Bugger        (const char *fmtP, int p1, int p2, int p3) {CALL_EMU Bugger(fmtP, p1, p2, p3);} 
XPORT(int)      SamCommandLine(const char **params, int countParams, char *fNameOut)
                      {return cEmulator::SamCommandLine(params, countParams, fNameOut);}
XPORT(int)      ExecutePgm    (int mode,uint64_t p1)  {CALL_EMU ExecutePgm((EX_MODE)mode, p1); }
XPORT(int)      DebugGui      (int loc, int ln, const char *s){CALL_EMU DebugGui(loc, ln, s);  }
XPORT(int)      GetClkCount   (char *bufP)            {CALL_EMU GetClkCount(bufP);             }
XPORT(int)      GetPc         (char *bufP)            {CALL_EMU GetPc(bufP);                   }
XPORT(int)      GetAssembler  (char *bufP,int h,int w){CALL_EMU GetAssembler(bufP, h, w);      }
XPORT(int)      GetCompilerMsg(char *bufP)            {CALL_EMU GetCompilerMsg(bufP);          }
XPORT(int)      GetCurLine    (char *bufP)            {CALL_EMU XlatPC2Line(-1);               }
XPORT(int)      GetCurOvly    (char *bufP)            {CALL_EMU GetCurOvly(bufP);              }
XPORT(int)      GetCurRow     (char *bufP)            {CALL_EMU GetCurRow(bufP);               }
XPORT(int)      GetExpectedVersusActual(char *bufP)   {CALL_EMU GetExpectedVersusActual(bufP); }
XPORT(int)      GetFileName   (char *bufP)            {CALL_EMU GetFileName(bufP);             }
XPORT(int)      GetInsPoint   (char *bufP)            {CALL_EMU GetInsPoint(bufP);             }
XPORT(int)      GetFormattedRow(char *bufP, bool koB) {CALL_EMU GetFormattedRow(bufP, koB);    }
XPORT(int)      GetOpName     (char *b1P, char *b2P)  {CALL_EMU GetOpName(b1P,b2P);            }
XPORT(int)      GetMessages   (char *bufP)            {CALL_EMU GetMessages(bufP);             }
XPORT(int)      GetSamReg     (int ii, char *bufP)    {CALL_EMU GetSamReg(ii, bufP);           }
XPORT(int)      GetStack      (int ii, char *bufP)    {CALL_EMU GetStack(ii, bufP);            }
XPORT(int)      GetSp         (char *bufP)            {CALL_EMU GetSp(bufP);                   }
XPORT(int)      GetSourceAll  (char *bufP,int l,int s,bool b){CALL_EMU GetSourceAll(bufP,l,s, b);}
XPORT(int)      GetSourceAtPc (char *bufP, int *l)    {CALL_EMU GetSourceAtPc(bufP, l);        }
XPORT(int)      GetLineNumAtPc()                      {CALL_EMU XlatPC2Line(GetPc(NULL));      }
XPORT(int)      GetSamStatus  (char *b1P, char *b2P)  {CALL_EMU GetSamStatus(b1P, b2P);        }
XPORT(int)      PutSamReg     (int ii, uint64_t val)  {CALL_EMU PutSamReg(ii, val);            }
XPORT(int)      PutStack      (int ii, uint64_t val)  {CALL_EMU PutStack(ii, val);             }
XPORT(int)      PutSamStatus  (uint8_t u8)            {CALL_EMU PutSamStatus(u8);              }
XPORT(int)      SetBreakPoint (const char*pgmP,int ln){CALL_EMU SetBreakPoint(pgmP, ln);       }
XPORT(int)      SetBugLevel   (int lvl)               {CALL_EMU SetBugLevel(lvl);              }
XPORT(int)      SetCurRow     (int row)               {CALL_EMU SetCurRow(row);                }
XPORT(int)      SetPc         (int pc)                {CALL_EMU SetPc(pc);                     }
XPORT(int)      SetRowFormat  (int fmt)               {CALL_EMU SetRowFormat(fmt);             }
XPORT(int)      SetSamReg     (int reg, uint64_t val) {CALL_EMU SetSamReg(reg, val);           }
XPORT(int)      StartLogging  (bool bb, char *fnP)    {CALL_EMU StartLogging(bb, fnP);         }
XPORT(int)      SaveSettings  (int h,  int w, int t, uint32_t o, int n, uint64_t v) 
                                               {return cEmulator::SaveSettings   (h,w,t,o,n,v);}
XPORT(int)      RestoreSettings(int*h, int*w, int*t, uint32_t*o, int*n, uint64_t*v) 
                                               {return cEmulator::RestoreSettings(h,w,t,o,n,v);}

XPORT(int)      ResetProgram (void)                  
   {bool guiB     =        g_emulatorP->GetGuiB();                              //get the parameters
    char *prefixP = strdup(g_emulatorP->m_samFilePrefixP);                      //   was originally created
    CloseEmulator();                                                            //
    int erC=OpenEmulator(prefixP, guiB, false);                                 //looks the same eh ?
    free(prefixP);                                                              //
    return erC;                                                                 //
   } //ResetProgram...                                                          //
} //extern "C"...

//end of file...
