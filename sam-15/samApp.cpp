// file: samApp.cpp : July 2022
// Program parameters: See CommandLine()
// This program simulates sort as implemented in Self Addressing Memory (SAM).
// When records are written to memory an index is created of the key and their locations.
// This is used to locate the record for CheckLocByRcdNum.
// In a real implementation this index structure is maintained by the hardware.
// Because of this data structures are limitted to the length of a memory row
// (typically 8192 bits = 1024 bytes). This requires a second or third level of
// index pointers.
//
// Program parameters: See CommandLine /h
//Improvements:

#define _CRT_SECURE_NO_WARNINGS 1

#include "samApp.h"
#include "sam-15.h"
#include "samHelper.h"
#include "samBug.h"
#include "generator.h"
#include <C3_timeFncs.h>
#include <C3_codeTimer.h>
#include <C3_errors.h>
#include <stdarg.h>
#include <crtdbg.h>
#include <math.h>

#define uint64_t        unsigned long long
#define uint32_t        unsigned int
#define uint16_t        unsigned short
#define uint8_t         unsigned char

#ifdef USE_TIMERS                                                               //
 cCodeTimer *g_timerP;                                                          //
 int         g_timer_write, g_timer_read_by_loc, g_timer_read_by_key,           //
             g_timer_pack_index;                                                //
#endif //USE_TIMERS...                                                          //
                                                                                //
char        g_errMsg[80], *g_errMsgP=g_errMsg;                                  //
uint32_t    g_memRowSize   = 1024,                                              //size of row in memory (bytes)
            g_rcdSize=8 + sizeof(sRECORD)-1,                                    //depends upon actual keySize
            g_keySize=8,                                                        //
            g_stopOnRcdNum =-1;                                                 //debugging: display indexes after stop
cHelper    *g_helpP;                                                            //
cBugIndex  *g_bugP;                                                             //
sRECORD    *g_dataP;                                                            //underlying 'user data'
uint32_t    g_bugLevel=0;                                                       //1 = summary of m_bookP[]
                                                                                //2 = summary of cPAGE[]'s
char        g_exeFileDir[_MAX_PATH];                                            //location of print output file
FILE       *g_bugFileP;                                                         //print output file for debugging

FILE *g_printFileP=NULL;                                                        //capture of Printf when != NULL
void Printf          (char const *fmtP,...);                                    //
bool IsDigit         (char ch) {return ch >= '0' && ch <= '9';}                 //standard fnc does not work !!
void DisplayMath     (SAM_CONTROLS *ctlP);                                      //
void TestDujour      (SAM_CONTROLS *ctlP, int nu);                              //ad-hoc code, varies by the day :)
bool GenVerilogFiles (cSadram *samP, SAM_CONTROLS *ctlP);                       //

cGenerator *g_generateP;
//                             0          1          2            3
const char *Generators[] = {"<file>", "random", "sequential", "reverse"};

char *BaseDirectory(const char *exeNameP)
   {static char base[MAX_PATH];
    char       *pp;
    strcpy(base, exeNameP);
    for (pp=&base[strlen(base)]; *(--pp) != '\\' && *pp != '/' && pp > base; ) {*pp = 0;}
    return base;
   } //BaseDirectory...

