/*========================================================================================
 File: c3_timeFncs.cpp. Version 7. Aug 8, 2022.
 Routines that support cTIMEVALUE, cCalendarTime.

 This code uses the high precision timer under Windows with precision to the 100nSec level.
 Adjustments set m_time in the cTIMEVALUE class to micro seconds since 1/1/2000.

 Programmer: Robert Trout.
 Copyright C3 Memories, Inc., 2022. All rights reserved.
==========================================================================================
Changes:
Local time in Calendar_time
*/

#define _CRT_SECURE_NO_WARNINGS 1
#pragma warning(disable:4100) //unreferenced parameter
#pragma warning(disable:4706) //assignment within an if ()

void Printf(char const *fmtP,...);

#undef Yield
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#if defined(_WIN32)
  #include <io.h> //for GetFileGmt()
  #include <fcntl.h>
#else
  #include <sys/time.h>
  #define strnicmp            strncasecmp
  #define _strnicmp           strncasecmp
  #define stricmp             strcasecmp
  #define _stricmp            strcasecmp
  #include <unistd.h>
#endif
#include "C3_timeFncs.h"

//Convert to/from yy/mm/dd, hh:mm:ss, microSeconds to cTIMEVALUE.
//TIMEUNITS are the microseconds since 1/1/2000 represented as a uint64_t.
//day and month are one based.
//year must be between 2000 & 2099
//hours, minutes, seconds and microSecs are zero based.
static uint8_t  DayMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
bool cTIMEVALUE::TimeValue(const cCalendarTime *ctP)
    {uint32_t days, yr=0, ii, minutes;

     if (ctP->m_year >= 2000) yr = ctP->m_year-2000;
     //'validate' year (i.e. between 2000 and 2199)
     if (yr > 199 || yr < 0)                                    return false;
     //validate day and month with adjustment for Feb and leap year.
     if (ctP->m_month < 1 || ctP->m_month > 12 || ctP->m_day < 1 ||
         (uint32_t) ctP->m_day > (uint32_t) DayMonth[ctP->m_month-1]
                                     +(ctP->m_month==2 && ((yr & 3) == 0)) && yr != 2100-2000)
                                                                return false;
     //validate hh:mm:ss
     if (ctP->m_hour > 24 || ctP->m_minute > 59 || ctP->m_second > 59)return false;
     //validate microseconds
     if (ctP->m_microSec >= 1000000)                              return false;

     days = yr * 365
            + ((yr+3) / 4)                             // plus leap year
            - (yr >= 2100-2000)                        // 2100 is not a leap year
            + ctP->m_day-1;                            // day is one based
     for (ii=1; ii < ctP->m_month; ii++) days += DayMonth[ii-1]; //append months
     // before Feb and a leap year, days has been incremented by (yr+3)/4
     if (ctP->m_month > 2 && ctP->m_year % 4 == 0 && ctP->m_year != 2100) days++;
     minutes = (days*24 + ctP->m_hour)*60 + ctP->m_minute;
     m_time  = (((uint64_t)minutes)*60 + ctP->m_second) *1000000 + ((uint64_t) ctP->m_microSec);
     return true;
    } // cTIMEVALUE::TimeValue...

//Timevalue to Calendar Time
bool cCalendarTime::SetCalendarTime(const cTIMEVALUE *tvP)
   {uint32_t    days, yr, daysThisMonth, mnth, seconds;
    uint64_t    second64;

    second64    = tvP->m_time / MICROSECS_PER_SEC;
    m_microSec  = (uint32_t) (tvP->m_time - second64 * MICROSECS_PER_SEC);
    days        = (uint32_t) (second64/SECS_PER_DAY); seconds  = (uint32_t)(second64 % SECS_PER_DAY);
    m_hour      = seconds/SECS_PER_HOUR;            seconds %= SECS_PER_HOUR;
    m_minute    = seconds/60;                       seconds %= 60;
    m_second    = seconds;
    yr          =(days / DAYS_PER_FOUR_YEARS)*4;   //# four year cycles
    days        = days % DAYS_PER_FOUR_YEARS;      //remaining days
    if (days >= 366 + 2*365) {days -= 366 + 2*365; yr+=3;} else
    if (days >= 366 + 1*365) {days -= 366 + 1*365; yr+=2;} else
    if (days >= 366 + 0*365) {days -= 366 + 0*365; yr+=1;}
    yr        = m_year = yr + 2000;                //get year offset properly
    for (mnth=1; mnth <= HOWMANY(DayMonth); mnth++)
       {daysThisMonth = DayMonth[mnth-1];
        if (mnth == 2 && (yr & 3) == 0 && yr != 2100) daysThisMonth++; //Feb adjustment for leap year
        if (days < daysThisMonth) break;
        days -= daysThisMonth;
       }
    m_month  = mnth;
    m_day    = days+1;
    return true;
   } // cCalendarTime::CalendarTime...

