/*===========================================================================
        File: pico_preProcessor.h. Version 7.0.0.2. Mar 11th, 2014.
        Refer to pico_preProcessor.cpp for comments.
===========================================================================*/

#ifndef PICO_PREPROCESSOR_H_INCLUDED
#define PICO_PREPROCESSOR_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <C3_timeFncs.h>
#include <C3_errors.h>

typedef struct {const char *textP; int len, type, index; sSRC_REF ref;} IATOM;

//Following routines used to track memory allocation
//NOTE: without overwriting the new operator this does not track new/delete
#if defined(_DEBUG) && defined(_WIN32)
  #define Free(vP)                                {_Free   (#vP, __FUNCTION__, __LINE__, vP); vP = NULL;}
  #define Calloc(vP, size, esize){*((void**)&vP) = _Calloc (#vP, __FUNCTION__, __LINE__, size, esize);  }
  #define Realloc(vP, size)      {*((void**)&vP) = _Realloc(#vP, __FUNCTION__, __LINE__, vP, size);     }
  void *_Realloc(const char *vpNameP, const char *functionP, int line, void *vP, int size);
  void _Free    (const char *vpNameP, const char *functionP, int line, void *vP);
#else
//#define _CrtCheckMemory() {}
  #define Free(vP) {free(vP); vP = NULL;}
  #define Calloc(vP, size, esize)  {*((void**)&vP) = calloc(size, esize);}
  #define Realloc(vP, size)        {*((void**)&vP) = realloc(vP, size);}
#endif //_DEBUG...

//Container class for a list of structures (called elements). 
//This class manages pointers to arrays of elements; each element is another struct/class.
//All accesses to an element of a list are through P(index) or AddP(index). A pointer so obtained
//will not change. The pointer array underlying the P() function is reallocated as required.
//Used in preference to std:vector because:
//   std:vector reallocates its members willy-nilly
//   std:vector does not check for bounds overflow using operator[] !!!
//NOTE: Can be used with a list of classes, but take care that the hidden members of the 
//      class do not get cloberred by the memset.
class cListOfStruct
     {public:
     ~cListOfStruct();
    //protected:
      int      m_count,                                  //count of elements in container
               m_countArray,                             //count of pointers in arrayPP[]
               m_elementSize,                            //size of elements in container
               m_allocInc,                               //increment amount for arrayPP[] count
               m_saveCount, m_saveCountArray, m_saveInc; //save locations for Save/Restore
      void   **m_saveArrayPP;                            //save locations for Save/Restore
      void   **m_arrayPP;                                //pointer to array of allocated spaces
      public:
      void     Initialize(int size);                     //clear counts, and save baseP
      void     SetAllocationInc(int allocInc);           //define the increment amount for reallocation
      int      Count  (void) {return m_count;}           //Count of elements in class
      int      Add    (int howmany=1);                   //Add element to container class
      void     Save   (void);                            //Save array & counters, and clear
      void     Restore(void);                            //Restore array & counters
      protected:                                         //
      void    *P(int ii);                                //return pointer to element[ii]
     }; //cListOfStruct...

typedef struct 
   {char *fileNameP; 
    char *textBlockP; 
    int   textSize;
   } INCLUDED_FILE;
class cINCLUDED_FILE : public cListOfStruct
   {public:
      cINCLUDED_FILE() {Initialize(sizeof(INCLUDED_FILE));}
     ~cINCLUDED_FILE();
       INCLUDED_FILE *P(int ii)  {return (INCLUDED_FILE*)cListOfStruct::P(ii);} //pointer to element
       INCLUDED_FILE *AddP(void) {return P(Add());}                             //return pointer to new element
   }; //cINCLUDED_FILE...

typedef struct 
   {char *nameP; int nameLen, countOccurrences;
   } DEFINE_PARAM; //parameters to a #define
class cDEFINE_PARAM : public cListOfStruct                                      //create a list of cDEFINE_PARAM
   {public:
      cDEFINE_PARAM() {Initialize(sizeof(DEFINE_PARAM));}
     ~cDEFINE_PARAM();
       DEFINE_PARAM *P(int ii)  {return (DEFINE_PARAM*)cListOfStruct::P(ii);}   //pointer to element
       DEFINE_PARAM *AddP(void) {return P(Add());}                              //return pointer to new element
   }; //cINCLUDED_FILE...