void Help(const char *exeNameP, const char *rmdrP)
   {bool detailB=(*rmdrP != 0);                                                                                 //
    int  jj;                                                                                                    //
    const char *cc;                                                                                             //
    switch (*rmdrP | 0x20)                                                                                      //
      {case ' ':                                                                                                //default: show command summary
          printf("\tSelf Addressing Memory (SAM) simulation in C++, revisions #%d (%s @ %s).\n",                //
                                                                      SAM_VERSION, __DATE__, __TIME__);         //
          printf("\t-------------------------------------------------------------------------------------\n");  //
          printf("sam.exe generates and sorts records in the style proposed in SAM.\n");                        //
          printf("Usage is:\n"                                                                                  //
                 "   sam recordCount <options>\n"                                                               //
                 "       recordCount is the number of records to be sorted;\n"                                  //
                 "       <options> comprises one or more commands. Each command comprises:\n"                   //
                 "       /<letter> or -<letter> followed by any supporting parameter.\n"                        //
                 "   sam generates the requested number of records, sorts them, reads them back in sorted\n"    //
                 "   order, and finally reads them back by key. Every step is verified.\n"                      //
                 "\nCommand summary:\n"                                                                         //
                 "    /b value\t"   "\tCheck the indexbase during write. Start debugging at record number == value.\n"   //
                 "    /bugLevel <lvl>""\tSet debugging level to <lvl>, lvl=0,1,2, or 3.\n"                      //
                 "    /d\t\t"       "\tDisplay sorted records on completion of sort.\n"                         //
                 "    /e value\t"   "\tSet early readback depth for sequential readback.\n"                     //
                 "    /gather\t"    "\tGather statistics on this run of sam.exe.\n"                             //
                 "    /genVerilog\t""\tGenerate verilog source files.\n"                                        //
                 "    /h\t\t"       "\tDisplay this help information. Use /h<letter> for detail information.\n" //
                 "    /i value\t"   "\tDebug indexBase on completion of sort.\n"                                //
                 "    /k");                                                                                     //
                 for (jj=1, cc="["; jj < HOWMANY(Generators); jj++, cc=" | ")                                   //skip Generators[<file>]
                                 printf("%s%s", cc, Generators[jj]);                                            //
          printf(" | <fileName>]\n"                                                                             //
                 "    \t\t\tSpecifies the key order of the source records.\n"                                   //
                 "    /mc /md /me\t""\tDisplay storage and overhead for specified recordCount.\n"               //
                 "    /pack\t"      "\tPack indexbase after sort.\n" // to specified %%age.\n"                  //
                 "    /pafter\t"    "\tDisplay parameters calculated by cSadram after sort.\n"                  //
                 "    /pbefore\t"   "\tDisplay parameters calculated by cSadram before sort.\n"                 //
                 "    /progress\t"  "\tDisplay progress percentage during write phase.\n"                       //
                 "    /q\t\t"       "\tSet quiet mode: limits output to essential messages.\n"                  //
                 "    /rcdSize\t"   "\tRecord size in bytes.\n"                                                 //
                 "    /strip\t"     "\tStrip key fields from indexbase after sort.\n"                           //
                 "    /rowSize\t"   "\tSet row size of physical memory (bytes).\n"                              //
                 "    /t\t\t"       "\tDisplay time taken to perform this sort.\n"                              //
                 "    /variable\t"  "\tCreate variable length records.\n");                                     //
          printf("\nExamples:\n"                                                                                //
                 "    sam 100000 -d          sort 100,000 records of randomly generated data, rowsize == 1024 (default).\n");
          printf("    sam 100K   /d /r128    sort 100,000 records of randomly generated data, rowsize == 128.\n\n");
          break;                                                                                                //
       case 'i':                                                                                                //
          printf("sam /i <buglevel>  Debug indexbase at bug level = <level> on completion of sort.\n");         //
          goto bug;                                                                                             //
       case 'b':                                                                                                //
          if (_strnicmp(rmdrP, "bugLevel", 8) == 0)                                                             //
             {printf("sam /bugLevel <bugLevel>  Set debugging level:\n");                                       //
          bug:printf("\t bugLevel == 0: Displays a single line containing #books, #pages, and #indexes.\n"      //
                     "\t bugLevel == 1: Displays the cBOOK table only.\n"                                       //
                     "\t bugLevel == 2: Displays the cBOOK table and cPAGEs.\n"                                 //
                     "\t bugLevel == 3: Displays the cBOOK, cPAGE, cINDX tables, and underlying record.\n"      //
                     "\t"             " The information is displayed before and after each record is"           //
                                      " written to DRAM.\n");                                                   //
              break;                                                                                            //
             }                                                                                                  //
          if (_strnicmp(rmdrP, "bugMem", 6) == 0)                                                               //short for bugMemory
             {printf("sam /bugMemory  Set memory debugging flag.\n"                                             //
                     "\t Allocation and de-allocation of memory will be auditted.\n");                          //
              break;                                                                                            //
             }                                                                                                  //
          printf("sam /b value\n"                                                                               //
             "\t    Initiate verification of indexBase starting with the record number == value.\n"             //
             "\t    This option will execute a number of internal checks as each record is written.\n"          //
             "\t    If an error is found in internal structures, bugLevel will be set appropriately.\n"         //
             "\t    and the index structures will be dumped to a file called sam.samlog.\n");                   //
          break;                                                                                                //
       case 'd':                                                                                                //
          printf("sam /d   Display sorted records on completion of sort.\n"                                     //
                 "\t"    " The records will be in the sorted order specified by the key.\n\n");                 //
          printf("sam /dump <FileName> Capture dump of indexbase into specified file on completion of sort.\n");//
          break;                                                                                                //
       case 'e':                                                                                                //
          printf("sam /early <value>\tSet early readback depth for sequential readback.\n");                    //
          printf("\t\t\tWhen data is read back using LocByRcdNum, readback depth will cause\n"                  //
                 "\t\t\tSAM to prepare this number of records in advance of the actual read.\n");               //
          break;                                                                                                //
       case 'g':                                                                                                //
          if (_strnicmp(rmdrP, "gather", 6) == 0 || *(rmdrP+1) == 0)                                            //
             {printf("sam /gather\n"
                     "\tGather statistics on this run and write into %ssam.csv.\n", BaseDirectory(exeNameP));
              printf("\t"      "File 'sam.csv' resides in the same directory as sam.exe.\n"                     //
                     "\t"      "Each run of 'sam ... /gather' will add to this file.\n"                         //
                     "\t"      "Invoke 'excel.exe sam.csv' to view these statistics.\n");                       //
              printf("Note:\t" "Before launching a group of 'sam /gather' commands the file sam.csv\n"          //
                     "\t"      "should be deleted to purge old information.\n\n");                              //
             }                                                                                                  //
          if (_strnicmp(rmdrP, "genVerilog", 10) == 0 || rmdrP[1] == 0)                                         //
              printf("sam <rcdCount> /genVerilog\n"                                                             //
                     "\tGenerate supporting files for FPGA simulation or hardware implementation.\n"            //
                     "\tsam.exe is run normally to create and sort the specified number of records.\n"          //
                     "\tData files are then generated from the indexbase and written to file for use by the\n"  //
                     "\tFPGA simulation/hardware. Addresses within the indexbase are translated to the\n"       //
                     "\tproper addresses in block RAM and structures are reformatted from their software\n"     //
                     "\trepresentation to their equivalent hardware format.\n"                                  //
                     "\tFPGA simulation loads these files into the FPGA using the $readmemh() function.\n"      //
                     "\tThe hardware monitor loads these files using port I/O.\n"                               //
                     "\tThis capability enables the FPGA/hardware to be tested with realistic data.\n\n"        //
                     "\tThe files so generated are:\n"                                                          //
                     "\t    blockRam.data containing a full copy of the indexbase.\n"                           //
                     "\t    firstRow.data containing just the first row of the indexbase.\n"                    //
//                   "\t    params.data   a text file containing operational parameters for this simulation.\n" //
//                   "\t    params.txt    a text file describing params.data. This file is read by sim.exe or\n"//
//                   "\t                  by emu.exe and used to REGENERATE params.data. It can, therefore,\n"  //
//                   "\t                  be editted for various tests scenarios.\n"                            //
//                   "\t       >>> Refer to sam /params.data for more information on these files. <<<\n"        //
                     "\t    user.data     a binary file containing all data records generated by sam.exe.\n"    //
                     "\t    userCmds.data a text file containing commands executed by sim.exe or emu.exe.\n"    //
                     "\t                  Each line of userCmds.data comprises a fixed length hex string.\n"    //
                     "\t       >>> Refer to sam /userCmds.data for more information on this file. <<<\n"        //
                     "\t                  The first keySize bytes are a key in LSB order.\n"                    //
                     "\t                  The key is followed by two bytes representing the opcode to run.\n"   //
                     "\tThe directory into which these files will be written is specified in the environment\n" //
                     "\tvariable 'verilogSimulationDir'. These files are not generated if rcdCount == 0.\n\n"   //
                                                                                                                //
                     "\tIn addition to these data files the following Verilog source files are created:\n"      //
                     "\t\t\t"          " - readWord.sv.  Read one word from specified DRAM/blockRAM row.\n"     //
                     "\t\t\t"          " - writeWord.sv. Write one word to specified DRAM/blockRAM row.\n"      //
                     "\t\t"       "and\t - groupSmear.sv. Promulgate compare results to supporting groups.\n"   //
                     "\tThe directory into which these files will be written is specified in the environment\n" //
                     "\tvariable 'verilogSourceDir'. These files will be generated regardless of rcdCount.\n\n" //
                     "\tThese files must be generated because the Verilog code should be sensitive to the\n"    //
                     "\tsize of the targetBus. This sensitivity cannot be expressed in System Verilog.\n"       //
                     "\ttargetBusize is locked to the FPGA/hardware architecture and does not change with\n"    //
                     "\tdifferent user key sizes; generating these files is only a design time task.\n\n"       //
                     "See also: sam /htests, sam /hkeySize, sam /htargetBusSize, and sam /hparams.data.\n\n"    //
                     "Example:\n"                                                                               //
                     "\tsam 1000 /test 123 /rowSize=256, /targetBusSize=8 /genVerilog\n");                      //
          break;                                                                                                //
       case 'h': case '?':                                                                                      //
          printf("sam /h\t\tDisplay this help information.\n"                                                   //
                 "\t\t"      "For more information on any command enter sam /h<command letter>.\n"              //
                 "\t\t"      "eg. 'sam /hk' will display more information on the /k command.\n");               //
          break;                                                                                                //
       case 'k':                                                                                                //
          printf("sam /keysize <value>\t Specify the size of the key in bytes.\n"
                 "\t"                "\t <value> must be >= 2 and < 64, ie., the key must be \n"
                 "\t"                "\t at least 2 bytes in length and not more than 63 bybtes in length.\n\n");
          printf("sam /k\t");                                                                                   //
          for (jj=1, cc="["; jj < HOWMANY(Generators); jj++, cc=" | ")                                          //
                              printf("%s%s", cc, Generators[jj]);                                               //
          printf(" | <fileName>]\n");                                                                           //
          printf("  This command specifies how the key in the source records is generated:\n"                   //
                 "    random     - generates a unique random number for the key.\n"                             //
                 "    sequential - generates sequential values: 1,2,3,4,... ie. already sorted.\n"              //
                 "    reverse    - generates keys in reverse sequence: 99, 98, 97,...\n");                      //
          printf("    <fileName> - specifies an input text file containing keys. The keys should be numbers\n"  //
                 "\t\t or \"strings\" values separated by a comma (,).\n"                                       //
                 "\t\t Comment fields: // to end of line, and /*....*/ can be interjected at will.\n"           //
                 "\t\t The following special commands are also accepted:\n"                                     //
                 "\t\t    $rowSize = specify rowSize of hardware (alternative to /rowsize <value>).\n"          //
                 "\t\t    $headers = Dump cBOOK, and cPAGE structures from indexBase.\n"                        //
                 "\t\t    $dump    = Dump cBOOK, cPAGE, and cINDX structures from indexbase.\n"                 //
                 "\t\t    $generate(start, end, step)\n"                                                        //
                 "\t\t        Generate numeric values start, start+step,..., end\n"                             //
                 "\t\t    $duplicate(reps, value)\n"                                                            //
                 "\t\t        Generate the same key repeatedly. value may be a number or a \"string\".\n"       //
                 "\t\t Example of input file:\n"                                                                //
                 "\t\t    type keys.txt\n"                                                                      //
                 "\t\t\t1,2, //buckle my shoe\n"                                                                //
                 "\t\t\t0x9753102, 0x12345678; /*close the gate*/\n"                                            //
                 "\t\t\t\"Jones\"\n"                                                                            //
                 "\t\t\t$generate(1,100,2)     //generates 1,3,5,7,... 99\n"                                    //
                 "\t\t\t$duplicate(20, \"Smith\")//generates \"Smith\", \"Smith\", .... 20 times\n");           //
          break;                                                                                                //
       case 'm':                                                                                                //
          Printf("sam /mc\t\tDisplay summary of storage requirement in compact CSV format.\n"                   //
                 "sam /md\t\tDisplay detail storage requirements for specified recordCount.\n"                  //
                 "sam /me\t\tDisplay summary of storage requirement in a CSV format.\n"                         //
                 "\t\tBoth /mc and /me create data in a format suitable for import to Excel.\n"                 //
                 "For example:\n"                                                                               //
                 "\t\t'sam 30M /md' will display the resources needed for a file of 30 megabytes.\n");          //
          break;                                                                                                //
       case 's':                                                                                                //
          printf("sam /strip\t\tPack indexbase and strip key information.\n"                                    //strip
                 "\t\t\tThis options strips out all key information and compresses the index structure\n"       //
                 "\t\t\tdown to a simple pointer. This is adequate for sorted access using ReadRecord().\n");   //
          break;                                                                                                //
       case 'p':                                                                                                //
          if (_strnicmp(rmdrP, "params.data", 11) == 0 || _strnicmp(rmdrP, "params.txt", 10) == 0)              //
             {printf("params.data and params.txt are created by the /genVerilog command.\n"                     //
                     "\tparams.data contains operational parameters for the FPGA simulation or the hardware execution.\n"
                     "\tparams.txt is a description of the data in params.data. params.data is regenerated by \n"
                     "\tsim.exe from params.txt. params.txt can, therefore, be editted for various test scenarios.\n"
                     "\tparams.data is a hex dump of the structure sPARAMS (see c++ source).\n"
                     "\tEach line represents a 16-bit values as follows:\n"
                     "\t\t 1: test/bug     = 0x0040\tbits [1:0] = bugLevel, bits[13:4] = bit mask of tests\n"
                     "\t\t 2: configAdr    = 0x0000\n"
                     "\t\t 3: indxSize     = 0x0010\n"
                     "\t\t 4:  indxesPerRow= 0x0010\n"
                     "\t\t 5:  indxAlign   = 0x0002\n"
                     "\t\t 6:  indxesUsed  = 0x0015\n"
                     "\t\t 7:  firstIndx   = 0x0004\n"
                     "\t\t 8: pageSize     = 0x0018\n"
                     "\t\t 9:  pagesPerRow = 0x000A\n"
                     "\t\t10:  pageAlign   = 0x0002\n"
                     "\t\t11:  pagesUsed   = 0x0002\n"
                     "\t\t12:  firstPage   = 0x0002\n"
                     "\t\t13: bookSize     = 0x0018\n"
                     "\t\t14:  booksPerRow = 0x000A\n"
                     "\t\t15:  bookAlign   = 0x0002\n"
                     "\t\t16:  bookUsed    = 0x0001\n"
                     "\t\t17:  firstBook   = 0x0001\n"
                     "\t\t18: firstFreeAdr = 0x0019\n"
                     "\t\t19: testAdr      = 0x0018\n"
                     "\t\t20: rcdStop      = 0x014C (=rcdCount-1)\n"
                     "\t\tline 19 = the row of blockRAM on which the test should be performed.\n"
                     "\t\tline 20 = howmany opcodes to execute from userCmds.data before $stopping.\n"
                    );
              break;
             }
          printf("sam \t/pack \t\t""Pack indexbase after sort.\n\n"                                             //pack
                 "sam \t/pbefore\t"   "Display internal parameters for sort but do not perform the sort.\n"     //shoPropsB4B
                 "sam \t/pafter\t\t"  "Display internal parameters after sort has been performed.\n\n"          //shoPropsAftB
                 "sam \t/progress\t"  "Show write progress during sort write phase.\n");                        //
          break;                                                                                                //
       case 'q':                                                                                                //
          printf("sam /q\t\t"  "Set quiet mode. In quiet mode messages are minimized.\n"                        //
                 "sam /~q\t\t" "Set verbose mode. Output progress messages during execution.\n");               //
          break;                                                                                                //
       case 'r':                                                                                                //
          printf("sam /rcdSize\t" "Record size of each record (bytes).\n\n"                                     //
                 "sam /rrowSize\t""Row size of physical memory (bytes).\n"                                      //
                 "\t\tThe memory rowsize specifies the width of each tile.\n"                                   //
                 "\t\tInternal structures are confined to fit in a row. \n"                                     //
                 "\t\tValid sizes are: 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, or 65536.\n");      //
          break;                                                                                                //
       case 't':                                                                                                //
          printf("sam /target <value>\n"                                                                        //
                 "\tSpecifies the size of the target bus in the FPGA/hardware implementation.\n\n");            //
          printf("sam /time\n"                                                                                  //
                 "\tDisplay time taken to perform the sort.\n"                                                 //
                 "\tThis command estimates the number of clock cycles required to perform various operations.\n"//
                 "\tThis cannot be predicted from sam.exe run time since the anticipated hardware\n"            //
                 "\tcan perform certain functions in parallel. For example\n"                                   //
                 "\tthe function hSequencer() presumes to operate in a single clock cycle,\n"                   //
                 "\twhereas simple C++ operations like inx++ might take longer depending upon the\n"            //
                 "\tnumber of bits used by inx.\n\n"                                                            //
                 "\tThere is no pretence that this estimate is accurate - merely indicative.\n");               //
          break;                                                                                                //
       case 'u':  printf("\tThe file userCmds.data is created by the /genVerilog command. This file contains\n" //
                         "\tkeys and opcodes used by sim.exe and emu.exe to driver test scenarios in SAM.\n"    //
                         "\tThe format of the file is dictated by the format of files accepted by $readmemh().\n"//
                         "\tEach line of userCmds.data contains (keySize+2) * 2 bytes of hex data:\n"           //
                         "\t   - the first keySize hex pairs are a key in LSB order.\n"                         //
                         "\t   - the next two hex pairs are an opcode:\n"                                       //
                         "\t\ttypedef struct {bit [5:0] op,     //OP_READ, OP_SCAN, etc\n"                      //
                         "\t\t                    [1:0] item,   //ITEM_INDX or ITEM_PAGE\n"                     //
                         "\t\t                    [5:0] address,//address within row\n"                         //
                         "\t\t               } OPCODE;\n"                                                       //
                         "\tFor example:\n"                                                                     //
                         "\t1\t3630303039324337_0088\n"                                                         //
                         "\t2\t3130303033323834_0088\n"                                                         //
                         "\t3\t3230303045423831_0088\n"                                                         //
                         "\t4\t0000000000000000_0E50\n"                                                         //
                         "\t\t123456789ABCDEF00_0E60\n"                                                         //
                         "\tlines 1 - 3, (OP_SCAN=5'h08) scans the current INDX row for keys '7C290006', etc.\n"//
                         "\tline 4,      (OP_READ=5'h10) reads word 14 (0x0E) from the current PAGE row.\n"     //
                         "\tline 5,      (OP_WRYT=5'h20) writes 0x123456789ABCDEF0 to word 14 (0x0E) of the current row.\n"); //
                  break;                                                                                        //
       case 'v':                                                                                                //
          printf("sam /variable\n"                                                                              //
                 "\t"    "Create variable length records. The record offset of each record is used\n"           //
                 "\t"    "as the sort key. Other key related commands are ignored.\n"                           //
                 "\t"    "sam.exe will verify the indexBase integrity and readback by record number,\n"         //
                 "\t"    "but NOT readback by key since this is incorporated in read by record number.\n");     //
          break;                                                                                                //
      } //switch...                                                                                             //
   } //Help...

