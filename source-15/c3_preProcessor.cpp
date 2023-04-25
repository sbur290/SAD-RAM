/*===========================================================================
        File: C3_preProcessor.cpp. Version 13, Jan, 2023.

        This module performs pass 0 -the pre-processing step of any compiler.
        This involves namely evaluating all the # directives including expanding #defines.
        This file contains six subordinate classes:
        class cListOfStruct                         manages a list of structures
        class cINCLUDED_FILE: public cListOfStruct  list of #included wiht [0] = original source file
        class cDEFINE_PARAM : public cListOfStruct  
        class cMACATOM      : public cListOfStruct  
        class cREPLACE_LIST : public cListOfStruct  list or replacements in #define text
        class cLINE_MAP     : public cListOfStruct  manages list of pointers to individual lies
        class cSamError                             general error handling

        The #define subsystems supports:
          - #define, #undef                                                                     same as C
          - positional parameters, ie. #define mac(p1, p2)<body>, invoked with mac(exp1, exp2)  same as C
          - #if, #ifdef, #ifndef, #if defined(), #else, #elif, #endif,                          same as C
          - #for (vbl=exp1; exp2; exp3) <multi-line body> #endfor,
          - #define macName()=<multi-line body> #endef, ie, block defines which may then
                include other # directives (such as #for, but excluding #define).
          - parameter pasting(##), char-izing(#@), string-izing(#),                             same as C
          - named parameters, ie. #define mac(p1, p2) <body> macName(.p1=exp1, .p2=exp2)
          - variable parameter ie. #define mac(p1,p2,...), mac(exp1, exp2, exp3, exp4,...)      similar to C

        Robert Trout.
===========================================================================
problems:
1. Wrong line numbers in error reports in #for (for example).
2. #defines containing defined variables (see wxWidgets)
*/

#include <C3_preProcessor.h>
#include <C3_timeFncs.h>

FILE *g_debugFileP=NULL;

#define PRECEDENCE_LEVELS 12                //levels of operators as defined in C++

#ifdef _DEBUG
   #define DEBUG_PREPROCESSOR               //allows more specific debugging
   static bool g_bugPrepB        = false,   //set to do detailed debugging of pre-processor
               g_stopNowB        = false;   //used for ad-hoc debugging
#else
   #undef  DEBUG_PREPROCESSOR
#endif
extern FILE *g_printFileP;                  //hook to Printf

//Switches to monitor memory management. Refer to madMemoryChecker.cpp.
#if defined(_DEBUG) && 00              //change 00 to 01 to do memory verification
  bool      g_checkAllocationB= true;  //set to track memory allocation (see madMemoryChecker)
#else
  bool      g_checkAllocationB = false;  //set to track memory allocation
#endif

extern void Printf(char const *fmtP,...);

#if defined(_DEBUG) && defined(_WIN32)
#include <crtdbg.h>
  _CRTIMP int __cdecl _CrtCheckMemory(void);

  void *_Calloc(const char *vpNameP, const char *functionP, int line, int count, int esize)
     {void *vP=calloc(count, esize); static int callNum;
      #ifdef DEBUG_PREPROCESSOR
      if (g_checkAllocationB)
         {Printf("0x%p Calloc(%s). %u * %u bytes, %s.line=%04u, #%d\r\n", vP, vpNameP, count, esize, functionP, line, ++callNum);
          _CrtCheckMemory();
         }
      #endif //DEBUG_PREPROCESSOR...
      if (callNum == 36)
          callNum = callNum; //proper style
      return vP;
     } //_Calloc ...

  void *_Realloc(const char *vpNameP, const char *functionP, int line, void *vP, int size)
     {void *wP=vP; static int callNum;
      vP = realloc(wP, size);
      #ifdef DEBUG_PREPROCESSOR
      if (g_checkAllocationB)
         {if (vP != wP && wP != NULL)
             vP = vP;
          if (vP == wP)
             Printf("0x%p Realloc(%s) %u bytes, %s.line=%04u #%d\r\n", 
                     vP, vpNameP, size, functionP, line, ++callNum);
          else
             Printf("0x%p!=0x%p Realloc(%s) %u bytes, %s.line=%04u #%d\r\n", 
                     vP, wP, vpNameP, size, functionP, line, ++callNum);
          _CrtCheckMemory();
      if (callNum == 36)
          callNum = callNum; //proper style
         }
      #endif //DEBUG_PREPROCESSOR...
      return vP;
     } //_Realloc ...

  void _Free(const char *vpNameP, const char *functionP, int line, void *vP)
     {uint8_t *u8P=(uint8_t*)vP;
      #ifdef DEBUG_PREPROCESSOR
      if (vP != NULL && g_checkAllocationB)
         Printf("0x%p Free(%s), %s.line=%04u 0x%02X %02X %02X %02X %02X %02X %02X %02X\r\n", 
                  vP, vpNameP, functionP, line, u8P[0], u8P[1], u8P[2], u8P[3], u8P[4], u8P[5], u8P[6], u8P[7]);
      #endif //DEBUG_PREPROCESSOR...
      free(vP);
      if (g_checkAllocationB) _CrtCheckMemory();
     } //_Free...
#endif //_DEBUG...

static cPreProcessor *g_prepP;
bool   cPreProcessor::m_allowDollarVblsB, cPreProcessor::m_allowPercentVblsB;

cPreProcessor::cPreProcessor(bool allowDollarVblsB, bool allowPercentVblsB, CC captureFileP)
   {uint32_t u32; uint8_t u8;                                                   //
    #define INTERESTING(a) (u8 == 0 || Isalpha_((char)u8) ||                    \
                            u8 == '\"' || u8 == '\'' || u8 == '$' || u8 == '%') //
    m_allowDollarVblsB  = allowDollarVblsB;                                     //
    m_allowPercentVblsB = allowPercentVblsB;                                    //
    m_captureFileP      = captureFileP;                                         //
    if (g_prepP) //for IsReallocated() in subordinate deletes.                  //
       {printf("only one instance of cPreProcessor please\n"); exit(1);}        //
    g_prepP = this;                                                             //
    //Build a vector of all uninteresting character pairs (signalled by 1).     //
    //A pair is deemed uninteresting if neither member of the pair has an       // 
    //alphabetic, ", ', '$', or '%'.                                            //  
    //This enables a fast scan over	meaningless text during macro expansion.    //
    for (u32=0; u32 != 65536; u32++)                                            //
       {u8 = (uint8_t)((u32 >> 8) & 0xFF); if (INTERESTING(u8)) continue;       //
        u8 = (uint8_t)((u32 >> 0) & 0xFF); if (INTERESTING(u8)) continue;       //
        m_uninteresting[u32] = 1;                                               //
       }                                                                        //
    m_definesP       = new cDEFINE;                                             //
    m_lineMapP       = new cLINE_MAP;                                           //
    m_includedFilesP = new cINCLUDED_FILE;                                      //
    m_macDepth       = 4;                                                       //
    m_debugMac       = 0;                                                       //
    m_errCode        = 0;                                                       //
    m_warning        = 0;                                                       //
    #undef INTERESTING                                                          //
   } //cPreProcessor::cPreProcessor...

//Allocate space and read file into buffer aqt dataPP; return size
int cPreProcessor::ReadAllFile(const char *fileNameP, char **dataPP)
   {int      size, ii;
    char    *bufP;
    FILE    *fileP=fopen(fileNameP, "rb");

    if (!fileP)                       return LogError(ERR_0003, "", fileNameP);
    fseek(fileP, 0, SEEK_END); size = ftell(fileP);
    Calloc(bufP, size+4, 1);
    if (bufP == NULL) {fclose(fileP); return LogError(ERR_0005, "", fileNameP);}
    fseek(fileP, 0, SEEK_SET); ii = (int)fread(bufP, size,1, fileP);
    fclose(fileP);
    *dataPP = bufP;
    return size;
   } //cPreProcessor::ReadAllFile...

int32_t cPreProcessor::strtoul32(const char *pp, char **ppp, int base)
   {char *qq;
    pp += strspn(pp, " \t");
    if (!ppp) ppp = &qq;
    if (pp[0] == '0' && pp[1] == 'x') return strtoul(pp+2, ppp, 16);
    return strtoul(pp, ppp, base);
   } //cPreProcessor::strtoul32...

int cPreProcessor::Precedence(uint32_t op)
   {switch (op)
      {case '=': case '||':                     return 1;
       case '&&':                               return 2;
       case '|':                                return 3;
       case '^':                                return 4;
       case '&':                                return 5;
       case '~':                                return 5;
       case '=!':case '==':                     return 6;
       case '>': case '<': case '=>':case '=<': return 7;
       case '>>':case '<<':                     return 8;
       case '+': case '-':                      return 9;
       case '*': case '/': case '%': case '%%': return 10;
       case '**':                               return 11;
      }
    return 0;
   } //cPreProcessor::Precedence...

//read fileNameP and perform all #processing.
int cPreProcessor::Process(const char *fileNameP, const char *includeDirectoryP, const sNAMEVALUE_LIST *presetsP, int countPresets, uint32_t options)
   {int           ii, defInx, erC, textSize;
    char         *textBlockP, includeName[_MAX_PATH], *pp, *qq;
    const char   *textP;
    INCLUDED_FILE*incP;
    uint64_t      startTime;
    cTIMEVALUE    tv;

    if (!(m_includeDirectoryP=includeDirectoryP))
       {//use directory of source file as the include directory
        m_includeDirectoryP = includeName;
        strncpy(includeName, fileNameP, sizeof(includeName));
        includeName[HOWMANY(includeName)-1] = 0;
        for (pp=qq=includeName; pp=strpbrk(qq, "\\/");) qq=pp+1;
        *qq = 0;
       }
    m_options           = options;
    m_replacementLength = 0;
    m_fileNum           = -1;
    m_fileNameP         = NULL;
    m_line              = 0;
    for (ii=0; ii < HOWMANY(m_hashTable); ii++) m_hashTable[ii] = HASH_UNUSED; //mark all as unused

    for (ii=0; ii < countPresets; ii++)
        {defInx = CreateNewDefine(presetsP[ii].nameP, -1);
         m_definesP->P(defInx)->bodyP = (presetsP[ii].valuP) ? strdupl(presetsP[ii].valuP, -1) : NULL;
        }

    //open file and read data
    if (m_options & cPREP_TEXT_NOT_FILENAME)
       {//no file involved: just raw text in fileNameP; re-arrange the deck chairs
        textP = fileNameP; fileNameP = ""; //input is text only
        Calloc(textBlockP, (textSize = (int)strlen(textP))+4,1);
        strcpy(textBlockP, textP);
        erC = 0;
       }
    else
    if ((erC=textSize=ReadAllFile(fileNameP, &textBlockP))  < 0) return LogError(erC, "", fileNameP);
    if ((erC=RemoveMultiLineComments(textBlockP, textSize)) < 0) 
                                              {Free(textBlockP); return LogError(erC, "", m_fileNameP);}
    m_fileNum        = m_includedFilesP->Add();
    incP             = m_includedFilesP->P(m_fileNum);
    incP->fileNameP  = m_fileNameP = strdupl(fileNameP, -1);
    incP->textBlockP = textBlockP;
    incP->textSize   = textSize;

    if ((erC=BuildLineMap(textBlockP, m_lineMapP, 0)) < 0) return erC;
    tv.GetGmtv();
    startTime        = tv.m_time;
    m_errCode        = AnalyzeDirectives(true); //analyze #commands and do #define expansion
    tv.GetGmtv();
    m_analyzeTime    =  tv.m_time -startTime;
    return m_errCode < 0 ? m_errCode : m_lineMapP->Count();
   } //cPreProcessor::Process...

int cPreProcessor::RemoveMultiLineComments(char *textP, int size)
   {char *pp, *endP=textP+size;
    for (pp=textP; pp=strchr(pp, '/'); pp++)
        {if (pp[1] == '/') {if (!(pp=strpbrk(pp, "\r\n"))) break; continue;}
         if (pp[1] != '*') continue;
         for (;pp[0] != '*' || pp[1] != '/'; pp++)
            {if (pp >= endP) return LogError(ERR_1030, textP, ""); // /* found without */ in a comment,
             if (*pp != '\r' && *pp != '\n') *pp = ' ';
            }
         pp[0] = ' '; pp[1] = ' ';
        }
    return 0;
   } //cPreProcessor::RemoveMultiLineComments...

//Remove tail end of line beginning at the // mark
//Check for quoted strings enclosing the // marks
char *cPreProcessor::RemoveOneLineComment(char *textP, bool *reallocatedP)
   {char *pp;                                                           //
    *reallocatedP = false;                                              //
    if ((pp=strstr(textP, "//")) == NULL)           return textP;       //easy cases: nothing to do
    if (strchr(textP, '\"')      == NULL) {*pp = 0; return textP;}      //no quotes, just prune off end of line  
    //quotes and // on this line. proceed carefully.                    //
    for (pp=textP; pp=strpbrk(pp, "/\""); )///pp++)                     // 10/3/2023
        {if (*pp == '\"') pp = ScanString(pp); else                     //
         if (pp[0] == '/' && pp[1] == '/') {*pp = 0; break;}            //prune at //
         if (pp == NULL) {LogError(ERR_1003, textP, ""); return textP;} //missing close quotes
        }                                                               //
    return textP;                                                       //
   } //cPreProcessor::RemoveOneLineComment...

const char *cPreProcessor::KeyWords[] = {MAC_NAMES};

