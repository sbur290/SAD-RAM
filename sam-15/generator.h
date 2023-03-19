#ifndef _GENERATOR_H
#define _GENERATOR_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sam-15.h"
#include "samApp.h"
#include <C3_atomize.h>

extern bool g_bugMemB;

class cGenerator
   {private:
    bool             m_allowDollarVblsB, m_allowPercentVblsB, m_autoB, m_repsB;
    class cAtomize  *m_atomizerP;
    IATOM            m_duplAtom;
    unsigned long    m_rcdNum;
    bool  Isdigit          (char ch) {return ch >= '0' && ch <= '9';}
    public:
    cGenerator             (SAM_CONTROLS *);
    ~cGenerator            ();
    IATOM GetAtom          (bool probeB) {return m_atomizerP->GetAtom(probeB);} //=false (ie step posn)
    void  PrintAtom        (const char *titleP, IATOM atom);
    bool  GenerateData     (SAM_CONTROLS *ctlP, uint32_t rcdNum, uint32_t rcdSize, sRECORD *rP, uint32_t keySize);
    private:
    bool  Command          (IATOM atom, bool theRealMcoyB);
    bool  MoveString       (char *keyP, int keySize, const char *srcP, int srcLen);
    int    m_start, m_end, m_step, m_count, m_reps;
   }; //cGenerator...

#endif //_GENERATOR_H...