//Get a number allowing Verilog format (eg., 8'h55) and k/m/b/t suffix
uint32_t Anumber(char *pp, char **ppP)
   {int   value = strtol(pp, &pp, 10);                                          //
    if (*pp == '\'')                                                            //
       {if (*(++pp) == 'h') value = strtol(pp+1, &pp, 16); else                 //Verilog representation
        if (*(  pp) == 'b') value = strtol(pp+1, &pp,  2); else                 //
        if (*(  pp) == 'd') value = strtol(pp+1, &pp, 10); else                 //
                            value = strtol(pp+1, &pp, 10);                      //
      }                                                                         //
    else                                                                        //
      {if ((*pp | 0x20) == 'k') {value *= 1000;          pp++;} else            // * 1000 kilo
       if ((*pp | 0x20) == 'm') {value *= 1000000;       pp++;} else            // * 1000,000 millions (megabytes)
       if ((*pp | 0x20) == 'b') {value *= 1000000000;    pp++;} //else          // * 1000,000,000 billions (gigabytes)
//     if ((*pp | 0x20) == 't') {value *= 1000000000000; pp++;}                 // * 1000,000,000,000 trillions (terabytes)
      }                                                                         //
    if (ppP) *ppP = pp;                                                         //
    return value;                                                               //
   } //Anumber...

