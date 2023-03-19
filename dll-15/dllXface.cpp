//Program smaDll.dll: dll interface to emulator class
#include "dllXface.h"
#include <samEmulator.h>

cEmulator *g_emulatorP;

#define XPORT(type) __declspec(dllexport) type __stdcall
extern "C"
{
//Initial call to setup cEmulator class; samFilePrefixP = program prefix only.
//The full source file name is getenv("samSourceDir") + samFilePrefixP + ".sam"
//Compiler output files reside in getenv('VerilogSimulationDir') - this to 
//conform to the directory structure used by Xilinx.
//OpenEmulator:
// - Compiles the specified code, 
// - Creates the cEmulator class, and 
// - Returns the value of $environment from the sam program.
//Remainder of program XPORTs the entry points for use by GUI/cmd-line program.
XPORT(int) OpenEmulator(const char *samFilePrefixP)
    {char *bufP, *sourceDirP=getenv("samSourceDir"),                            //
                 *includeDirP=getenv("samIncludeDir");                          //
     int   erC, ii=_MAX_PATH+50, keySize=8;                                     //keySize is arbitrary
     g_emulatorP = NULL;                                                        //
     bufP        = (char*)calloc(ii, 1);                                        //
     snprintf(bufP, ii-1, "cmd.exe /c samCompile %s\\%s.sam /include \"%s\"\n", //
                        sourceDirP, samFilePrefixP, includeDirP);               //
     erC = system(bufP);                                                        //Compile target program
     if (erC == 255) {free(bufP); return -1;}                                   //255 = compiler error, otherwise
                                                                                // == environment (ENV_SW, etc)
     snprintf(bufP, ii-1,"%s\\%s.sam", sourceDirP, samFilePrefixP);             //
     g_emulatorP = new cEmulator(keySize, samFilePrefixP, bufP, 0);             //create emulator
     if ((erC=g_emulatorP->m_errorCode) < 0)                                    //or perhaps not
        {delete g_emulatorP; g_emulatorP = NULL;}                               //
     free(bufP);                                                                //
     return erC;                                                                //return environment setting/error
    } //OpenEmulator...

//other redirected calls to cEmulator class
XPORT(void)     CloseEmulator(void)                 {delete g_emulatorP; g_emulatorP = NULL; }
XPORT(int)      ExecutePgm   (int mode)             {return g_emulatorP->ExecutePgm(mode);   }
XPORT(uint64_t) GetSamReg    (int ii)               {return g_emulatorP->GetSamReg(ii);      }
XPORT(int)      GetSp        (void)                 {return g_emulatorP->GetSp();            }
XPORT(int)      GetPc        (void)                 {return g_emulatorP->GetPc();            }
XPORT(uint64_t) GetSamStatus (int ii)               {return g_emulatorP->GetSamStatus();     }
XPORT(void)     PutSamReg    (int ii, uint64_t data){       g_emulatorP->PutSamReg(ii, data);}
XPORT(void)     PutStack     (int ii, uint64_t data){       g_emulatorP->PutStack(ii, data); }
XPORT(void)     PutSamStatus (uint8_t u8)           {return g_emulatorP->PutSamStatus(u8);   }
XPORT(int)      SetBugLevel  (int lvl)              {return g_emulatorP->SetBugLevel(lvl);   }
XPORT(int)      CheckE       (void)                 {return g_emulatorP->CheckE();           }
} //extern "C"...

//end of file...
