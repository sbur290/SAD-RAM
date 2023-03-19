/*file generate.cpp. July 27th, 2022.
 Generate input records from a text file.
 The input file comprises a list of numeric or string literals separated by commas.
 This input file only provides the keys; the function GenerateData inserts the
 body of sRECORD comprising 'Original Record ###' and record number.
 The keys must be consistent with the specifications provided to cSadram,
 in particular keySize; no checking is performed.
*/
#define _CRT_SECURE_NO_WARNINGS 1
#include <assert.h>
#include "generator.h"
#include "samHelper.h"

extern char       *g_errMsgP;                                           //
extern cHelper    *g_helpP;                                             //
extern cBugIndex  *g_bugP;                                              //
extern sRECORD    *g_dataP;                                             //
extern uint32_t    g_memRowSize,                                        //size of row in memory (bytes)
                   g_bugLevel;                                          //
                                                                        //
cGenerator::cGenerator(SAM_CONTROLS *ctlP)
   {IATOM         atom;                                                 //
    m_autoB = m_repsB = false;                                          //
    if (ctlP->generator != 0) return;                                   // == 0 means: sam /k fileName
                                                                        //
    //Read thru the input file and verify the input numbers/strings.    //
    //Set ctlP->rcdCount.                                               //
    m_atomizerP = new cAtomize(ctlP->genFile);                          //open, expands defines, and clean the source of comments
    //Calculate number on integer values in the file                    //
    ctlP->rcdCount = 0;                                                 //in case of error
    for (m_count=0; (atom=GetAtom(false)).type != GC_NULL;)             //
        {//PrintAtom("generator: atom=", atom); Printf("\n");           //
         switch (atom.type)                                             //
           {case GC_INT:                                                //
            case GC_STRING:  m_count++;                       continue; //
            case GC_NAME:    if (!Command(atom,false)) return;continue; //return if error
            case GC_NULL:                                     break;    //end of file
            case GC_COMMENT:                                  continue; //comment - ignore
            case GC_GETX: if(atom.textP[0] == ',')            continue; //comma = syntax decoration - ignore
                          if(atom.textP[0] == '/')            continue; //don't ask where this comes from
                 sprintf(g_errMsgP, "Unexpected data '%*s' in input key file\n",    //whatzat ?
                                                 atom.len, atom.textP); //
                 return;                                                //
        }  }                                                            //
                                                                        //
    m_atomizerP->Rewind();                                              //rewind data stream
    ctlP->rcdCount = m_count;                                           //
   } //cGenerator::cGenerator...

cGenerator::~cGenerator()
   {delete m_atomizerP; m_atomizerP = NULL;}

void cGenerator::PrintAtom(const char *titleP, IATOM atom)
   {static const char *gcTypes[] =
       {"null",   "paren", "error", "getx",        "name",  "integer", "double",
        "string", "reserveWord",    "reservedFnc", "array", "char",    "comment"};
    Printf("%stype=%d(%s), '", titleP, atom.type, gcTypes[atom.type]);
    for (int ii=0; ii < atom.len; ii++) Printf("%c", atom.textP[ii]);
    Printf("'");
   } //cGenerator::PrintAtom...

//Generate key for record at *rP
bool cGenerator::GenerateData(SAM_CONTROLS *ctlP, uint32_t rcdNum, uint32_t rcdLen, sRECORD *rP, uint32_t keySize)
   {uint32_t     u32=0, len;                                                    //
    IATOM        atom={"", 0, GC_NULL, 0};                                      //
    const char  *pp;                                                            //
                                                                                //
    rP->rcdLen = rcdLen;                                                        //
    rP->rcdNum = m_rcdNum = rcdNum;                                             //
    snprintf(rP->txt, sizeof(rP->txt), "Original Record %d", rcdNum);           //body of record
    switch (ctlP->generator)                                                    //
      {case 1: u32 = (((uint32_t)rand()) << 16) + rcdNum; break;                //generate random key
       case 2: u32 = rcdNum   + 1000;                     break;                //sequential key
       case 3: u32 = ctlP->rcdCount - rcdNum + 1000;      break;                //reverse key order
       case 0: while (true)                                                     //
                  {if (m_autoB)                                                 //$generate(start, end, step)
                      {u32      = m_start;                                      //
                       m_start += m_step;                                       //
                       if (m_step > 0) m_autoB = m_start <= m_end;              //
                       else            m_autoB = m_start >= m_end;              //
                       goto store;                                              //
                      }                                                         //
                   if (m_repsB && m_reps-- > 0) atom = m_duplAtom;              //$duplicate(reps, key)
                   else {m_repsB = false; atom = GetAtom(false);}               //end duplication
                   pp  = atom.textP; len = atom.len;                            //
                   switch (atom.type)                                           //
                     {case GC_COMMENT:                                continue; //ignore comments
                      case GC_NULL: return false;                               //end of file
                      case GC_NAME:                                             //
                           if (!Command(atom, true)) break;           continue; //break to error
                      case GC_INT:                                              //
                           if (pp[0] == '0' && pp[1] == 'x')                    //
                                u32 = strtol(pp+2, NULL, 16);                   //
                           else u32 = strtol(pp,   NULL, 10);                   //
                           goto store;                                          //
                      case GC_STRING:                                           //
                           return MoveString((char*)rP->key, keySize, pp,len); //
                      case GC_GETX:                                             //
                           if (len == 1 && *pp == ',') continue;                //ignore ','
                      default: break;                                           //
                     } //switch(atom.type...                                    //
                   Printf("**** Invalid token '%*s'\n", len, pp); return false; //
                  } //while (true)                                              //
       default: assert(false); return false;                                    //
      }                                                                         //
store:                                                                          //
    #ifdef FULLY_ALPHABETIC_KEYS                                                //
      snprintf((char*)rP->key, keySize, "%0*X", (int)(keySize), u32);           //
      Assert(keySize <= 8, (const char*)rP->key);                               //
    #else                                                                       //
      bufP += keyOffset;                                                        //
      bufP[4] = 'Y'; bufP[5] = 'E'; bufP[6] = 'K';                              //
      *(unsigned long*)bufP = u32;                                              //
    #endif                                                                      //
    return true;                                                                //
   } //cGenerator::GenerateData...