//Process command line. Return 0 if successful, otherwise -ve error and message in g_errMsg.
#define ERR(fmt, what, rtn) {snprintf(g_errMsg, sizeof(g_errMsg), __FUNCTION__ ": " fmt, what); return -rtn;}
#define IF_WORDIS(whatP)     if (WordIs(pp, whatP))
#define BREAK2ERR            break
//Upper case characters in whatP are mandatory
bool WordIs(char *pp, const char *whatP)
   {if (strnicmp(--pp, whatP, strlen(whatP)) != 0)                                //full word
      return _strnicmp(pp, whatP,strspn(whatP,"ABCDEFGHIJKLMNOPQRSTUVWXYZ")) == 0;//allowable abbreviation
    return true;                                                                  //
   } //WordIs...

void TestCommandLine(SAM_CONTROLS *ctlP);

int CommandLine(int argc, char **argv, SAM_CONTROLS *ctlP)
   {int         ii=0, jj, gg, sz, crashProof=0;                                         //poor coding escape
    uint32_t    value;                                                                  //
    char       *pp, *qq, *rr, cmd, *exeNameP=argv[0], *nextP=(char*)"";                 //
    bool        truB=true, stepB=false;                                                 //
    FILE       *fileP=NULL;                                                             //
                                                                                        //
    memset(ctlP, 0, sizeof(SAM_CONTROLS));                                              //initialize everything
    ctlP->verboseB            = true;                                                   //          "
    ctlP->rcdSize             = 512;                                                    //          "
    ctlP->targetBusSz        = 8;                                                      //
    ctlP->hParams.targetBusSz= 8;                                                      //
    ctlP->hParams.rowSize     = g_memRowSize;                                           //
    ctlP->generator           = 1;                                                      //random.
    ctlP->keySize             = g_keySize;                                              //overwritten perhaps by /keySize=<value>
    //Throw out plain '=' parameters. eg., 'sam keySize = 8'. Should be 'sam /keysize 8' or 'sam /keySize=8'
    for (gg=jj=0; gg < argc; gg++) if (strcmp(argv[gg], "=") != 0)argv[jj++] = argv[gg];//
    argc = jj;                                                                          //
    sz                        = Min(sizeof(ctlP->csvFile)-5, istrlen(exeNameP)+1);      //
    strncpy(pp=ctlP->csvFile, BaseDirectory(exeNameP), sz);                             //
    strcpy(&pp[strlen(pp)], "sam.csv");                                                 //add suffix .csv
    if (argc > 1 && IsDigit(*argv[1]))                                                  //1st program parameter == rcdCount
       ctlP->rcdCount = Anumber(pp=argv[ii=1], &pp);                                    //continue processing argv[2,3,...]
                                                                                        //
    //Process options                                                                   //
    while (++ii < argc)                                                                 //
        {if (stepB && ++ii >= argc)                                                     //finished, eh?
            {if (!ctlP->rowSizedB) ctlP->hParams.rowSize = 1024; return 0;}             //happy exit
         if (crashProof++ > 100) ERR("Internal software error%s", "", 1);               //
         cmd = ' ';                                                                     //
         if (*(pp=argv[ii]) == '/' || *pp == '-') pp++; else goto bad;                  // / or - preceeding each flag
         if ((cmd = *pp++ | 0x20) == '~' || cmd == '!')                                 // <word> or ~<word>
             {truB = false; cmd = *pp++ | 0x20;}                                        // /~ or /! set subsequent flag = false
         else truB = true;                                                              //          set subsequent flag = true
         value = 0;                                                                     //
         qq    = (char*)"";                                                             //
         stepB = false;                                                                 //
         //Lookahead to the next arg and prepare; saves analysis in every case          //
         if ((rr=strchr(pp, '=')) && IsDigit(rr[1]))                                    //
                           {value = Anumber(rr+1, &qq); nextP=rr+1; stepB = false;}else //<word>=<number>
         if (IsDigit(*pp)) {value = Anumber(pp,   &qq); nextP=rr+1; stepB = false;}else //<word><number> without space
         if (ii+1 < argc && IsDigit(*(argv[ii+1])))                                     //
                           {value = Anumber(nextP=argv[ii+1], &qq); stepB = true; }     //<word>  <number>
         if (*qq != 0) {ERR("Invalid numeric value: '%s'", pp, 1);}                     //
                                                                                        //
         //Invoke individual routines: pp    = command stripped of / and ~              //
         //                            value = value of next param if numeric           //
         //                            nextP = next argument, or text following '='     //
         switch (cmd)                                                                   //
            {default:                                                         BREAK2ERR;//unknown command
             #ifdef _DEBUG                                                              //
             case 'b': IF_WORDIS("BUGLEVEL") {g_bugLevel=ctlP->bugLevel=value;continue;}//sam /bugLevel #
                       IF_WORDIS("BUGMEM")   {ctlP->bugMemB  = true;          continue;}//sam /bugMemoryB
                       IF_WORDIS("Break")    {g_stopOnRcdNum = value;         continue;}//sam /b # setp stopOnRcdNum
                       ERR("Missing record number on /b command: %s", pp, 4);           //
             #endif                                                                     //
             case 'd': IF_WORDIS("DUMP")                                                //sam /dump <fileName>
                          {if (++ii >= argc)                                  BREAK2ERR;//missing filename
                           ctlP->bugLevel = 3;                                          //
                           if (!(g_printFileP=fopen(argv[ii], "wb")))                   //
                              ERR("Unable to create file: %s", argv[ii], 0);            //
                                                                              continue; //
                          }                                                             //
                       ctlP->shoAllB     = truB;                              continue; //sam /d       display sorted results
             case 'e': IF_WORDIS("Early")     {ctlP->earlyCache  = value;     continue;}//sam /early value set EarlyCache depth
                                                                              BREAK2ERR;//
             case 'g': IF_WORDIS("GENVERILOG"){ctlP->genVerilogB = truB;      continue;}//sam /genVerilog fileName
                       IF_WORDIS("GATHER")    {ctlP->gatherB = truB;          continue;}//sam /gather fileName
                                                                              BREAK2ERR;//
             case 'h': case '?': Help(argv[0], pp); exit(0);                            //sam /h
             case 'i': ctlP->bugLevel    = Max(value, 1);                     continue; //sam /i <bugLevel>
             case 'j': ctlP->testDujourB = truB;                              continue; //sam /j test du jour
             case 'k': IF_WORDIS("KEYSIZE")                                             //
                          {if (value > sizeof(sRECORD::key) || value < 2 || value>= 32)//keySize must be >= 2 & <= 32
                             {snprintf(g_errMsg, sizeof(g_errMsg), __FUNCTION__         //
                                     ": Invalid keySize(=%d), should be >= 2 and < %d", //
                                      value, (int)sizeof(sRECORD::key));                //
                              return -2;                                                //
                             }                                                          //
                           ctlP->keySize = g_keySize = value;                           //
                           g_rcdSize     = sizeof(sRECORD)-1 + g_keySize;               //
                           continue;                                                    //
                          }                                                             //
                       for (gg=HOWMANY(Generators); --gg >= 1;)                         //sam /k <generatorName>
                          if (_stricmp(pp, Generators[gg]) == 0) break;                 //
                       if (gg == 0)                                                     //
                          {if (*pp == 0 && ii+1 < argc) pp=argv[++ii];                  // /kfileName of /k fileName
                           if ((fileP=fopen(pp, "rb"))) fclose(fileP); else             //check fileName
                              {ERR("Unable to open file %s in /k command.", pp, 2);}    //
                           strncpy(ctlP->genFile, pp, sizeof(ctlP->genFile));           //
                          }                                                             //
                       ctlP->generator = gg;                                  continue; //
             case 'm': IF_WORDIS("MC")        ctlP->math = 1; else                      //sam /mc compact csv file with total space only
                       IF_WORDIS("MD")        ctlP->math = 2; else                      //sam /md detail
                       IF_WORDIS("ME")        ctlP->math = 3; else                      //sam /me summary Excel table
                       IF_WORDIS("MQS")       ctlP->math = 5; else                      //sam /mq work out mapping to sequencer-groups detail
                       IF_WORDIS("MQ")        ctlP->math = 4; else            goto bad; //sam /mq work out mapping; summary for Excel
                       continue;                                                        //sam /md full display with all fields
             case 'p': IF_WORDIS("PROGRESS") {ctlP->progressB   = truB;       continue;}//sam /progress
                       IF_WORDIS("PACK")     {ctlP->pack = stepB ? value : 0; continue;}//sam /pack [%age]
                       IF_WORDIS("PBefore")  {ctlP->shoPropsB4B = truB;       continue;}//sam /pbefore
                       IF_WORDIS("PAfter")   {ctlP->shoPropsAftB= truB;       continue;}//sam /pafter
                                                                              goto bad; //
             case 'q': ctlP->verboseB    = !truB;                             continue; //sam /q  quiet mode
             case 'r': IF_WORDIS("Rowsize")                                             //sam /rowSize value
                          {if (value == 128  || value == 256  || value == 512   ||      //valid row sizes
                               value == 1024 || value == 2048 || value == 4096  ||      //      "
                               value == 8192 || value ==16384 || value == 32768 ||      //
                                                                 value == 65536)        //
                              {g_memRowSize    = ctlP->hParams.rowSize = value;         //
                               ctlP->rowSizedB = true;                        continue; //
                           }  }                                                         //
                       ERR("Invalid row size in /rowSize parameter: %s", pp-1, 3);      //
             case 's': IF_WORDIS("STRIP")   {ctlP->stripB      = truB;                  //sam /strip
                                             ctlP->pack        = 100;         continue;}//
                                                                              BREAK2ERR;//
             case 't': IF_WORDIS("TARGET")  {ctlP->targetBusSz = value;       continue;}//
                       IF_WORDIS("TEST")    {strncpy(ctlP->testNums,nextP,10);continue;}//sam /test <string>
                       IF_WORDIS("Time")    {ctlP->shoTimeB  = truB;          continue;}//sam /t
                                                                              BREAK2ERR;//
             case 'v': IF_WORDIS("VARiable"){ctlP->variableB = true;          continue;}//sam /variable
                       IF_WORDIS("Verbose") {ctlP->verboseB  = truB;          continue;}//sam /verbose
                                                                              BREAK2ERR;//break to error
            } //switch ..., while (++ii ...                                             //
bad:snprintf(g_errMsg, sizeof(g_errMsg), "Invalid option /%c%s", cmd, pp); return -1;   //
        } //while(...                                                                   //
    return 0;                                                                           //
   } //CommandLine...