//Scan thru textP and build line map.
//Extract all # directives and calculate howmany lines between each on.
//Return number of elements in line map and new line map at *mapPP
int cPreProcessor::BuildLineMap(char *textP, cLINE_MAP *mapcP, int depth)
   {char     *pp, *qq, *rr;
    int       ii, jj, kk, erC=0, nextii, count, len, lastIf=-1, lastFor=-1, lastDef=-1, saveLineNum=m_line;
    sLINE_MAP *mapP=NULL;

    //count lines in textP
    for (count=0, pp=qq=textP; *pp; count++)
        {if (!(pp = strpbrk(qq=pp, "\r\n"))) break;
         if (pp[0] == '\r' && pp[1] == '\n') pp += 2; else pp++;
        }
    if (qq[0]) count++;
    mapcP->SetAllocationInc(count+1);
    mapcP->Add(count+1);                                  //over allocated by one element
    //build line map                                      //
    for (ii=0, pp=textP; *pp; ii = nextii)                //
       {mapP                = mapcP->P(m_line=ii);        //save m_line for errors
        mapP->originalP     = (char*)pp;                  //
        mapP->expandedP     = rr = pp + strspn(pp, " \t");//remove leading spaces
        mapP->ref.lineNum   = ii+1;                       //line number starts at one
        mapP->ref.fileNum   = m_fileNum;                  //
        mapP->ref.lineOffset=(int)(pp - textP);           //offset within source file
        mapP->depth         = depth;                      //
        mapP->reallocatedB  = false;                      //
        if (*rr != '#') goto notDirective;                //
        //Scan # line and store mac_inx in line_map. Match up #if, #else, & #endif, #for & #endfor, #define & #endef.
        for (kk=1; kk < HOWMANY(KeyWords); kk++)
           if (strncmp(rr, KeyWords[kk], (len=(int)strlen(KeyWords[kk]))) == 0 && !Isalpha(rr[len]) && !Isdigit(rr[len]))
              {mapP->expandedP = rr = rr + len + strspn(rr+len, " \t");
               switch(mapP->macInx=(MAC_INX)kk)
                  {case _DEFINE:rr += SpanValidName(rr);
                                if ((*rr == '(' || *rr == '['))
                                   {if (!(rr = MatchChar(rr))) goto err1035;                            //missing close ] or close )
                                    if (*++rr == '=') {mapP->macLines = lastDef; lastDef= ii; depth++;} //block define: #define name(...)=
                                   }
                                goto ok;
                   case _IFDEF:
                   case _IFNDEF:
                   case _IF:    mapP->macLines = lastIf; lastIf = ii; depth++;       goto ok;
                   case _FOR:   mapP->macLines = lastFor;lastFor= ii; depth++;       goto ok;
                   case _ELSE:
                   case _ELSEIF:if ((kk=lastIf)  < 0) goto err1035;
                                kk = mapcP->P(kk)->macLines;  mapcP->P(lastIf)->macLines  = ii - lastIf;  lastIf  = kk; //replace macLines with count
                                mapP->macLines = lastIf; lastIf = ii; mapP->depth--; goto ok;
                   case _ENDIF: if ((kk=lastIf)  < 0) goto err1035;
                                kk = mapcP->P(kk)->macLines;  mapcP->P(lastIf)->macLines  = ii - lastIf;  lastIf  = kk; //replace macLines with count
                                depth--; goto ok;
                   case _ENDEF: if ((kk=lastDef) < 0) goto err1035;
                                kk = mapcP->P(kk)->macLines;  mapcP->P(lastDef)->macLines = ii - lastDef; lastDef = kk; //replace macLines with count
                                depth--; goto ok;
                   case _ENDFOR:if ((kk=lastFor) < 0) goto err1035;
                                kk = mapcP->P(kk)->macLines;  mapcP->P(lastFor)->macLines = ii - lastFor; lastFor = kk; //replace macLines with count
                                depth--; goto ok;
                   case _PRAGMA://if (strnicmp(rr, "bugLineMap", 12) == 0) BugLineMap(mapcP, 0, -1);  goto ok;
                   case _ERROR: case _WARNING: case _UNDEF: case _INCLUDE: goto ok;
              }   }
        return LogError(ERR_1034, "", rr);                                      //unknown # directive
notDirective: ok:                                                               //
        if ((pp = strpbrk(qq=pp, "\r\n")) == NULL)                              //last line is without \r\n
           {mapP->len = (int)strlen(qq);                                        //
            if (strstr(qq, "//") != NULL)                                       //
                mapP->expandedP = RemoveOneLineComment(qq, &mapP->reallocatedB);//process the line fragment
            ii++; goto end;                                                     //
           }                                                                    //
        jj = ii; nextii = ii+1;                                                 //
        for (rr=qq; *(pp-1) == '\\'; nextii++)                                  //line continuation
           {mapcP->P(jj)->erasedB = true;                                       //
            //replace '\ \n' or '\ \r\n' with 2 or 3 spaces                     //
            if ((rr=pp)[0] == '\r' && pp[1] == '\n') {rr++; pp[1] = ' ';}       //
            *(pp-1)          = ' ';  pp[0] = ' '; rr++;                         //
            if (!(pp=strpbrk(pp, "\r\n")))                                      //
               {mapcP->P(ii++)->len = (int)strlen(qq); goto end;}               //
           }                                                                    //
        mapP->len             = (int)(pp-qq);                                   //
        mapcP->P(jj)->erasedB = false;                                          //
        if (pp[0] == '\r' && pp[1] == '\n') *pp++ = 0;                          //terminate line ending in CR/LF
        *pp++ = 0;                                                              //terminate line
        if ((qq=strstr(rr=mapP->expandedP, "//")) != NULL)                      //
            mapP->expandedP = RemoveOneLineComment(rr, &mapP->reallocatedB);    //
        if (m_errCode < 0) return m_errCode;                                    //
       } //for (ii=0, pp=textP;....

end://fill out with extra blank lines to full count
    for (;ii < count; ii++)
        {mapP              = mapcP->P(ii);
         mapP->ref.lineNum = ii+1;
         mapP->ref.fileNum = m_fileNum;
         mapP->depth       = depth;
        }
    if (lastIf >= 0 || lastDef >= 0 || lastFor >= 0)              goto err1035;
    for (ii=0; ii < count; ii++) {if (mapcP->P(ii)->macLines < 0) goto err1035;}//unmatched #if, #for, or #define
    mapcP->m_count = count;                                                     //forced to proper value
    #ifdef DEBUG_PREPROCESSOR                                                   //
      if (false)                                                                //
          BugLineMap(m_lineMapP, 0, count);                                     //accesible when debugging in VC++
    #endif //DEBUG_PREPROCESSOR...                                              //
    m_line = saveLineNum;                                                       //
    return mapcP->Count();                                                      //
err1035: erC = LogError(ERR_1035, mapcP->P(ii)->originalP, "");                 //#endif, #endfor, #endef without opening #if, #for, #define
    return erC;                                                                 //
   } //cPreProcessor::BuildLineMap...

char *cPreProcessor::GetSource(int fileNum, int lineNum)
   {int ii, count=m_lineMapP->Count(); sLINE_MAP *mapP;
    for (ii=0; ii < count; ii++)
        {mapP = m_lineMapP->P(ii);
         if (fileNum == mapP->ref.fileNum && mapP->ref.lineNum == lineNum)
            return mapP->originalP;
        }
    return (char*)"";
   } //cPreProcessor::GetSource...

//Called to log information about an error. Calls cSamError m_err functions.
int cPreProcessor::LogError(int erC, const char *contextP, const char *paramP)
   {char        buf[MAX_PATH];
    const char *fileNameP="";
    int         fileNum=0;
    sLINE_MAP   mm={0}, *mapP=&mm;

    if (m_lineMapP && (mapP    = m_lineMapP->P(m_line))
                   && (fileNum = mapP->ref.fileNum) >= 0
                   && m_includedFilesP->P(fileNum))
           fileNameP = m_includedFilesP->P(fileNum)->fileNameP;
    m_errCode = m_err.LogError(erC, contextP, paramP);
    if (m_options & cPREP_TEXT_NOT_FILENAME) //input is straight text, not a file name
       snprintf(buf, sizeof(buf), "%s", fileNameP);
    else
       snprintf(buf, sizeof(buf), "%s(line %u)", fileNameP, mapP ? mapP->ref.lineNum : 0);
    m_err.AddLocation(buf);
    return m_errCode;
   } //cPreProcessor::LogError...

void cListOfStruct::Initialize(int size)
   {m_countArray    = 0;
    m_count         = 0;
    m_allocInc      = 16;
    m_elementSize   = size;
    m_arrayPP       = NULL;
   } //cListOfStruct::Initialize...

void cListOfStruct::SetAllocationInc(int allocInc)
   {if (m_countArray == 0) m_allocInc = allocInc;}

//Save array & counters, and clear
void cListOfStruct::Save(void)
   {m_saveCountArray = m_countArray;  m_countArray = 0;
    m_saveCount      = m_count;       m_count      = 0;
    m_saveArrayPP    = m_arrayPP;     m_arrayPP    = NULL;
    m_saveInc        = m_allocInc;
   } //cListOfStruct::Save...

//Restore array & counters
void cListOfStruct::Restore(void)
   {m_countArray = m_saveCountArray;
    m_count      = m_saveCount;
    m_allocInc   = m_saveInc;
    m_arrayPP    = m_saveArrayPP;
   } //cListOfStruct::Restore(void)...

//Allocate space for howmany new entries to the list
int cListOfStruct::Add(int howmany) //==1
   {int      oldCount=m_count, inc;
    m_count  += howmany;
    if (((m_count + howmany-1) / m_allocInc) >= m_countArray)
       {inc       = (m_allocInc + howmany - 1) / m_allocInc;
        Realloc(m_arrayPP, (m_countArray+inc) * sizeof(void*));
        while (inc-- > 0) Calloc(m_arrayPP[m_countArray++], m_allocInc, m_elementSize);
       }
    return oldCount;
   }  //cListOfStruct::Add...

//return a pointer to the ii'th element of the list.
//Members of derived classes typecast this pointer to the proper type.
void *cListOfStruct::P(int ii)
   {if (ii < 0 || (ii &=0xFFFFFF) >= m_count) return NULL;
    return (void*)(((uint8_t*)m_arrayPP[ii/m_allocInc]) + m_elementSize * (ii % m_allocInc)); //pointer to element
   } //cListOfStruct::P...

cListOfStruct::~cListOfStruct()
   {while (m_countArray-- > 0) Free(m_arrayPP[m_countArray]);
    Free(m_arrayPP);
    m_arrayPP = NULL;
    m_count   = 0;
   } //cListOfStruct::~cListOfStruct...

cPreProcessor::~cPreProcessor()//destructor
    {delete m_definesP;       m_definesP       = NULL;
     delete m_lineMapP;       m_lineMapP       = NULL;
     delete m_includedFilesP; m_includedFilesP = NULL;
     g_prepP = NULL;
    } //cPreProcessor::~cPreProcessor...

void cPreProcessor::ReassignLine(sLINE_MAP *lineP, char *textP)
   {if (lineP)
       {if (lineP->reallocatedB) Free(lineP->expandedP);
        lineP->expandedP = textP;
   }   } //cPreProcessor::ReassignLine...

//Individual delete functions for each class derived from cListOfStruct
cLINE_MAP::~cLINE_MAP()
   {int ii;
    for (ii=0; ii < m_count; ii++)
        cPreProcessor::ReassignLine(P(ii));
   } //cLINE_MAP::~cLINE_MAP..

cINCLUDED_FILE::~cINCLUDED_FILE()
   {INCLUDED_FILE *incP; int ii;
    for (ii=0; ii < m_count; ii++)
       {if (incP=P(ii))
           {Free(incP->textBlockP); incP->textBlockP = NULL;
            Free(incP->fileNameP);  incP->fileNameP  = NULL;
       }   }
   } //cINCLUDED_FILE::~cINCLUDED_FILE...

//Define parameters could be block allocated from ReadLibFile()
cDEFINE_PARAM::~cDEFINE_PARAM()
    {int ii;
     for (ii=0; ii < m_count; ii++) 
         {if (g_prepP->IsReallocated(P(ii)->nameP)) Free(P(ii)->nameP);}
    } //cDEFINE_PARAM::~cDEFINE_PARAM...

cDEFINE::~cDEFINE()
   {DEFINE *defP; int ii;
    for (ii=0; ii <  m_count; ii++)
       {if (!(defP = P(ii))) continue;
        if (g_prepP->IsReallocated(defP->nameP)) Free(defP->nameP);
        if (g_prepP->IsReallocated(defP->bodyP)) Free(defP->bodyP);
        delete defP->replaceListcP;
        delete defP->formalsP;
       }
   } //cDEFINE::~cDEFINE...

cREPLACE_LIST::~cREPLACE_LIST(void)
   {
   } //cREPLACE_LIST::~cREPLACE_LIST...

cMACATOM::~cMACATOM()
   {int ii; for (ii=0; ii < m_count; ii++) {if (P(ii)) Free(P(ii)->valuP);}
   } //cMACATOM::~cMACATOM...

char *cPreProcessor::strdupl(const char *srcP, int len) //== -1
   {char *destP;
    if (len < 0) len = (int)strlen(srcP);
    Calloc(destP, len+1, 1);
    memmove(destP, srcP, len);
    destP[len] = 0;
    return destP;
   } //cPreProcessor::strdupl..

char *cPreProcessor::Endof(char *srcP)
   {return &srcP[strlen(srcP)];}

//Scan past closing quote in a string at pp.
//Return location of matching quote, or NULL if error.
char *cPreProcessor::ScanString(char *pp)
   {for (char ch=*pp++;; pp++)                                                  //
       {if (*pp == '\\')                                                        //
          {if (*++pp == 'x') pp += 2;}                                          //
        else                                                                    //
        if (*pp == ch) return pp+1;                                             //
        else                                                                    //
        if (*pp == 0 || *pp == '\r' || *pp == '\n') return NULL;                //end of line
       }                                                                        //
    return NULL; //not executed                                                 //
   } //cPreProcessor::ScanString...

//If char* points to a memory address within the range of m_includedFilesP->m_textBlockP[] return false
bool cPreProcessor::IsReallocated(char *pp)
   {int ii; INCLUDED_FILE *inP;
    for (ii=0; ii < m_includedFilesP->Count(); ii++)
        {inP = m_includedFilesP->P(ii);
         if (pp >= inP->textBlockP && pp <= inP->textBlockP + inP->textSize) //can safely use '<=' since there is always space between allocated blocks
             return false;
        }
    return true;
   } //cPreProcessor::IsReallocated...

int cPreProcessor::GetError(char *erMsgP, int msgSize)
   {m_err.ShortError(m_errCode, erMsgP, msgSize);
    return m_errCode;
   }; //cPreProcessor::GetError...

//Find matching ) or ]
char *cPreProcessor::MatchChar(char *strP)
   {char open=*strP, close; int count;
    if (open == '(') close = ')'; else
    if (open == '[') close = ']'; else return NULL;
    for (count=0; *strP; strP++)
         {if (*strP == open) count++; else
          if (*strP == close && --count == 0) return strP;
         }
    return NULL;
   } //cMACRO_EXPANDER::MatchChar...

//trim spaces from left of pp
char *cPreProcessor::strLtrim(char *pp)
   {while (*pp == ' ') pp++; return pp;}

