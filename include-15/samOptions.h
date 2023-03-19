//Structure containing command line parameters
#ifndef _SAM_OPTIONS_H_INCLUDED
#define _SAM_OPTIONS_H_INCLUDED
#include <stdlib.h>
#include <hStructures.h>

typedef struct
   {unsigned int
         rcdCount,                                      //number of records to write
         roSize,                                        //bytes in DRAM row
         keySize,                                       //
         bugLevel,                                      // /i debuglevel     display indexbase on completion
         pack,                                          // /pack %age
         targetBusSz;                                   //
    int  generator,                                     // /k [...]          method of generating source keys
         rcdSize,                                       // /rcdSize value    nominal size of record
         earlyCache,                                    // /e #              early cache(value)
         math,                                          // /mc, /md, or /me  display math of keysize/rowsize/filesize
         dupLocn;                                       //
    char testNums[11]; //1+10=number of digits, ie 0 - 9// /test value       specify test numbers, eg., 1235 specifies tests 1, 2, 3, and 5
    bool gatherB,                                       // /g                gather statistics for /stats
         shoAllB,                                       // /d                show all records in sorted order on completion
         shoTimeB,                                      // /t                show time taken as sorts are performed
         shoPropsAftB,                                  // /pafter           show properties of cSadram after  sort
         shoPropsB4B,                                   // /pbefore(no sort) show properties of cSadram before sort
         progressB,                                     // /progress         show progress on loading underlying data
         shoStatsB,                                     // /stats  (no sort) display statistics gather in earlier runs using /g optin
         verboseB,                                      // /v                minimal messages
         testDujourB,                                   // /j     (no sort)  test du jour
         compressB,                                     // /c                compress index indexBase on completion
         variableB,                                     // /variable         use variable length records
         csvB,                                          // /mc               csv file with total index requirements only
         excelB,                                        // /me               csv file summary for Excel(used to create graph)
         genVerilogB,                                   // /export fileName  output statistics into fileName
         rowSizedB,                                     //                   /r parameter specified
         stripB,                                        // /strip <%age>     strip index structures down to pointers
         bugMemB;                                       // /bugMem           output allocate and deallocate messages
    unsigned long pagesUsed,  indexLoad, pageLoad,      // results
                  earlyTries, earlyHits, booksUsed;     //    "

    char genFile    [_MAX_PATH];                        // /k fileName       specify input file
    char verilogBase[_MAX_PATH];                        // /genVerilog <fileName>
    char csvFile    [_MAX_PATH];                        //
    sHDW_PARAMS     hParams;                            //
   } SAM_CONTROLS;
#endif //_SAM_OPTIONS_H_INCLUDED
