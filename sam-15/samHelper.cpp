/* samhelper.cpp : July 2022
* This modules contains various function to support cSadram.
     void PrintSorted    (void)           outputs the entire indexBase in sorted order.
*/

#define _CRT_SECURE_NO_WARNINGS 1

#include "sam-15.h"
#include "samHelper.h"

extern const char      *g_errMsgP;
extern uint32_t         g_memRowSize;
extern uint32_t         g_bugLevel;
extern class cHelper   *g_helpP;
extern class cBugIndex *g_bugP;
extern class cSadram   *g_sP;

#define MBOOK         (*g_sP->m_bookP)

#define PrintError(causeP, buk, page, inx) _PrintError(__FUNCTION__, __LINE__, causeP, buk, page, inx);
extern void _PrintError(const char *fncO, int line, const char *causeP, uint32_t buk, uint32_t page, uint32_t inx);

cHelper::cHelper(cSadram *)
   {m_maxPages = g_sP->WorstCase(g_sP->m_xMax, cPAGE_perRow); //# of cPAGE[]'s reqd; each comprising m_xMax cINDX[]'s
   } //cHelper::cHelper...

int cHelper::CompareKey(CV aV, CV bV)
   {return g_sP->CompareKey(aV, bV, g_sP->m_keySize);}

//bool cHelper::BugIndexes(const char *titleP, uint32_t buk, uint32_t startPg)
//   {return g_bugP->Bug(titleP, buk, startPg);}

bool cHelper::DatabaseStats(void)
   {Printf("\nDatabase statistics --------\n");                                 //
    Printf("  #cBOOK's allocated(m_bMax)= %d"                                   //
           ", #cBOOK's used (m_bUsed) = %d\n",g_sP->m_bMax, g_sP->m_bUsed);     //
                                                                                //
    Printf("  cPAGE size(cPAGE_size) = %d, pages/row(cPAGE_perRow) = %d\n",     //
           cPAGE_size, cPAGE_perRow);                                           //
    Printf("          allocated rows(pMax) = %d, "                              //
           "#cPAGE's = %d (pMax*cPAGE_perRow), #cPAGE's used = %d\n",           //
           m_maxPages, m_maxPages * cPAGE_perRow, GetPagesUsed());              //
                                                                                //
    Printf("  cINDX size(cINDX_size) = %d, indexes/row(cINDX_perRow) <= %d\n",  //
           cINDX_size, cINDX_perRow);                                           //
    Printf("          allocated rows(m_xMax) = %d, "                            //
           "#cINDX's = %d (m_xMax*cINDX_perRow), #cINDX's used = %d (m_xUsed)\n",//
           g_sP->m_xMax, g_sP->m_xMax * cINDX_perRow, g_sP->m_rcdNum+1);        //
                                                                                //
    if (g_bugP->m_fails > 0)                                                    //
        cBugIndex::PrintError("Failed sequence checks",                         //
                  g_bugP->m_fail.buk, g_bugP->m_fail.page, g_bugP->m_fail.inx); //
     return g_bugP->m_fails == 0;                                               //
    } //cHelper::DatabaseStats...

//Calculate #cINDX's actually used versus #cINDX's allocated
//Up to m_bMax books could have been used, each having cPAGE_perRow pages in book[i].pagesP.
//Each book[i].pagesP has cPAGE_perRow entries with possibly cINDX:perRow cINDXs
//Therefore, the maximum potentially allocated = m_bMax * cPAGE_perRow * cINDX_perRow.
int cHelper::IndexLoading(void)
   {uint32_t buk, page, used, allocated=g_sP->m_bMax * cPAGE_perRow * cINDX_perRow;
    cPAGE   *pageP;                                                             //
    for (buk=used=0; buk < g_sP->m_bUsed; buk++)                                //
       for (page=0; page < MBOOK[buk]->count; page++)                           //
           {pageP = g_sP->BPadr(buk, page);                                     //
            used += pageP->count;                                               //
           }                                                                    //
    return used*100/allocated;                                                  //
   } //cHelper::IndexLoading...