//move string to bufP.key and interpret \x...
bool cGenerator::MoveString(char *keyP, int keySize, const char *srcP, int srcLen)
   {int         ii, jj;
    char        buf[3] = {0};                                                   //
    memset(keyP, 0, keySize);                                                   //clear field
    for (ii=jj=0; ii < srcLen; ii++)                                            //move one char
        if ((keyP[jj++] = *srcP++) == '\\' && keyP[jj] == 'x')                  //escape
           {memmove(buf, ++srcP, 2);                                            //step over \x
            keyP[jj-1] = (char) strtol(buf, NULL, 16);                          //store hex value
            srcP++; ii += 3;                                                    //skip over \x..
           }                                                                    //
     return true;                                                               //
   } //cGenerator::MoveString...

#define IS_WORD(word) atom.len == (int)strlen(word) && _strnicmp(atom.textP, word, atom.len) == 0
#define IS_CHAR(ch)  if ((atom=GetAtom(false)).type != GC_GETX || atom.textP[0] != ch) goto syntaxErr;
#define GET_INTEGER(dst) if ((atom=GetAtom(false)).type != GC_INT) goto syntaxErr;\
                         dst = strtol(atom.textP, NULL, 10)
//theRealMcoyB = true  called from write loop in cSadram
//theRealMcoyB = false call from InitGen() to get rcdCount.
bool cGenerator::Command(IATOM atom, bool theRealMcoyB)
   {uint32_t bugLevel=g_bugLevel;                                               //
    bool     bb, signB;                                                         //
    int      ii;                                                                //
    const char *msgP="**** Invalid token:";                                     //
    if (bb=IS_WORD("$rowSize"))                                                 //
       {IS_CHAR('='); GET_INTEGER(g_memRowSize); IS_CHAR(','); return true;}    //happy ending
    if (bb=IS_WORD("$headers")) g_bugLevel = 2; else                            //for BugIndexes()
    if (bb=IS_WORD("$dump"))    g_bugLevel = 3;                                 //for BugIndexes()
    if (bb)                                                                     //
       {if (theRealMcoyB)                                                       //called from write loop in samApp.cpp
           g_bugP->Bug("$headers request");                                     //
//?/      for (; *m_pp != '\n' && *m_pp != '\r'; m_pp++)                          //
//?/          {if (theRealMcoyB) Printf("%c", *m_pp);}                            //
        if (theRealMcoyB)                                                       //
           Printf("\n\n");                                                      //break point
        g_bugLevel = bugLevel;                                                  //
        return true;                                                            //
       }                                                                        //
    // $generate(start, end, step)
    if (IS_WORD("$generate"))                                                   //
       {IS_CHAR('('); GET_INTEGER(m_start);                                     //start value
        IS_CHAR(','); GET_INTEGER(m_end);                                       //end value
        IS_CHAR(',');                                                           //
        if (signB=(atom=GetAtom(false)).type != GC_GETX || atom.textP[0] != '-')//-ve step amount
                   m_atomizerP->Backup(atom);                                   //
        GET_INTEGER(m_step); if (!signB) m_step = - m_step;                     //step value
        IS_CHAR(')');                                                           //
        m_repsB = false;                                                        //
        if (m_autoB=theRealMcoyB) return true;                                  //let GenerateData step m_count
        //otherwise adjust the record count for the generated values            //
        m_count += iabs(m_end - m_start + 1) / iabs(m_step);                    //
        return true;                                                            //ie no error
       }                                                                        //
    // $duplicate(reps, key). key = integer or string                           //
    if (IS_WORD("$duplicate"))                                                  //eg. $duplicate(15, "smith")
       {m_start = 0; m_step = 1;                                                //
        IS_CHAR('('); GET_INTEGER(m_reps);                                      //replication #
        IS_CHAR(',');                                                           //
        m_duplAtom = GetAtom(false);                                            //field value to replicate
        m_autoB    = false;                                                     //
        IS_CHAR(')');                                                           //
        if (m_duplAtom.type != GC_INT && m_duplAtom.type != GC_STRING) goto syntaxErr;//
        if (m_repsB=theRealMcoyB) return true;                                  //let GenerateData step m_count
        //otherwise adjust the record count for the duplicated values           //
        m_count += m_reps;                                                      //
        return true;                                                            //ie no error
       }                                                                        //
    msgP = "**** Invalid input stream command:";                                //
syntaxErr:                                                                      //
    Printf("%s '", msgP);                                                       //
    for (ii=0; ii < atom.len; ii++) Printf("%c", atom.textP[ii]);               //
    Printf("'\n");                                                              //
    return false;                                                               //
   } // cGenerator::Command...

//end of file...
