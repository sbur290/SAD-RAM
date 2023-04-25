// C3_printf.cpp: July 2022.

#define _CRT_SECURE_NO_WARNINGS 1
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include "C3_timeFncs.h"
#include "C3_codeTimer.h"

extern FILE       *g_printFileP;

//printf with a few gimmicks and facility to copy to a file.
void Printf(char const *fmtP,...)
    {va_list arg;                                                               //
     char    buf[512], *bufP=buf, ch;                                           //
     int     len, first, l1=0, bufSize=sizeof(buf);                             //
     static bool lastWasLfB=true;                                               //
                                                                                //
     first = 0;                                                                 //
     if (fmtP[0] == CONDITIONAL_CRLF[0])                                        //
        {if (!lastWasLfB) {bufP[0] = '\n'; first = 1;}                          //insert lf (if necessary)
         fmtP++;                                                                //ignore always
        }                                                                       //
     va_start(arg, fmtP);                                                       //
     len = vsnprintf(&bufP[first], bufSize-first-1, fmtP, arg) + first;         //
     va_end(arg);                                                               //
     if (len < 0)                                                               //whoops
        {bufP    = (char*)calloc(10, sizeof(buf)); bufP[0] = buf[0];            //need more space
         bufSize = 10 * sizeof(buf);                                            //
         va_start(arg, fmtP);                                                   //
         len     = vsnprintf(&bufP[first], bufSize-2, fmtP, arg) + first;       //try again
         va_end(arg);                                                           //
         }                                                                      //
     if (len < 0) strcpy(&bufP[bufSize-3], "\n");                               //still failed
     bufP[bufSize-1]    = ch = 0;                                               //
     len                = (int)strlen(bufP);                                    //
     lastWasLfB         = bufP[len-1] == '\n';                                  //
 //  if (len <= 3) l1 = len; else l1 = len-2;                                   //short messages are all translated
 //  if ( options & NO_CTRL_OPTION)                                             //
 //     for (char *pp=bufP; pp < &bufP[l1]; pp++) if (*pp < ' ') *pp = '.';     //replace control chars
     printf("%s", bufP);                                                        //output to stdout
     if (g_printFileP) fwrite(bufP, len,1, g_printFileP);                       //output to fileOutP
     if (bufSize > sizeof(buf)) free(bufP);                                     //
    } //Printf...

//end of file...