//Calculate #cPAGE's actually used versus number of cPAGE's potentially allocated.
//Up to m_bMax books could have been used, each having cPAGE_perRow pages in book[i].pagesP.
//Therefore, the maximum potentially allocated = m_bMax * cPAGE_perRow.
//The number actually used = sum(book[i]->count)
int cHelper::PageLoading(void)
   {uint32_t  buk, used=0, allocdPages=g_sP->m_bMax * cPAGE_perRow;              //
    for (buk=0; buk < g_sP->m_bUsed; buk++) used += MBOOK[buk]->count;          //
    return used*100/allocdPages;                                                //
   } //cHelper::PageLoading...

int cHelper::BookLoading(void)
   {return g_sP->m_bUsed*100/g_sP->m_bMax;                                      //
   } //cHelper::BookLoading...

//Print one line in table of basic parameters.
//f<i>P may contain %d or %s which is used to 'sprintf' the entry.
//f<i>P beginning with . are centered in ww[col] spaces (and the '.' is removed).
void cHelper::Col(CC f1P, CC f2P, CC f3P, CC f4P, int p1, int p2, int p3, int p4)
   {char    buf[64], fill=f1P[1], *eBufP=&buf[sizeof(buf)-1], *pp;              //
    int     len, col, ww, widths[]= {28, 12, 14, 37}, bufSize=sizeof(buf);      //colum widths
                                                                                //
    buf[bufSize-1] = 0;                                                         //
    for (col=1; col <= 4; col++)                                                //
        {snprintf(buf, bufSize, f1P, p1, p2, p3, p4);                           //
         ww = widths[col-1];                                                    //
         if ((int)strlen(buf) > ww && (pp=(char*)strchr(f1P, '%')) != NULL)     //
            {//patch up buf by replacing %d field with %dK or %dM               //
             if (p1 > 10000000) snprintf(&buf[(int)(pp-f1P)], bufSize-10, "%dM", (p1+500000)/1000000); else
             if (p1 > 10000)    snprintf(&buf[(int)(pp-f1P)], bufSize-10, "%dK", (p1+500   )/1000);
             pp += strspn(pp, "%d");                                            //
             snprintf(&buf[(int)strlen(buf)], bufSize-10, pp, p2, p3, p4);      //
            }                                                                   //
         if (f1P[0] == '.')                                                     //rqst to center column
            {buf[0] = ' ';                                                      //
             len    = (ww-(int)strlen(buf))/2;                                  //leading spaces required
             for (; len > 0; len--, ww--) Printf("%c", fill);                   //center buf on column width
            }                                                                   //
         if (strchr(f1P, '%') != NULL)                                          //column has dynamic field
            {p1 = p2; p2 = p3; p3 = p4;}                                        //'left shift' parameters for next %
         for (len=(int)strlen(buf); len < ww;) buf[len++] = fill;               //fill to proper width
         buf[len++] = fill == ' ' ? '|' : fill == '-' ? '+' : fill;             //terminate line
         buf[len]   = 0;                                                        //
         Printf("%s", buf);                                                     //and print
         f1P = f2P; f2P = f3P; f3P = f4P;                                       //'left shift' for next loop
        } //for (col=...                                                        //
    Printf("\n");                                                               //
   } //cHelper::Col...