//trim spaces from end of pp.
char *cPreProcessor::strRtrim(char *pp)
   {char *qq; int ii;
    qq = pp + (ii=(int)strlen(pp));
    if (ii) {while (*--qq == ' ' && ii-- > 0); *(qq+1) = 0;}
    return pp;
   } //cPreProcessor::strRtrim..

//Hash table used for speedy lookup of define name when doing macro expansions
uint32_t cPreProcessor::ComputeDefineHash(const char *nameP, int len)
   {uint64_t hash;
    for (hash=0; len-- > 0; ) hash = hash * 17 + *nameP++;
    hash = hash - ((hash / HOWMANY_HASH) * HOWMANY_HASH);
    return (uint32_t)hash;
   } //cPreProcessor::ComputeDefineHash...

//Search for #define named (nameP, len). Return index into m_definesP.
//LookupDefine is called for every word in the source code. Therefore, all
//this screwing around with hash tables makes a BIG difference in overall speed.
//The straight line case is defInx = hashTable[hash(name,len)].
//Only in the rare circumstance that this fails, must we chase the link to the 
//next entry in m_definesP (ie., m_definesP[defInx].hasLink) and try again.
int cPreProcessor::LookupDefine(const char *nameP, int len, bool allowInactiveB)//=false
   {int defInx; DEFINE *defP; uint32_t hash;                                    //
    defInx = m_hashTable[hash=ComputeDefineHash(nameP, len)];                   //
    #ifdef DEBUG_PREPROCESSOR                                                   //
    if (g_bugPrepB)                                                             //
       {printf("%04d: LookupDefine: len=%d '", __LINE__, len);                  //
        for (int ii=0; ii < len; ii++) Printf("%c", nameP[ii]);                 //
        Printf("', hashTable[0x%X]=0x%X\n", hash, defInx);                      //
       }                                                                        //
    #endif //DEBUG_PREPROCESSOR...                                              //
    while (defInx != HASH_UNUSED)                                               //end of linked list
        {defP = m_definesP->P(defInx);                                          //
         if (defP->nameLen == len && strncmp(defP->nameP, nameP, len) == 0)     //exact match please
             {if (defP->activeB || allowInactiveB) return defInx;               //happiness 
              return -ERR_1039;                                                 //found, but inactive; 1039==Unknown #directive name
             }                                                                  //
         else{defInx = defP->hashLink;}                                         //link to next entry with the same hash code.
        } //for (; defInx...                                                    //
    //Do not prepare a full error mesage here since there are circumstances in  //
    //failure of LookupDefine merely signals 'not defined'.                     //
    return -ERR_1039; //Unknown #directive name.                                //
   } //cPreProcessor::LookupDefine...

//delete text in the range (startDelP, endDelP - both pointers into textInP) and replace with macro body.
//actualcP = actual parameters, m_definesP[defInx] = m_definesP entry.
//Upon exit the (always) re-allocated textP is returned in *textOutPP.
int cPreProcessor::ExpandNormalDefine(char **textOutPP, char *textInP, 
                                      char *startDelP,  char *endDelP, 
                                      cDEFINE_PARAM *actualcP, int defInx)
   {DEFINE *defP=m_definesP->P(defInx); 
    return ExpandFromList(textOutPP, textInP, startDelP, endDelP, defP->bodyP, 
                          defP->formalsP, actualcP, defP->replaceListcP, defP->extraSpace);
   } //cPreProcessor::ExpandNormalDefine...

int cPreProcessor::ExpandFromList(char **textOutPP, char *textInP,          //
                                  char *startDelP,  char *endDelP,          //beginning and end of macro to expand (including params)
                                  char *bodyP,                              //body of the macro
                                  cDEFINE_PARAM *formalsP, cDEFINE_PARAM *actualcP, 
                                  cREPLACE_LIST *replaceListcP,             //
                                  int extraSpace)                           //
   {int               newSize, ii, jj, len,                                 //
                      bodyOffset,                                           //offset with body of define
                      paramInx,                                             //
                      firstReplacement=0,                                   //location of first replacement -
                                                                            //next iteration of define expansion will start here
                      actualCount=actualcP->Count(),                        //number of actual parameters
                      formalCount=formalsP ? formalsP->Count() : 0;         //number of formal parameters
    REPLACE_LIST     *repP;
    char             *textOutP, *dP, *pp;

    //calculate size of output buffer when each parameter is substituted
    newSize      = (int)strlen(bodyP) + extraSpace +
                      + (startDelP ? (int)(startDelP-textInP) : 0)
                      + (endDelP   ? (int)strlen(endDelP)     : 0);
    for (ii=0; ii < formalCount; ii++)
        newSize += (1 + actualcP->P(ii)->nameLen - (int)strlen(formalsP->P(ii)->nameP)) * formalsP->P(ii)->countOccurrences; //nameLen is frequency of occurrence
    //add space for actual parameters in excess of declared parameters (eg variable number of parameters case)
    while (ii < actualCount) newSize += 2 + actualcP->P(ii++)->nameLen;     //should be 1+, but 2+ will suffice
    Calloc(textOutP, newSize+1,1);                                          //new space for expanded text
    dP = *textOutPP = textOutP;                                             //
    if (startDelP)                                                          //
       {strncpy(dP, textInP, len=(int)(startDelP-textInP)); dP += len;}     //text up to location of macro name

    //It is duck soup from here on: we already know all the locations of define parameters in
    //the define body and what they are supposed to do - thats what replaceListcP is.
    //Walk through replaceListcP and substitute actual parameters for formal parameters.
    for (ii=0, bodyOffset=0; replaceListcP && (ii < replaceListcP->Count()); ii++)
        {repP       = replaceListcP->P(ii);
         paramInx   = repP->paramInx;                                       //which define parameter is it again ?
         if ((len   = repP->offset - bodyOffset))                           //bytes from body to move upto next formal parameter
            {strncpy(dP, bodyP + bodyOffset, len); dP += len;}              //text before next param
         bodyOffset = repP->offset + formalsP->P(paramInx)->nameLen;        //step over formal parameter in bodyP
         if (ii == 0) firstReplacement = (int)(dP-textOutP);                //may get tweaked as the plot emerges
         len        = actualcP->P(paramInx)->nameLen;                       //length of actual parameter
         pp         = actualcP->P(paramInx)->nameP;                         //actual parameter (yes its true!)
         switch (repP->type)
            {case RL_REPLACE:                                               //straightforward replacement
                strncpy(dP, pp, len); dP += len;                            //replace with actual param
                *dP++ = ' ';                                                //separator between parameter
                break;
             case RL_CHARIFY:                                               //replace param with 'param'
                 if (ii == 0) firstReplacement -= 2;                        //backup over #@
                 if (*pp == '\"') {pp++; len -= 2;}                         //strip quotes(") if present
                 if (*pp == '\'') {strncpy(dP-2, pp, len); dP += len-2;}    //actual parameters is already a 'string'
                 else {*(dP-2) = '\'';                                      //opening quote(')
                       strncpy(dP-1, pp, len); dP += len;                   //actual param
                       *(dP-1) = '\'';                                      //closing quote(')
                      }
                 break;
             case RL_STRINGIFY:                                             //replace param with "param"
                 if (ii == 0) firstReplacement--;                           //backup over #
                 if (*pp == '\'') {pp++; len -= 2;}                         //strip quotes(') if present
                 if (*pp == '\"') {strncpy(dP-1, pp, len); dP += len-1;}    //actual parameters is already a string
                 else {*(dP-1) = '\"';                                      //opening quote(")
                       strncpy(dP, pp, len); dP += len;                     //actual param
                       *dP++ = '\"';                                        //closing quote(")
                      }
                 break;
             case RL_PASTE:                                                 //pasting of define parameters
                 if (ii == 0) firstReplacement -= 2;                        //backup over ##
                 for (dP-=2; *(--dP) == ' ';); *(++dP) = '<';               //scan back over trailing spaces
                 strncpy(dP, pp, len); dP += len;                           //replace with actual param
                 break;
             case RL_FREE_PASTE:                                            //pasting of variables other than define parameters
                 if (ii == 0) firstReplacement -= 2;                        //backup over ##
                 for (dP-=2; *(--dP) == ' ';); *(++dP) = '<';               //scan back over trailing spaces
                 bodyOffset = repP->offset;                                 //
                 break;                                                     //
             case RL_VARARG:                                                //
                 for (jj=formalCount-1; jj < actualCount; jj++)             //formalCount includes ...
                     {strncpy(dP, actualcP->P(jj)->nameP, len=actualcP->P(jj)->nameLen);
                      dP += len; *dP++ = ','; *dP = 0;                      //
                     }                                                      //
                 *(--dP) = 0;                                               //erase final comma
                 break;                                                     //
             case RL_COUNT_ARGS:                                            //replace #@countArgs() with actual parameter count
                 snprintf(dP, 10, "%u", actualCount); dP += strlen(dP);     //                 "
                 bodyOffset += (int)strlen("#@countArgs()");                //
                 break;                                                     //
            }                                                               //
         *dP = '<';                                                         //for debugging
        } //walking replaceListcP...                                        //
    strcpy(dP, bodyP + bodyOffset);                                         //tail of #define body
    if (endDelP) strcat(dP, endDelP);                                       //tail of textinP line
    return firstReplacement;                                                //
   } //cPreProcessor::ExpandFromList...

uint16_t cPreProcessor::m_uninteresting[65536];
int cPreProcessor::Uninteresting (const char *pp)
   {uint8_t *u8P=(uint8_t*)pp;

    while (m_uninteresting[*((uint16_t*)u8P)]) {u8P+=2;};
    while (*u8P != 0 && !Isalpha_(*((char*)u8P)) && *u8P != '\"' && *u8P != '\'') u8P++;
    return (int)(((char*)u8P) - pp);
   }

int cPreProcessor::SpanValidName(const char *pp)                                //
    {int len = (m_allowDollarVblsB  && *pp == '$' ||                            //
                m_allowPercentVblsB && *pp == '%') ? 1 : 0;                     //
     return len + (int)strspn(pp+len, ALPHANUMERICS "_");                       //
    } //cPreProcessor::SpanValidName...

//Replace the line textInP with all valid defines and return the newly allocated data in *textOutPP
//Return 0 if not changes made (ie *textOutPP = textinP), 1 for changes, -ve for error
#ifdef DEBUG_PREPROCESSOR                                                       //
int cPreProcessor::ExpandDefinesByLine(char **resultPP, char *textP, bool bugB) //
    {int ret;                                                                   //
     if (bugB=bugB && g_bugPrepB) Printf("%04d:Expanding %s\n", __LINE__,textP);//
     ret = _ExpandDefinesByLine(resultPP, textP, bugB);                         //do the deed
     if (bugB)             Printf("%04d:Expanded to %s\n", __LINE__, *resultPP);//
     return ret;                                                                //
    } //cPreProcessor::ExpandDefinesByLine...
#else
   #define ExpandDefinesByLine _ExpandDefinesByLine
#endif //DEBUG_PREPROCESSOR...                                                    
int cPreProcessor::_ExpandDefinesByLine(char **textOutPP, char *textInP, bool bugB)
    {char    *pp, *qq, *rr, *textOutP=NULL;                                              //
     int      len, defInx, paramCnt, actualCount=0, offset=0, erC, ii;                   //
     bool     defineFoundB, changedB=false;                                              //
     DEFINE  *defP=NULL;                                                                 //
                                                                                         //
     if (!textInP || *textInP == '\"')                                                   //NULL or quoted string ?!?
        {*textOutPP = textInP; return 0;}                                                //what about '"blah blah" stuff' ?
     do {defineFoundB = false;                                                           //
         for (ii=m_definesP->Count(); --ii >= 0; )                                       //mark all defines
             {m_definesP->P(ii)->lastOffset = -1; m_definesP->P(ii)->depth = 0;}         //  as 'not used'
         for (pp=textInP; *pp; pp+=Uninteresting(pp))                                    //
            {cDEFINE_PARAM actual;                                                       //
             if (*pp == '"')                                                             //
                {if (!(pp=ScanString(pp))) return LogError(ERR_1003, pp, ""); continue;} //Missing close quotes in a string parameter '%s'.
             if (!Isalpha_(*pp))                                  {pp++;      continue;} //not alphabetic or '_'. cannot be a defined name
             //have an alphanumeric, figure length and lookup define                     //
             len = (int)SpanValidName(pp);                                               //
             if ((defInx=LookupDefine(pp, len)) < 0)              {pp += len; continue;} //Not a defined word
             if (!(defP=m_definesP->P(defInx))->activeB)          {pp += len; continue;} //
             //we have a defined word, do substitution                                   //
             if ((erC=ActualParameters(&(qq=pp+len), &actual, defInx)) < 0) return erC;  //erC = count of parameters
             paramCnt = (defP->formalsP ? defP->formalsP->Count() : 0);                  //
             //Check param count.                                                        //
             if ((defP->variableB && (actual.Count()) <  paramCnt) ||                    //has variable num of params: count must exceed fixed params
                (!defP->variableB && (actual.Count()) != paramCnt))                      //not variable num of params: count must equal  fixed params
                 return LogError(ERR_3026, pp, "");                                      //Num of params to a #define does not match the declaration
             //check for overly deep macro expansion. Change with #macro_depth=...       //
             if ((pp-textInP) == defP->lastOffset && defP->depth > m_macDepth)           //
                   return LogError(ERR_3033, defP->nameP, "");                           //
             defP->lastOffset = (int)(pp-textInP);                                       //
             defP->depth++;                                                              //
             //replace text between pp and qq with defined text                          //
             if (defP->blockMacroB)         //outP   inP  <instance> params  whichDefine //
                  offset = ExpandBlockDefine( &rr, textInP, pp, qq, &actual, defInx);    //
             else offset = ExpandNormalDefine(&rr, textInP, pp, qq, &actual, defInx);    //
             if (m_errCode < 0) return m_errCode;                                        //
             if (IsReallocated(textInP)) Free(textInP);                                  //
             textOutP     = textInP = rr;                                                //
             pp           = &textInP[offset];                                            //relocate in rr space
             defineFoundB = changedB = true;                                             //
            }                                                                            //
        } while (defineFoundB);                                                          //
     if (changedB) {*textOutPP = textOutP; return 1;}                                    //
     return 0;                                                                           //
    } //cPreProcessor::_ExpandDefinesByLine...

