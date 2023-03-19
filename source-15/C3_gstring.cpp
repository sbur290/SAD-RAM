/*
============================================================================

    File: C3_gString.cpp. Version 7.0.0.2. Nov 23, 2012.

   This class is a partial implementation of MS's CString class, and as far as it is implemented, it
     is compatible with MS's CString class, and is provided to replace it on non-MS platforms.
   It also has some optimizations that make it faster and more efficient than MS's CString for some things.

    Programmer: Greg Edvenson.
    Copyright PicoComputing, Inc., 2003. All rights reserved.

   defines that control how stuff in here works:
      GSTRING_DEBUG:       nowhere near fully implemented, and may not even compile.
      GSTRING_EXTRA_ALLOC: how much extra memory we should keep around reduce realloc calls.
                           (it's actually a const, not a define. see definition for details)
      GSTRING_FANCY_PRINTF:tells the Format function to use DvprintfForGstring for formatting.
                           (the prototype is in this file, so you don't need to include it)

============================================================================
*/

#pragma warning(disable:4706) //assignment within an if ()

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#if defined(DEBUG) && !defined(DASSERT)
   #define DASSERT assert
   #include <assert.h>
#endif
#include <assert.h>
#include "..\include\C3_always.h"
#include "..\include\C3_string.h"
#include "..\include\C3_gstring.h"
#ifndef WIN32
  //#include <common.h>
  #define strnicmp            strncasecmp
  #define _strnicmp           strncasecmp
  #define stricmp             strcasecmp
  #define _stricmp            strcasecmp
#endif

#define GSTRING_EXTRA_ALLOC 1    // Reserve will actually allocate size * (1 + GSTRING_EXTRA_ALLOC) bytes
#define SM_BUFSIZE 512
#define LG_BUFSIZE (16384-SM_BUFSIZE)

cGString::cGString() {m_superP = m_subP = NULL; m_size = 0;}//ordinary constructor. empty string.

cGString::cGString(const cGString& gsR) {m_superP = m_subP = NULL; m_size = 0; *this = gsR; }//copy constructor

// if count is zero, strP needs ascii 00. If count < 0 take substring from end of string
cGString::cGString(const char* strP, int count)         // constructor. count = -1
    {m_superP = m_subP = NULL; m_size = 0;
     if (!strP || !*strP) return; // leaves us in the same state as the default constructor
     if (count < 0) count = (int)strlen(strP);
     Reserve(count);
     memcpy(m_superP, strP, m_size = count);
     m_subP = m_superP;
    }

cGString::~cGString() { Release(); }

cGString::cGString(char ch, int repCount)
   {m_superP = m_subP = NULL; m_size = 0;
    Reserve(repCount);
    m_superP[repCount] = '\0';          // null terminate string after requested number
    m_size = repCount;
    m_subP = m_superP;
    memset(m_superP, ch, repCount);
   }

//Find substring strP in this string
int cGString::Find(const char* subStrP, int offset /* = 0 */) const
   {if (offset < 0 || GetLength() <= offset || !subStrP)  return -1;
    const void *posP = strstr(((const char*) *this) + offset, subStrP);
    return posP ? (int)(((char*) posP) - ((char*) m_subP)) : -1;
   }

//Find where this string has any character from breakerP
int cGString::Break(const char* breakersP, int offset /* = 0 */) const
   {if (offset < 0 || GetLength() <= offset || !breakersP)  return -1;
    const void *posP = strpbrk(((const char*) *this) + offset, breakersP);
    return posP ? (int)(((char*) posP) - ((char*) m_subP)) : -1;
   }

void cGString::MakeLower()
   {if (m_subP && GetLength())
       {PrePreWrite();
        //strlwr(m_subP);
        for (int i=0; i < GetLength(); ++i)
            m_subP[i] = (char)tolower(m_subP[i]);
   }   }

void cGString::MakeUpper()
   {if (m_subP && GetLength())
       {PrePreWrite();
        //strupr(m_subP);
        for (int i=0; i < GetLength(); ++i)
            m_subP[i] = (char)toupper(m_subP[i]);
   }   }

int cGString::Remove(char ch)
   {char str[2];
    int      count;
    str[0] = ch; str[1] = '\0';
    count  = Replace(str, "");
    m_size -= count;
    return count;
   }

cGString::operator const char*() const
   {if (!m_subP) return "";
    PrePreWrite();
    m_subP[m_size] = '\0';
    return m_subP;
   }

