/* Program compile-<version>.cpp : Version 13, Jan 2023.
   See Help() for operational information.
*/
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#ifdef _WIN32
    #include <direct.h>
    #define getcwd _getcwd
#elif
    #include <unistd.h>
#endif
#include "compile.h"
#include "..\include-15\c3_errors.h"

using namespace std;                                                            //
FILE            *g_printFileP=NULL;                                             //required for Printf

#include <samVersion.h>
static int Help(const char *a, const char *b)
   {Printf("SamCompiler rev %d" __DATE__ "\n", SAM_VERSION);
    Printf("Usage is:\n");
    Printf("samCompiler samProgramName [options]\n");
    Printf("Options:\n");
    Printf("/include \"includeDirectory\"\n");
    Printf("/macro    expand macros only\n");
    Printf("/capture  <fileName> specify output file for macro\n");
    Printf("/patchDrv patch size of onject file into samDefines.sv\n");
    return 0;
   } //Help...

int main(int argc, char **argv)
   {int         ii, erC=0, keySize=8;                                           //
    bool        expandOnlyB=false, patchDrvB=false;                             //
    const char *captureFileP=NULL, *includeDirP=NULL, *srcFileNameP=NULL;       //
    cCompile   *compilerP;
    #define IF_ARG(what) if(argv[ii][0] == '/' && stricmp(argv[ii]+1,what) == 0)//
                                                                                //
    if (false)                                                                  //
        patchDrvB   = true;                                                     //patch SimulationDriver.sv
    if (false)                                                                  //
        expandOnlyB = true;                                                     //
    for (ii=1; ii < argc; ii++)                                                 //
      {IF_ARG("capture")  captureFileP = argv[++ii];                       else //specify capture file
       IF_ARG("include")  includeDirP  = argv[++ii];                       else //specify include directory
       IF_ARG("macro")    expandOnlyB  = true;                             else //
       IF_ARG("patchDrv") patchDrvB    = true;                             else //patch 'parameter MICROCODE_SIZE' in SimulationDriver.sv
       if(strncmp(argv[ii], "/h", 2) == 0)return Help(argv[ii],argv[ii+1]);else //
       if (srcFileNameP == NULL) srcFileNameP = argv[ii];                  else //
          {cCompile::Error(ERR_2005, NULL, argv[ii]); return 1;}                //2005 = Missing or unknown command line argument'%s'
      }                                                                         //
                                                                                //
    if (!srcFileNameP) {cCompile::Error(ERR_2741, "", ""); return 1;}           //2741 missing source file name
                                                                                //
    compilerP = new cCompile(srcFileNameP, includeDirP);                        //input files/dir
    compilerP->m_patchDrvB   = patchDrvB;                                       //patch SimulationDriver.sv
    compilerP->m_expandOnlyB = expandOnlyB;                                     //
    //Compile microcode to objFile                                              //
    if (expandOnlyB) erC = compilerP->PrintProgram();                           //
    else             erC = compilerP->CompileProgram();                         //
    Printf("%s Compiled, error=%d\n", srcFileNameP, erC);                       //
    delete compilerP;                                                           //
    return erC < 0 ? 1 : 0;                                                     //
    #undef ARG_IS                                                               //
   } //main...

//end of file