//Create new define and link through hash table. Return index of new define in m_definesP
//m_hashTable[] contains the index of a define, ie. DEFINE * = m_definesP->P(m_hashTable[hash(name)])
//If two (or more) defines hash to the same value, then the hash table points to the
//most recent DEFINE entry, which then links to the less recent DEFINE index.
int cPreProcessor::CreateNewDefine(const char *nameP, int len)
   {int         defInx= m_definesP->Add(), hash;                                //
    DEFINE     *defP  = m_definesP->P(defInx);                                  //
    if (len < 0)  len = (int)strspn(nameP, ALPHANUMERICS "_");                  //
    defP->nameP       = strdupl(nameP, len);                                    //
    defP->activeB     = true;                                                   //
    hash              = ComputeDefineHash(nameP, defP->nameLen=len);            //
    defP->hashLink    = m_hashTable[hash];                                      //link to previous entry
    m_hashTable[hash] = defInx;                                                 //hashtable points to head of linked list
    return defInx;
   } //cPreProcessor::CreateNewDefineP...

//Process #define at bolP
//returns zero or negative error code.
//Allows #define macro    body
//       #define macro()  body
//       #define macro[]  body
//       #define macro(param1, param2, ..., param[n])  body
//       #define macro[param1, param2, ..., param[n]]  body
//       #define macro(param1, param2, ..., param[n])=
//           body of macro (multiline)
//       #endef
//       #define macro[param1, param2, ..., param[n],...]  body (variable parameter list)
//
//A macro may be active and redefined, provided it is exactly the same as the original
//A macro marked as inactive (ie. undef'd) may be redefined in any fashion.
int cPreProcessor::Define(void)
   {char              *pp, *qq, *eolP, close, *blockDefP=NULL, *bolP, *oldBodyP;//
    const char        *pNameP;                                                  //
    DEFINE            *defP;                                                    //
    int                len, ii, kk, erC=0, line, size, defInx, oldFormalsCount; //
    bool               duplicateB=false, oldActiveB=false;                      //
    cDEFINE_PARAM     *formalsP, *oldFormalsP;                                  //
    cREPLACE_LIST     *oldReplaceP;                                             //
    DEFINE_PARAM      *fmlP;                                                    //
    sLINE_MAP          *mapP;                                                   //
                                                                                //
    bolP = pp     = m_lineMapP->P(m_line)->expandedP;                           //
    eolP          = Endof(bolP);                                                //
    len           = SpanValidName(pp);                                          //name of define
    if (len == 0) return LogError(ERR_1095, pp, "");                            //Invalid characters in define macro name
                                                                                //
    if ((ii=defInx=LookupDefine(pp, len, true)) >= 0)                           //true = allowInactiveB. we will use this entry regardless of activeB
       {defP               = m_definesP->P(ii);                                 //
        duplicateB         = true;                                              //
        oldActiveB         = defP->activeB;                                     //Entry is a duplicate, ok if inactive
        oldFormalsCount    = defP->formalsP ? defP->formalsP->Count() : 0;      //
        oldFormalsP        = defP->formalsP;                                    //
        oldReplaceP        = defP->replaceListcP;                               //
        oldBodyP           = defP->bodyP;                                       //body of define
        defP->formalsP     = NULL;                                              //
        defP->replaceListcP= NULL;                                              //
        defP->isStringB    = false;                                             //re-initialize
        defP->emptyParamsB = false;                                             //re-initialize
        defP->activeB      = true;                                              //re-initialize
        defP->variableB    = false;                                             //re-initialize
        defP->blockMacroB  = false;                                             //re-initialize
        defP->extraSpace   = 0;                                                 //re-initialize
        defP->lineNum      = 0;                                                 //re-initialize
        defP->fileNum      = 0;                                                 //re-initialize
        defP->lastOffset   = 0;                                                 //re-initialize
        defP->depth        = 0;                                                 //re-initialize
       }
    else
       {ii = defInx = CreateNewDefine(pp, len);
        defP        = m_definesP->P(ii);
        oldFormalsP = NULL;
        oldReplaceP = NULL;
        oldBodyP    = NULL;
       }

    formalsP        = new cDEFINE_PARAM;
    if (*(pp+=len) == '(' || *pp == '[') //do not allow spaces between macro name and ( or [
       {//define with parameters
        close = *pp == '(' ? ')' : ']';
        if (!(qq=MatchChar(pp)) || qq >= eolP) return LogError(ERR_1096, pp, ""); //Incomplete parameter list for define
        //return LogError(ERR_1096, "", defEntry.nameP); //Incomplete parameter list for define
        //parse parameter list
        if (pp[1+strspn(pp+1, " \t")] == close)
           {defP->emptyParamsB = true; pp += 2 + strspn(pp+2, " \t");}         //#define macro() of macro[] ...
        else
           {do {pp += 1 + strspn(pp+1, " \t");
                if (pp >= eolP) break;
                if (defP->variableB=(strncmp(pNameP=pp, "...", len=3) == 0))
                   {pp      += len + strspn(pp+len, " \t");
                    pNameP   = "...";
                   }
                else
                if (defP->variableB=(strncmp(pNameP=pp, "_VA_ARGS_", len=9) == 0))
                   {pp      += len + strspn(pp+len, " \t");
                    pNameP   = "_VA_ARGS_";
                   }
                else
                if (len=SpanValidName(pp))
                    pp      += len + strspn(pp+len, " \t");                       //step over parameter name
                else
                    return LogError(ERR_1096, pp, "");                            //Incomplete parameter list for define
                fmlP         = formalsP->AddP();
                fmlP->nameP  = strdupl(pNameP, len);                              //save parameter name in define structure
                fmlP->nameLen= len;
                if (defP->variableB) break;
               } while(*pp == ',');
            if (*pp++ != close) return LogError(ERR_7184, "", defP->nameP);       //Missing close parenthesis ) in an expression '%s'.
       }   }
    defP->formalsP          = formalsP;
    defP->blockMacroB       = false;
    defP->fileNum           = m_fileNum;
    defP->lineNum           = m_line;
    if (duplicateB)
       {if (oldActiveB &&
           (formalsP->Count() != oldFormalsCount ||
            strcmp(defP->bodyP, strRtrim(oldBodyP)) != 0))
            erC = LogError(ERR_3030, "", defP->nameP);                            //A #define of the same name has different define body
        Free(oldBodyP);                                                           //These will be replaced
        delete oldFormalsP; oldFormalsP = NULL;                                   //by new define
        delete oldReplaceP; oldReplaceP = NULL;                                   //          "
        if (erC < 0) return erC;                                                  //
       }

    //Following allows a block define, ie, //#define mac(param1,...)= ...... #endef
    //Block defines may be multi-lined and may also contain other macro statements like #for, #if, etc,
    //(but not #define, #undef, or #include).
    if (*pp == '=')
       {//re-assemble body of block define as a single string separaed by \r\n
        for (line=m_line+1, kk=m_lineMapP->P(m_line)->macLines, size=1; --kk > 0; line++)
            {mapP  = m_lineMapP->P(line);
             if (mapP->macInx == _DEFINE || mapP->macInx == _UNDEF || mapP->macInx == _INCLUDE) return LogError(ERR_3029, "", defP->nameP);
             size += (int)strlen(mapP->originalP) - (int)strspn(mapP->originalP, " ") + 2;
            }
        Calloc(blockDefP, size, 1);
        for (kk=m_lineMapP->P(m_line)->macLines, pp=blockDefP; --kk > 0; )
            {mapP                    = m_lineMapP->P(++m_line);
             ReassignLine(mapP);
             mapP->erasedB           = true;
             defP->blockMacroB       = true;
             qq                      = mapP->originalP;
             len                     = (int)strlen(qq+=strspn(qq, " "));
             strcpy(pp, qq);     pp += len;
             strcpy(pp, "\r\n"); pp += 2;
            }
        defP->bodyP = pp = blockDefP;
       }
    else
        defP->bodyP = strRtrim(strdupl(pp+=strspn(pp, " "), -1));
    defP->isStringB = pp[strspn(pp, "0123456789")] != 0;
    //add 1 for each # because they expand to "actual parameter"; can ignore # and #@
    for (pp=defP->bodyP, size=0; pp=strchr(pp, '#'); pp++) size++;                     //over estimate
    defP->extraSpace = size;

    //locate each occurrence of the define parameters in the body of the define
    LocateReplacements(defP->nameP, &defP->replaceListcP, defP->bodyP, defP->formalsP);
    #ifdef DEBUG_PREPROCESSOR
       if (false)
          BugDefines(defInx, defInx);
    #endif //DEBUG_PREPROCESSOR...
    return 0;
   } //cPreProcessor::Define...

//find location of each parameter in body of the #define.
//Create list in ascending location order of each occurrence in bodyP.
//Update *replaceListPP, and return number of locations in *replaceListPP
int cPreProcessor::LocateReplacements(const char *nameP, cREPLACE_LIST **replaceListPP, char *bodyP, cDEFINE_PARAM *formalsP)
   {char             *pp, *eolP, *formalNameP;
    int               len, ii, erC=0, type, formalCount=formalsP->Count(), len13=(int)strlen("#@countArgs()");
    bool              varArgsB, pastingB;
    cREPLACE_LIST    *replaceListcP;
    REPLACE_LIST     *repP;

    if (formalCount == 0) {*replaceListPP = NULL; return 0;}            //no parameters to #define - straight replacement
    replaceListcP = new cREPLACE_LIST;                                  //
    for (pp=bodyP, eolP=Endof(bodyP); pp != NULL && pp < eolP; )        //
        {if (*pp == '\"' || *pp == '\'')                                //
            {if (pp = ScanString(pp)) continue;                         //no expansion inside string: 
             return LogError(ERR_1003, bodyP, "");                      //1003 = unbalanced '"'
            }                                                           //
         if (Isalpha_(*pp))                  pastingB = false;          //variable: check for formal parameter
         else                                                           //
         if (strncmp(pp, "...", len=3) == 0) pastingB = false;          //
         else                                                           //
         if (pastingB=pp[0] == '#' && pp[1] == '#' && Isalpha_(pp[2])) pp += 2; //free standing ## - ie not preceeded by formal parameter
         else
         if (strncmp(pp, "#@countArgs()", len13) == 0)
            {repP                        = replaceListcP->AddP();       //
             repP->offset                = (int)(pp - bodyP);           //offset within macro
             repP->type                  = RL_COUNT_ARGS;               //type
             pp                          += len13;                      //step over '#@countArgs()'
             continue;
            }
         else {pp++; continue;}                                         //not interesting
         for (ii=0; ii < formalCount; ii++)                             //find formal parameter
            {len         = formalsP->P(ii)->nameLen;
             formalNameP = formalsP->P(ii)->nameP;
             varArgsB = (len == 3) && strcmp(formalNameP, "...") == 0;
             if ((varArgsB && strncmp(pp, "...", 3) == 0) ||            //matching ...
                 (pp <= (eolP - len)                 &&                 //enough space left
                  strncmp(pp, formalNameP, len) == 0 &&                 //variable name is equal
                  !Isalnum_(*(pp-1))                 &&                 //not preceeded by alphanumeric
                  !Isalnum_(*(pp+len))))                                //or followed   by alphanumeric
                {type                         = varArgsB                         ? RL_VARARG    :
                                              (*(pp-2) == '#' && *(pp-1) == '@') ? RL_CHARIFY   :
                                              (*(pp-2) == '#' && *(pp-1) == '#') ? RL_PASTE     :
                                              (*(pp-1) == '#')                   ? RL_STRINGIFY : RL_REPLACE;
                 repP                         = replaceListcP->AddP();      //store location within macro for replacement to occur
                 repP->offset                 = (int)(pp - bodyP);          //offset of #define parameter
                 repP->paramInx               = ii;                         //link back to which parameter
                 if (formalsP) formalsP->P(ii)->countOccurrences++;         //count replacement locations
                 repP->type                   = type;
                 pp                          += len;                        //step over parameter name
                 goto end_ppLoop;                                           //continue in outer loop so that order is by location
                }
            } //formalCount...
         //Variable found in define body, but not a formal parameter
         if (pastingB)
            {repP                           = replaceListcP->AddP();    //store location within macro for replacement to occur
             repP->offset                   = (int)(pp - bodyP);        //offset of #define parameter
             repP->paramInx                 = 0;                        //not used
             repP->type                     = RL_FREE_PASTE;            //pasting operations not associated with formal parameter
            }
         pp += SpanValidName(pp);                                      //step over alphabetic name in source
         end_ppLoop: continue;
        }
    *replaceListPP = replaceListcP;
    return replaceListcP->Count();
   } //cPreProcessor::LocateReplacements...

//#undefine a macro. Leaves macro in place but marks it inactive.
//Acceptable syntax is #undef vbl1, vbl2, vbl3,...
int cPreProcessor::Undef(void)
   {char *bolP=m_lineMapP->P(m_line)->expandedP, *pp;
    int   ii, len;
    do {bolP += strspn(bolP, " \t");
        if (pp=strchr(bolP, ',')) {len = (int)(pp - bolP); *pp++ = 0;}
        len = (int)strlen(strRtrim(bolP));
        if ((ii=LookupDefine(bolP, len)) >= 0) m_definesP->P(ii)->activeB = false;
       } while ((bolP=pp) != NULL);
    return 0;
   } //cPreProcessor::Undef...

//process one # command.
int cPreProcessor::OneDirective(void)
   {char  *bolP=m_lineMapP->P(m_line)->expandedP;
    int    erC;

    switch (m_lineMapP->P(m_line)->macInx)
       {default:      return LogError(ERR_1034, bolP, "");   //unknown # directive
        case _ELSE:
        case _ENDIF:  return LogError(ERR_1037, bolP, "");   //#endif, #else without #if
        case _ERROR:  return LogError(ERR_1792, "", bolP);   //#error userText ....
        case _IF:     return If     (_IF);
        case _IFDEF:  return If     (_IFDEF);
        case _IFNDEF: return If     (_IFNDEF);
        case _DEFINE: return Define ();
        case _INCLUDE:erC =  Include(); if (m_line) m_line--; return erC;
        case _UNDEF:  return Undef  ();
        case _WARNING:return LogError(ERR_1793, "", bolP);
        case _PRAGMA: return Pragma ();
        case _FOR:    return For    ();
        case _ENDEF:
        case _ENDFOR: break; //to error
       } //switch...
    return LogError(ERR_3016, bolP, ""); //Unknown #directive
   } //cPreProcessor::OneDirective...