cGString operator+(const cGString& gsR1, const cGString& gsR2)  // gsR1 - gstring left Reference.  gsR2 - gstring right Reference
   {cGString   gs;

    gs.Reserve(gsR1.GetLength() + gsR2.GetLength());
    if (gsR1.m_subP)  memcpy(gs.m_subP, gsR1.m_subP, gsR1.GetLength());  //gs.m_subP is not null terminated
    if (gsR2.m_subP)  memcpy(gs.m_subP + gsR1.GetLength(), gsR2.m_subP, gsR2.GetLength());
    gs.m_size = gsR1.GetLength() + gsR2.GetLength();
    gs.m_superP[gs.m_size] = '\0';
    return gs;
   }

cGString cGString::Mid(int offset, int count /* = -1 */) const
   {if (count == -1) count = GetLength() - offset; //mid(#): Single parameter means thru end of string.
    else             count = Min(GetLength() - offset, count);
    if (count  <= 0 || offset < 0)           return "";
    if (offset == 0 && count == GetLength()) return *this;
    Info()->refs++;
    cGString strm;
    strm.m_superP = m_superP;
    strm.m_size   = count;
    strm.m_subP   = m_subP + offset;
    return strm;
   } //cGString::Mid...


#ifdef GSTRING_FANCY_PRINTF //{
extern int Dvprintf(const char *fmtP, OUTPUT_ROUTINE, char**baseBufP, char**pBufP, int *bufLenP, va_list arg);
#define FORMAT_BUF_LEN 4096
//#define OUTPUT_ROUTINE void (*outputRoutineP)(char *msgP, char **baseBufPP, char **pBufPP, int *bufLenP)

//Kernel routine to save characters from Dvprintf in buffer as *baseBufPP.
//*baseBufPP points to base of buffer (which may be reallocated)
//*pBufPP    points to the current location in the buffer
//These pointers may point to a static buffer, and they may be reallocated as we march along.
static void SaveString(char*msgP, char**baseBufPP, char**pBufPP, int *bufLenP)
   {char*baseBufP=*baseBufPP, *pBufP=*pBufPP, *newBufP;
    int   len=sizeof(char)*strlen(msgP), bufLen=*bufLenP;
    if (len + (pBufP - baseBufP) >= bufLen) //is there room in the buffer for this information ?
       {newBufP  = (char*)Kalloc(bufLen*=2);
        memcpy(newBufP, baseBufP, pBufP - baseBufP);
        pBufP   += (newBufP - baseBufP);    //relativize pBufP to new buffer
        if (bufLen != 2*(FORMAT_BUF_LEN-1)) Kfree(baseBufP); //free if not still pointing to static buffer
        baseBufP = *baseBufPP = newBufP;    //update base pointer
       }
    memmove(pBufP, msgP, len+1);
    *pBufPP  = pBufP + len;                 //update current pointer
    *bufLenP = bufLen;                      //update length
   } //SaveString...

static void DvprintfForGstring(const char *fmtP, char**baseBufPP, va_list arg)
   {char buf[FORMAT_BUF_LEN], *pBufP = buf, *baseBufP = buf, *pp; int bufLen = FORMAT_BUF_LEN-1, len;
    ::Dvprintf(fmtP, SaveString, &baseBufP, &pBufP, &bufLen, arg);
    len = strlen(baseBufP);
    if (bufLen != FORMAT_BUF_LEN-1)
         {pp = (char*)realloc(baseBufP, (len+1)*sizeof(char)); free(baseBufP);} //allocated baseBufP
    else {pp = (char*)malloc((1+len)*sizeof(char)); strlcpy(pp, baseBufP, len+1);}      //static baseBufP
    *baseBufPP = pp;
   } //DvprintfForGstring...
#endif //}

#ifdef _MSC_VER
    #define Vsnprintf _vsnprintf
#else
    #define Vsnprintf  vsnprintf
#endif