//samsort /pa or samsort /pb commands
//Format and output table of sort geometry.
void cHelper::ShowProperties(SAM_CONTROLS *ctlP, bool afterB)
 {int         rows, rcdSz=(int)sizeof(sRECORD), rcdCount=g_sP->m_rcdCount,
              pUsed=GetPagesUsed(), bUsed=g_sP->m_bUsed, generator=ctlP->generator,
              bMax = g_sP->m_bMax;
  uint32_t    options = g_sP->m_options;
  bool        lsb  = (options & OPTION_LSB) != 0,
              vblB = (options & OPTION_VBL_RCD) !=0;
  const char *generators[] = {".<file>", ".random", ".sequential", ".reverse"}, *fmtP="%d";
  #define Heading(title) Col("+--- " title)
  Col("+--- Parameter ",                   "---value ",    "--field name",    "--formula ");
  Col("| Record Count",                    ".%d",          " m_rcdCount",     " user specified",                 rcdCount);
  Col("| KeyOffset",                       ".%d",          " m_keyOffset",    " hard coded=offset(sRECORD,key)", g_sP->m_keyOffset);
  Col("| KeySize",                         ".%d",          " m_keySize",      " /keySize <value>",               g_sP->m_keySize);
  Col("| RcdSize",     vblB ? "variable" : ".%d",          " m_rcdSize",      " hard coded=sizeof(sRECORD)",     g_sP->m_rcdSize);
  Col("| Source key order",                generators[1+generator], "",       " user specified: /k command");
  Col("| Compare order",lsb ? ".true":".false",            "",                lsb ? "LSB first" : " MSB first");
  Col("| Memory Row Size",                 ".%d bytes",    " m_rowsize",      " from hardware specs",            g_sP->m_rowBytes);
  Col("| Space for User data",             " %d bytes",    "",                " %d records* %d bytes/record)",   rcdSz*rcdCount, rcdCount, rcdSz);
  Heading("cINDX detail");
  Col("| Actual sizeof(cINDX)",            ".%d bytes",    " cINDX size",      "",                               cINDX_size);
  Col("| cINDX's per row",                 ".%d",          " cINDX_perRow",    " m_rowBytes / cINDX_size",        cINDX_perRow);
  Col("| #cINDX[]'s (worst case)",         ".%d rows",     " m_xMax",         " WorstCase(m_rcdCount, cINDX_perRow)",g_sP->m_xMax);
  rows = g_sP->m_xMax;
  Col("| Actual space for cINDX's",        ".%d bytes",    "",                " %d rows*%d(=m_rowsize)",         rows * g_memRowSize, rows, g_memRowSize);

  Heading("cPAGE detail");
  Col("| Actual sizeof(cPAGE)",            ".%d bytes",    " cPAGE size",     "",                                cPAGE_size);
  Col("| cPAGE's per row",                 ".%d",          " cPAGE_perRow",   " m_rowBytes/cPAGE_size",           cPAGE_perRow);
  Col("| #cPAGE[]'s (worst case)",         ".%d rows",     " m_pMax",         " WorstCase(m_xMax, cPAGE_perRow)", m_maxPages);
  rows = m_maxPages;
  Col("| Actual space for cPAGE's",        ".%d bytes",    "",                " %d rows * %d(=m_rowsize)",       rows * g_memRowSize, rows, g_memRowSize);

  Heading("cBOOK detail");
  Col("| sizeof(cBOOK)",                   ".%d bytes",    " m_cSize",        "",                                cBOOK_size);
  Col("| cBOOK's per row",                 ".%d",          " cBOOK_perRow",   " m_rowBytes/cBOOK_size",           cBOOK_perRow);
  Col("| #cBOOK[]'s (worst case)",         ".%d rows",     " m_bMax",         " WorstCase(m_pMax, m_sPerRow)",   g_sP->m_bMax); // <<<<------ ????
  rows = g_sP->m_bMax;
  Col("| Actual space for cBOOK's",        ".%d bytes",    "",                " %d rows * %d(=m_rowsize)",       rows * g_memRowSize, rows, g_memRowSize);

  if (afterB)
     {Col("+--- Post sort statistics ");
      Col("| cBOOKs used",                 ".%d",          "allocd=%d",       " loading=%d%%",                   bUsed,    bMax       * cBOOK_perRow, BookLoading());
      Col("| cPAGEs used",                 ".%d",          "allocd=%d",       " loading=%d%%",                   pUsed,    m_maxPages * cPAGE_perRow, PageLoading());
      Col("| cINDXs used",                 ".%d",          "allocd=%d",       " loading=%d%%",                   rcdCount, m_maxPages * cINDX_perRow, IndexLoading());
      if (g_sP->m_earlyTries != 0)
          Col("| Early Cache efficiency",  ".%d%%",        "",                " m_earlyHits=%d, m_earlyTries=%d",g_sP->m_earlyHits*100/g_sP->m_earlyTries, g_sP->m_earlyHits, g_sP->m_earlyTries);
    }
  Col("+-");
  #undef Heading
 } //cHelper::ShowProperties...