//Go through each line of the file and:
//1. Parse and execute all #directives, including....
//2. #define.  
//3. #include. 
//For non #directive lines, replace defined named wherever found.
//Normally doReplaceB = true, however, may be called with false for block defines.
int cPreProcessor::AnalyzeDirectives(bool doReplaceB)
    {char        *textP;                                                            //
     int          erC=0;                                                            //
     sLINE_MAP   *mapP;                                                             //
                                                                                    //
     //Search lines for # directives or instance of #defined variable.              //
     for (m_line=0; m_line < m_lineMapP->Count(); m_line++)                         //
         {(mapP=m_lineMapP->P(m_line))->debugMac = m_debugMac;                      //copy debug flag to individual line
          if ((textP=mapP->expandedP) == NULL)    continue;                         //empty line
          if (mapP->erasedB) {ReassignLine(mapP); continue;}                        //
          if (textP[0] == 0)                      continue;                         //
          if (mapP->macInx == _NONMAC)                                              //not $ directive
             {//Not preprocessor directive; replace any defines on this line.       //
              if (doReplaceB)                                                       //
                 {erC = ExpandDefinesByLine(&m_lineMapP->P(m_line)->expandedP,textP);//textP <= expanded text
                  if (erC < 0) return erC;                                          //
                  if (erC == 1) m_lineMapP->P(m_line)->reallocatedB = true;         //
             }   }                                                                  //
          else                                                                      //
             {//pre-process #directive                                              //
              mapP->erasedB = true;                                                 //# line is no longer valid
              if (doReplaceB && (mapP->macInx == _INCLUDE || mapP->macInx == _IF))  //
                 {if ((erC=ExpandDefinesByLine(&mapP->expandedP, textP)) < 0) return erC;
                  if (erC == 1) (mapP=m_lineMapP->P(m_line))->reallocatedB = true;  //textP has expandable content
                 }                                                                  //
              if ((erC=OneDirective()) < 0)                                         //
                 {if (m_err.Severity(erC) >= XSEVERITE_ERROR)   return m_errCode;   //may re-assert erasedB
                  if (m_err.Severity(erC) >= XSEVERITE_WARNING && m_warning >= 0)   //
                                                                  m_warning = erC;  //
                 }                                                                  //
              m_lineMapP->P(m_line)->debugMac = m_debugMac;                         //maybe changed by #pragma
             }                                                                      //
          #ifdef DEBUG_PREPROCESSOR                                                 //
          if (false)                                                                //
              BugLineMap(m_lineMapP, 0, -1);                                      //accesible when debugging in VC++
          #endif //DEBUG_PREPROCESSOR...                                            //
        //if (g_checkAllocationB) _CrtCheckMemory();                                //
         } //for m_line=...                                                         //
    #ifdef DEBUG_PREPROCESSOR                                                       //
       if (g_bugPrepB) BugDefines(0, -1);                                         //
    #endif //DEBUG_PREPROCESSOR...                                                  //
    return 0;
   } //cPreProcessor::AnalyzeDirectives...

//Process #macros in *textPP. Block macro expansion differ from regular macros in one important respect:
//They may contain #macros within the body of the block Macro. The block macro is treated like a mini
//program using the same AnalyzeDirectives as a full program would use. Block macros may have recursive
//#macros (eg #for (ii=... ) #for (jj=...) #endfor #endfor) so the program must be collapsed back to a single
//string then rebuilt back into a line_map structure as depth is steadily increased.
int cPreProcessor::ExpandBlockDefine(char **textOutPP, char *textP, char *startDelP, char *endDelP, cDEFINE_PARAM *actualcP, int defInx)
    {int         ii, l1, l2, l3, erC, saveLine=m_line,
                 actualCount=actualcP->Count(), depth=m_lineMapP->P(m_line)->depth;
     char       *textOutP=NULL, *newBodyP;
     sLINE_MAP   *mapP;
     bool        bb;

     //replace the parameters of the define body with the actual parameters from this invocation
     ExpandNormalDefine(&newBodyP, m_definesP->P(defInx)->bodyP, NULL, NULL, actualcP, defInx);

     for (bb=true; bb; depth++)
        {m_lineMapP->Save();
         if ((erC=BuildLineMap(newBodyP, m_lineMapP, 0)) < 0)
            {m_lineMapP->Restore();
             m_line = saveLine;
             return erC;
            }
         m_line = 0;
         erC    = AnalyzeDirectives(false); //increments m_line
         #ifdef DEBUG_PREPROCESSOR
           if (false)
              BugLineMap(m_lineMapP, 0, -1);                
         #endif //DEBUG_PREPROCESSOR...
         for (ii=0, bb=false; ii < m_lineMapP->Count(); ii++)
             {mapP = m_lineMapP->P(ii);
              if (bb=mapP->expandedP != NULL && !mapP->erasedB && mapP->macInx != _NONMAC) break;
             }
         textOutP     = ConcatenateFor(true, depth); //rebuild lines back into a single string result including internal macro's
         for (ii=m_lineMapP->Count(); --ii >= 0;) ReassignLine(m_lineMapP->P(ii));
         Free(m_lineMapP->m_arrayPP[0]); //only one entry
         Free(m_lineMapP->m_arrayPP);
         Free(newBodyP);
         newBodyP = textOutP;
         m_line   = saveLine;
         m_lineMapP->Restore();
        }

     //now prepend textOutP with beginning of line from caller, and append end of line from caller
     l1           = (int) (startDelP - textP);
     l2           = (int) strlen(textOutP);
     l3           = (int) (Endof(endDelP) - endDelP);
     Calloc(newBodyP, l1 + l2 + l3 + 1, 1);
     strncpy(newBodyP,        textP, l1);
     strcpy(&newBodyP[l1],    textOutP);
     strcpy(&newBodyP[l1+l2], endDelP);
     Free(textOutP);
     *textOutPP   = newBodyP;;
     return 0;
    } //cPreProcessor::ExpandBlockDefine...

//Prepare a list in *actualPP of actual parameters passed to this define
int cPreProcessor::ActualParameters(char **textPP, cDEFINE_PARAM *actualP, int defInx)
   {char   *pp, *qq, *rr, open, close=(char)0xFF, ch=(char)0xFF;
    const char *nameP="";
    DEFINE *defEntryP=m_definesP->P(defInx);
    int     parenCount, inx, nameLen, actualCount, formalCount,
            declaredCount=defEntryP->variableB ? 0x7FFFFFFF : defEntryP->formalsP ? defEntryP->formalsP->Count() : 0;
    bool    positionalB=false, namedB=false; //true when parameters are deemed to be positional, eg, mac(.name=value)

    pp   = *textPP + strspn(*textPP, " \t");
    open = *pp;
    if (open == '(') close = ')'; else
    if (open == '[') close = ']'; else 
                    {close = ';'; open = ' ';}
    //if (m_options & cPREP_FOR_ASSEMBLER) open = ' '; else open = 0;
    //find actual parameters (separated by comma) and matching close paren.
    for (qq=pp, parenCount=0, actualCount=0; open && actualCount < declaredCount || defEntryP->emptyParamsB; pp++)
      {switch (*pp)
         {case '[':
          case '(': if (*pp == open) parenCount++; break;//count up parens
          case ']':
          case ')': if (*pp != close) break;
                    if (--parenCount >= 0)               //count down parens
                       {if (parenCount != 0) break;
                        if (defEntryP->emptyParamsB) {pp++; goto dun;} 
                        else                         {qq++; goto store;}
                       }
                    return LogError(ERR_1095, pp, "");                                 //Invalid characters in define macro name: '%s'
          case ';': if (open == ' ') {ch = 0; goto store;}                             //end of statement in masm file 
                    continue;                                                          //otherwise ignore
          case 0:   case '\n':                                                         //end of string or end of line and no ')'
                    if (parenCount == 0) break;
                    return LogError(ERR_1095, pp, "");                                 //Invalid characters in define macro name: '%s'
          case '\"':if (pp=strchr(rr=pp+1, '\"')) break;
                    return LogError(ERR_1406, "", rr-1);
          case ',': if (parenCount != 1)  break; qq++;                                 //comma inside parens
             store: ch = *pp; *pp = 0;                                                 //end of parameters. Overwrite ',' or ')'.
                    if (*(qq+=strspn(qq, " \t")) == '.')                               //named parameter ?
                       {if (defEntryP->variableB)return LogError(ERR_3024,*textPP,qq); //named parameters may not be used with variable number of parameters
                        if (actualCount == 0) actualP->Add(defEntryP->formalsP->Count());//add slots for all parameters
                        if (positionalB) return LogError(ERR_3024, *textPP, qq);       //started out positional, now named
                        namedB  = true;                                                //henceforth must be named parameters
                        nameLen = SpanValidName(++qq);                                 //
                        if (*(rr=qq+nameLen+strspn(qq+nameLen, " \t")) != '=')         //
                                          return LogError(ERR_3025, *textPP, rr);      //named parameters must be followed by '='
                        rr += 1+strspn(rr+1, " \t");                                   //points to value of parameter
                        qq[nameLen] = 0;                                               //delimit parameter name
                        for (inx=0; inx < defEntryP->formalsP->Count(); inx++)
                            {if (strcmp(qq, defEntryP->formalsP->P(inx)->nameP) == 0)
                                {if (actualP->P(inx)->nameLen) return LogError(ERR_3027, *textPP, qq); //named parameters used twice
                                 actualP->P(inx)->nameP   = rr = strdupl(strRtrim(rr), -1);
                                 actualP->P(inx)->nameLen = (int)strlen(rr);
                                 break; //from for(inx=
                       }    }   }
                    else
                       {if (namedB && actualP->Count() != 0) return LogError(ERR_3024, *textPP, qq); //started out named, now a positional
                        if (defEntryP->emptyParamsB) goto dun;
                        positionalB         = true;
                        inx                 = actualP->Add();
                        actualP->P(inx)->nameP  = rr = strdupl(strRtrim(strLtrim(qq)), -1); //text of parameter to #define
                        actualP->P(inx)->nameLen= (int)strlen(rr);                     //length of said text
                       }
                    actualCount++;
                    if ((*pp = ch) == close) {pp++; goto dun;}                         //restore delimitter after actual parameter
                    if (defEntryP->variableB && ch == close) goto dun;
                    qq = pp;                                                           //start of next parameter
     }  } //for (qq=pp, parenCount=0;...
dun: //Verify all parameters have been defined
     formalCount = (defEntryP->formalsP ? defEntryP->formalsP->Count() : 0);
     if (namedB) actualP->m_count = formalCount;
     if (( defEntryP->variableB && actualP->Count() <  formalCount) ||
         (!defEntryP->variableB && actualP->Count() != formalCount))
         return LogError(ERR_3026, *textPP, defEntryP->nameP);
     if (namedB)
        {for (inx=0; inx < formalCount; inx++)
           if (actualP->P(inx)->nameLen == 0) return LogError(ERR_3026, *textPP, defEntryP->nameP);
         }
   //*actualPP = actualP;
     *textPP   = pp;
     return actualP->Count();
    } //cPreProcessor::ActualParameters...

//#debug [on | off |] [macro | compile | logfile | run | depth #] *
int cPreProcessor::Pragma(void)
   {char    *bolP=m_lineMapP->P(m_line)->expandedP, *bufP, name[32];
    int      len, ii, bufSize=256;
    bool     onB;
    static struct 
       {const char *nameP; uint32_t bits;} 
      debug[] =
       {{"macro",         DEBUG_MAC       }, //CISH       
        {"compile",       DEBUG_COMPILE   }, //     MAD   
        {"createDebuglog",DEBUG_LOGFILE   }, //     MAD   
        {"opcodes",       DEBUG_BY_OPCODE }, //     MAD
        {"run",           DEBUG_RUN       }, //CISH       
       };                                    
    if (strnicmp(bolP, "macDepth", len=8) == 0)
       {m_macDepth = strtoul32(bolP+len, &bolP); return 0;}
    if (strnicmp(bolP, "bugLineMap",   10) == 0) 
       {BugLineMap(m_lineMapP, 0, -1);           return 0;}
    if (strnicmp(bolP, "debug",len=5) != 0) goto err;
    for (onB=true; *(bolP += len + strspn(bolP+len, " "));)
       {if (strnicmp(bolP, "on", len=2) == 0){onB = true;           continue;}
        if (*bolP == '~' || *bolP == '-')    {onB = false; len = 1; continue;}
        if (strnicmp(bolP, "off",len=3) == 0){onB = false;          continue;}
        for (ii=HOWMANY(debug); --ii >= 0;)
           if (strnicmp(bolP, debug[ii].nameP, len=(int)strlen(debug[ii].nameP)) == 0)
              {if (onB) m_debugMac |= debug[ii].bits; else m_debugMac &= ~debug[ii].bits;
               break;
              }
        if (ii < 0) goto err;
       };
    return 0;
err://Create text comprising list of acceptable debug variables
    strncpy(name, bolP, sizeof(name)); name[sizeof(name)-1] = 0;
    bufP = (char*)malloc(bufSize=256 + sizeof(name) * HOWMANY(debug));
    for (ii=0; ii < sizeof(name)-1; ii++) if ((name[ii] = *bolP++) <= 0x20) break; name[ii] = 0;
    strncpy(bufP, "Expecting debug followed by: on, off, ~, -, ", bufSize);
    for (ii=0; ii < HOWMANY(debug); ii++)
        {len = (int)strlen(bufP); snprintf(&bufP[len], bufSize-len-1, ", %s", debug[ii].nameP);}
    len = (int)strlen(bufP); strncat(bufP, "\nor macdepth #", bufSize-len);
    bufP[bufSize-1] = 0;
    ii  = LogError(ERR_1031, bufP, name); //unknown #pragma directive (warning)
    Free(bufP);
    return ii;
   } //cPreProcessor::Pragma...