cGString cGString::Format(const char *formatP, ...)
   {
#ifndef NO_VA
    char         *baseBufP=NULL;
    int           size, len;
    va_list       arg;
    cGString      gstr;
    GSTRING_INFO *gsiP;

    if (!formatP) formatP = "";
    #ifdef GSTRING_FANCY_PRINTF //{ use the extended functionality of dprintf.
        DvprintfForGstring(formatP, &baseBufP, arg);
        len      = strlen(baseBufP);
        baseBufP = (char*)Krealloc(baseBufP, (size=len + 1) + sizeof(GSTRING_INFO));
        memmove(&baseBufP[sizeof(GSTRING_INFO)], baseBufP, size); //right shift baseBufP by sizeof(GSTRING_INFO).
    #else //} { conventional printf functionality
        //Otherwise allocate buffer SM_BUFSIZE, then LG_BUFSIZE
        for (size=SM_BUFSIZE, baseBufP=NULL; size <= LG_BUFSIZE+SM_BUFSIZE; size += LG_BUFSIZE)
            {baseBufP = (char*)realloc(baseBufP, size);
             baseBufP[size-1] = 0;
             va_start(arg, formatP);
             len      = Vsnprintf(&baseBufP[sizeof(GSTRING_INFO)], size-sizeof(GSTRING_INFO)-1, formatP, arg);
             va_end(arg);
             if (len >= 0 && len < (int)(size-sizeof(GSTRING_INFO)-1)) break; //some implementations return bytes needed.
             // _vsnprintf returns -1 if formatP string is larger than buffer
            }
    #endif //}
    //assign baseBufP to m_superP.
////baseBufP           = (char*)realloc(baseBufP, size=len+sizeof(GSTRING_INFO)+1); //hrgt: trim to proper size
    gstr.m_superP      = baseBufP+sizeof(GSTRING_INFO);
    gsiP               = gstr.Info();
    gsiP->allocSize    = size;
    gsiP->refs         = 1;
    gstr.m_superP[len] = 0;
    gstr.m_subP        = gstr.m_superP;
    gstr.m_size        = len;
    return gstr;
#endif
   } // cGString::Format...

// returns the number of replacements made
int cGString::Replace(const char *origP, const char *repP, bool ignoreCase) //=false
   {int      pos, start, count = 0, origSize = (int)strlen(origP), repSize = (int)strlen(repP), ii;
    cGString resultStr;

    if (!m_subP || !origP || !repP) return 0;
    for (pos=0, start=0; ; start = pos + origSize, count++)
       {if (!ignoreCase) pos = Find(origP, start);
        else
           for (ii=pos+start, pos=-1; ii <= GetLength() - origSize; ii++)
               if (_strnicmp(&m_subP[ii], origP, origSize) == 0) {pos = ii; break;} //using _strnicmp stop MsDev8 getting pissed off.
        if (pos < 0) break;
        resultStr.Reserve(resultStr.GetLength() + (pos - start) + repSize);
        if (!count) resultStr += Mid(0, pos);
        if (count != 0 && (pos - start) != 0)
        // copy everything between between the last and current matches
        resultStr += Mid(start, pos - start);
        resultStr += repP;
       }
    if (!count) return 0; // no changes
    resultStr += Mid(start);
    *this      = resultStr;
    return count;
   } //cGString::Replace...

cGString& cGString::TrimRight(const char* targets)
   {if (m_subP == NULL) return *this;
    while (m_size > 0)
        if (strchr(targets, m_subP[m_size-1]) == NULL)  break;
        else                                            m_size--;
    return *this;
   }
#if 0
cGString& cGString::GetName()
   {char         *baseP=m_subP;
    if (m_subP == NULL) return *this;

    while ((baseP = strpbrk(m_subP, ":/\\")) != NULL)
       {m_subP = &baseP[1];
       m_size = strlen(m_subP);
       }
    if ((baseP = strrchr(m_subP, '.')) != NULL)
       m_size -= strlen(baseP);
    return *this;
   }
#endif
cGString& cGString::TrimLeft(const char* targets)
   {size_t ii;
    if (m_subP == NULL) return *this;
    ii = strspn(m_subP, targets);
    m_size -= ii; m_subP += ii;
    return *this;
   }

// this could be sped up a little by telling PreWrite to allocate <size> bytes if it needs to make a copy
// maybe the two functions should even be merged
// m_subP == m_superP after this is called
void cGString::Reserve(int size) const
   {bool first = m_subP == NULL; // we'll init the info struct if this is true
    size_t offset;               // the offset of m_subP from m_superP

    if (size + 1 < 1) return;    // the +1 isn't incompetence insurance, it's needed because size doesn't include the null.
    PrePreWrite();               // i don't think there's ever a time we'd want Reserve without PreWrite
    // don't leave any wasted space at the beginning
    if (m_subP != m_superP)  {memmove(m_superP, m_subP, m_size);  m_subP = m_superP;  }
    if (m_superP && (int)Info()->allocSize >= (INT)(size + sizeof(GSTRING_INFO) + 1)) return; //already enough space

    offset = m_subP - m_superP;
    if (m_superP) m_superP -= sizeof(GSTRING_INFO);

    if (GSTRING_EXTRA_ALLOC > 0 && size < 1024)  size = size * (1 + GSTRING_EXTRA_ALLOC); //-1;
    size = size + 1 + sizeof(GSTRING_INFO);
    if (size & 0x0f) size = (size & 0xfffffff0) + 16;    // equiv to size = ((size-1) / 16) + 1)) * 16
    //gme: couldn't the 'long' cast just be a char* cast?
    m_superP = (char*) sizeof(GSTRING_INFO) + (size_t) realloc(m_superP, size);
    if (first)
       {m_subP       = m_superP;
        m_size       = 0;
        Info()->refs = 1;
       }
    else  m_subP = m_superP + offset;
    Info()->allocSize = size; // - sizeof(GSTRING_INFO); ; //hrgt:
   } //cGString::Reserve...

