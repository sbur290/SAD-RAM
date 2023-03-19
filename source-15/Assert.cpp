//File Assert.cpp: Gussied up assert() function
#include <stdarg.h>
#include "..\sam-13\samBug.h"
#include <hStructures.h>
#include <windows.h>

extern class cBugIndex *g_bugP;
extern class cSadram   *g_sP;

class cAssert : public cSadram
   {public: void a(const char *msgP, const char *noteP, int line, const char *fileNameP);};        //to get access to cSadram internals

void cAssert::a(const char *msgP, const char *noteP, int line, const char *fileNameP)
   {char buf[700]={0}, hdr[64]={0};                                             //
    int  ii, len, response=MB_YESNOCANCEL;                                      //
    snprintf(hdr, sizeof(hdr)-1, "Assert(%s)\n", msgP);                         //
    len = (int)strlen(buf);                                                     //
    if (noteP) snprintf(&buf[len], sizeof(buf)-len-1,"\t%s\n\n", noteP);        // 
    len = (int)strlen(buf);                                                     //
    if (g_sP) snprintf(&buf[len], sizeof(buf)-len,                              //
         "Program location:\n %s, line %d\n\n"                                  //
         "Operational parameters:\n"                                            //
         " rcdNum=%d\trcdCount=%d\trowSize=%d\n\n"                              //m_rcdNum, m_rcdCount, m_rowBytes
         "Internal parameters:\n"                                               //
         " cINDX_size =%2d,\t cPAGE_size =%2d,\t cBOOK size =%2d\n"             //
         " hINDX_padSz=%2d,\t hPAGE_padSz=%2d,\t hBOOK padSz=%2d\n"             //
         " hINDX_align=%2d,\t hPAGE_align=%2d,\t hBOOK align=%2d\n"             //
         " m_bMax     =%2d,\t m_sUsed    =%2d,\t m_sMax     =%2d,\n\n",         //m_xMax, m_bUsed, m_bMax
       fileNameP, line,                                                         //
       m_rcdNum,       m_rcdCount,      m_rowBytes,                              //
       cINDX_size,     cPAGE_size,      cBOOK_size,                             //
       hINDX_padSz,    hPAGE_padSz,     hBOOK_padSz,                            //
       hINDX_align,    hPAGE_align,     hBOOK_align,                            //
       m_bMax,         m_bUsed,         m_bMax);                                //
    len = (int)strlen(buf);                                                     //
    if (g_bugP && (g_bugP->m_fail.buk != 0 || g_bugP->m_fail.page != 0 || g_bugP->m_fail.inx != 0)) 
      snprintf(&buf[len], sizeof(buf)-len-1,                                    //
               "Failure location:\n"                                            //
               "    m_failBuk\t= %d,\tm_failPage= %d,\tm_failInx\t= %d\n\n",    //m_failBuk, m_failPage, m_failInx
             g_bugP->m_fail.buk, g_bugP->m_fail.page, g_bugP->m_fail.inx);      //
    len = (int)strlen(buf);                                                     //
    if (g_sP && g_bugP)                                                         //
       snprintf(&buf[len], sizeof(buf)-len-1,                                   //
               "Responses:\n"                                                   //
               "  Yes    \t= Write detail dump file to file samsort.dump\n"     //
               "  No     \t= Write summary dump file to file samsort.dump\n"    //
               "  Cancel \t= Terminate without creating a dump file");          //
    else {snprintf(&buf[len], sizeof(buf)-len-1, "Terminate"); response=MB_OK;} //
    ii = MessageBoxA(NULL, buf, hdr, response);                                 //
#ifdef _DEBUG                                                                   //
    if (ii == IDYES) g_bugLevel = 2; else                                       //book, pages, and indexes
    if (ii == IDNO)  g_bugLevel = 1; else exit(1);                              //book and pages only
    if (g_printFileP != NULL) fclose(g_printFileP);                             //can current tracking efforts
    if (!(g_printFileP = fopen("samsort.dump", "wb")))                          //create new file
       {MessageBoxA(NULL, "Error creating samsort.dump", "bad bongoes",MB_OK);  //fuck !!
        exit(1);                                                                //
       }                                                                        //
    g_bugLevel = 3;                                                             //
 //   g_bugP->Bug();                                                              //
    //Add a little local zest to the bug output.                                //
    if (g_sP)                                                                   //
        Printf("\n%s %s, line %d\nrcdNum=%d, rcdCount=%d, rowSize=%d\n",        //
            hdr, fileNameP, line, m_rcdNum, m_rcdCount, m_rowBytes);             //
    fclose(g_printFileP); g_printFileP = NULL;                                  //
    printf("Index Database dumped to samsort.dump\n");                          //
#endif                                                                          //
    exit(line);                                                                 //
   } //cAssert::a...

//Entry point
void _Assert(const char *msgP, const char *noteP, int line, const char *fileNameP)
   {cAssert *sP=(cAssert*)g_sP; sP->a(msgP, noteP, line, fileNameP);}

//end of file...

