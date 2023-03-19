#ifndef _PICO_TIMEFNCS_H_INCLUDED_
#define _PICO_TIMEFNCS_H_INCLUDED_

#define _CRT_SECURE_NO_WARNINGS 1
#include "C3_types.h"
#include "C3_always.h"
#ifndef WIN32
  #include <sys/stat.h>
#endif

#define MILLISECS_PER_SEC   ((uint64_t)1000)                                // :)
#define MICROSECS_PER_SEC   ((uint64_t)1000000)                             // :)
#define TV2SECONDS(a)       ((uint32_t)((a)/MICROSECS_PER_SEC))             //convert between TIMEVALUE to seconds
#define TV2MILLISECONDS(a)  ((uint32_t)((a)/1000))                          //convert TIMEVALUE to milliseconds
#define MILLISECONDS2TV(a)  ((int64_t)((a)*1000))                           //convert milliSecs to TIMEVALUE
#define SECONDS2TV(a)       ((int64_t)((a)*MICROSECS_PER_SEC))              //convert seconds to TIMEVALUE
#define SECS_PER_DAY        (60*60*24)                                      // :)
#define SECS_PER_HOUR       (60*60)                                         // :)
#define MICROSECS_PER_DAY   (((uint64_t)SECS_PER_DAY)*MICROSECS_PER_SEC)    // :)
#define MILLISECS_PER_DAY   (((uint64_t)SECS_PER_DAY)*1000)                 // :)
#define DAYS_PER_FOUR_YEARS (4*365+1)                                       // :)

class cTIMEVALUE;
class cCalendarTime
   {public:
    uint32_t m_year, m_month, m_day, m_hour, m_minute, m_second, m_microSec;
    cCalendarTime         ();
    char      *Format      (char *bufP, int bufSize, const char *fmtP=NULL);    //format as directed using /%Y/%M/%D %H:%M:%S%. (%.=microsecs)
    cTIMEVALUE GetTime     (char **ppp);                                        //get time from ascii string and update *ppp
    cTIMEVALUE GetDate     (char **ppp);                                        //get date from ascii string and update *ppp
    cTIMEVALUE GetDateTime (char **ppp);                                        //get date & time from ascii string and update *ppp
    void       Init        (void);                                              //
    bool       SetCalendarTime(const cTIMEVALUE*);                              //convert from timeValue to calendar time
    bool operator==        (const cCalendarTime &ct) const;                     //
    bool operator!=        (const cCalendarTime &ct) const;                     //
    private:
    void GetTriple         (char **ppP, uint32_t *aP, uint32_t *bP, uint32_t *cP, uint32_t *msecP);
   };

class cTIMEVALUE
   {private:
    static int64_t m_nSecondsSince1601;                         //after initialization this is r/o. Units = 100 nSecs
    static   int   m_biasMinutes;                               //UTC = local + bias
    public:                                                     //
    int64_t        m_time;                                      //contains number of microseconds since 1/1/2000.
    cTIMEVALUE             ();                                  //
    bool       TimeValue   (const cCalendarTime *);             //Convert from CalendarTime to timeValue
    char      *Format      (char *outP, int outSize, const char *fmtP=NULL); //%M/%D/%Y:%H:%M:%S%. or %numberM etc. %.=microsecs
    char      *FormatUTC   (char *outP, int outSize, const char *fmtP);      //
    cTIMEVALUE GetFileGmtv (const char *fNameP, int which);     //w=0 file create time, w=1 file access time, w=2 file write time
    cTIMEVALUE GetGmtvBase (void);                              //set GMT and return said value
    cTIMEVALUE GetGmtv     (void);                              //return high precision timer
   };
#endif //_PICO_TIMEFNCS_H_INCLUDED_

ENTRY_POINT void TimerDeallocate(void);
//end of file