// if we're sharing data (refs > 1), make our own copy
// (this does the same things to the orignal string as Release, but leaves us with a copy)
void cGString::PreWrite() const
   {char*  old_sP;
    size_t newAllocSize;
    GSTRING_INFO* gstrInfoP;

    if (m_superP && (gstrInfoP = Info())->refs > 1)
       {old_sP       = m_subP;
        newAllocSize = m_size + 1 + sizeof(GSTRING_INFO);
        if (GSTRING_EXTRA_ALLOC > 0 && newAllocSize < 1024) newAllocSize = newAllocSize * (1 + GSTRING_EXTRA_ALLOC)-1;
        if (newAllocSize & 0x0f) newAllocSize = (newAllocSize & 0xfffffff0) + 16;
        gstrInfoP->refs--;
        m_superP = m_subP    = (char*) sizeof(GSTRING_INFO) + (size_t) malloc(newAllocSize);
        memcpy(m_subP - sizeof(GSTRING_INFO), old_sP - sizeof(GSTRING_INFO), m_size + sizeof(GSTRING_INFO));
        gstrInfoP            = Info(); // because m_superP changed
        gstrInfoP->allocSize = newAllocSize;
        gstrInfoP->refs      = 1;
   }   } //cGString::PreWrite...

// don't call this if m_superP isn't allocated!
//What possible value is this function. The same functionality is built into PreWrite.
void cGString::PrePreWrite () const {if (m_superP && Info()->refs > 1) PreWrite(); }  //Info()->refs

void cGString::CopyPart(const cGString& src, int offset /* = 0 */, int count /* = 0 */)
   {if (!count)     count = src.GetLength() - offset;
    if (count < 0)  return;
    if (!count || !src.GetLength())  {Release();  return;}
    src.Info()->refs++;
    m_superP = src.m_superP;
    m_size   = count;
    m_subP   = src.m_subP + offset;
   } //cGString::CopyPart...

//Return the allocated string and disown it.
char *cGString::Disown(void)
   {char *resultP;
    PreWrite(); //guarrantess we own the string
    memmove(resultP = m_superP-sizeof(GSTRING_INFO), m_subP, m_size);
    resultP[m_size] = 0;
    m_superP        = m_subP = NULL; m_size = 0; //disown
    return resultP;
   }

//Return -1 if strP is greater than this,
//       +1 if strP is less    than this,
// or     0 is strP is equal to this.
int cGString::CompareNoCase(const char* strP) const
   {size_t len, result;
    if (!strP) strP = "";
    if (!m_subP || *m_subP == 0)
       {if (!*strP)   return  0; //equal
                      return -1; //strP is greater
       }
    if ((result = _strnicmp(m_subP, strP, len=GetLength()))) return (int)result; //differ in early chars
    if((result = strlen(strP)) == len) return 0;
    return result > len ? -1 : +1;
   }

int cGString::GetFieldCount(char sep)
   {int count, i;
    char sepStr[] = {sep, '\0'};

    if (GetLength() == 0) return 0;
    for (i=-1, count=1; i < GetLength(); ++i, ++count)
        if ((i = Find(sepStr, i+1)) < 0) break;
    return count;
   } //cGString::GetFieldCount...

//Return the fieldIndex'th field in the string. A field is delimitted by the sep character.
//For example: "abc\tuvw\txyz".GetField('\t',2) == "uvw"
cGString cGString::GetField(char sep, int fieldIndex)
   {int   i, inx;
    char  sepStr[] = {sep, '\0'};

    if (fieldIndex < 0 || GetLength() == 0) return cGString();
    for (i=0, inx=-1; i < fieldIndex && inx < GetLength(); ++i)
        inx  = Find(sepStr, inx+1);
    int next = Find(sepStr, inx+1);
    next = (next >= 0 ? next : GetLength()) - inx - 1; //hrgt: -1
    return next == 0 ? "" : cGString(&m_subP[inx+1], next); //hrgt cGString(, 0) does not work is as expected.
   } //cGString::GetField...