//Format based on format string, eg.,
// %M/%D/%Y:%H:%M:%S%.    Format as MM/DD/YY:HH:MM:SS.<fraction of second> 
// %M/%D/%Y at %H:%3M:%3S Format as MM/DD/YY at HH:MMM:SSS
//Note: time is GMT, not local time.
char *cCalendarTime::Format(char *outP, int outSize, const char *fmtP)
   {char        *bP;
    int         ii=0, len;
    bool        hourB=false;
    outP[--outSize] = 0;
    if (!fmtP) fmtP = "%Y/%M/%D %H:%M:%S%.";
    for (bP=outP; ; )
        {if (--outSize < 2) break;
         if (*fmtP == 0)    break;
         if ((*bP = *fmtP) != '%') {bP++; fmtP++; continue;}
         if (*++fmtP >= '0' && *fmtP <= '9') len = strtol(fmtP, (char**)&fmtP, 10); else len = 2;
         switch (*fmtP++)
            {case 'y': case 'Y': ii = m_year; if (len < 4) ii -= 2000;    hourB = false; break;
             case 'm': case 'M': if (*fmtP == ':' || hourB) ii = m_minute; else ii = m_month; break;
             case 'd': case 'D': ii = m_day;                              hourB = false; break;
             case 'h': case 'H': ii = m_hour;                             hourB = true;  break;
             case 's': case 'S': ii = m_second;                           hourB = false; break;
             case '.':           ii = m_microSec; *bP++ = '.'; outSize--; hourB = false; break;
             case '%':           if (outSize <= (len=1)) break;
                                 *bP++ = '%'; outSize--; *bP = 0;         hourB = false; continue;
            }
         if (outSize <= len+1) break;
         snprintf(bP, outSize, "%0*u", len, ii); ii = (int)strlen(bP); outSize -= ii; bP += ii;
        }
    len = (int)strlen(fmtP); if (outSize > len+1) strcat(bP, fmtP);
    return outP;
   } //Format...

//Format time to buf: mm/dd/yyyy at hh:mm:ss.micSec (local time)
//if yymmdd is zero just output time, if hhmmss.lll is zero just output date.
char *cTIMEVALUE::Format(char *outBufP, int bufSize, const char *fmtP)
   {m_time -= SECONDS2TV(60*m_biasMinutes); return FormatUTC(outBufP, bufSize, fmtP);}

//Format time to buf: mm/dd/yyyy at hh:mm:ss.micSec (UTC time)
char *cTIMEVALUE::FormatUTC(char *outP, int outSize, const char *fmtP)
   {cCalendarTime ct; ct.SetCalendarTime(this); return ct.Format(outP, outSize, fmtP);} //cTIMEVALUE::FormatUTC...

// xxx - gme: look over all this time stuff again.
// xxx - this should really be called m_nSecondsSinceEpoch
//   AND ITS NOT EVEN NANOSECONDS!!!
int64_t cTIMEVALUE::m_nSecondsSince1601=0;
int     cTIMEVALUE::m_biasMinutes=0;

#if defined(_WIN32) //{
cTIMEVALUE::cTIMEVALUE()
   {TIME_ZONE_INFORMATION timeZoneInformation; int timeZone;
    m_time               = -1;                                  //invalid
 // if (m_nSecondsSince1601 != 0) return;                       //
    m_nSecondsSince1601  = (2000-1601)*365+                     //days 1/1/1601 to 1/1/2000
                           (2000-1601)/4;                       //plus leap days
    m_nSecondsSince1601 -= 3;                                   //1700, 1800 and 1900 were not leap years
    m_nSecondsSince1601  = m_nSecondsSince1601 *(24*60*60);     //seconds per day
    m_nSecondsSince1601 *= 10000000;                            //100 nSec / second
    timeZone             = GetTimeZoneInformation(&timeZoneInformation);
    if (timeZone == TIME_ZONE_ID_STANDARD) m_biasMinutes = timeZoneInformation.Bias + timeZoneInformation.StandardBias;
    if (timeZone == TIME_ZONE_ID_DAYLIGHT) m_biasMinutes = timeZoneInformation.Bias + timeZoneInformation.DaylightBias;
   } //cTIMEVALUE::cTIMEVALUE...

