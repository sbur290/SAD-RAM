#ifndef _CODE_TIMER_H_INCLUDED_
#define _CODE_TIMER_H_INCLUDED_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#if defined(_WIN32)
  #include "C3_types.h"
  #include "C3_always.h"
#else
  #define strnicmp            strncasecmp
  #define _strnicmp           strncasecmp
  #define stricmp             strcasecmp
  #define _stricmp            strcasecmp
  #include <unistd.h>
#endif
#include "..\sam-15\samOptions.h"
#include "C3_timeFncs.h"
#include "C3_codeTimer.h"

typedef struct
    {char         *nameP;         //name from user
     LARGE_INTEGER start;         //when timer[ii] started
     int64_t       delta;         //accumulated time, units = 100ns
     int           count;         //number of calls
    } TIMER_INFO;

const int cCodeTimerInfoSize=25;
class cCodeTimer
   {private:                                                                //
    double m_freq;                                                          //
    char   m_buf[96];                                                       //for Format()
    double Li2Double(int64_t ii);                                           //
    int    TimerLookup(const char *nameP);                                  //
    public:                                                                 //
    TIMER_INFO m_info[25];                                                  //
    cCodeTimer(const char *exeNameP);                                       //
   ~cCodeTimer();                                                           //
    int    TimerOn    (int which, const char *nameP);                       //
    int    TimerOff   (int which, const char *nameP);                       //
    double GetTimer   (const char *nameP);                                  //
    void   TimerShow  (const char *nameP, uint32_t maxCount=0);             //
    char  *TimerFormat(const char *nameP, bool shortB, uint32_t width=4);   //
    void   SetFileName(const char *exeNameP);                               //
  }; //cCodeTimer...

#ifdef USE_TIMERS
#define TIMER_ON(name)              g_timer_##name = g_timerP->TimerOn(g_timer_##name, #name)
#define TIMER_OFF(name)             g_timerP->TimerOff(g_timer_##name, #name)
#define GET_TIMER(name)             g_timerP->GetTimer(#name)
#define TIMER_SHOW(name)            g_timerP->TimerShow(#name)
#define TIME_THIS(name, statement) {g_timer_##name = g_timerP->TimerOn(g_timer_##name, #name); statement; g_timerP->TimerOff(g_timer_##name, #name);}
#define TIMER_FORMAT(name)          g_timerP->TimerFormat(#name)
#define TIMER_FMT(valu)             g_timerP->TimerFmt(valu)
#else
#define TIMER_ON(name)
#define TIMER_OFF(name)
#define TIMER_SHOW(name)
#define TIME_THIS(name, statement) {statement;}
#endif

#endif //_CODE_TIMER_H_INCLUDED_ ...
//end of file