int cGString::Compare(const char* strP) const
   {size_t len, result;
    if (!strP) strP = "";
    if (!m_subP || *m_subP == 0)
        {if (!*strP)  return  0; //equal
                      return -1; //strP is greater
        }
    if ((result=strncmp(m_subP, strP, len=GetLength())) != 0) return (int)result;
    if ((result=strlen(strP)) == len) return 0;
    return result > len ? -1 : +1;
   } //cGString::Compare...

cGString cGString::Left (int count) const  {return Mid(0, count);}
cGString cGString::Right(int count) const  {return Mid(Max(0, GetLength() - count), count);}

int cGString::GetLength() const   {return (int)(m_superP ? m_size : 0);}

const char* cGString::c_str() const {return (const char*) (*this);}

const cGString& cGString::operator=(const cGString& gsR)
    {if(*this != gsR)                                     // check for self assignment
        {if (!gsR.m_superP)            {Release(); m_superP = m_subP = NULL; m_size = 0; return *this;}
         if (m_superP != gsR.m_superP) {Release(); gsR.Info()->refs++; m_superP = gsR.m_superP;}
         m_subP = gsR.m_subP; m_size = gsR.m_size;
        }
     return *this;
    }

const cGString& cGString::operator=(const char* strP)
   {if (!strP || !*strP)
       {Release(); m_superP = m_subP = NULL; m_size = 0; return *this;}
    if (!strP) strP = "";
    size_t size = strlen(strP);

    Reserve((int)size); // calls PreWrite, which will Release the current data if it's shared
    strlcpy(m_subP, strP, size+1); m_size = size;
    return *this;
   }

// Wide char assignment into a GString
//const cGString& operator=(const unsigned short* wstrP);

const cGString& cGString::operator+=(const cGString& strR)
   {Reserve(GetLength() + strR.GetLength());
    memcpy(m_subP + GetLength(), strR.m_subP, strR.GetLength());
    m_size += strR.GetLength();
    return *this;
   }

const cGString& cGString::operator+=(const char *strP)
   {if (!strP) return *this;
    size_t  appLen = strlen(strP);

    Reserve((int)(GetLength() + appLen));
    memcpy(m_subP + GetLength(), strP, appLen);
    m_size += appLen;
    return *this;
   }

// these operator== functions should be checked for the cases where at least one string is empty
bool operator==(const cGString& strl, const cGString& strr)
   {if (strl.GetLength() == strr.GetLength() &&
        !strncmp(strl.m_subP, strr.m_subP, strl.GetLength()))
        return true;
    return false;
   }

bool operator==(const cGString& strl, const char* strr)
   {if (!strr) strr = "";
    size_t rLen = strlen(strr);
    if (strl.GetLength() == (int)rLen && !strncmp(strl.m_subP, strr, rLen)) return true;
    return false;
   }

bool operator==(const char* gslP,     const cGString& gsr) {return gsr == gslP;}
bool operator!=(const cGString& gsl,  const cGString& gsr) {return !(gsl == gsr);}
bool operator!=(const cGString& gsl,  const char* gsrP)    {return !(gsl == gsrP);}
bool operator!=(const char* gslP,     const cGString& gsr) {return !(gslP == gsr);}

// decrements the ref count and if it's zero, frees and zeros m_subP
void cGString::Release()
   {if (m_superP && --Info()->refs <= 0)
       {free(m_superP - sizeof(GSTRING_INFO));
        m_superP = m_subP = NULL; m_size = 0;
   }   } //cGString::Release...

#ifdef PICO_WIDGETS
   #include <wx/string.h>
   wxString cGString::wxStr() const {return wxString(m_subP, wxConvLibc, GetLength());}
#endif

//friend std::ostream & operator<<(std::ostream & os, const cGString& gstrR);

#ifdef _DEBUG
void TestGstring(void)
   {cGString cs1, cs2, cs3;
    const char *pp;
    cs1 = "a string";
    cs2 = cs1.Mid(3);
    pp  = (const char*) cs2;
    cs1 = "another string";
    //cs3.Format("field%s=\"%s\"", "_x", 14); // where was TestTestGstring when i needed it? :)
    cs3.Format("field%s=\"%i\"", "_x", 14);
   } //TestGstring...
#endif

//end of file
/* vim:set ts=4 sts=4 sw=4 et list nowrap: */
