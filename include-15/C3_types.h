/* =============================================================================
    File: C3_types.h. Version 7. Aug 20, 2022.

    This file contains various types used throughout C3 software and some
    compatibility definitions between MSdev and Posix.
   =============================================================================
*/

#ifndef _C3_TYPES_H_INCLUDED_
#define _C3_TYPES_H_INCLUDED_

#if defined(_WIN32)
    #ifdef _MSC_VER
        #pragma warning(disable:4793)       //warning about vararg.
        #ifndef _WIN32_WINNT                //proxy for called from Pico2k project
            #define WIN32_LEAN_AND_MEAN
            #include <windows.h>
        #endif
    #endif
#else
    #if !defined(__KERNEL__)                //not building Linux driver
        #if !defined(_KERNEL)               //not BSD driver
//            #include "posix.h"
        #endif
//        #if !defined(__PPC__)
            #include <stdint.h>             // /dhl/ this should work for windows. It is the C99 standard include for intN_t/uintN_t ...
    #endif
  #define __cdecl
#endif

#ifndef PACKED
   #define PACKED
#endif
#ifdef _WIN32
  #pragma pack(push,1) //1byte packing
#endif

//****************************************
// this is just a workaround for MS typenames, and not pico-specific

#ifndef NULL
    #define NULL    0
#endif
// Under GCC in Linux the following is defined
// #define __linux__ 1
//
// Under any x86 versions of GCC
// #define __i386 1
// #define i386 1
// #define __i386__ 1
// #define __i486__ 1
// #define __i486 1

// non-ms types when we're in windows
// gme: please leave the !LINUX in here to make kernel modules work without defining POSIX
// dhl: building a linux kernel module should always define __KERNEL__
//      building OpenBSD kernel code always defines _KERNEL
//      building Darwin(Mac OS X) kernel code always defines KERNEL
// #if !defined(int32_t) && !defined(POSIX) && !defined(KERNEL) && !defined(_KERNEL) && !defined(__KERNEL__) //{
// #if !defined(int32_t) && (defined(_WIN32) || defined(__PPC__)) //{
#if !defined(int32_t) && (defined(_WIN32) && !defined(__PPC__)) //{
// /dhl/ it should be possible to pick all of these up with #include <stdint.h> - this should work for most everything - except maybe VC6.
    #ifndef uint32_t
        #ifndef _STDINT_H
        #define _STDINT_H    1
        // the __* types can't be automatically cast to an "equivalent" type like __int8 -> char!!!!!!!!!!!
        // xxx - figure out a way to make these sizes portable between 32 and 64-bit
            typedef unsigned char           uint8_t;
            typedef unsigned short          uint16_t;
            typedef unsigned int            uint32_t;
        #ifdef _MSC_VER //{
            typedef unsigned __int64        uint64_t;
        #else
            typedef unsigned long long      uint64_t;
        #endif // }_MSC_VER
        #endif /* stdint.h */
    #endif // !uint32_t
    #if defined(_MSC_VER)
        typedef signed   __int8             int8_t;
        typedef signed   __int16            int16_t;
        typedef signed   __int32            int32_t;
        typedef signed   __int64            int64_t;
    #else
        typedef signed long long            int64_t;
    #endif
#endif // int32_t

#if defined(_KERNEL)
    typedef unsigned long               uintptr_t;
#endif

//$/typedef uint64_t    pico_size_t;
//$/#define      FLASHROM_ADDR int64_t
//$/typedef uint32_t    pico_version_t;
//$/typedef uint32_t    pico_model_t;

#if defined(_WIN32) || (defined(POSIX) && !defined(__KERNEL__)) //proxy for called from Pico.sys project
#if !defined(_MINGW32MSVC)
    #define pico_dll_t HINSTANCE //should be typedef HINSTANCE   pico_dll_t; but not under VSE 7
#endif
#endif

#define PICO_PROGRESS_CALLBACK(name) int name(int progress, bool setRange, const char *msgP)

#ifdef _WIN32
  #pragma pack(pop)
#endif
#endif // _C3_TYPES_H_INCLUDED_

//end of file