#undef ERR
#undef IF_WORDIS
#undef BREAKERR

#ifdef _DEBUG
//Work thru all the options to CommandLine and verify that they set the right flags
void TestCommandLine(SAM_CONTROLS *ctlP)
   {sRECORD  temp;                                                      //
    uint32_t rcdNum, keySize=ctlP->keySize;                             //
    #define KEY_FILE "keys.tmp"                                         //
    const char *pp="-stop", *stop[] = {"exeName", "-stop"},             //should cause error
               *argv[] =                                                //simulate argv[] for program startup
      {"exeName",        "1000",                                        //program name, rcdCount
       "/BUGLEVEL","3",  "/BUGMEM",     "/b13",      "/d", "/early","3",//
       "/genVerilog",    "/gather",     "/i",        "/j",              //
       "/krandom",       "/ksequential","/kreverse", "/k" KEY_FILE,     //
       "/MC", "/MD",     "/ME",         "/MQS",      "/MQ",             //
       "/PROGRESS",      "/PB",         "/PA",       "/STRIP",          //before '/pack #' please
       "/PACK",          "/PACK",       "99",        "/q ",             //
       "/rRowsize","128","/TARGET","8", "/Time",     "/test=1352",      //
       "/var", "/verbose"};                                             //
    FILE *fileP;                                                        //
                                                                        //
    delete g_generateP; g_generateP = NULL;                             //
    //Create manual key input file for /k test                          //
    if (!(fileP=fopen(pp=KEY_FILE, "wb")))                  goto bad;   //create input file
    fprintf(fileP, "1,2,3,/*comment*/\n$generate(4,6,1),\n"             //ie., 1, 2, 3, 4, 5, 6
                   "$duplicate(3, \"smith\")//note\n$dump\n$headers\n");//ie., smith, smith, smith
    fclose(fileP);                                                      //
                                                                        //
    if (CommandLine(HOWMANY(stop), (char**)stop, ctlP) >=0) goto bad;   //error is correct
    if (strcmp(g_errMsg, "Invalid option /stop") != 0)      goto bad;   //
    g_errMsgP[0] = 0;                                                   //
    if (CommandLine(HOWMANY(argv), (char**)argv, ctlP) < 0) goto bad;   //
    if ( ctlP->rcdCount!= 1000) {pp = "rcdCount";           goto bad;}  //check all flags and settings
    if (g_bugLevel        != 3) {pp = "bugLevel";           goto bad;}  //            "
//  if (!g_bugFileP           ) {pp = "bugMemB";            goto bad;}  //            "
    if (g_stopOnRcdNum    !=13) {pp = "g_stopOnRcdNum";     goto bad;}  //            "
    if (!ctlP->shoAllB        ) {pp = "shoAllB";            goto bad;}  //            "
    if (!ctlP->genVerilogB    ) {pp = "genVerilogB";        goto bad;}  //            "
    if (!ctlP->gatherB        ) {pp = "gatherB";            goto bad;}  //            "
    if ( ctlP->earlyCache != 3) {pp = "earlyCache";         goto bad;}  //            "
    if (!ctlP->testDujourB    ) {pp = "testDujourB";        goto bad;}  //            "
    if ( ctlP->generator  != 0) {pp = "generator";          goto bad;}  //            "
    if ( ctlP->genFile[0] == 0) {pp = "genFile";            goto bad;}  //            "
    if ( ctlP->math       != 4) {pp = "math";;              goto bad;}  //            "
    if (!ctlP->stripB         ) {pp = "stripB";             goto bad;}  //            "
    if ( ctlP->pack       !=99) {pp = "pack";               goto bad;}  //            "
    if (!ctlP->progressB      ) {pp = "progressB";          goto bad;}  //            "
    if (!ctlP->shoPropsB4B    ) {pp = "shoPropsB4B";        goto bad;}  //            "
    if (!ctlP->shoPropsAftB   ) {pp = "shoPropsAft";        goto bad;}  //            "
    if (!ctlP->verboseB       ) {pp = "verboseB";           goto bad;}  //            "
    if (g_memRowSize     !=128) {pp = "g_memRowSize";       goto bad;}  //            "
    if ( ctlP->targetBusSz!=8) {pp = "targetBusSz";         goto bad;}  //            "
    if (!ctlP->shoTimeB)        {pp = "shoTimeB";           goto bad;}  //            "
    if (strcmp(ctlP->testNums, "1352") != 0) {pp="testNum"; goto bad;}  //            "
    if (!ctlP->variableB      ) {pp = "variableB";          goto bad;}  //            "
                                                                        //
    //now verify that g_generateP->GenerateData() works correctly.      //
    memset(&temp, 0, sizeof(temp));                                     //
    ctlP->generator = 0;                                                //from file, ctlP->genFileP
    g_generateP     = new cGenerator(ctlP);                             //
    if (g_errMsgP[0]   != 0)        {pp = "new cGenerator"; goto bad;}  //
    if (ctlP->rcdCount != 9)        {pp = "rcdCount";       goto bad;}  //
    for (rcdNum=1; rcdNum <= ctlP->rcdCount; rcdNum++)                  //
       {g_generateP->GenerateData(ctlP, 0, g_rcdSize, &temp, keySize);  //
        if (rcdNum <= 6 && strtol((char*)temp.key, NULL, 10) != rcdNum) //
                          {pp = "generated numeric record"; goto bad;}  //
        if (rcdNum > 6 && strcmp((char*)temp.key, "smith") != 0)        //
                          {pp = "generated string record";  goto bad;}  //
       }                                                                //
    unlink(ctlP->genFile);                                              //
    return;                                                             //
bad:if (g_errMsg[0] != 0) printf("%s\n", g_errMsg);                     //
    printf("Internal selftest of CommandLine() failed: %s", pp);        //
#undef KEY_FILE
   } //TestCommandLine...
