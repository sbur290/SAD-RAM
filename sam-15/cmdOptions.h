//Structure containing command line parameters
#ifndef _CMD_OPTIONS_H_INCLUDED
#define _CMD_OPTIONS_H_INCLUDED
#include <stdlib.h>
typedef struct
   {unsigned int rcdCount,                              //number of records to write
         shoHeaders;                                    // /i debuglevel     display index page headers on completion
    int  generator,                                     // /k [...]          method of generating source keys
         rcdSize,                                       // /rcdSize value    nominal size of record
         earlyCache;                                    //                   != 0 means check early cache(value)
    bool progressB,                                     // /progress         show progress on loading underlying data
         gatherB,                                       // /g                gather statistics for /stats
         shoAllB,                                       // /d                show all records in sorted order oncompletion
         shoTimeB,                                      // /t                show time taken as sorts are performed
         shoPropsAftB,                                  // /pa               show properties of cSam after  sort
         shoPropsB4B,                                   // /pb    (no sort)  show properties of cSam before sort
         shoStatsB,                                     // /stats (no sort): display statistics gather in earlier runs using /g optin
         verboseB,                                      // /v                minimal messages
         testDujourB,                                   // /j     (no sort)  test du jour
         compressB,                                     // /c                compress index database on completion
         variableB,                                     // /variable         use variable length records
         mathB;                                         // /math             display math of keysize/rowsize/filesize
    FILE *genFileP;                                     // /k fileName       specify input file
    char statsFileName[_MAX_PATH];                      // /s fileName       specify input log file (stdlib.h)
    unsigned long pagesUsed, indexLoad, pageLoad;       // results
   } SAM_OPTIONS;
#endif //_CMD_OPTIONS_H_INCLUDED
