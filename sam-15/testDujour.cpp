#include "samApp.h"
#include "sam-15.h"
#include <math.h>
#include <conio.h>
#include "samHelper.h"

#if 0
//Test of qsort for a nice comparison
int QsortCompare(CV aV, CV bV)
   {uint8_t *aP=((uint8_t*)aV) + offsetof(sRECORD, key),
            *bP=((uint8_t*)aV) + offsetof(sRECORD, key);
    return memcmp(aP, bP, sizeof(sRECORD::key));
   } //SortCompare...

//sam /j use qsort and print timing:
//sam  1,000,000 /j == 0.82 seconds
//sam 10,000,000 /j == 8.22 seconds
void TestDujour(SAM_CONTROLS *ctlP, int nu)
   {uint32_t rcdNum;                                                            //
    time_t   qTime;                                                             //
    sRECORD temp={0}, *dataP = new sRECORD[ctlP->rcdCount];                     //underlying data records
    qTime = clock();                                                            //
    for (rcdNum=0; rcdNum < ctlP->rcdCount; rcdNum++)                           //
        {g_generateP->GenerateData(ctlP, rcdNum, sizeof(sRECORD), &temp);       //
         memmove(&dataP[rcdNum], &temp, sizeof(sRECORD));                       //write record
        }                                                                       //
    qsort(dataP, ctlP->rcdCount, sizeof(sRECORD), &QsortCompare);               //
    qTime = clock() - qTime;                                                    //
    Printf("rcdCount=%d, qsort time=%2.2f\n", ctlP->rcdCount, ((float)(qTime))/CLOCKS_PER_SEC);
    free(dataP);                                                                //
   } //TestDujour...
#elif 0
void TestDujour(SAM_CONTROLS *ctlP, int nu)
   {char buf[90];
    cTIMEVALUE     tv; tv.GetGmtv();
    cCalendarTime ct; ct.SetCalendarTime(&tv);
    char *pp=ct.Format(buf, sizeof(buf), "now it is %Y/%M/%D %H:%M:%S%. GMT");
   } //TestDujour...
#elif 0
void TestDujour(SAM_CONTROLS *ctlP, int nu)
   {uint8_t present[4]={0}; int pSz, rcdCnt;
    for (rcdCnt=1; rcdCnt < 32; rcdCnt++)
       {pSz = (rcdCnt+7)/8;
        *(uint32_t*)&present = (1 << rcdCnt) - 1;
        if (present[pSz-1] != (1 << (rcdCnt&7))-1 && (rcdCnt & 7) != 0)   goto bad; //last location unless exact multiple of 8 rcds were writ
        if (rcdCnt == 8 && present[0] != 0xFF)                            goto bad; //Special case 8
        if (rcdCnt > 8)                                                             //
           {if (present[0] != 0xFF)                                       goto bad; //
            if (memcmp(present, &present[1], pSz-2) != 0)                 goto bad; //is the remainder all 0xFF ?
           }
        Printf("present::%d ok\n", rcdCnt);
       }
    return;                                                                         //
bad:Printf("present:%d failed ****\n");
   } //TestDujour...
#elif 1
const int tableSize=32, radix=16;
//Using a classifier for divide.
typedef struct {uint16_t value, dividend;} sDIVISORS;

static int DivSequencer(const sDIVISORS *divP, uint16_t key)
   {for (int ii=1; ii < tableSize; ii++)                                         //for each item in this vector
         if (key < divP[ii].value) return ii-1;                                  //
    return tableSize-1;                                                                    //not found condition
   } //cSadram::DivSequencer...

//DivideBy==171 example.
void TestDujour(SAM_CONTROLS *nuP, int nu)
   {sDIVISORS ra[tableSize];
    int      countRa;
    uint16_t u16, ii, jj, lo, hi, step, rslt, rr, divideBy=171;
    //create the lookup table: 1-9 * divideBy, 10-90, 100-900, 1000-9000 * divideBy
    ra[0].value = 0; ra[0].dividend = 0;
    for (lo=step=1, hi=radix, countRa=1; ; lo*=radix, hi*=radix, step*=radix)
       {for (ii=lo; ii < hi; ii+=step)
            {ra[countRa].value    = divideBy*ii;
             ra[countRa].dividend = ii;
             if (++countRa >= HOWMANY(ra)-1) goto tri;
       }    }
tri:ra[countRa].value = (0xFFFF/divideBy)*divideBy; ra[countRa].dividend = 0xFFFF/divideBy;
    //Try some divides of random values
    for (jj=0, u16=0xFFFF; jj < 1000; jj++, u16=rand())
        {rslt = 0; rr = u16;
         ii   = DivSequencer(ra, rr); rslt |= ra[ii].dividend; rr -= ra[ii].value;
         ii   = DivSequencer(ra, rr); rslt |= ra[ii].dividend; rr -= ra[ii].value;
         ii   = DivSequencer(ra, rr); rslt |= ra[ii].dividend; rr -= ra[ii].value;
         Printf("%5d / %5d = %5d, rmdr=%5d", u16, divideBy, rslt, rr);
         if ((u16 /divideBy) == rslt && (u16 % divideBy) == rr) Printf(" (ok)\n");
         else                                                   Printf(" (wrong)\n");
        }
    return;
   } //TestDujour...
#elif 0
//Extractor treatment of floats and doubles
int CompareMSB(const void *aV, const void *bV, int keySz)
   {uint8_t *aP=((uint8_t*)aV), *bP=((uint8_t*)bV);                             //typecast aV and bV
    //Compare aP against bP                                                     //
    for (; keySz-- > 0; aP++, bP++)                                             //
      {if (*aP < *bP) return -1;                                                //return -1 if aP < bP
       if (*aP > *bP) return +1;                                                //return +1 if aP > bP
      }                                                                         //
    return 0;                                                                   //return 0  if aP == bP
   } //CompareMSB...

void TestDujour(SAM_CONTROLS *ctlP, int ii)
   {double   aF, bF;                                                            //
    uint64_t a64, b64;                                                          //
    bool     bb;                                                                //
    #define rrr ((uint64_t)rand())                                              //
    for (int ii=0; ii < 1000000; ii++)                                          //
        {a64 = (rrr << 48) + (rrr << 32) + (rrr << 16) + rand();                //generate garbage values
         b64 = (rrr << 48) + (rrr << 32) + (rrr << 16) + rand();                //           "
         memmove(&aF, &a64, sizeof(aF));                                        //save in aF and bF
         memmove(&bF, &b64, sizeof(bF));                                        //
         a64 = HiLo64(a64) ^ 0x80;                                              //Flip mantissa sign bit
         b64 = HiLo64(b64) ^ 0x80;                                              //flip exponent sign bit
         bb  = CompareMSB((void*)&a64, (void*)&b64, 8) > 0;                     //do MSB compare
         if (isnan(aF) || isnan(bF)) continue;                                  //dont care about NANs
         if (bb != (aF > bF))                                                   //is MSB comparison the same as floating comparison ?
             Printf("Bad Bongoes: %10e %s %10e\n", aF, bb ? ">" : "<=", bF);    //oh dear !
        }                                                                       //
    return;                                                                     //
  } //TestDujour...
#endif