#else
void TestCommandLine(SAM_CONTROLS *ctlP) {}
#endif

//Format information from m_timerP->m_info[] into a comma delimitted file sam.csv.
//Body of MainAction() is peppered with TIMER_ON(name), TIMER_OFF(name), and TIME_THIS(name).
//This routine scans through the m_info[] array and formats one line. For example,
//with TIME_THIS(write), TIME_THIS(read_by_loc), TIME_THIS(read_by_key) the output looks like:
//  "Record,rowSize,books,index,write,read,  read,  source,index,  page,  ,Test-Datime\n"
//  "count,(bytes),      ,pages,     ,by_loc,by_key,order, loading,loading,22/08/21@07:20:46.117049\n"
//  "10000,  5, 256,  948,   28.38ms,   7.08ms,   4.00ms,random    ,46%,30%\n"
//The characters before the '_' in the name are on the first line,
//the characters following the '_' are on the second line.
//The output is designed to be fed into Excel: 'excel.exe sam.csv'
#ifdef USE_TIMERS
void GatherData(SAM_CONTROLS *ctlP, const char *sourceOrderP)
   {FILE          *fileP;                                                       //
    char           buf[50], *pp;                                                //
    const char    *leadP, *tailP;                                               //leading and trailing text on each line
    int            ii, line;                                                    //
    TIMER_INFO    *tP;                                                          //
    cTIMEVALUE     tv; tv.GetGmtv();                                            //
    cCalendarTime  ct; ct.SetCalendarTime(&tv);                                 //Get current date
                                                                                //
    fileP = fopen(ctlP->csvFile, "a+b");                                        //create/open file
    if (fileP == NULL) {Printf("Unable to open %s\n", ctlP->csvFile); return;}  //
    fseek(fileP, 0, SEEK_END);                                                  //
    if (ftell(fileP) == 0)                                                      //file is empty; create two lines of heading
       {leadP = "Record," "rowSize," "books," "index,";                         //First four fields on top line are fixed
        tailP = "source," "index," "page," "early,,Test-Datime(GMT)\n";         //last  five fields on 2nd line are fixed
        for (line=0; line < 2; line++)                                          //
           {fprintf(fileP, leadP);                                              //
            for (ii=1; ii < HOWMANY(g_timerP->m_info); ii++)                    //
                {if ((tP=&g_timerP->m_info[ii])->nameP == NULL) break;          //end of m_info[]
                 if (tP->nameP[0] == '_')                       continue;       //ignore labels beginning with '_'
                 strncpy(buf, tP->nameP, Min(sizeof(buf),istrlen(tP->nameP)+1));//
                 pp = strchr(buf, '_');                                         //Split name at '_'
                 if (pp == NULL) pp = (char*)""; else *pp++ = 0;                //left on 1st line, rmdr on 2nd line
                 fprintf(fileP, "%s,", line ? pp : buf);                        //
                }                                                               //
            fprintf(fileP, tailP);                                              //
            leadP = "count,"  "(bytes)," "," "pages,";                          //First four fields on top line are fixed
            tailP = "order,"  "loading," "loading,"  "cache\n";                 //last  five fields on 2nd line are fixed
           }                                                                    //
      }                                                                         //
    //Now output one line of data.                                              //
    fprintf(fileP, "%5d,%5d,%5d,%5d,", ctlP->rcdCount,                          //
                       ctlP->hParams.rowSize, ctlP->booksUsed, ctlP->pagesUsed);//first four fixed fields
    for (ii=1; ii < HOWMANY(g_timerP->m_info); ii++)                            //first slot not used
        {if ((tP=&g_timerP->m_info[ii])->nameP == NULL) break;                  //end of m_info[]
         if (tP->nameP[0] == '_')                       continue;               //ignore labels beginning with '_'
         fprintf(fileP, "%s", g_timerP->TimerFormat(tP->nameP, true));          //individual timer value (has comma)sam /s
        }                                                                       //
    fprintf(fileP, "%s,%02d%%,%02d%%,%2d%%,",                                   //last three columns
                   sourceOrderP, ctlP->indexLoad, ctlP->pageLoad,               //
                   100*ctlP->earlyHits / Max(1,ctlP->earlyTries));              //
    fprintf(fileP, ",%s\n", ct.Format(buf, sizeof(buf), "%Y/%M/%D@%H:%M:%S%."));//add datetime to second line
    fclose(fileP);                                                              //
   } //GatherData...