typedef enum {MACTYPE_INT=0x8000, MACTYPE_STR,    MACTYPE_VBL, MACTYPE_OP, MACTYPE_DOLLAR,
              MACTYPE_LPAREN,     MACTYPE_RPAREN, MACTYPE_NOT, MACTYPE_TRUE, MACTYPE_FALSE,
              MACTYPE_DEFINED,    MACTYPE_ERROR,
             } MACATOM_TYPE;
#define MACATOM_TYPE_NAMES    "INT", "STR", "VBL", "OP", "$", "(", ")", "NOT", "TRUE", "FALSE", "DEFINED", "ERROR",
typedef struct 
   {char         *valuP; 
    MACATOM_TYPE  type;
   } MACATOM;
class cMACATOM: public cListOfStruct
   {public:
      cMACATOM() {Initialize(sizeof(MACATOM));}
     ~cMACATOM();
       MACATOM *P(int ii)  {return (MACATOM*)cListOfStruct::P(ii);} //pointer to element
       MACATOM *AddP(void) {return P(Add());}                       //return pointer to new element
   }; //cMACATOM...

typedef struct 
   {const char *nameP, *valuP;
   } sNAMEVALUE_LIST;   //generic name/value pair

typedef struct 
   {int   offset; uint16_t paramInx, type;
   } REPLACE_LIST;     //locations in body of #define to replace text
enum {RL_REPLACE, RL_STRINGIFY, RL_CHARIFY, RL_PASTE, RL_VARARG, RL_FREE_PASTE, RL_COUNT_ARGS};//type field of REPLACE_LIST
class cREPLACE_LIST: public cListOfStruct
   {public:
      cREPLACE_LIST() {Initialize(sizeof(REPLACE_LIST));}
     ~cREPLACE_LIST();
       REPLACE_LIST *P(int ii)  {return (REPLACE_LIST*)cListOfStruct::P(ii);} //pointer to element
       REPLACE_LIST *AddP(void) {return P(Add());}     //return pointer to new element
   }; //cREPLACE_LIST...

//----------- bit settings of #pragma debug -----------
//Various options are used by different programs that invoke the preprocessor:
//      name                  bit       //<---- used by modules ---->     Meaning ---------------------------------------------
#define DEBUG_MAC              0x001    //CISH                          //debug
#define DEBUG_COMPILE          0x002    //     MAD                      //debug opcodes during Pass1
#define DEBUG_LOGFILE          0x004    //     MAD                      //debug
#define DEBUG_BY_OPCODE        0x008    //     MAD                      //debug during simulation/compilation: display each opcode
#define DEBUG_RUN              0x010    //CISH                          //debug invoke CISH run time debugger
#define DEBUG_BY_STE           0x020    //     MAD                      //debug during simulation: step by each STE evaluation
#define DEBUG_BY_LINE          0x040    //     MAD                      //debug during simulation: step line by line
#define DEBUG_DISPLAY_OPCODES  0x080    //     MAD                      //debug display opcodes during line/STE debugging

#define MAC_NAMES  "",  "#ifdef", "#ifndef", "#if", "#error", "#elif", "#else", "#endif", "#define", "#undef", "#include", "#pragma", "#warning", "#for", "#endfor", "#endef"
     //        0         1         2          3      4         5        6        7         8          9         10          11         12          13      14         15
typedef enum {_NONMAC, _IFDEF=1,  _IFNDEF,    _IF,    _ERROR,    _ELSEIF,  _ELSE,  _ENDIF,   _DEFINE,   _UNDEF,   _INCLUDE,   _PRAGMA,    _WARNING,    _FOR,   _ENDFOR,  _ENDEF} MAC_INX;

//structure used to separate a text block into individual lines.
typedef struct {char    *originalP,     //original line before macro expansion
                        *expandedP;     //line after macro expansion
                sSRC_REF ref;           //file/line/offset
                int      len,           //length of line excluding closing cr/lf
                         macLines,      //number of lines in this macro when macInx != _NONMAC
                         depth,         //depth of macro
                         debugMac;      //set if #debug is encountered on this line
                MAC_INX  macInx;        //index into above table when line begins with a #directive
                bool     erasedB,       //erased because preceding line has a continuation mark or is a define
                         definedUsedB,  //r/nuB,
                         reallocatedB,  //if expandedP has been reallocated, otherwise it is a pointer to file buf
                         commentB;      //line is entirely enclosed in /* comment
               } sLINE_MAP;
#define HOWMANY_HASH 63535              //hash table used to speed up lookup of defines <<< not 65535 eh ?
#define HASH_UNUSED  (HOWMANY_HASH+1)   //marks location as unused