//#for (variable=value; variable < limit; variable++);
//The expressions for start, limit and increment can be arbitrary complex - hence the use
//of PrepExpression() and Expression() in this function.
int cPreProcessor::For(void)
   {char             *vblP, *pp, *expandedP=NULL, buf[16], *hdrBaseP, *forHdrP, *bodyP, zz[1]={0};
    int               len, erC, kk, offset, size, defInx, depth=m_lineMapP->P(m_line)->depth;
    bool              oldGoodB=false, bb=false;
    int64_t           start, temp;
    DEFINE            saveDefine,       //value of previous define usurped by forVariable
                     *defP=NULL;        //pointer to #define created to represent forVariable
    sLINE_MAP        *lineMapP;
    cREPLACE_LIST    *replaceListcP=NULL;
    cDEFINE_PARAM     actual, formal;   //
    MACATOM_TYPE      macType=MACTYPE_INT;
    cMACATOM          initAtoms, limAtoms, incAtoms;

    hdrBaseP = forHdrP = strdupl(m_lineMapP->P(m_line)->expandedP, -1);

    //Look for variable name
    if (*(forHdrP+=strspn(forHdrP, " ")) != '(')                               goto err;
    forHdrP += 1+strspn(forHdrP+1, " ");
    len      = SpanValidName(vblP=forHdrP);                                                //vblP=control variable
    forHdrP  = forHdrP+len+strspn(forHdrP+len, " ");

    //must be followed by '='
    if (*forHdrP++ != '=')                                                     goto err;
    vblP[len] = 0; //terminate variable name(probaby will sit on the '=')

    //If a #define already exists for the control variable, save it and replace; otherwise create a new one
    if (oldGoodB=((defInx=LookupDefine(vblP, len, true)) >= 0))
         saveDefine  = *m_definesP->P(defInx);
    else  defInx     = CreateNewDefine(vblP, -1);

    defP             = m_definesP->P(defInx);
    defP->bodyP      = NULL;
    defP->activeB    = true;
    defP->isStringB  = false;

    //Initial expression must be terminated with ';'
    if ((pp=strchr(forHdrP+=strspn(forHdrP, " "), ';')) == NULL)               goto err;
    *pp++ = 0;
    if ((erC=PrepExpression(forHdrP, &initAtoms))                         < 0) goto err;   //analyse initial expression (for vbl=<initialExpression>;...
    if ((erC=Expression(&initAtoms, _FOR, 0, erC-1, &start))              < 0) goto xit;   //evaluate expression following #for

    //prepare limit expression (for vbl=....; <limit expression>;...
    if ((pp=strchr(forHdrP=pp, ';')) == NULL)                                  goto err;   //limit expression must be terminated by a ';'
    *pp++   = 0;
    if ((erC=PrepExpression(forHdrP, &limAtoms))                          < 0) goto err;

    //prepare increment expression (for vbl=....; ....; <inc expression>;...
    for (pp=Endof(forHdrP=pp+strspn(pp, " ")); *(--pp) == ' ';){}
    if (*pp != ')')                                                            goto err;
    *pp = 0;
    if ((erC=PrepExpression(forHdrP, &incAtoms))                          < 0) goto err;   //increment expression (for vbl=<initialExpression>;<incrementExpression>;...)

    bodyP                = ConcatenateFor(false, depth);                                   //cat all lines between #for and #endfor
    formal.Add();                                                                          //create a synthetic formal parameter list for ExpandFromList
    formal.P(0)->nameLen = (int) strlen(vblP);                                             //
    formal.P(0)->nameP   = strdupl(vblP, -1);                                              //
    if ((erC=LocateReplacements("#for", &replaceListcP, bodyP, &formal)) < 0) return erC;  //locate all the replacements in bodyP
    actual.Add();
    //Create n copies of bodyP substituting vbl name (in formal) with value of expression (in actual)
    snprintf(buf, (int)sizeof(buf), "%d", (int)start); defP->bodyP = strdupl(buf, -1);
    for (offset=0, size=1, expandedP=NULL;;)
       {if((erC=Expression(&limAtoms, _FOR, 0, limAtoms.Count()-1, &temp)) < 0)goto xit;   //evaluate limit expression
        if (temp == 0)                                                         break;      //limit expression fails
        actual.P(0)->nameLen = (int)strlen(actual.P(0)->nameP=defP->bodyP);
        kk = ExpandFromList(&pp, zz, NULL, NULL, bodyP, &formal, &actual, replaceListcP, 0);//0 = extraSpace
        Realloc(expandedP, size+=(len=(int)strlen(pp)));                                   //expand for concatenate
        strcpy(&expandedP[offset], pp);                                                    //concatenate onto expandedP
        Free(pp);
        offset        += len;
        if ((erC=Expression(&incAtoms, _FOR, 0, incAtoms.Count()-1, &temp))< 0)goto xit;   //evaluate increment expression
       }

    //replace line containing the original #for with expanded text
    defP->activeB    = false;                                                              //do not apply macro for control variable twice
    erC                   = ExpandDefinesByLine(&expandedP, expandedP);                    //replace all other active macros
    lineMapP              = m_lineMapP->P(m_line);
    ReassignLine(lineMapP, expandedP);
    lineMapP->reallocatedB= true;
    lineMapP->erasedB     = false;
    lineMapP->macInx      = _NONMAC;
    Free(defP->bodyP);                                                                     //value of control variable
xit:Free(hdrBaseP);
    if (oldGoodB)  *m_definesP->P(defInx) = saveDefine; else
    if (defP) {defP->bodyP = NULL; defP->activeB = false;}
    if (actual.P(0)) actual.P(0)->nameP = NULL;
    delete replaceListcP;
    return erC;
err:erC = LogError(ERR_3016, forHdrP, ""); goto xit;                                       //3016=Unknown #directive or syntax error in #directive
   } //cPreProcessor::For...

//Get text between #for line and #endfor line, concatenate and return in allocated buffer
char *cPreProcessor::ConcatenateFor(bool allB, int targetDepth)
   {char        *bodyP=NULL, *dp, *sp;
    int          ii, endFor, size;
    sLINE_MAP    *mapP;
    INCLUDED_FILE *baseAllocationP;

    if (allB) {endFor = m_lineMapP->Count(); m_line = -1;}                                 //Concatenate entire set of lines
    else       endFor = m_line + m_lineMapP->P(m_line)->macLines;

    for (mapP=m_lineMapP->P(ii=m_line+1), size=0; ii < endFor; ii++, mapP++)               //compute size of text between #for and #endfor
        {if ((sp=mapP->expandedP) != NULL && !mapP->erasedB)
             size += 3 + (int)strlen(sp) + ((mapP->macInx == _NONMAC) ? 0 : 10);           //arbitrary size boost for any #command
        }
    bodyP = dp = (char*)malloc(size);                                                      //allocate space for concatenated results
    for (mapP=m_lineMapP->P(ii=m_line+1); ii < endFor; ii++, mapP++)
        {sp = mapP->expandedP;
         if (!sp || mapP->erasedB) continue;
         if (mapP->depth > targetDepth && mapP->macInx != _NONMAC)
            {strcpy(dp, KeyWords[mapP->macInx]); strcat(dp, " "); dp += strlen(dp);
             if (sp == NULL) {*dp++ = '\r'; *dp++ = '\n'; *dp = 0;}
            }
         if (sp == NULL) continue;
         strcpy(dp, sp);                                                                   //concantenate each line
         dp   += strlen(sp);
         *dp++ = '\r'; *dp++ = '\n'; *dp = 0;                                              //with crlf between
         mapP->erasedB   = true;
         baseAllocationP = m_includedFilesP->P(mapP->ref.fileNum);
         ReassignLine(mapP);
        }
    if (m_lineMapP->P(endFor))                    //r/this does not make sense
       {m_lineMapP->P(endFor)->erasedB = true;
        ReassignLine(m_lineMapP->P(endFor));
       }
    return bodyP;
   } //cPreProcessor::ConcatenateFor...

//Process #if (calledFrom=_IF), #ifdef (calledFrom=_IFDEF), or #ifndef (calledFrom=_IFNDEF).
//Replace unwanted text with 0xFF (using Wipe)
int cPreProcessor::If(MAC_INX calledFrom)
    {char        *bolP, *atmP, *eolP; //, *ifExprP=NULL;
     MACATOM_TYPE macType=MACTYPE_INT;
     cMACATOM     atomList;
     int          len, erC=0, endLine=-1, elseLine=-1, elifLine=-1;
     int64_t      result;

     bolP = atmP = m_lineMapP->P(m_line)->expandedP;
     eolP        = &bolP[strlen(bolP)];
     if (calledFrom == _IFDEF || calledFrom == _IFNDEF)
        {if (!Isalpha_(atmP[0])) {erC = LogError(ERR_1038, atmP, ""); goto err;}  //Variable in #if is not found in a #define directive or formal parameter list.
         erC      = LookupDefine(atmP, len=SpanValidName(atmP));                  //errors (ie not found) are ok
         result   = (calledFrom == _IFNDEF) == (erC < 0 || !m_definesP->P(erC)->activeB) ? 1 : 0;
         atmP    += len + strspn(atmP+len, " \t");
         if (*atmP) {erC = LogError(ERR_3016, atmP, "");              goto err;}
        }
     else
        {if ((erC=PrepExpression(atmP, &atomList))  < 0)              goto err;  //evaluate expression following #if
         erC = Expression(&atomList, calledFrom, 0, erC-1, &result);
         if (erC < 0) goto err; //was goto xit;
        }

     //Locate #endif matching this #if. Watch #if, #endif, #else and #elif on the way.
     if ((erC=LocateEndif(&elifLine, &elseLine, &endLine)) < 0) {atmP = bolP; goto err;}

     //result > 0 if expression is asserted. Wipe out the text that does not apply.
     if (elifLine >= 0)
        {if (result > 0) Wipe(elifLine+1, endLine);              //#else | #elif .... #endif
         else           {Wipe(m_line,     elifLine-1);           //#if(..... #else | #elseif
                         //replace #elif with #if. #if is re-processed later on.
                         m_lineMapP->P(elifLine)->macInx = _IF;
                         goto xit;                               //don't touch #endif line
        }               }
     else
     if (elseLine >= 0)
        {if (result > 0) Wipe(elseLine, endLine);                //#else | #elif .... #endif
         else            Wipe(m_line,   elseLine);               //#if(..... #else | #elif
        }
     else
        {if (result <= 0)Wipe(m_line,   endLine);                //#if .... #endif
        }
     m_lineMapP->P(endLine)->erasedB = true;                     //always mark #endif line
xit: erC = 0;
err: return erC;
    } //cPreProcessor::If...

//Scan for matching #endif  when processing  #if, #ifdef; note also location of #else or #elif.
//Take care to ignore text inside other #if ... #endif. Starts on #if line.
//If ..LineP == -1, then the corresponding #directive was not found.
int cPreProcessor::LocateEndif(int *elifLineP, int *elseLineP, int *endLineP)
    {int   line, ifCounter=0, elifLine=-1, endLine=-1, elseLine=-1;

     for (line=m_line; line < m_lineMapP->Count(); line+=m_lineMapP->P(line)->macLines)                           //step by macLines to next #directive
        {switch (m_lineMapP->P(line)->macInx)
            {case _IFDEF:
             case _IFNDEF:
             case _IF:     ifCounter++;                                                                continue;
             case _ELSEIF: if (ifCounter == 1    ) {if (elifLine >= 0 || elseLine >= 0) goto err1035;
                                                                                     elifLine = line;} continue; //missing #endif
             case _ELSE:   if (ifCounter == 1    ) {if (elseLine >= 0) goto err1035; elseLine = line;} continue; //missing #endif
             case _ENDIF:  if ((--ifCounter == 0)) {endLine = line;    goto xit;}                      continue;
             default:                                                                                  continue;
        }   }
err1035:  return LogError(ERR_1035, m_lineMapP->P(m_line)->expandedP, ""); //missing #endif, or #else / #elif out of sequence
xit: *elifLineP = elifLine;
     *elseLineP = elseLine;
     *endLineP  = endLine;
     return 0;
    } //cPreProcessor::LocateEndif...

//Build array of atoms for each element in the expression and return in atomListP.
int cPreProcessor::PrepExpression(char *atmP, cMACATOM *atomListP)
    {MACATOM      *atomP;
     MACATOM_TYPE  macType=MACTYPE_INT;
     const char   *opsP="<=!|&>+-*/%";
     char         *pp, *rr, *eolP, z0[2]={'0',0}, z1[2]={'1', 0};
     int           len, kk, erC=0;

     //scan over variable names and operators and assemble in atomsP[]
     for (pp=atmP, len=0, eolP=&atmP[strlen(atmP)]; (pp+=strspn(pp, " \t")) < eolP; pp+=len)
        {len = 1; rr = pp;
         if (Isalpha_(pp[0]))
            {len     = SpanValidName(pp);
             macType = MACTYPE_VBL;
             if (len == 7 && strncmp(pp, "defined", 7) == 0 && *(pp+7+strspn(pp+7, " \t")) == '(')
                {pp     += len + (int)strspn(pp+7, " \t"); pp++;
                 len     = SpanValidName(pp);
                 macType = MACTYPE_INT;
                 if ((kk=LookupDefine(pp, len)) < 0) rr = z0; else
                 if (m_definesP->P(kk)->activeB)         rr = z1; else rr = z0;
                 pp     += len + strspn(pp+len, " \t");
                 len     = 1;
                 if (*pp != ')') {erC = LogError(ERR_7184, pp, "");                 goto xit;} //Missing close parenthesis ) in an expression: %s.
                }
             else
                {if ((erC=LookupDefine(pp, len)) < 0)
                    {pp += len-1; rr = z0; len = 1; macType = MACTYPE_ERROR;        goto xit;} //Unknown #directive name (%s).
            }   }
         else
         if (*pp == '(')                 macType = MACTYPE_LPAREN;
         else
         if (*pp == ')')                 macType = MACTYPE_RPAREN;
         else
         if (strchr("\x22'", *pp))
            {if ((rr=strchr(pp+1, *pp)) == NULL) {erC = LogError(ERR_1406, pp, ""); goto xit;}  //missing close quote
             len     = (int)(rr - (++pp));macType = MACTYPE_STR;
            }
         else
         if (strchr(opsP, *pp))
            {len = (int)strspn(pp, opsP); macType = MACTYPE_OP;}                                //scan over operator
         else
         if (len=(int)strspn(pp, "0123456789"))
            {                             macType = MACTYPE_INT;}
         else
            {if (*pp == '#' && pp[1] == '(' && strchr(pp, ')')) {pp +=2; *strchr(pp, ')') = ' ';}
             if ((len = (int)strspn(pp, ALPHANUMERICS "_.")) == 0)
                {erC = LogError(ERR_1039, pp, "");                                  goto xit;}  //Unknown #directive name (%s).
            }
         atomP        = atomListP->AddP();
         atomP->valuP = strdupl(rr, Max(1,len));
         atomP->type  = macType;
         if (macType == MACTYPE_STR) pp += 1;
        }
    atmP     = eolP;
    erC      = atomListP->Count();
xit:if (erC >= 0) return erC;
    return LogError(erC, "", atmP);
   } //cPreProcessor::PrepExpression...