#else
void GatherData(SAM_CONTROLS *ctlP, const char *sourceOrderP) {}
#endif

int CreateAndWriteRecords(cSadram *sortP, SAM_CONTROLS *ctlP)
  {uint32_t rcdNum, bugLevel=0;                                      //duplLocn = location of forced duplicate
   int      err=0, instance=0, keySize=ctlP->keySize,                           //
                               rcdSize=SizeofRecord(keySize);                   //
   sRECORD  temp={0};                                                           //
   uint8_t *dstRcdP;                                                            //
   bool     okB=true;                                                           //hope springs eternal

   if (ctlP->verboseB) Printf("Writing records.\n");                            //
   for (rcdNum=0, dstRcdP=(uint8_t*)g_dataP; rcdNum < ctlP->rcdCount; rcdNum++) //
       {g_generateP->GenerateData(ctlP, rcdNum, rcdSize, &temp, keySize);       //
        if (ctlP->progressB && (rcdNum & 0xff) == 0)                            //
            Printf("\r%d%%\r", rcdNum*100/ctlP->rcdCount);                      //output percentage done
        if (rcdNum == ctlP->dupLocn)                                                  //force duplicate key condition
           {memmove(temp.key, g_dataP[0].key, keySize);                         //
            strncpy(temp.txt, "DuplicateRecord 0", sizeof(sRECORD::txt));       //
           }                                                                    //
        temp.rcdLen = rcdSize - (ctlP->variableB ? rcdNum & 15 : 0);            //with variable length (if requested)
        #ifdef _DEBUG                                                           //
          if (rcdNum >= g_stopOnRcdNum)                                         //from command line: /break <rcdNum>
             {if (!g_bugP->CheckIndexbase(rcdNum))                              //
                 {g_bugP->BugB4Insert(rcdNum, &temp);                           //Dump indexbase if verify failed
                  Assert(false, NULL);                                          //throw a hissy-fit
             }   }                                                              //
        #endif //DEBUG...                                                       //
        TIME_THIS(write, sortP->WriteRecord(dstRcdP, &temp, temp.rcdLen));      //write record
        dstRcdP += temp.rcdLen;                                                 //
       } //for (rcdNum=...                                                      //
   TIME_THIS(write, sortP->WriteFinish());                                      //
                                                                                //
   g_generateP->GenerateData(ctlP, rcdNum, rcdSize, &temp, keySize);            //Flush out any remaining script commands
   return rcdNum;                                                               //
  } //CreateAndWriteRecords...