class cLINE_MAP: public cListOfStruct
   {public:
      cLINE_MAP() {Initialize(sizeof(sLINE_MAP));}
     ~cLINE_MAP();
       sLINE_MAP  *P(int ii)  {return (sLINE_MAP*)cListOfStruct::P(ii);}//pointer to element
       sLINE_MAP  *AddP(void) {return P(Add());}                        //return pointer to new element
       cLINE_MAP *MergeLineMap(cLINE_MAP *incMapP, int at, int delCount);
   }; //cLINE_MAP...

typedef struct
    {char              *nameP;                   //name of define
     char              *bodyP;                   //body of define
     bool               isStringB,               //body is a numeric value
                        emptyParamsB,            //true = #define has [] or () but no actual parameters
                        activeB,                 //=false if #undefined
                        variableB,               //variable parameter list, eg, #define foo(a, b...) body of foo
                        blockMacroB,             //#define foo()= .... #endef
                        nuB[3];                  //pad to div 4 address
     int                extraSpace,              //count of ## in define body, needed for accurate allocation
                        nameLen,                 //
                        lineNum, fileNum,        //originating source 
                        lastOffset,              //used for infinite loop detection
                        depth,                   //depth of defines
                        hashLink;                //index of previous entry with this hash code
     cDEFINE_PARAM     *formalsP;                //list of formal parameter names
     cREPLACE_LIST     *replaceListcP;           //location and types of parameters in body
    } DEFINE;
class cDEFINE : public cListOfStruct
   {public:
      cDEFINE() {Initialize(sizeof(DEFINE));}
     ~cDEFINE();
       DEFINE *P(int ii)  {return (DEFINE*)cListOfStruct::P(ii);}                 //pointer to element
   }; //cDEFINE...

enum {SHOW_EXPANSION=1, SHOW_IGNORED_LINES=2, SHOW_CLEAN=4};
enum {XSEVERITE_WARNING=1, XSEVERITE_ERROR,  XSEVERITE_CRASH};  