//Display Underlying Data Records in sorted order.
void cHelper::PrintSorted(bool vblRcdB)
   {sRECORD  *rP;                                                               //
    uint32_t  sortNum;                                                          //
    for (sortNum=0; sortNum < g_sP->m_rcdCount; sortNum++)                      //
        {rP = (sRECORD*)g_sP->LocByNum(sortNum);                                //location of next UDR
         if ((g_sP->m_options & OPTION_LSB) != 0)                               //
           {Printf("%d: %c%c%c_", sortNum, rP->key[4], rP->key[5], rP->key[6]); //
            for (int jj=4; --jj >= 0;) Printf("%02X", rP->key[jj]);             //hex of key
           }                                                                    //
         else                                                                   //
            Printf("%d: %s", sortNum, rP->key);                                 //
         if (vblRcdB) Printf(", rcdSize=%d", rP->rcdLen);                       //
         Printf(": %s\n", rP->txt);                                             //display record
        }                                                                       //
   } //cHelper::PrintSorted...

uint32_t cHelper::GetPagesUsed(void)
   {uint32_t buk, used=0;                                                       //
    for (buk=0; buk < g_sP->m_bUsed; buk++) used += MBOOK[buk]->count;          //
    return used;                                                                //
   } //cHelper::GetPagesUsed...

bool cHelper::CheckLocByRcdNum(SAM_CONTROLS *ctlP)
   {uint32_t rcdCount=ctlP->rcdCount, sortNum;                                  //
    sRECORD *rP;                                                                //
    if (ctlP->verboseB) Printf("Read back in sorted order (LocByRcdNum).\n");   //
    for (sortNum=0; sortNum < rcdCount; sortNum++)                              //
       if ((rP=(sRECORD*)g_sP->LocByNum(sortNum)) == NULL)                      //locate by rcdNum
          {Printf("**** LocByRcdNum. Error at record number=%d ****\n",sortNum);//
           return false;                                                        //
          }                                                                     //
    return true;                                                                //
   } //cHelper::CheckLocByRcdNum...

//Read key on each original record and access the index indexBase with this key.
//Possible duplicate keys pose a particular problem:
//LocByKey(key, instance=0) will locate the first instance of the key.
//This may not be the udr[rcdNum]. Accordingly instance is stepped thru all
//the duplicates to see if there is one with the proper rcdNum.
bool cHelper::CheckLocByKey(SAM_CONTROLS *ctlP, uint32_t rcdSize)
   {uint32_t rcdNum;                                                            //rcdNum = original record number
    int      instance;                                                          //must be 'int' not 'uint32_t'
    sRECORD *r0P, *r1P;                                                         //
    if (false)                                                                  //
       g_bugP->Bug("CheckByLocalKey");                                          //
    if (ctlP->verboseB) Printf("Read back using keyed access (LocByKey).\n");   //
    for (rcdNum=0; rcdNum < ctlP->rcdCount; rcdNum++)                           //
        {g_sP->m_rcdNum = rcdNum;                                               //debugging: for BugIndexes()
         r1P            = ADR_ARITHMETIC(sRECORD, g_dataP, rcdSize, rcdNum);    //original record
         for (instance=0; instance >= 0;)                                       //step thru all duplicate (if any)
             {r0P       = (sRECORD*)g_sP->LocByKey(r1P->key, &instance);        //locate by key,updates instance
              if (r0P == NULL) break;                                           //break to error
              if (r0P == r1P) goto eLup;                                        //keep stepping until rcd matches
             }                                                                  //
         //Did not find a match for key and rcdNum. That's bad news.            //
         g_bugP->BugKey("**** LocByKey. Error at key=", r1P->key);              //
         Printf("; record.txt=%s ****\n",  r1P->txt);                           //
         return false;                                                          //
   eLup: continue;                                                              //
        }                                                                       //
     return true;                                                               //
    } //cHelper::CheckLocByKey...

//Read all file into memory at *bufPP; return size or zero if unable to open file
int cHelper::ReadAllFile(const char *fileNameP, char **bufPP)
   {size_t sz;                                                          //
    char  *bufP;                                                        //
    FILE  *fileP=fopen(fileNameP, "rb");                                //
    if (fileP == NULL) {*bufPP = NULL; return 0;}                       //
    fseek(fileP, 0, SEEK_END);                                          //
    sz   = ftell(fileP);                                                //
    bufP = (char*)malloc(sz+2);                                         //room for \nand 0x00
    fseek(fileP, 0, SEEK_SET);                                          //
    fread(bufP, sz,1, fileP);                                           //read entire file into memory
    bufP[sz] = 0;                                                       //
    fclose(fileP);                                                      //
    *bufPP = bufP;                                                      //
    return (int)sz;                                                     //
   } //cHelper::ReadAllFile...

//end of file...
