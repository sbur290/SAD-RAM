#ifndef _SAM_BUG_H_INCLUDED
#define _SAM_BUG_H_INCLUDED

#include <stdarg.h>
#include <malloc.h>
#include "sam-15.h"
#include "samApp.h"

#define DFAULT 0xFFFFFFFF

class cBugIndex
  {public:
   int                    m_keySize;                                            //
   uint32_t               m_signature;                                          //
   LOCATOR                m_fail;                                               //failures noted by BugIndex()
   uint32_t               m_fails;                                              //
   uint32_t               m_rowBytes;                                           //
   sRECORD                m_record;                                             //
   const char            *m_userDataFileP;                                      //
   bool                   m_alphaKeyB;                                          //
   cBugIndex             (){}                                                   //
   cBugIndex             (cSadram*ssP, int keySize);                            //
   bool BugKey           (const char *titleP, const uint8_t *keyP);             //
   bool BugKey           (char *bufP, int bufSize, const char *titleP, const uint8_t *keyP);
   void BugData          (const char *titleP, uint32_t rcdNum, CV rcdV, bool absB);//
   bool BugRaw           (const char *titleP);                                  //
   void BugToFile        (const char *fileNameP, int bugLevel);                 //
   void BugB4Insert      (uint32_t rcdNum, void *rcdP);                         //
   bool CheckSequence    (SAM_CONTROLS *ctlP);                                  //
   bool CheckIndexbase   (bool ccB);                                            //
   bool CheckPresent     (SAM_CONTROLS *ctlP, uint32_t duplRcd);                //
   bool CheckRecords     (SAM_CONTROLS *ctlP);                                  //
   void CheckMemory      (SAM_CONTROLS *ctlP);                                  //
   int  CompareKey       (const void *aV, const void *bV, int keySize=0);       //
   uint8_t *HiKey        (uint32_t buk, uint32_t page);                         //
   static void ShowAdr   (void *adrP);                                          //
   static bool _PrintError(const char *fncP, int line, const char *causeP, uint32_t buk, uint32_t page, uint32_t inx);
   bool Bug              (CC titleP=NULL, bool doubleBookB=false,               //Display interpretted dump of indexbase. return true if happy.
                          void (*showAdrP)(void *)=&ShowAdr,                    //routine to display an address
                          const char *userDataFileP=NULL);                      //underlying data fileName (used by sim.exe)
   //Virtual addressing function replaced by derived classes such as cBugSim    //
   virtual cBOOK   *Badr     (uint32_t buk);                                    //
   virtual cPAGE   *Padr     (cPAGE *pageP, uint32_t page);                     //
   virtual cPAGE   *BPadr    (uint32_t buk, uint32_t page);                     //
   virtual cINDX   *BPIadr   (uint32_t buk, uint32_t page, uint32_t inx);       //
   //Virtual field access function replaced by derived classes such as cBugSim  //
   virtual uint16_t BookCount(cBOOK *bookP) {return bookP->count;}              //
   virtual uint64_t BookTotal(cBOOK *bookP) {return bookP->total;}              //
   virtual cPAGE   *BookPageL(cBOOK *bookP) {return bookP->page1P;}             //
   virtual cPAGE   *BookPageH(cBOOK *bookP) {return bookP->page2P;}             //
   virtual uint8_t *BookLoKey(cBOOK *bookP) {return bookP->loKey;}              //
   virtual uint32_t PageCount(cPAGE *pageP) {return pageP->count;}              //
   virtual uint64_t PageTotal(cPAGE *pageP) {return pageP->total;}              //
   virtual cINDX   *PageLoP  (cPAGE *pageP) {return pageP->x1P;}                //
   virtual cINDX   *PageHiP  (cPAGE *pageP) {return pageP->x2P;}                //
   virtual uint8_t *PageLoKey(cPAGE *pageP) {return pageP->loKey;}              //
   virtual uint8_t *IndxKey  (cINDX *inxP)  {return inxP->key;}                 //
   virtual uint64_t IndxRelAdr(cINDX *inxP) {return UDR_OFFSET(inxP->dataP);}   //
  }; //cBugIndex...

//end of file...
#endif //_SAM_BUG_H_INCLUDED...