#define CC const char*
class cPreProcessor
    {private:
     char                      *m_fileNameP;
     static uint16_t            m_uninteresting[65536];   //all char pairs except letters, _, quotes, $ and %
     const char                *m_includeDirectoryP, *m_captureFileP;
     int                        m_debugMac, m_replacementLength, m_errCode, m_warning, m_fileNum, m_line, m_macDepth;
     int                        m_hashTable[HOWMANY_HASH]; //speed up LookupDefine; HOWMANY_HASH = unused
     uint32_t                   m_options;
     static bool                m_allowDollarVblsB, m_allowPercentVblsB;
     void                      *m_parentP;
     uint64_t                   m_analyzeTime;
     static const char         *KeyWords[];

     //subordinate classes
     cINCLUDED_FILE            *m_includedFilesP;  //from #include
     cLINE_MAP                 *m_lineMapP;        //all lines in the program
     cDEFINE                   *m_definesP;        //all #defines

     public:
     cPreProcessor(bool allowDollarVblsB=false, bool allowPercentVblsB=false, CC captureFileP=NULL);
    ~cPreProcessor();
     cSamError                  m_err;
     int GenerateLibFile      (const char *fileNameP);
     uint32_t GetDebugSettings(void)             {return m_debugMac;}
     char *GetSource          (int fileNum, int lineNum);
     const char *GetDirectiveName(MAC_INX inx)   {return KeyWords[inx];}
     int   GetError           (char *messageP=NULL, int messageSize=0);
     char *GetFileName        (int fileNum)      {return fileNum < m_includedFilesP->Count() ? (m_includedFilesP->P(fileNum))->fileNameP : NULL;}
     int   GetLineNumber      (int lineInx)      {return m_lineMapP->P(lineInx)->ref.lineNum;}
     int   GetLineMap         (sLINE_MAP **mapPP){*mapPP = m_lineMapP->P(0); return m_lineMapP->Count();}
     uint32_t GetOptions      (void)             {return m_options;}
     bool  IsReallocated      (char *pp);
     int   Process            (const char *fileNameP, const char *m_includeDirectoryP, const sNAMEVALUE_LIST *presetsP=NULL, int countPresets=0, uint32_t options=0);
     int   ReadAllFile        (const char *fileNameP, char **dataPP);
     protected:
     int   ActualParameters   (char **bodyPP, cDEFINE_PARAM *actualP, int defInx);
     int   AddInclude         (const char *includeP);
     int   AnalyzeDirectives  (bool doReplaceB);
     int   BuildLineMap       (char *textP, cLINE_MAP *mapP, int depth);
     uint32_t ComputeDefineHash(const char *name, int len);
     int   CreateNewDefine    (const char *nameP, int len) ;
     char *ConcatenateFor     (bool allB, int targetDepth);
     int   For                (void);
     int   Pragma             (void);
     int   Precedence         (uint32_t op);
     int   Define             (void);
     int  _ExpandDefinesByLine(char **resultPP, char *textP, bool bugB=false);
     int   ExpandDefinesByLine(char **resultPP, char *textP, bool bugB=false);
     int   ExpandBlockDefine  (char **resultPP, char *textBlockP, char *startDelP, char *endDelP, cDEFINE_PARAM *actualP, int defInx);
     int   ExpandNormalDefine (char **resultPP, char *textP,      char *startDelP, char *endDelP, cDEFINE_PARAM *actualP, int defInx);
     int   ExpandFromList     (char **resultPP, char *textBlockP, char *startDelP, char *endDelP, char *bodyP, cDEFINE_PARAM *formalsP, cDEFINE_PARAM *actualP, cREPLACE_LIST *replaceListcP, int extraSpace);
     int   Expression         (cMACATOM *atomListP, int calledFrom, int start, int count, int64_t *resultP);
     int   Evaluate           (int64_t *resultP, int op, int64_t left, int64_t rite);
     int   EvaluateString     (int op, char *left, char *rite);
     static void ReassignLine (sLINE_MAP *lineP, char *textP=NULL);

     private:
     int   If                 (MAC_INX calledFrom);
     int   Include            (void);
     int   LocateEndif        (int *elifLineP, int *elseLineP, int *endLineP);
     int   LocateReplacements (const char *nameP, cREPLACE_LIST **replaceListPP, char *bodyP, cDEFINE_PARAM *paramsP);
     int   LogError           (int erC, const char*, const char*);
     int   LookupDefine       (const char *nameP, int len, bool allowInactiveB=false);
     char *MatchChar          (char *strP);
     int   OneDirective       (void);
     int   PrepExpression     (char *atmP, cMACATOM *atomListP);
     int   ReadLibFile        (const char *fileNameP); 
     int   RemoveMultiLineComments(char *textP, int size);
     char *RemoveOneLineComment(char *textP, bool *reallocatedP);
     char *ScanString         (char *pp);
     int   SpanValidName      (const char *pp);
     int   Undef              (void);
     int   Uninteresting      (const char *pp);
     int   AlphaNumerics_     (const char *pp);
     void  Wipe               (int startLine, int endLine);
     //utility functions
     static char *Endof       (char *srcP);
     static bool  Isalpha     (char a) {return a >= 'a' && a <= 'z' || a >= 'A' && a <= 'Z';}
     static bool  Isalpha_    (char a) {return a >= 'a' && a <= 'z' || a >= 'A' && a <= 'Z' || a == '_' ||
                                              (a == '$' && m_allowDollarVblsB) || (a == '%' && m_allowPercentVblsB);}
     static bool  Isdigit     (char a) {return a >= '0' && a <= '9';}
     static bool  Isalnum     (char a) {return Isalpha(a) || Isdigit(a);}
     static bool  Isalnum_    (char a) {return Isalpha_(a)|| Isdigit(a);}
    public:
     static char *strdupl     (const char *srcP, int len=-1);
     static char *strLtrim    (char *pp);
     static char *strRtrim    (char *pp);
     inline int   Min         (int aa, int bb) {return aa > bb ? bb : aa;}
     inline int   Max         (int aa, int bb) {return aa > bb ? aa : bb;}
     int32_t      strtoul32   (const char *pp, char **ppp=NULL, int base=10);
     void        _BugLineMap  (cLINE_MAP *mapcP, int start=0, int end=-1, const char *fromFuncP=NULL, int fromLine=-1);
     void        _BugDefines  (int start, int end, const char *fromFuncP=NULL, int fromLine=-1);
     int          PrintProgram(void);
     #define cPREP_TEXT_NOT_FILENAME      256
     #define cPREP_FOR_ASSEMBLER           32
     friend class cLINE_MAP;
    }; //cPreProcessor...
#define BugLineMap(m,s,e)   _BugLineMap(m,s,e,   __FUNCTION__, __LINE__)
#define BugDefines(s,e)     _BugDefines(s,e,     __FUNCTION__, __LINE__)

#endif //# PICO_PREPROCESSOR_H_INCLUDED...

//end of file

