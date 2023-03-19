#ifndef _SAM_HELPER_H_INCLUDED
#define _SAM_HELPER_H_INCLUDED

#include <stdarg.h>
#include <malloc.h>
#include <assert.h>
#include "sam-15.h"
#include "samApp.h"
#include "samBug.h"

#define DFAULT 0xFFFFFFFF
extern class cSadram   *g_sP;

class cHelper
  {public:
   uint32_t              m_xUsed, m_maxPages;                                   //
   cHelper               (cSadram *ssP);                                        //
   bool CheckLocByKey    (SAM_CONTROLS *ctlP, uint32_t rcdSize);                //
   bool CheckLocByRcdNum (SAM_CONTROLS *ctlP);                                  //
   bool DatabaseStats    (void);                                                //
   bool GenVerilogFiles  (SAM_CONTROLS *ctlP);                                  //
   uint32_t GetPagesUsed (void);                                                //
   uint32_t GetBooksUsed (void) {return g_sP->m_bUsed;      }                   //
   uint32_t GetEarlyHits (void) {return g_sP->m_earlyHits;  }                   //
   uint32_t GetEarlyTries(void) {return  g_sP->m_earlyTries;}                   //early cache statistics
   int  GetVerilogParam  (const char *fileNameP, const char *paramNameP);       //
   char*HexOfKey         (const uint8_t *keyP);                                 //
   int  IndexLoading     (void);                                                //
   int  PageLoading      (void);                                                //
   int  BookLoading      (void);                                                //
   void PrintSorted      (bool vblRcdB);                                        //
   void ShowProperties   (SAM_CONTROLS *ctlP, bool afterB);                     //Display sort parameters
   static int ReadAllFile(const char *fileNameP, char **bufPP);                 //
   private:                                                                     //
   int  CompareKey       (CV aV, CV bV);                                        //
   void Col              (CC f1P, CC f2P="", CC f3P="", CC f4P="",              //align col for ShowProperties
                          int p1=0, int p2=0, int p3=0, int p4=0);              //
   friend class cSadram;                                                        //
   friend class cBugIndex;                                                      //
  }; //cHelper ...

//end of file...
#endif //_SAM_HELPER_H_INCLUDED...
