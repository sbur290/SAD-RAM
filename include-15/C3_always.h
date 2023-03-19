/*============================================================================
     File: C3_always.h. Version 7. Aug 23, 2022.
     Defines and structures used everywhere.

     Programmer: Robert Trout
     Copyright C3 Memories, Inc., 2022. All rights reserved.

============================================================================ */
#ifndef _C3_ALWAYS_H_INCLUDED_
#define _C3_ALWAYS_H_INCLUDED_

#ifdef _DEBUG
   #define DEBUG_LETTER     "D"
#else
   #define DEBUG_LETTER     ""
#endif
#define ENTRY_POINT

#if defined(POSIX)
    #define isPosix()   1
#else
    #define isPosix()   0
#endif
#ifndef PACKED
  #define PACKED
#endif
#ifdef _WIN32
  #pragma pack(push,1) //1byte packing
#endif

#include "c3_types.h"
#ifdef _WIN32
  #include <tchar.h>
#else
  #include <linux/limits.h>
#endif

//pico_version_t stores version information for pico products.
// there are four numbers in a version number - eg 3.1.4.0
// each may range from 0-255 since they are eight bits.
// they are stored with major number in the most significant byte, so you can compare version numbers.
#define C3_VMAJOR             7 //major structural changes required to application code.
#define C3_VMINOR             0 //software compatible - requires recompilation.
#define C3_VREVISION          0 //binary compatible   -
#define C3_VCOUNTER           9 //bug fixes etc.
#define JVERSION  ((((((C3_VMAJOR << 8) + C3_VMINOR) << 8) + C3_VREVISION) << 8) + C3_VCOUNTER)

//fancy footwork to convert binary to ascii representation
#define JVERSION_ASCII  C3_BASE_VERSION_STRING1(C3_VMAJOR,    \
                                                  C3_VMINOR,    \
                                                  C3_VREVISION, \
                                                  C3_VCOUNTER)
#define C3_BASE_VERSION_STRING0(a,b,c,d) #a "." #b "." #c "." #d
#define C3_BASE_VERSION_STRING1(a,b,c,d) PICO_BASE_VERSION_STRING0(a,b,c,d)

#define CONDITIONAL_CRLF "\x80"

//Fundamental link between source and object code
typedef struct {uint16_t pc, fileNum, lineNum, srcOffset;} sSRC_REF;

//JET from always.h 1/10/12
#if defined (_MSC_VER)
    #define strncasecmp strnicmp
    #if _MSC_VER < 1500 // vc 9.0
        #define vsnprintf   _vsnprintf
    #endif
    #define snprintf        _snprintf
#endif
#ifndef _WIN32
    #ifndef PICO_WIDGETS
       #define _T(t) (t)
    #endif
    #define _snprintf_s      snprintf
    #define  snprintf_s      snprintf
    #define  sprintf_s       snprintf
    #define  PICO_HANDLE     int
    #define  IHMODULE        int
    #undef   INVALID_HANDLE_VALUE
    #define  INVALID_HANDLE_VALUE ((int)-1)
    #define  MAX_PATH         260 //4096
    #define  MAX_FILENAME_LENGTH 260 //4096
    #define  WINAPI
    #define  PVOID            void *
    #define  strnicmp         strncasecmp
    #define  stricmp          strcasecmp
    typedef unsigned long ULONG;
    DWORD    GetLastError     (void);
#else
    #define  C3_HANDLE  HANDLE
#endif

 inline int Min        (int a, int b) {return a < b ? a : b;}
 inline int Max        (int a, int b) {return a > b ? a : b;}
 inline uint64_t Max64 (uint64_t a, uint64_t b) {return a > b ? a : b;}
 inline uint64_t Min64 (uint64_t a, uint64_t b) {return a < b ? a : b;}
 #define istrlen (int)strlen

#ifdef WIN32
    #define MilliSleep      Sleep //for the unwary: Under windows this is more like 2 milli second sleep !!!!
#elif defined(POSIX)
    #define MilliSleep(t)   usleep((__useconds_t)(1000*(t)))
#elif defined(__linux__)
    #define MilliSleep(t)   usleep((__useconds_t)(1000*(t)))
#elif defined(posix)
    #define MilliSleep(t)   usleep((__useconds_t)(1000*(t)))
#else
    #error MilliSleep undefined in always.h
#endif

#define ENTRY_POINT             //signals a routine that is called from outside current module.
#define ENTRY                   //signals a routine that is called from outside current module.
#define ALPHABETICS             "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz"
#define NUMBERS                 "0123456789"
#define HEXNUMBERS              NUMBERS "ABCDEFabcdef"
#define ALPHANUMERICS           ALPHABETICS NUMBERS
#ifndef HOWMANY
   #define HOWMANY(a) ((int)(sizeof(a)/sizeof((a)[0])))
#endif

#if defined(_WIN32)
    #define SIZE_FN     _MAX_PATH
    #undef PATH_MAX
    #define PATH_MAX    _MAX_PATH
    #define isWin()     1
    #define DLL         "dll"
    #define EXE         "exe"
    #define DOTEXE      ".exe"
    #define SHELL       "cmd.exe"
    #define SHELLEX     "cmd.exe /C"
#else
    #define SIZE_FN     PATH_MAX
    #define O_BINARY    0       // for open()
    #define isWin()     0
    #define PICODLL     "pico.so"
    #define DLL         "so"
    #define EXE         ""
    #define DOTEXE      ""
    #define SHELL       "bash"
    #define SHELLEX     "bash"
    #define MessageBoxA(nul, msg, title, key) printf("%s\n", msg)
#endif
//!JET

#if defined(_WIN32)
 #pragma warning(disable:4996)
 #pragma warning(disable:4296) //if (expression true)
#endif

//recent versions of GCC have deliberately limited offsetof and recommend the following as a replacement /dhl/
#define Offsetof(s,m) ((uint32_t)((char*)&(((s *)0)->m)-(char*)0))
#define isizeof(a)          ((int)sizeof(a))

//Following macros turn a name into a string value, eg
// #define THING 1234, then STRINGIFY(THING) == "1234"
#define STRINGIFY(s)  STRINGIFY1(s)
#define STRINGIFY1(s) #s

#ifdef _WIN32
  #pragma pack(pop)
#endif

#define HERE Printf("%s: %s line %04d ****\n", __FILE__, __FUNCTION__,  __LINE__);
void Printf(char const *fmtP,...);

#endif //_C3_ALWAYS_H_INCLUDED_

//end of file
