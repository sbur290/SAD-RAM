#ifndef ATOMIZE_H_INCLUDED
#define ATOMIZE_H_INCLUDED
#include <stdarg.h>
#include <malloc.h>
#include <assert.h>
#include <C3_preProcessor.h>
//                 0         1         2        3        4          5          6          7             8                9            10       11        12
typedef enum {GC_NULL=0, GC_PAREN, GC_ERROR, GC_GETX, GC_NAME, GC_INT, GC_DOUBLE, GC_STRING, GC_RESERVE_WORD, GC_RESERVED_FNC, GC_ARRAY, GC_CHAR, GC_COMMENT} GC_TYPE;

#define CC const char*
class cAtomize
   {public:
    char              *m_pp, *m_sourceDataP;
    bool               m_allowDollarVblsB, m_allowPercentVblsB, m_allowVerilogFormatB;
    IATOM              m_backupAtom, m_appendAtom, m_lastAtom;
    int                m_lineInx, m_lines;
    static const char *m_operators[];
    cPreProcessor     *m_prepP;
    sLINE_MAP         *m_lineMapP;
    cAtomize                   (CC fileNameP, CC includeDirP=NULL, CC captureFileP=NULL);
   ~cAtomize                   ();
    IATOM  AtomErr             (int erC, char *pp) {IATOM vbl; vbl.type = GC_ERROR; vbl.textP = pp; vbl.len = -erC; return vbl;}
    void   Backup              (IATOM atom)        {m_backupAtom = atom;}
    void   ComputeSourceffsets (sSRC_REF *refP, const char *hereP);                
    IATOM  GetAtom             (bool probeB=false);
    int    GetLineNumber       (int lineInx) {return m_prepP->GetLineNumber(lineInx);}
    char  *GetFileName         (int fileNum)       {return m_prepP->GetFileName(fileNum);}
    char  *GetSource           (int fileNum, int lineNum);
    bool   Isdigit             (char ch)           {return ch >= '0' && ch <= '9';}
    int    LookupName          (const char *, int) {return -1;}
    int    LookupReserveWord   (const char *, int) {return -1;}
    int    PrintProgram        (void) {return m_lines < 0 ? m_lines : m_prepP->PrintProgram();}
    int    RemoveUnderScore    (char *pp, int len);
    int    ReservedFunctionName(const char *, int) {return -1;}
    void   Rewind              (void);
   }; //cAtomize...

#endif //ATOMIZE_H_INCLUDED...