cTIMEVALUE cTIMEVALUE::GetGmtvBase(void)
    {FILETIME       ft;
     GetSystemTimeAsFileTime(&ft);
     m_time = (*(int64_t*)&ft - m_nSecondsSince1601) / 10; //convert to microseconds
     return *this;
    } //GetGmtv...

//Use performance counter clicks to fill in the 10ms precision of above timer
cTIMEVALUE cTIMEVALUE::GetGmtv(void)
   {cTIMEVALUE            tv;
    LARGE_INTEGER         li;
    static LARGE_INTEGER  baseCounters;
    static cTIMEVALUE     baseTime;
    static double countersPerUsec = 0;

    if (baseTime.m_time < 0) //ie just initialized
       {QueryPerformanceFrequency(&li);
        countersPerUsec = ((double)li.QuadPart) / 1000000.0;
       }
    QueryPerformanceCounter(&li);
    tv.GetGmtvBase();
    if (baseTime.m_time != tv.m_time)
       {baseCounters = li;
        baseTime.m_time= tv.m_time;
       }
    m_time = baseTime.m_time + (int64_t)((li.QuadPart - baseCounters.QuadPart) / countersPerUsec);
    return *this;
   } //cTIMEVALUE::GetGmtv...

#ifdef _WIN32
  //Return cTIMEVALUE containing create/update/access time of fileNameP
  //which==0, return file creation Time,
  //which==1, return file last Access Time,
  //which==2, return file last Write Time
  cTIMEVALUE cTIMEVALUE::GetFileGmtv(const char *fileNameP, int which)
   {FILE        *fileP=fopen(fileNameP, "rb");
    FILETIME     ft[4]={0,0,0,0};
    if (fileP)
       {if (GetFileTime((HANDLE)_get_osfhandle(_fileno(fileP)), &ft[0], &ft[1], &ft[2]))
           {m_time = (*((int64_t*)&ft[which&3]) - m_nSecondsSince1601) / 10;} //convert 100nSecs to microseconds
        fclose(fileP);
       }
    else m_time = 0;
    return *this;
   } //cTIMEVALUE::GetFileGmtv...
#else
  cTIMEVALUE cTIMEVALUE::GetFileGmtv(const char *fNameP, int w)
   {m_time = 0; return *this;}
#endif
#else //}...{
cTIMEVALUE::cTIMEVALUE()
   {m_time               = -1;                              //invalid
    if (m_nSecondsSince1601 == 0)
       {m_nSecondsSince1601  = (2000-1970)*365+                 //days 1/1/1970 to 1/1/2000
                               (2000-1970)/4;                   //plus leap days
        m_nSecondsSince1601  = m_nSecondsSince1601 *(24*60*60); //seconds per day
        m_nSecondsSince1601 *= 10000000;                        //100 nSec / second
        #if 0 // xxx - get posix timezone
        if (GetTimeZoneInformation(&timeZoneInformation))
           {m_biasMinutes = timeZoneInformation.Bias;}
        #else
        m_biasMinutes = 0;
        #endif
   }   }

// returns the number of microseconds since 1/1/2000 (utc)
cTIMEVALUE cTIMEVALUE::GetGmtv(void)
   {timeval tv;
    m_time = -1;
    if (gettimeofday(&tv, NULL) == 0)
        m_time = (tv.tv_sec * 1000000) + tv.tv_usec;
    return *this;
   }
//JET
  //Return cTIMEVALUE containing create/update/access time of fileNameP
  //w==0, return creation Time,
  //w==1, return last Access Time,
  //w==2, return last Write Time
cTIMEVALUE cTIMEVALUE::GetFileGmtv(const char *fileNameP, int w)
{
    FILE        *fileP=fopen(fileNameP, "rb");
    if (fileP)
    {
        stat(fileNameP, &stbuf);
        switch (w & 3)
        {
            case 0:            m_time = 0; break;                                                          //JET linux filesystems do not keep a file's creation time. o.0
            case 1:            m_time = stbuf.st_atim.tv_nsec / 1000 - (m_nSecondsSince1601 * 100); break; //linux timestamps claim to have nanosecond precision
            case 2: default:   m_time = stbuf.st_mtim.tv_nsec / 1000 - (m_nSecondsSince1601 * 100); break;
    }    }
    else m_time = 0;
    return *this;
} //cTIMEVALUE::GetFileGmtv...
//!JET

#endif // !_WIN32 }

