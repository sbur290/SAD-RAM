//Atomize texual input following #define expansion.
//Simplified version of GetAtom() from GenericCompiler
#include "C3_atomize.h"

const char *cAtomize::m_operators[]                                             //
         = {"++", "--", "+=", "-=", "&=", "|=", "^=",                           //post ops
            "&&", "&",  "||", "|", "!=", "!",                                   //logical
            "^", "%", "++", "+", "--", "-", "**", "*", "/",                     //arithmetics
            "<<", "==", ">>", ">=", "<=", "<>", "<", "=", ">",                  //comparators
            "(", ")", "::"};                                                    //
                                                                                //
#ifdef _DEBUG                                                                   //
    sNAMEVALUE_LIST      preset[] = {{"_DEBUG", ""}};                           //prefix '#define _DEBUG'
#else                                                                           //
    sNAMEVALUE_LIST      preset[] = {{"_RELEASE", ""}};                         //prefix '#define _RELEASE'
#endif                                                                          //

cAtomize::cAtomize(CC fileNameP, CC includeDirP, CC captureFileP)
   {int erC, presCnt=HOWMANY(preset);                                           //
    m_backupAtom.type    = GC_NULL;                                             //
    m_allowVerilogFormatB= true;                                                //
    m_prepP              = new cPreProcessor(m_allowDollarVblsB = true,         //
                                             m_allowPercentVblsB= false,        //
                                             captureFileP);                     //
    m_lines = m_prepP->Process(fileNameP, includeDirP, preset, presCnt);        //returns # lines in file or error code
    if (m_lines < 0)                                           return;          //  (after #define expansion and #include processing)
    if ((erC=m_prepP->GetLineMap(&m_lineMapP)) < 0)                             //
       {Printf("Error #%d expanding #defines\n", -erC);        return;}         //if (fileSize == 0) return;                                          //
    m_pp = m_sourceDataP = m_lineMapP[m_lineInx=0].expandedP;                   //
   } //cAtomize::cAtomize...

cAtomize::~cAtomize()
   {delete m_prepP; m_prepP = NULL;
   } //cAtomize::~cAtomize...

inline char Hex2(char **ppp)
    {char hex[3] = {(*ppp)[0], (*ppp)[1], 0}; *ppp = (*ppp)+2; return (char)strtol(hex, NULL, 16);}
 