//Main program entry point.
int MainAction(SAM_CONTROLS *ctlP)
  {uint32_t bugLevel=0;                                                         //
   int      err=0, instance=0, keySize=ctlP->keySize,                           //
                               rcdSize=SizeofRecord(keySize);                   //
   bool     okB=true;                                                           //hope springs eternal
   uint32_t samOptions = (ctlP->stripB ? OPTION_STRIP : 0) |                    //
                          #ifdef FULLY_ALPHABETIC_KEYS                          //
                            OPTION_MSB;                                         //direction=hi-lo order
                          #else                                                 //
                            OPTION_LSB;                                         //direction=lo-hi order
                          #endif                                                //
   if (ctlP->variableB) samOptions |= OPTION_VBL_RCD;                           //
   samOptions |= (ctlP->earlyCache & OPTION_EARLY_BITS);                        //early caching in lower 8 bits of samOptions
   if (ctlP->verboseB)                                                          //
      Printf("Sam.exe rev#%d (debug), " __DATE__ ", rcdCount=%d\n",             //
                                                SAM_VERSION, ctlP->rcdCount);   //
                                                                                //
   g_dataP = (sRECORD*)malloc(ctlP->rcdCount * rcdSize);                        //user data records (over committment)
   cSadram sort(ctlP->rcdCount,                                                 //
                offsetof(sRECORD, key), keySize,                                //locn & sizeof key in record
                ctlP->variableB ? 0 : rcdSize,                                  //
               &ctlP->hParams,                                                  //
                samOptions);                                                    //
   g_helpP = new cHelper(&sort);                                                //
                                                                                //
   if (ctlP->shoPropsB4B) {g_helpP->ShowProperties(ctlP, false);      goto xit;}//sam /pbefore; false = before sort
                                                                                //
   ctlP->dupLocn = (ctlP->generator == 0) ? -1 : 5;                             //duplLocn = location of forced duplicate
                                                                                //let use define dups when driven from a file
   //Write records into data array; hardware creates indexes contemporaneously. //
   //loop overhead does not significantly contribute to write time except when  //
   //reading input data from a file (/k filename option).                       //
   if ((err=CreateAndWriteRecords(&sort, ctlP)) < 0) okB = false;               //
                                                                                //
   //index has now been built. Validate indexBase and possibly display it       //
   if (ctlP->bugLevel > 0 && !ctlP->stripB)                                     //sam /i <level> display page headers for indexBase
      {bugLevel   = g_bugLevel;                                                 //save user settings
       g_bugLevel = ctlP->bugLevel;                                             //
       okB        = g_bugP->CheckIndexbase(ctlP->rcdCount);                     //
       if (!ctlP->genVerilogB) g_bugP->Bug("Post sort");                        //display index tables
       if (!okB) Printf("**** Failed CheckIndexbase; location=(%d.%d.%d)\n",    //make visible after possibly massive
                   g_bugP->m_fail.buk, g_bugP->m_fail.page, g_bugP->m_fail.inx);//  dump generated by BugIndexes()
       g_bugLevel = bugLevel;                                                   //restore user settings
      }                                                                         //
                                                                                //
   if (!ctlP->stripB && !(okB=g_bugP->CheckSequence(ctlP)))           goto xit; //uses index tables
   if     (!(okB=g_bugP->CheckPresent (ctlP, ctlP->dupLocn)))         goto xit; //uses LocByRcdNum (ie, user read fnc)
   if     (!(okB=g_bugP->CheckRecords(ctlP)))                         goto xit; //uses LocByRcdNum (ie, user read fnc)
                                                                                //
   if (ctlP->genVerilogB && okB) okB = GenVerilogFiles(&sort, ctlP);  goto xit; // sam /genVerilog generate Verilog data files
                                                                                //
   if (ctlP->shoPropsAftB) g_helpP->ShowProperties(ctlP, true);                 // sam /pa
                                                                                //
   //index has now been built. Read back in sorted order.                       //
   TIME_THIS(read_by_loc, okB=g_helpP->CheckLocByRcdNum(ctlP));                 //
   if (!okB)                                                          goto xit; //
                                                                                //
   //readback by key (unless variable length records for which CheckLocByRcdNum)//
   if (!ctlP->variableB && !ctlP->stripB)                                       //why variable ??
      {TIME_THIS(read_by_key, okB=g_helpP->CheckLocByKey(ctlP, rcdSize));       //
       if (!okB)                                                      goto xit; //
      }                                                                         //
                                                                                //
   ctlP->pagesUsed = g_helpP->GetPagesUsed();                                   //gather statistics on this performance
   ctlP->indexLoad = g_helpP->IndexLoading();                                   //                  "
   ctlP->pageLoad  = g_helpP->PageLoading();                                    //                  "
   ctlP->earlyTries= g_helpP->GetEarlyTries();                                  //                  "
   ctlP->earlyHits = g_helpP->GetEarlyHits();                                   //                  "
   ctlP->booksUsed = g_helpP->GetBooksUsed();                                   //                  "
                                                                                //
   if (ctlP->gatherB) GatherData(ctlP, Generators[ctlP->generator]);            //log statistics for this run
                                                                                //
   //Print sorted data if user requested.                                       //
   if (ctlP->shoAllB)  g_helpP->PrintSorted(ctlP->variableB);                   // sam /d command
   if (ctlP->verboseB) Printf("Sorted %d records Correctly\n", ctlP->rcdCount); // sam /v:
   #ifdef USE_TIMERS
   if (ctlP->shoTimeB)                                                          //
      {g_timerP->TimerShow(NULL, ctlP->rcdCount);                               //
       if (ctlP->earlyHits != 0)                                                //
           Printf("%20s %d%%\n", "EarlyCache Efficiency",                       //
                               (100*ctlP->earlyHits)/Max(1, ctlP->earlyTries)); //
      }                                                                         //
   #endif //USE_TIMERS...
xit:                                                                            //
   delete g_dataP; g_dataP = NULL;                                              //
   delete g_helpP; g_helpP = NULL;                                              //
   return okB ? 0 : 1;                                                          //0 == OK, 1 == failed
  } //MainAction...                                                             //sort deleted on exit scope

int main(int argc, char **argv)
   {int          err=0, leaks=0, ii=0;                                          //
    SAM_CONTROLS controls;                                                      //
    CommandLine(argc, argv, &controls);                                         //sets g_errMsg if unhappy
    if (controls.genVerilogB)                                                   //
       {if (!controls.rowSizedB) g_memRowSize = controls.hParams.rowSize = 256; //default unless user overrides with /rowSize
        g_bugLevel = 0;                                                         ///buglevel applies to FPGA/hardware setting
       }                                                                        //  (still available in options.bugLevel.
    #ifdef USE_TIMERS                                                           //
       g_timerP = new cCodeTimer(argv[0]);                                      //
    #endif                                                                      //
    g_generateP = new cGenerator(&controls);                                    //sets g_errMsg if unhappy
    if (g_errMsg[0] != 0) {Printf("**** %s\n", g_errMsg); err = 1; goto xit;}   //error from above routines
                                                                                //
    #ifdef _DEBUG                                                               //
      strncpy(g_exeFileDir, argv[0], sizeof(g_exeFileDir)-1);                   //
      for(ii=istrlen(g_exeFileDir); --ii > 0 && g_exeFileDir[ii] != '\\';) {}   //
      g_exeFileDir[ii+1] = 0;                                                   //
    #endif                                                                      //
    if (controls.bugMemB)                                                       //
       {int ii=(int)strlen(g_exeFileDir);                                       //
        strncat(&g_exeFileDir[ii], "\\memory.sambug", sizeof(g_exeFileDir)-ii); //
        g_bugFileP       = fopen(g_exeFileDir, "wb");                           //
        g_exeFileDir[ii] = 0;                                                   //
       }                                                                        //
                                                                                //
    if (false)                                                                  //
       {TestCommandLine(&controls); exit(0);}                             else  //
    if (argc == 1)            Help(argv[0], "");                          else  //
    if (controls.testDujourB){TestDujour(&controls, 3); exit(0);}         else  // sam /j command, experimental tests
    if (controls.math != 0)   DisplayMath(&controls);                     else  // sam rcdCount /rrowSize /mkeySize
                              err = MainAction(&controls);                      //
    #ifdef USE_TIMERS                                                           //
       delete g_timerP;                                                         //
    #endif                                                                      //
    delete g_generateP;                                                         //do these before calling Leaks()
    delete g_bugP;                                                              //
    leaks = _CrtDumpMemoryLeaks();                                              //(please tell the Welsh)
    ClosePrintFile();                                                           //
    #ifdef _DEBUG                                                               //
      if (g_bugFileP) fclose(g_bugFileP);                                       //
      if (leaks != 0  && err == 0)                                              //skip if there is another error
         {char buf[_MAX_PATH+250]; int len, ok=MB_OK;                           //
          sprintf(buf, "\t%d memory block(s) still allocated \n\n",leaks);      //
          len = (int)strlen(buf);                                               //
          if (g_bugFileP != NULL)                                               //
             {ok = MB_OKCANCEL;                                                 //
              snprintf(&buf[len], sizeof(buf) - len,                            //
                     "Reponses:\n"                                              //
                     "\tOK"  "\tInvoke postProcessMemoryBug.bat <bugfile>\n"    //
                     "\tCancel\tExit sam.exe\n");                               //
             }                                                                  //
          else                                                                  //
              snprintf(&buf[len], sizeof(buf) - len,                            //
                 "Use\t'sam /bugMemory' to track memory allocation. This will\n"//
                 "\tproduce a file called \n\t    %s\n"                         //
                 "Then\tcall postProcessMemoryBug.bat to process this file.",   //
                 g_exeFileDir);                                                 //
          if (MessageBoxA(NULL, buf, __FILE__, ok) == IDOK && controls.bugMemB) //
             {sprintf(buf, "postProcessMemoryBug %s&exit", g_exeFileDir);       //
              system(buf);                                                      //
         }   }                                                                  //
 // assert(err == 0);                                                           //not Assert() please
    #endif //_DEBUG...                                                          //
xit:return err;                                                                 //
   } //main...

//end of file...