void cCalendarTime::GetTriple(char **ppP, uint32_t *aP, uint32_t *bP, uint32_t *cP, uint32_t *msecP)
   {char *pp=*ppP;
                                    *aP = strtol(pp+0, &pp, 10);
    if (strchr(":/-", *pp) && *pp)  *bP = strtol(pp+1, &pp, 10);
    if (strchr(":/-", *pp) && *pp)
       {*cP = strtol(pp+1, &pp, 10);
        if (*pp == '.' && msecP) *msecP = (int)(strtod(pp, &pp) * MICROSECS_PER_SEC);
       }
    *ppP = pp;
   } //cCalendarTime::GetTriple...

ENTRY_POINT cCalendarTime ThisTime;    // time of last call to Rosetta Initialize

//Get Time of day only from text at *ppP, update *ppP
cTIMEVALUE cCalendarTime::GetTime(char **ppP)
   {cTIMEVALUE tv;
    memset(this, 0, sizeof(cCalendarTime));
    m_month = 1; m_day = 1; m_year = ThisTime.m_year;
    GetTriple(ppP, &m_hour, &m_minute, &m_second, &m_microSec);
    if (!tv.TimeValue(this)) tv.m_time  = -1; else
                             tv.m_time %= MICROSECS_PER_DAY;
    return tv;
   } //cCalendarTime::GetTime...

//Get Date only from text at *ppP, update *ppP
cTIMEVALUE cCalendarTime::GetDate(char **ppP)
   {cTIMEVALUE tv;
    memset(this, 0, sizeof(cCalendarTime));
    m_month = 1; m_day = 1; m_year = ThisTime.m_year;
    GetTriple(ppP, &m_month, &m_day, &m_year, NULL);
    if (!tv.TimeValue(this)) tv.m_time = -1; else
                             tv.m_time = (tv.m_time / MICROSECS_PER_DAY) * MICROSECS_PER_DAY;
    return tv;
   } //cCalendarTime::GetDate...

//Get Time and Date from text at *ppP, update *ppP
cTIMEVALUE cCalendarTime::GetDateTime(char **ppP)
   {char *pp=*ppP; cTIMEVALUE tv;
    Init(); m_year = ThisTime.m_year;
    GetTriple(&pp, &m_month, &m_day,    &m_year,    NULL);
    pp += 1+strspn(pp+1, " ");
    if (_strnicmp(pp, "at ", 3) == 0) pp += 3;
    GetTriple(&pp, &m_hour,  &m_minute, &m_second, &m_microSec);
    if (!tv.TimeValue(this)) tv.m_time = -1;
    *ppP = pp;
    return tv;
   } //cCalendarTime::GetDateTime...

bool cCalendarTime::operator==(const cCalendarTime &ct) const
   {return m_year     == ct.m_year && m_month  == ct.m_month  && m_day    == ct.m_day    &&
           m_hour     == ct.m_hour && m_minute == ct.m_minute && m_second == ct.m_second &&
           m_microSec == ct.m_microSec;
   } //cCalendarTime::operator==

bool cCalendarTime::operator!=(const cCalendarTime &ct) const {return !operator==(ct);}

void cCalendarTime::Init(void) {m_year = m_month = m_day = m_hour = m_minute = m_second = m_microSec = 0;}
cCalendarTime::cCalendarTime() {Init();}

#ifdef _DEBUG //{
//Try all dates between 1/1/2000 and 12/31/2099
void TestTimeConversion(void)
  {int64_t        lastDayNo=-1;
   char           buf[25]="12/25/2001:23:59:59", *pp=buf;
   cTIMEVALUE     tv;
   cCalendarTime ct, nt;

   ct.GetDateTime(&pp);
   for (ct.m_year=2000; ct.m_year < 2100; ct.m_year++)
      for (ct.m_month=1; ct.m_month <= 12; ct.m_month++)
         for (ct.m_day=1; ct.m_day <= (uint32_t) DayMonth[ct.m_month-1]+
                       ((ct.m_year & 3)==0 && ct.m_month==2); ct.m_day++, lastDayNo+= (24*60*60*MICROSECS_PER_SEC))
           {if (!tv.TimeValue(&ct))                 goto err;
            if (!nt.SetCalendarTime(&tv))           goto err;
            if (lastDayNo < 0) lastDayNo = tv.m_time;
            if (nt != ct || tv.m_time != lastDayNo) goto err;
           }
   return;
err:
   Printf("Error in cTIMEVALUE %02u/%02u/%04u\n", ct.m_month, ct.m_day, ct.m_year);
   return;
  } //TestTimeConversion...
#endif // } _DEBUG

//end of file...