IATOM cAtomize::GetAtom(bool probeB) //=false (ie step posn)
   {int     ii, jj, len, srcOffset;
    char   *pp =m_pp, *qq, ch;
    IATOM   vbl={NULL,0,0,0};
    if (m_backupAtom.type != GC_NULL)
       {vbl = m_backupAtom;
        if (!probeB) {m_backupAtom.type = GC_NULL; m_backupAtom.textP = NULL;} //lookahead - don't destroy
        if (vbl.index < 0 && vbl.type == GC_NAME)
           {if (vbl.textP[0] == '$' && m_allowDollarVblsB ||
                vbl.textP[0] == '%' && m_allowPercentVblsB)
                 {if ((vbl.index=ReservedFunctionName(vbl.textP, vbl.len)) >= 0)
                      vbl.type  = GC_RESERVED_FNC;
                  if (probeB) m_backupAtom = vbl;
                 }
            else  vbl.index = LookupName(vbl.textP, vbl.len);                   //not used in samCompile
            m_backupAtom.index = vbl.index;                                     //
           }                                                                    //
    vbl.ref.srcOffset = (int)(vbl.textP - m_lineMapP[m_lineInx].expandedP) +            //srcOffset in line
                     m_lineMapP[m_lineInx].ref.srcOffset;                       //srcOffset of line
        return vbl;                                                             //
       }                                                                        //
    while (m_pp == NULL || *m_pp == 0 )                                         //end of line, get next line
       {if (++m_lineInx >= m_lines)                                             //end of file
           {if (m_appendAtom.type != GC_NULL)                                   //appending atom to input stream
               {if (probeB) return m_appendAtom;                                //  (not used by samCompile) 
                vbl               = m_appendAtom;                               //
                m_appendAtom.type = GC_NULL; m_appendAtom.textP = NULL;         //
                return vbl;                                                     //
               }                                                                //
            vbl.len = 0; vbl.type = GC_NULL; vbl.textP = "";                    //
            return vbl;                                                         //
           }                                                                    //
        if (m_lineMapP[m_lineInx].erasedB) m_pp = (char*)"";                    //keep looping
        else m_pp = m_lineMapP[m_lineInx].expandedP;                            //
       }                                                                        //
    vbl.textP = pp = (m_pp+= strspn(m_pp, " \t"));                              //
    srcOffset = (int)(vbl.textP - m_lineMapP[m_lineInx].expandedP) +            //srcOffset in line
                     m_lineMapP[m_lineInx].ref.srcOffset;                       //srcOffset of line
    len       = (*pp == '$' && m_allowDollarVblsB) ||                           //
                (*pp == '%' && m_allowPercentVblsB) ? 1 : 0;                    //
    if(*pp >= 'a' && *pp <= 'z' || *pp >= 'A' && *pp <= 'Z' || *pp == '_' || len)//alphabetics plus underscore
       {len+= (int)strspn(pp+len, ALPHANUMERICS "_");                           //           "
        vbl.type = GC_NAME; vbl.len = len; vbl.index = 0;                       //           "
       }                                                                        //
    else                                                                        //
    if (pp[0] == '0' && pp[1] == 'x')                                           //hex values
       {len      = 2+(int) strspn(pp+2, "0123456789ABCDEFabcdef_");             //    "
        vbl.len  = RemoveUnderScore(pp, len);
        vbl.type = GC_INT;                                                      //    "
        if (vbl.len == 2) vbl.len = -ERR_1002;                                  //1002 = invalid number
       }                                                                        //
    else                                                                        //
    if (pp[strspn(pp, "0123456789")] == '\'' && m_allowVerilogFormatB)          //Verilog format, eg 31'h123_45
       {len      = (int) strspn(pp, "0123456789ABCDEFabcdefh'_");               //          "
        vbl.len  = RemoveUnderScore(pp, len);                                   //          " 
        vbl.type = GC_INT;                                                      //          "
       }                                                                        //          "
    else                                                                        //
    if (Isdigit(*pp) || *pp == '.' && Isdigit(pp[1]))                           //ordinary number
       {len     = (int) strspn(pp, "0123456789._");                             //       "
        vbl.len = RemoveUnderScore(pp, len);                                    //       "
        vbl.type = GC_INT;                                                      //       "
        for (ii=0; ii < len; ii++)                                              //       "
            {if (pp[ii] == '.')                                                 //       "
                {if (vbl.type == GC_DOUBLE) vbl = AtomErr(ERR_1002, pp);        //1002 = invalid number (two '.'s)
                 else                       vbl.type = GC_DOUBLE;               //       "
       }    }   }                                                               //
    else                                                                        //
    if (*pp == '\"')                                                            //
       {//string value                                                          //
        vbl.textP = ++pp;                                                       //
        vbl.type  = GC_STRING;                                                  //
        for (qq=pp, len=0; (ch=*(pp++)) != '\"' ; qq++, len++)                  //translate control chars inline
            {if (ch == '\\')                                                    //
                  *qq = (ch=*pp++) == 'n' ? '\n' : ch == 'r' ? '\r' :           //
                         ch        == '\"'? '\"' : ch == '\''? '\'' :           //
                         ch        == 'v' ? '\v' : ch == '\\'? '\\' :           //
                         ch        == 'b' ? '\b' : ch == 'a' ? '\a' :           //
                         ch        == 'a' ? '\a' : ch == 't' ? '\t' :           //
                         ch        == 'x' ? Hex2(&pp)               : ch;       //
             else if (strchr("\r\n", ch))                                       //
                     {vbl.type = GC_ERROR; ii = -ERR_1003; len = 2; break;}     //1003 = mising close quote in a string
             else    *qq = ch;                                                  //
             }                                                                  //
        *qq++ = '\"'; while (qq < pp) *qq++ = ' ';                              //restore " after ctrl chars removal and
        vbl.len = len;                                                          //   fill out original space with ' '
        len     = 0;                                                            //pp already updated
       }                                                                        //
     else                                                                       //
     if (pp[0] == '/' && pp[1] == '/')                                          //
       {vbl.len = len = (int)strlen(pp); vbl.type = GC_COMMENT;}                //
     else                                                                       //
     if (pp[0] == '*' && pp[1] == '/'){vbl.len = len = 2; vbl.type =GC_COMMENT;}//
     else                                                                       //
     if (pp[0] == '/' && pp[1] == '*')                                          //
        {for (m_pp=pp+2; (m_lastAtom=GetAtom(false)).type != GC_COMMENT || strncmp(m_lastAtom.textP, "*/", 2) != 0;){}
         vbl.len   = len = (int)(m_lastAtom.textP - pp) +2;                     //
         vbl.textP = pp;                                                        //
         vbl.type  = GC_COMMENT;                                                //
        }                                                                       //
     else                                                                       //
       {vbl.len = len = 1; vbl.type = GC_GETX;                                  //
        for (ii=0; ii < HOWMANY(m_operators); ii++)                             //
            if (strncmp(pp, m_operators[ii], jj=istrlen(m_operators[ii])) == 0) {vbl.len = len = jj; break;}
       }                                                                        //
    m_pp             = pp + len + strspn(pp+len, "\t\r\n ");                    //how dem '\r\n' get there eh?
    vbl.ref.fileNum  = m_lineMapP[m_lineInx].ref.fileNum;                       //
    vbl.ref.lineNum  = GetLineNumber(m_lineInx);                                //
    vbl.ref.srcOffset= srcOffset;                                               //
    return m_lastAtom=vbl;                                                      //
   } //cAtomize::GetAtom...

//Pack number in place with '_' removed.
int cAtomize::RemoveUnderScore(char *numP, int len)
   {char *dd, *ss; int ii, cc;                                                  //
    for (dd=ss=numP, ii=cc=0; ii < len; ss++, ii++)                             //scan for '_'
        {if (*ss == '_') cc++; else *dd++ = *ss;}                               //
    len = (int)(dd-numP);                                                       //updated length
    while (cc-- > 0) *dd++ = ' ';                                               //wipe out end of number
    return len;                                                                 //trimmed length
   } //cAtomize::RemoveUnderScore...

//rewind data stream
void cAtomize::Rewind(void)
   {m_pp      = m_sourceDataP;
    m_lineInx = 0;
   } //cAtomize::Rewind...

char*cAtomize::GetSource(int fileNum, int lineNum)
   {return m_prepP->GetSource(fileNum, lineNum);}