//Process #include <fileName> or #include "fileName"
//replace #include line with included file and update m_lineMapP.
int cPreProcessor::Include(void)
   {char          *bolP, *incTextP, ch='>', *pp, *qq, fileName[MAX_PATH], nameOnly[MAX_PATH];
    int            incSize, len, cat, erC=-ERR_1033; //Syntax error in #directive
    cLINE_MAP     *incMapP=new cLINE_MAP, *newMapP;
    INCLUDED_FILE *ifileP;

    bolP = m_lineMapP->P(m_line)->expandedP;                                               //line sans '#define'
    pp   = bolP + strspn(bolP, " \t");                                                     //
                                                                                           //
    if (*pp == '<')                                                                        //
       {//filename in include directory                                                    //
        if (m_includeDirectoryP       == NULL) goto err;                                   //
        if ((qq=strchr(++pp, ch='>')) == NULL) goto err;                                   //
        strncpy(fileName, m_includeDirectoryP, sizeof(fileName)-2);                        //
        fileName[sizeof(fileName)-1] = 0;                                                  //bullet proofing
        if (fileName[len=(int)strlen(fileName)-1] != '\\' && fileName[len] != '/')         //terminated with a / or \ separator
        #ifdef _WIN32                                                                      //
            fileName[++len] = (strchr(fileName,'/') == NULL) ? '\\' : '/';                 //add separator respecting current / \ usage
        #else                                                                              //
            fileName[++len] = '/';                                                         //
          strncat(fileName, "/", sizeof(fileName));                                        //add separator
        #endif                                                                             //
        strncpy(&fileName[len+1], pp, cat=Min((int)(qq+1-pp), sizeof(fileName)-len-2));    //append filename
        fileName[Min(len+cat,sizeof(fileName)-1)] = 0;                                     //add asciiZ
       }                                                                                   //
    else                                                                                   //
    if (*pp == '"')                                                                        //
       {// "absoluteFileName"  or "relativeFileName"                                       //
        if ((qq=strchr(++pp, ch='"')) == NULL) goto err;                                   //
        strncpy(nameOnly, pp, len=Min((int)(qq-pp), (int)sizeof(nameOnly)-1));             //copy up to closing quote
        nameOnly[len] = 0;                                                                 //
        strcpy(fileName, m_includeDirectoryP);                                             //
        strncpy(&fileName[strlen(fileName)], nameOnly, sizeof(fileName)-strlen(fileName)); //
       }                                                                                   //
    else                                       goto err;                                   //where dat filename ??

    if (strnicmp(&fileName[strlen(fileName)-4], ".lib", 4) == 0) return ReadLibFile(fileName);

    incSize = ReadAllFile(fileName, &incTextP);                                            //read file
    if (incSize < 0) {m_err.AddContext(bolP); return incSize;}                             //unable to read file
    if (RemoveMultiLineComments(incTextP, incSize) < 0) {erC = m_errCode; goto err;}       //
                                                                                           //
    m_fileNum          = m_includedFilesP->Add();                                          //index into m_includedFilesP
    ifileP             = m_includedFilesP->P(m_fileNum);                                   //
    ifileP->fileNameP  = strdupl(fileName, -1);                                            //store name
    ifileP->textBlockP = incTextP;                                                         //    text
    ifileP->textSize   = incSize;                                                          //       and size
    if ((erC=BuildLineMap(incTextP, incMapP, m_lineMapP->P(m_line)->depth+1)) < 0)goto err;//build line map for included file
    newMapP            = m_lineMapP->MergeLineMap(incMapP, m_line, 1);                     //delete #include line and merge included line map
    delete m_lineMapP;                                                                     //
    m_lineMapP         = newMapP;                                                          //
    incMapP->m_count   = 0;                                                                //clear pointer and count
    delete incMapP;                                                                        //then delete skeleton class
    return 0;
err:delete incMapP; return LogError(erC, bolP, "");                                        //Syntax error in #directive
   } //cPreProcessor::Include...

//merge incMapP into this line map at lines (at, at+delcount)'.
cLINE_MAP *cLINE_MAP::MergeLineMap(cLINE_MAP *incMapP, int at, int delCount)
   {cLINE_MAP   *newMapP=new cLINE_MAP;
    int          tailCount, lineCount, incCount=incMapP->Count(), ii,jj, kk=0;
    tailCount  = Count() - (at-1) - delCount - 1;                                                                      //lines following deletion
    lineCount  = m_count + (incCount - delCount);                                                                      //increment size of new line map
    newMapP->SetAllocationInc(lineCount);
    newMapP->Add(lineCount);                                                                                           //allocate to new size
    //Note: there is nothing fancy here; we are just transfer line_map entries from one place to another - including
    //      reallocatedB flag, data pointers and all the flags.
    for (ii=0;         ii < at;       ii++) {*(newMapP->P(kk++)) = *P(ii);          P(ii)         ->expandedP = NULL;} //move upto at-line
    for (jj=0;         jj < incCount; jj++) {*(newMapP->P(kk++)) = *incMapP->P(jj); incMapP->P(jj)->expandedP = NULL;} //add new insertion
    for (ii+=delCount; ii < m_count;  ii++) {*(newMapP->P(kk++)) = *P(ii);          P(ii)         ->expandedP = NULL;} //add tail of old line map
    return newMapP;
   } //cPreProcessor::MergeLineMap...

//Evaluate expression
int cPreProcessor::Expression(cMACATOM *atomListP, int calledFrom, int minInx, int maxInx, int64_t *resultP)
   {char        *pp, *atomP;
    MACATOM_TYPE macType;
    int          ii, lparen, si, prec, pi, erC=0, lastVbl=-1, increment, count=maxInx - minInx + 1;
    int64_t     *stakP=NULL;
    bool         operatorNxt, invert=false, strB, *isStringP=NULL;
    char         buf[16];
    const char  *errTextP="";
    struct OPERATORS {int prec, op;} pStak[PRECEDENCE_LEVELS]; //table of deferred operators

    #ifdef DEBUG_PREPROCESSOR
       if (false)
          {const char *macTypes[] = {MACATOM_TYPE_NAMES};
           Printf("Expression min=%d, max=%d:\r\n", minInx, maxInx);
           for (ii=minInx; ii <= maxInx; ii++)
               Printf("%d: %04X(%s) %s\r\n", ii, atomListP->P(ii)->type, macTypes[atomListP->P(ii)->type & 0xFF],atomListP->P(ii)->valuP);
          }
    #endif //DEBUG_PREPROCESSOR...
    if (minInx > maxInx) return LogError(ERR_1033, "", "");    //nothing in expresion. 1033=Syntax error in #directive
    Calloc(stakP, count, sizeof(int64_t));                     //run time stack (gross over-allocation)
    Calloc(isStringP, count, sizeof(bool));                    //atom is a string(gross over-allocation)
    if (stakP == NULL || isStringP == NULL) {errTextP = "allocation error"; erC = ERR_0005; goto err;}
    pStak[0].prec = si = 0; pi = 1;                            //initialize precedence stack
    atomP = atomListP->P(minInx)->valuP;
    if (atomListP->P(minInx)->type == MACTYPE_OP && (atomP[0] == '!' || atomP[0] == '~') && atomP[1] == 0)
       {stakP[si++] = 1; pStak[pi].prec = 1; minInx++; pStak[pi++].op = atomP[0];} //simulate xor.
    for (operatorNxt=false; minInx <= maxInx; minInx++)
        {atomP     = atomListP->P(minInx)->valuP;
         macType   = atomListP->P(minInx)->type;
         increment = +1;
         if (macType == MACTYPE_OP) macType = (MACATOM_TYPE) (atomP[0] + (atomP[1] << 8));
         switch (macType)
            {default: err1039: erC = LogError(ERR_1039, atomP, ""); goto err; //Unknown #directive name (%s).
             case MACTYPE_LPAREN:
                       if (operatorNxt) goto err1039;                         //Unknown #directive name (%s).
                       for (lparen=0, ii=minInx; ii <= maxInx; ii++)
                           {if (atomListP->P(ii)->type == MACTYPE_LPAREN) lparen++; else
                            if (atomListP->P(ii)->type == MACTYPE_RPAREN) lparen--;
                            if (lparen == 0)
                               {if (Expression(atomListP, calledFrom, minInx+1, ii-1, &stakP[si++]) < 0) goto err;
                                minInx = ii; goto xItem; //skip over items between ()
                           }   }
             case MACTYPE_RPAREN:
                       erC = ERR_1526; goto err; //unbalanced parenthesis
             case MACTYPE_TRUE:
             case MACTYPE_FALSE:
             case MACTYPE_INT:
                       if (operatorNxt) goto err1039; //Unknown #directive name (%s).
                       stakP[si++] = strtoul32(atomP, &pp);
                       if (*pp) goto err1039; //Unknown #directive name (%s), trash after number
             xItem:    operatorNxt = true;    continue;
             case '!': invert      = !invert; continue;
             case MACTYPE_STR:
                       stakP[si] = (intptr_t)atomP; isStringP[si++] = operatorNxt = true; continue;
             case MACTYPE_DOLLAR:
                       if (++minInx > maxInx || atomListP->P(minInx)->type != MACTYPE_VBL) goto err1039; //Unknown #directive name (%s).
                       atomP = atomListP->P(minInx)->valuP;
             case MACTYPE_VBL:
                       if (operatorNxt) goto err1039; //Unknown #directive name (%s).
                       if ((ii=lastVbl=LookupDefine(atomP, (int)strlen(atomP))) < 0)
                          {if (calledFrom == _IFDEF)  stakP[si] = 0; else
                           if (calledFrom == _IFNDEF) stakP[si] = 1; else
                                               {LogError(ERR_1038, atomP, ""); goto err;}//Variable in #if not found in a #define directive or formal parameter list.
                          }
                       else
                          {strB = m_definesP->P(ii)->isStringB;
                           if (strB && calledFrom == _FOR && strspn(m_definesP->P(ii)->bodyP, "0123456789 ") == strlen(m_definesP->P(ii)->bodyP)) strB = false;
                           if (calledFrom == _IFDEF) stakP[si] = 1; else
                           if (calledFrom == _IFNDEF)stakP[si] = 0; else
                           if (calledFrom == _IF || calledFrom == _FOR)
                                               stakP[si] = (isStringP[si]=strB) ? (intptr_t)m_definesP->P(ii)->bodyP : strtoul32(m_definesP->P(ii)->bodyP);
                                                              else
                                              {LogError(ERR_1038, atomP, ""); goto err;}//Variable in #if not found in a #define directive or formal parameter list.
                          }
                       if (invert && !isStringP[si]) stakP[si] = 1 - stakP[si]; //was preceded by not
                       operatorNxt = true; invert = false; si++;
                       continue;
                       //operators
             case '--': increment = -1;
             case '++':if (!operatorNxt) goto err1039; //Unknown #directive name (%s).
                       if (lastVbl < 0) {LogError(ERR_1038, atomP, ""); goto err;}//Variable in #if not found in a #define directive or formal parameter list.
                       snprintf(buf, sizeof(buf), "%d", strtoul32(m_definesP->P(ii)->bodyP) + increment);
                       if (IsReallocated(m_definesP->P(ii)->bodyP)) Free(m_definesP->P(ii)->bodyP);
                       m_definesP->P(ii)->bodyP = strdupl(buf, -1);
                       continue;
             case '||':case '|': 
             case '&&':case '&': 
             case '==':case '=':
             case '>': case '<':
             case '>>':case '<<':
             case '=>':case '=<':
             case '=!':case '><':
             case '+': case '-': 
             case '*': case '/':
             case '%': case '%%':
             case '**':if (!operatorNxt) goto err1039; //Unknown #directive name (%s).
                       prec = Precedence(macType); 
                       while (pStak[pi-1].prec >= prec)
                             {si--;
                              if (isStringP[si-1] != isStringP[si]) goto err;
#ifdef _WIN32 //r/
                              if (isStringP[si]) stakP[si-1] = EvaluateString(pStak[--pi].op, (char*)stakP[si-1], (char*)stakP[si]);
#else
if(false) {}
#endif
                              else erC = Evaluate(&stakP[si-1], pStak[--pi].op, stakP[si-1], stakP[si]);
                              isStringP[si-1] = false;
                             }
                       pStak[pi].prec = prec; pStak[pi++].op = macType;
                       operatorNxt    = false; continue;
            } //switch sw
        } //for (minInx=...
    while (--pi > 0)
          {si--; erC = 0;
           if (isStringP[si-1] != isStringP[si])
              {if (calledFrom != _IF)  stakP[si-1] = 0;
               else             {LogError(ERR_1033, "", ""); goto err;}
              }
           else
              {
#ifdef _WIN32
              if (isStringP[si]) stakP[si-1] = EvaluateString(pStak[pi].op, (char*)stakP[si-1], (char*)stakP[si]);
#else
              if (false) {} //no string operators in Linux land ???
#endif
               else  erC =  Evaluate(&stakP[si-1], pStak[pi].op,        stakP[si-1], stakP[si]       );
              }
           isStringP[si-1] = false;
           if (erC < 0) goto err;
          }
    *resultP = stakP[0];
    if (si != 1) erC = ERR_1033; //something is wrong. Stack should contain one item
err:Free(stakP); Free(isStringP);
    if (erC == 0) return 0;
    return LogError(erC, errTextP, "");
   } //cPreProcessor::Expression...

//Do a small set of string expressions
int cPreProcessor::EvaluateString(int op, char *leftP, char *riteP)
   {switch (op)
       {default:              return LogError(ERR_3031, leftP, riteP); //Impermissible string operation if #directive
        case '==': case '=':  return stricmp(leftP, riteP) == 0;       //equality              (ignoring case)
        case '>':             return stricmp(leftP, riteP) >  0;       //greater than          (ignoring case)
        case '<':             return stricmp(leftP, riteP) <  0;       //less than             (ignoring case)
        case '=>':            return stricmp(leftP, riteP) >= 0;       //greater than or equal (ignoring case)
        case '=<':            return stricmp(leftP, riteP) <= 0;       //less    than or equal (ignoring case)
        case '=!': case '><': return stricmp(leftP, riteP) != 0;       //not equal             (ignoring case)
   }   } //cPreProcessor::EvaluateString...

