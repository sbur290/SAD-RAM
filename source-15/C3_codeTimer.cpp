/*---------------------------------------------------------------------------
-------------------- Timing Code --------------------------------------------
-----------------------------------------------------------------------------
 Following code is used to time segments of code using the macros
     TimerOn("userName")
 and TimerOff("userName").
 "userName" can be assigned arbitrarily.
 These accumulate the count and elapsed time in m_info[].delta and m_count[].count
 The calls to TimerShow return the total accumlated time and count.
 Time is accumulated in units of 100nSec (the precision of the high precision timer).
*/
#include "C3_codeTimer.h"

//Class constructor: initialize m_info, and m_freq, create csv file name from exeNameP
//Note: there is some strange bug in the allocation of m_info. It looks like a compiler bug; 
//      by allocating cCodeTimer::m_buf to be an exact multiple of 8 bytes the problem goes away !
//      It looks a lot like some screwup with packed/unpacked structures.
cCodeTimer::cCodeTimer(const char *exeNameP)
   {LARGE_INTEGER li;                                                           //
    QueryPerformanceFrequency(&li);                                             //clicks of high precision timer / second
    m_freq = Li2Double(*(int64_t*)&li);                                         //
    memset(m_info, 0, sizeof(m_info));                                          //
   } //cCodeTimer::cCodeTimer...

cCodeTimer::~cCodeTimer()
   {int ii;
    for (ii=HOWMANY(m_info); --ii >= 0;) free(m_info[ii].nameP);
  //free(m_info); m_info = NULL;
   } //cCodeTimer::TimerDeallocate...

//Return slot for specified name.
int cCodeTimer::TimerLookup(const char *nameP)
  {int ii;                                          //
   for (ii=1; ii < HOWMANY(m_info)-1; ii++)         //Howmany-1: haha
     {if (m_info[ii].nameP == NULL) break;          //
      if (stricmp(m_info[ii].nameP, nameP) == 0) return ii;
     }//for (ii=...                                 //
  m_info[ii].nameP = _strdup(nameP);                //may overuse last timer
  return ii;
  } //cCodeTimer::TimerLookup...

//conversion of uint64_t to double
double cCodeTimer::Li2Double(int64_t ii)
   {double dd;
    dd  = ((int)((ii >> 32) & 0xFFFFFFFF)) * 65536.0*65536.0;
    dd += ((int) (ii & 0xFFFFFFFF));
    return dd;
   } //cCodeTimer::Li2Double...

int cCodeTimer::TimerOn(int which, const char *nameP)
   {if (which == 0) which = TimerLookup(nameP);                 //
    QueryPerformanceCounter(&m_info[which].start);              //
    m_info[which].count ++;                                     //
    return which;                                               //
   } //cCodeTimer::TimerOn...

int cCodeTimer::TimerOff(int which, const char *nameP)
   {LARGE_INTEGER li;                                           //
   _int64         current, start;                               //
                                                                //
    QueryPerformanceCounter(&li);                               //
    if (which == 0) which = TimerLookup(nameP);                 //should never be called; which should be valid
    current                = *(_int64*)&li;                     //
    start                  = *(_int64*)&m_info[which].start;    //
    m_info[which].delta += current - start;                     //accumulate time since TimerOn()
    return which;                                               //
   } //cCodeTimer::TimerOff...

double cCodeTimer::GetTimer(const char *nameP)
   {return Li2Double(m_info[TimerLookup(nameP)].delta)/m_freq;}

char *cCodeTimer::TimerFormat(const char *nameP, bool shortB, uint32_t width)
   {double        seconds, tt;                                                      //
    const char   *fmtP;                                                             //
    int           len, which;                                                       //
    which   = TimerLookup(nameP);                                                   //
    seconds = tt = (double)m_info[which].delta/m_freq;                              //
    len     = shortB ? 0 : snprintf(m_buf, sizeof(m_buf), "%20s ", nameP);          //
    if (tt <= .001)   {fmtP = "%7.2f us,"; tt *= 1000000;} else                     //
    if (tt <= 1.0)    {fmtP = "%7.2f ms,"; tt *= 1000;}    else                     //
    if (tt <= 99999.0) fmtP = "%7.2f s, ";                 else                     //
                       fmtP = "%7.2e s, ";                                          //
    len += snprintf(&m_buf[len], sizeof(m_buf)-len, fmtP, tt);                      //
    if (shortB) return m_buf;                                                       //
    tt      = seconds/Max(m_info[which].count, 1);                                  //average
    if (tt <= .001)   {fmtP = " count=%*u, avg=%7.2f us"; tt *= 1000000;} else      //
    if (tt <= 1.0)    {fmtP = " count=%*u, avg=%7.2f ms"; tt *= 1000;}    else      //
    if (tt <= 99999.0) fmtP = " count=%*u, avg=%7.2f";                    else      //
                       fmtP = " count=%*u, avg=%7.2e";                              //
    snprintf(&m_buf[len], sizeof(m_buf)-len, fmtP, width, m_info[which].count, tt); //
    return m_buf;                                                                   //
   } //cCodeTimer::TimerFormat...

//Display specific timer, or all m_info if nameP == NULL.
void cCodeTimer::TimerShow(const char *nameP, uint32_t rcdCount)
   {int           hi, which, width;                                                 //
    if (nameP == NULL) {which = 0; hi = HOWMANY(m_info)-1;   }                      //
    else               {which =    hi = TimerLookup(nameP)-1;}                      //
    if (rcdCount >= 1000000) width = 8; else                                        //calculate field width of count
    if (rcdCount >=  100000) width = 6; else                                        //
                             width = 4;                                             //
    while (++which <= hi && (nameP=m_info[which].nameP) != NULL)                    //
       Printf("%s, \n", TimerFormat(nameP, false, width));                          //false = full format
   } //cCodeTimer::TimeShow...

//end of file