//Do a select set of arithmetic operations
int cPreProcessor::Evaluate(int64_t *resultP, int op, int64_t left, int64_t rite)
   {int64_t u64;
    switch (op)
       {case '+':             *resultP = left +  rite;              return 0;
        case '-':             *resultP = left -  rite;              return 0;
        case '*':             *resultP = left *  rite;              return 0;
        case '%':             *resultP = rite==0 ? 0 : left % rite; return 0;
        case '/':             *resultP = rite==0 ? 0 : left / rite; return 0;
        case '==': case '=':  *resultP = left == rite;              return 0;
        case '>':             *resultP = left  > rite;              return 0;
        case '<':             *resultP = left  < rite;              return 0;
        case '>>':            *resultP = left >> rite;              return 0;
        case '<<':            *resultP = left << rite;              return 0;
        case '=>':            *resultP = left >= rite;              return 0;
        case '=<':            *resultP = left <= rite;              return 0;
        case '=!': case '><': *resultP = left != rite;              return 0;
        case '&':  case '&&': *resultP = left && rite;              return 0;
        case '|':  case '||': *resultP = left || rite;              return 0;
        case '^':             *resultP = left ^ rite;               return 0;
        case '!':             *resultP = !rite;                     return 0;
        case '~':             *resultP = ~rite;                     return 0;
        case '**':            for (u64=left; --rite > 0;) u64 *= left; 
                                       *resultP = u64;              return 0;
       }
    return -ERR_7183; //Expression has invalid syntax.
   } //cPreProcessor::Evaluate...

//Wipeout lines between startLine and endLine.
void cPreProcessor::Wipe(int startLine, int endLine)
   {while (startLine < endLine) m_lineMapP->P(startLine++)->erasedB = true;
    m_lineMapP->P(endLine)->erasedB = true;
   } //cPreProcessor::Wipe...

/*A lib file comprises one or more block macros encoded in binary:
 String fields are encoded as 16bit length followed by text followed by asciiZ.
 Arrays of any kind are prefixed by 32bit length or count
 The format of the lib file is:
 'madlib:'     signature                                  once only
 32bit length of macro                              \
 name of macro                                       \
 number of formal parameters                          \
   formal parameter name      \ repeated               \
   32bit count of occurrences / formal count times      \
 body of macro (string)                                 | repeated for
 count of replacement list                              | each macro
    type,                      \ repeated              /
    index of formal parameter, | countof replacement  /
    offset                     / times               /
*/

//Write all block macros to lib file.
int cPreProcessor::GenerateLibFile(const char *libFileNameP)
   {int           ii, jj, bufSize=8192, oneSize, size, formalCount, replaceCount;
    FILE         *fileP;
    DEFINE       *defP;
    char         *bufP;
    DEFINE_PARAM *fmlP;
    REPLACE_LIST *repP;
    #define OUTPUT_STRING(str) /*output 16bit length field, then text, then asciiz*/           \
       *((uint16_t*)&bufP[oneSize])  = size = (int)strlen(str);                                \
       snprintf(&bufP[oneSize+sizeof(uint16_t)], bufSize-oneSize-sizeof(uint16_t), "%s", str); \
       oneSize                      += (size + 1 + sizeof(uint16_t));                          \
       bufP[oneSize]                 = '<'; //for debugging
    #define OUTPUT_U32(u32) *((uint32_t*)&bufP[oneSize])  = u32; oneSize += sizeof(uint32_t);
    #define OUTPUT_U16(u16) *((uint16_t*)&bufP[oneSize])  = u16; oneSize += sizeof(uint16_t);
    #define OUTPUT_U8(u8)                 bufP[oneSize++] = u8;

    if ((fileP = fopen(libFileNameP, "wb")) == NULL) return LogError(ERR_0003, "", (const char*)libFileNameP);
    fprintf(fileP, "madlib:");
    Calloc(bufP, bufSize, 1);
    for (ii=0; ii < m_definesP->Count(); ii++)
       {if (!(defP = m_definesP->P(ii))->blockMacroB) continue;
        //Generate lib file record
        memset(bufP, 0, bufSize);
        oneSize            = sizeof(uint32_t);                                 //leave room for length field
        OUTPUT_STRING(defP->nameP);                                            //output deifne name
        formalCount = defP->formalsP->Count(); OUTPUT_U32(formalCount);        //output # parameters
        for (jj=0; jj < formalCount; jj++)                                     //
            {fmlP = defP->formalsP->P(jj);                                     //
             OUTPUT_STRING(fmlP->nameP);                                       //output name of formal parameter
             OUTPUT_U32(fmlP->countOccurrences);                               //output number of occurrences
            }                                                                  //
        OUTPUT_STRING(defP->bodyP);                                            //output body of define
        replaceCount = defP->replaceListcP->Count(); OUTPUT_U32(replaceCount); //
        for (jj=0; jj < replaceCount; jj++)                                    //output list of occurrences
            {repP = defP->replaceListcP->P(jj);                                //
             OUTPUT_U16(repP->type);                                           //type,
             OUTPUT_U16(repP->paramInx);                                       //      formal parameter index,
             OUTPUT_U32(repP->offset);                                         //        & offset in define body
            }                                                                  //
        *((uint32_t*)bufP) = oneSize;                                          //update length of structure
        fwrite(bufP, oneSize,1, fileP);                                        //write to output file
       } //for (ii...                                                          //
    Free(bufP);
    return 0;
   } //cPreProcessor::GenerateLibFile...

//Read lib file and rebuld define structures
int cPreProcessor::ReadLibFile(const char *libFileNameP)
   {int           ii, len, size, defLen, defInx, formalCount, replaceCount, extra;
    DEFINE       *defP;
    char         *bufP, *bP, *dP, *nameP, *pp;
    DEFINE_PARAM *fmlP;
    INCLUDED_FILE*incP;
    REPLACE_LIST *repP;
    #define INPUT_STRING(vblP) vblP = dP+2; dP += sizeof(uint16_t)+1+*(uint16_t*)dP;
    #define INPUT_U32(vbl)     vbl  = *((uint32_t*)dP); dP += sizeof(uint32_t);
    #define INPUT_U16(vbl)     vbl  = *((uint16_t*)dP); dP += sizeof(uint16_t);

    if ((size=ReadAllFile(libFileNameP, &bufP)) < 0) return size;
    incP             = m_includedFilesP->AddP();
    incP->fileNameP  = strdupl(libFileNameP, -1);
    incP->textBlockP = bufP;
    incP->textSize   = size;
    if (strnicmp(bufP, "madlib:", len=7) != 0)    return LogError(-1, "", libFileNameP);
    for (dP=bP=bufP + len; dP < bufP + size; )
        {INPUT_U32(defLen);                                                    //size of entire define entry
         INPUT_STRING(nameP);                                                  //name of define
         defInx            = CreateNewDefine(nameP, -1);                       //create new define entry
         defP              = m_definesP->P(defInx);
         defP->blockMacroB = true;
         defP->isStringB   = false;
         defP->variableB   = false;
         defP->formalsP    = new cDEFINE_PARAM;
         INPUT_U32(formalCount); defP->formalsP->Add(formalCount);             //Get count of format parameters and create an array
         for (ii=0; ii < formalCount; ii++)                                    //for each formal parameter
            {fmlP = defP->formalsP->P(ii);                                     //
             INPUT_STRING(fmlP->nameP); fmlP->nameLen=(int)strlen(fmlP->nameP);//add name and length
             INPUT_U32(fmlP->countOccurrences);                                //add the number of occurrences
             if (stricmp(fmlP->nameP, "...") == 0) defP->variableB = true;
            }
         defP->emptyParamsB = (formalCount == 0);
         INPUT_STRING(defP->bodyP);                                            //body of define
         for (pp=defP->bodyP, extra=0; pp=strchr(pp, '#'); pp++) extra++;      //over estimate
         defP->extraSpace    = extra;                                          //
         defP->replaceListcP = new cREPLACE_LIST;                              //
         INPUT_U32(replaceCount); defP->replaceListcP->Add(replaceCount);      //Get count of replacements and create an array
         for (ii=0; ii < replaceCount; ii++)                                   //
            {repP = defP->replaceListcP->P(ii);                                //
             INPUT_U16(repP->type);                                            //add type
             INPUT_U16(repP->paramInx);                                        //   formal parameter index,
             INPUT_U32(repP->offset);                                          //       and offset in define body
        }   }                                                                  //
    #ifdef DEBUG_PREPROCESSOR                                                  //
       if (false)                                                              //
          BugDefines(0, -1);                                                 //
    #endif //DEBUG_PREPROCESSOR...                                             //
    return 0;                                                                  //
   } //cPreProcessor::ReadLibFile...

#ifdef DEBUG_PREPROCESSOR
//Debugging routines, called typically manually from Visual Studio.
//lines such as if (false) BugLineMap(...) are scattered around the code.
void cPreProcessor::_BugLineMap(cLINE_MAP *mapcP, int start, int end, const char *fromFuncP, int fromLine)
   {sLINE_MAP      *mapP;
    INCLUDED_FILE *incP;
    int            fileNum=-1;
    if (end < 0) end = mapcP->Count();
    g_checkAllocationB = true;
    Printf("\n-------- BugLineMap called from %s #%04d", fromFuncP, fromLine);
    for (; mapcP && start < end; start++)
       {mapP = mapcP->P(start);
        if (fileNum != mapP->ref.fileNum)
           {fileNum  = mapP->ref.fileNum;
            incP     = m_includedFilesP->P(fileNum);
            Printf("\nfile[%d]=%s (0x%p, 0x%0X)", fileNum, incP->fileNameP, incP->textBlockP, incP->textSize);
           }
        Printf("\n%d.%d: len=%d, debugMac=0x%X, macInx=%s, macLines=%d, erasedB=%d, depth=%d",
              mapP->ref.fileNum, mapP->ref.lineNum, mapP->len, mapP->debugMac,
              mapP->macInx == 0 ? "<none>" : KeyWords[mapP->macInx], 
              mapP->macLines, mapP->erasedB, mapP->depth);
        if (mapP->reallocatedB)      Printf(", reallocated");
        if (mapP->originalP != NULL) Printf("\n   originalP=%s", mapP->originalP + strspn(mapP->originalP, " "));
        if (mapP->expandedP != NULL) Printf("\n   expandedP=%s",  mapP->expandedP);
        Printf("\n");
   }   } //cPreProcessor::_BugLineMap...

void cPreProcessor::_BugDefines(int start, int end, const char *fromFuncP, int fromLine)
   {int               ii, jj, kk, count;
    DEFINE           *defP;
    DEFINE_PARAM     *fmlP;
    REPLACE_LIST     *repP;
    static const char*rlNames[] = {"replace", "stringify", "charify", "paste", 	//
                                   "vararg",  "freePaste", "countArgs()"};      //type field of REPLACE_LIST
    char              buf[64], *pp;

    Printf("\n-------- BugDefines called from %s #%04d\n",fromFuncP,fromLine);//
    if (end < 0 || end > m_definesP->Count()) end = m_definesP->Count()-1;      //
    for (ii=start; ii <= end; ii++)                                             //
        {defP  = m_definesP->P(ii);                                             //
         count = defP->formalsP ? defP->formalsP->Count() : 0;                  //
         Printf("%d: #define %s%s", ii, defP->nameP,count==0 ? " " : "(");      //
         for (jj=0, count; jj < count; jj++)                                    //for each formal parameter
            {fmlP = defP->formalsP->P(jj);                                      //
             Printf("%s[occurrances=%d]%s", fmlP->nameP, fmlP->countOccurrences,//
                                  (jj < count-1) ? ", " : ")\r\n");             //
            }                                                                   //
         pp = defP->bodyP;                                                      //
         Printf("   body of #define=%s", pp && *pp ? "" : "<empty>");           //
         while (pp && *pp)                                                      //
            {if (*pp == '\r' && pp[1] == '\n')                                  //
                {if (pp[2] == 0) break; Printf("\r\n    "); pp += 2;}           //
             else                       Printf("%c", *pp++);                    //
            }                                                                   //
         Printf("\r\n   blockMacroB=%d, isStringB=%d, variableB=%d, ",          //
                                                    defP->blockMacroB,  defP->isStringB,  defP->variableB); 
         Printf("empty=%d, extra=%d, activeB=%d, ", defP->emptyParamsB, defP->extraSpace, defP->activeB);
         Printf("fileNum=%d, lineNum=%d, ",         defP->fileNum,      defP->lineNum);
         Printf("hashLink=%d%s)\r\n",               defP->hashLink,     defP->hashLink == HASH_UNUSED ? "(end" : "");
         count = defP->replaceListcP ? defP->replaceListcP->Count() : 0;        //
         for (jj=0; jj < count; jj++)                                           //for each replace location
            {repP = defP->replaceListcP->P(jj);                                 //
             pp   = &defP->bodyP[repP->offset];                                 // 
             if ((kk=SpanValidName(pp)) > sizeof(buf)-1) kk = sizeof(buf)-1;    //
             strncpy(buf, pp, kk); buf[kk] = 0;                                 //
             Printf("    [%d]+%d: %s '%s'\r\n", repP->paramInx, repP->offset, rlNames[repP->type], buf);
        }   } 
    Printf("------------------------------------------------------\n");
   } //cPreProcessor::_BugDefines...
#else
void cPreProcessor::_BugLineMap(cLINE_MAP *mapcP, int start, int end, const char *fromFuncP, int fromLine) 
  {printf("***** would that not be nice *****\n");}

#endif //DEBUG_PREPROCESSOR...

int cPreProcessor::PrintProgram(void)
   {sLINE_MAP  *mapP;                                               //
    int         start=0, end = m_lineMapP->Count();                 //
    bool        blankB=false;                                       //
    if (g_printFileP) fclose(g_printFileP);                         //
    g_printFileP = fopen(m_captureFileP, "wb");                     //causes Printf to write to file
    for (; m_lineMapP && start < end; start++)                      //
       {if ((mapP=m_lineMapP->P(start))->expandedP)                 //
            if (mapP->macInx == _NONMAC)                            //ignore macro lines (#define, #include, etc)
               {if (blankB && mapP->expandedP[0] == 0) continue;    //ignore multiple blank lines
                Printf("%s\n", mapP->expandedP);                    //
                blankB = mapP->expandedP[0] == 0;                   //
       }       }                                                    //
    if (g_printFileP)                                               //
       {fclose(g_printFileP); g_printFileP = NULL;                  //
        Printf("Expanded file written to %s\n", m_captureFileP);    //
        #ifdef _DEBUG                                               //
        OutputDebugStringA("Expanded file written to ");            //
        OutputDebugStringA(m_captureFileP);                         //
        OutputDebugStringA("\n");                                   //
        #endif                                                      //
       }                                                            //
    return end;                                                     //
   } //cPreProcessor::_PrintProgram...

//end of file
