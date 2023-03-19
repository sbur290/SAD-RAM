/* program math.cpp Versin 9, Sept 11, 2022.
*
*/
#include "sam-15.h"
#include <math.h>
#include "samOptions.h"

#define USER_ADDRESS_BITS   40 //bits required to store address in cPAGE/cBOOK structures
#define ADDRESS_BITS        32 //bits required to store address in cPAGE/cBOOK structures
#define COUNT_BITS          12 //bits required to store count of cPAGE/cBOOK entries
#define TOTAL_BITS          40 //bits required to count total number of records

#pragma pack (push, 1)
//These three structures model how the data is really stored in a hardware implementation
typedef struct
   {uint8_t         dataP[USER_ADDRESS_BITS/8]; //index of underlying record.
    uint8_t         key[1];                     //key of underlying record, actual size = keySize.
   } qINDX;

typedef struct
   {uint8_t         x1P[ADDRESS_BITS/8],        //pointers to arrays of cINDX elements
                    x2P[ADDRESS_BITS/8];        //
    uint8_t         count[COUNT_BITS/8];        //number of cINDX[]'s in x1P[] + x2P[]
    uint8_t         total[TOTAL_BITS/8];        //total # cINDX's in this and all preceding pages
    uint8_t         loKey[1];                   //.... [keySize]; lowest key on x1P[] this index page
   } qPAGE;

typedef struct
   {uint8_t         x1P[ADDRESS_BITS/8],       //pointers to arrays of cINDX elements
                    x2P[ADDRESS_BITS/8];       //
    uint8_t         count[COUNT_BITS/8];       //number of cPAGE[]'s in pagesP
    uint8_t         total[TOTAL_BITS/8];       //total # cINDX's in this and preceding books
    uint8_t         loKey[1];                  //lowest key in m_pages[0]
   } qBOOK;

typedef struct
    {uint32_t keySize, xSize, pSize, bSize, xPerRow, pPerRow, bPerRow, rowSize;
     double   xSpaceF, pSpaceF, bSpaceF, totalIndexSpace;
     double   xCompactF, pCompactF, bCompactF, totalCompactSpace;
     double   xStripedF, pStripedF, bStripedF, totalStripedSpace;
     double   overheadF, compactF,  stripedF;
    } sRESULTS;
#pragma pack(pop)

uint32_t KeySizes[] = {8, 16, 27, 58},
         RowSizes[] = {256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
class cMath
   {public:
    cMath               (SAM_CONTROLS *m_ctlP);
   ~cMath               () {free(m_calcsP);}
    void Sam_mc         (void);
    void Sam_md         (void);
    void Sam_me         (void);
    void Sam_mq         (bool detailB=true);
    private:
    uint32_t           m_rcdCount, m_rsCount, m_rcdSize, m_col, m_roSize;
    double             m_fileSpaceF;
    sRESULTS          *m_calcsP;
    SAM_CONTROLS      *m_ctlP;
    bool               m_detailB;
    double ReqdRows     (double count, int perRow);
    double CompactRows  (double count, int perRow);
    double StripedRows  (double count, int perRow);
    double DivM         (double dd);
    void   PrintByUnits (const char *fmtP, double value);
    void   Every8       (uint32_t ii);
   }; //cMath...

double cMath::ReqdRows(double count, int perRow)
    {int mitosis = (perRow+1)/2, rmdr = (perRow-mitosis);
     count = 1 + (count - mitosis + rmdr - 1) / rmdr;
     return (count < 1.0) ? 1.0 : count;
    } //cMath::ReqdRows...

double cMath::CompactRows(double count, int perRow)
    {count = (count + perRow - 1) / perRow;
     return (count < 1.0) ? 1.0 : count;
    } //cMath::CompactRows...

double cMath::StripedRows(double count, int perRow)
    {count = (count + perRow - 1) / perRow;
     return (count < 1.0) ? 1.0 : count;
    } //cMath::StripedRows...

double cMath::DivM(double dd) {return (dd+500000.0)/1000000.0;}

void cMath::PrintByUnits(const char *fmtP, double value)
    {int    jj;         //terabytes,    gigabytes,  megabytes, kilobytes, bytes
     double units  [] = {1000000000000, 1000000000, 1000000,   1000,      1};
     char unitNames[] = {'T',           'G',        'M',       'K',     ' '};
     for (jj=0; jj < HOWMANY(units); jj++)
         {if (value < units[jj]) continue;
          value = value/units[jj];
          if (strnicmp(fmtP, "%d", 2) == 0) Printf(fmtP, (int)floor(value), unitNames[jj]);
          else                              Printf(fmtP, value, unitNames[jj]);;
          return;
         }
     Printf(fmtP, value, ' ');
    } //cMath::PrintByUnits...

cMath::cMath(SAM_CONTROLS *ctlP)
   {m_ctlP = ctlP;
    uint32_t    keySize, rowSize, kk, rr, maxRr, addrsPerRow;
    double      xRows, pRows, bRows;
    sRESULTS   *rsP;

    m_rcdSize    = m_ctlP->rcdSize;
    m_rcdCount   = Max(m_ctlP->rcdCount, 1000000);
    m_calcsP     = (sRESULTS*)calloc(HOWMANY(KeySizes)*HOWMANY(RowSizes)+1, sizeof(sRESULTS));
    m_fileSpaceF = ((double)m_rcdCount) * ((double)m_rcdSize);

    if (m_ctlP->rowSizedB)
        {maxRr = 1; RowSizes[0] = m_ctlP->hParams.rowSize;} // 'sam /rowSize': limit output to that rowSize
    else maxRr = HOWMANY(RowSizes);

    //calculate usages etc and store in m_calcsP[]
    for (rr=m_rsCount=0, rsP=m_calcsP; rr < maxRr; rr++)
        {for (kk=0; kk < HOWMANY(KeySizes); kk++)
            {rsP->rowSize    = rowSize = RowSizes[rr];
             rsP->keySize    = keySize = KeySizes[kk];
             rsP->xSize      = (USER_ADDRESS_BITS+7)/8 + keySize;
             rsP->pSize      = (2*ADDRESS_BITS + COUNT_BITS + TOTAL_BITS+7)/8 + keySize;
             rsP->bSize      = (2*ADDRESS_BITS + COUNT_BITS + TOTAL_BITS+7)/8 + keySize;
             rsP->xPerRow    = rowSize / rsP->xSize;
             rsP->pPerRow    = rowSize / rsP->pSize;
             rsP->bPerRow    = rowSize / rsP->bSize;
             addrsPerRow     = rsP->rowSize / sizeof(void*);
             if (rsP->xPerRow > 1 && rsP->pPerRow > 1 && rsP->bPerRow > 1)  //otherwise won't work
                {rsP->xSpaceF           = (xRows=ReqdRows   (m_rcdCount, rsP->xPerRow)) * rsP->rowSize;
                 rsP->pSpaceF           = (pRows=ReqdRows   (xRows,    rsP->pPerRow)) * rsP->rowSize;
                 rsP->bSpaceF           = (bRows=ReqdRows   (pRows,    rsP->bPerRow)) * rsP->rowSize;
                 rsP->xCompactF         = (xRows=CompactRows(m_rcdCount, rsP->xPerRow)) * rsP->rowSize;
                 rsP->pCompactF         = (pRows=CompactRows(xRows,    rsP->pPerRow)) * rsP->rowSize;
                 rsP->bCompactF         = (bRows=CompactRows(pRows,    rsP->bPerRow)) * rsP->rowSize;
                 rsP->xStripedF         = (xRows=StripedRows(m_rcdCount, addrsPerRow )) * rsP->rowSize;
                 rsP->pStripedF         = (pRows=StripedRows(xRows,    addrsPerRow )) * rsP->rowSize;
                 rsP->bStripedF         = (bRows=StripedRows(pRows,    addrsPerRow )) * rsP->rowSize;
                 rsP->totalIndexSpace   = rsP->xSpaceF   + rsP->pSpaceF   + rsP->bSpaceF;
                 rsP->totalCompactSpace = rsP->xCompactF + rsP->pCompactF + rsP->bCompactF;
                 rsP->totalStripedSpace = rsP->xStripedF + rsP->pStripedF + rsP->bStripedF;
                 rsP->overheadF         = (100*rsP->totalIndexSpace)  /m_fileSpaceF;
                 rsP->compactF          = (100*rsP->totalCompactSpace)/m_fileSpaceF;
                 rsP->stripedF          = (100*rsP->totalStripedSpace)/m_fileSpaceF;
                 rsP++; m_rsCount++;
   }    }   }   } //cMath::cMath...

//sam /mc. Shortened csv file format (for documentation)
void cMath::Sam_mc(void)
    {uint32_t    rowSize, ii, kk;
     sRESULTS   *rsP;
     Printf(  "row,"    ); for (kk=0; kk < HOWMANY(KeySizes); kk++) Printf("keysize=%d,,", KeySizes[kk]);
     Printf("\nsize,"   ); for (kk=0; kk < HOWMANY(KeySizes); kk++) Printf("index,over-,"); Printf("striped,over-\n");
     Printf(  "(bytes),"); for (kk=0; kk < HOWMANY(KeySizes); kk++) Printf("space,head," ); Printf("space,head\n");
     for (ii=0, rsP=m_calcsP; ii < m_rsCount;)
         {Printf("%5dB,", rsP->rowSize);
          for (rowSize=rsP->rowSize; rsP->rowSize == rowSize && ii < m_rsCount; rsP++, ii++)
              {PrintByUnits("%5.0f%cB,", rsP->totalIndexSpace);
               Printf("%3.1f%%,", rsP->overheadF);
              }
           PrintByUnits("%5.2f%cB,", rsP->totalStripedSpace);
           Printf("%4.2f%%,",        rsP->stripedF);
           Printf("\n");
         }
     Printf("Space occupied by user data = %dB ", m_rcdSize);
     PrintByUnits("* %5.0f%cB", m_rcdCount);
     PrintByUnits(" =%5.0f%cB", m_fileSpaceF);
    } //cMath::Sam_mc

//sam /md. Full disclosure.
void cMath::Sam_md(void)
    {uint32_t    ii, lastRowSize=0;
     sRESULTS   *rsP;
     const char
       *separatorP = "+----+-----++------+-----+-----+-----+-------+-------+-------+-------+-------+-------+-------+--------+\n",
       *title1P    = "|key | row || page |indx/|pages|books| cINDX | cPAGE | cBOOK | index |striped| file  | over- |striped |\n",
       *title2P    = "|size| Size|| size | row | /row| /row| space | space | space | space | space | space |  head | o/head |\n";
     Printf("Storage required to index ");
     PrintByUnits("%d%c", m_rcdCount);
     Printf(" records of size=%d with various keysize's and rowSize's.\n", m_rcdSize);
     Printf(separatorP); Printf(title1P); Printf(title2P); Printf(separatorP);
     for (ii=0, rsP=m_calcsP; ii < m_rsCount; ii++, lastRowSize=rsP++->rowSize)
         {if (ii >= 1 && rsP->rowSize != lastRowSize) Printf(separatorP);
          Printf("|%3d ",             rsP->keySize);
          Printf("|%5d|",             rsP->rowSize);
          Printf("|%5d ",             rsP->pSize  );
          Printf("|%4d ",             rsP->xPerRow);
          Printf("|%4d ",             rsP->pPerRow);
          Printf("|%4d |",            rsP->bPerRow);
          PrintByUnits("%5.0f%c |",   rsP->xSpaceF);
          PrintByUnits("%5.0f%c |",   rsP->pSpaceF);
          PrintByUnits("%5.0f%c |",   rsP->bSpaceF);
          PrintByUnits("%5.0f%c |",   rsP->totalIndexSpace);
          PrintByUnits("%5.0f%c |",   rsP->totalStripedSpace);
          PrintByUnits("%5.0f%c |",   m_fileSpaceF);
          Printf(     " %4.1f%% |",   rsP->overheadF);
          Printf(     " %5.2f%% |\n", rsP->stripedF);
         }
     if (!m_ctlP->rowSizedB)
        {Printf(separatorP); Printf(title1P); Printf(title2P);}
     Printf(separatorP);
    } //cMath::Sam_md...

//sam /me. Prepare data (for Excel graph)
void cMath::Sam_me(void)
    {uint32_t    ii, lastRowSize=0;
     sRESULTS   *rsP;
     Printf("keySize\trowSize\toverhead\tstriped\n");
     for (ii=0, rsP=m_calcsP; ii < m_rsCount; ii++, lastRowSize=rsP++->rowSize)
         Printf("%s%d\t%d\t%3.1f\t%3.1f\n",
               ii >= 1 && rsP->rowSize != lastRowSize ? "\n" : "",
               rsP->keySize, rsP->rowSize, rsP->overheadF, rsP->stripedF);
    } //cMath::Sam_me...

//sam /mq. Output one character and a space at 8 byte boundaries
void cMath::Every8(uint32_t ii)
   {char ch='0'+ii;                                                             //
    if (m_detailB)                                                              //
       {if (ch > '9') ch += 'A'-'9'-1;                                          //letters G-Z
      //if (ch > '9') ch = '-';                                                 //use '-' for non hex letters
        Printf("%c%s", ch, ((m_col & 7) == 7 ? " " : ""));                      //
        if ((m_col & 127) == 127 && m_col != (m_roSize-1))                      //
                 Printf("\n          ");                                        //
       }            //    "DRAM row: "                                          //
    m_col++;                                                                    //
   } //cMath::Every8...

//mq and /mqs commands; illustrate the layout of a DRAM row required to align
//with the array of sequencer-cells.
//detailB = true /mq command, detailB=false /mqs command
void cMath::Sam_mq(bool detailB)
   {uint32_t tgtSizes[]={4,8,16,32,64},
             keySizes[]={2,3,4,6,8,10,12,14,16,17,18,20,22,24,25,26,28,30,40,50},
             tSize, kSize, ii, jj, kk, tt, rr=0, used, n, w, b,                 //
             worstTsize, worstKSize, worstRSize, realSz, baseSz=sizeof(qINDX)-1;//-1 excluding key[1]
    double   worstPercentage=0, percentage;                                     //
    qINDX    indx = {0x12,0x34,0x56,0x78,'$'};                                  //
    qPAGE    page = {1,2,3,4,5,6,7,8};                                          //
    qBOOK    book = {8,7,6,5,4,3,2,1};                                          //
                                                                                //
top:m_roSize    = m_ctlP->rowSizedB ? m_ctlP->hParams.rowSize : RowSizes[rr++]; //
    if (m_detailB=detailB)                                                      //
       {Printf("DRAM row: ");                                                   //heading (memory row)
        for (ii=0; ii < m_roSize; ii++) Every8(22); //generates 'M'             //   'MMMMMMMM MMMMMMMM ...
       }                                                                        //
    else                                                                        //
       {Printf("Target bus,,,Row Size=%d\n(#cells),", m_roSize);                //Column headings for Excel
        for (kk=0; kk < HOWMANY(keySizes); kk++)                                //        "
             Printf("K=%02d, ", keySizes[kk]);                                  //        "
       }                                                                        //
    Printf("\n");                                                               //
    for (tt=0; tt < HOWMANY(tgtSizes); tt++)                                    //
        {if ((tSize=m_ctlP->targetBusSz) == 0) tSize = tgtSizes[tt];            //
         if (detailB)                                                           //
             Printf("Target bus=%d (cells), rowSize=%d (bytes)\n", tSize, m_roSize);//
         else                                                                   //
             Printf("T=%d,", tSize=tgtSizes[tt]);                               //overrides setting at top of loop
         for (kk=0; kk < HOWMANY(keySizes); kk++)                               //
             {realSz = (kSize=keySizes[kk]) + baseSz;                           //key + 'extra fields'
              if (detailB) Printf("keySz=%02d: ", kSize); else                  //
              n    = tSize / realSz;                                            //count of keys in each target group
              b    = (realSz+tSize-1)/tSize * tSize;                            //
              used =  0;                                                        //cells used
              for (m_col=jj=w=0; jj < m_roSize; jj+=b, w=0)                     //repeat to fill row (m_roSize bytes)
                  {if ((m_roSize-jj) < realSz)                                  //cannot fit another keysize elements
                      {while (jj++ < m_roSize) Every8(-2); break;}              //generates '.'
                   while (w <= b - realSz)                                      //pack multiple compares into group
                       {for (ii=0; ii < kSize; ii++, w++, used++) Every8(ii);   //one bank
                        for (ii=0; ii < baseSz;ii++, w++, used++) Every8(-6);   //generates '*'
                       }                                                        //
                   while (w++ % tSize != 0) Every8(-2);  //generates '.'        //fill out to group boundary
                  }                                                             //
              percentage = 100.0*(m_roSize-used)/m_roSize;                      //
              if (detailB) Printf("unused=%2d(%4.1f%%)\n", m_roSize-used, percentage);//
              else         Printf("%4.1f%%,", percentage);                      //
              if (percentage > worstPercentage)                                 //snag the parameters
                 {worstPercentage = percentage; worstKSize=kSize;               //for that nasty little
                  worstRSize      = m_roSize;     worstTsize = tSize;           //bastard
             }   } //for (kSize=...                                             //
          Printf("\n");                                                          //
          if (detailB && m_ctlP->targetBusSz != 0) break;                      //shortened version for detail
         }                                                                      //
    if (!detailB) return;                                                       //
    Printf("Worst case: targetSize=%d, keySize=%d, rowSize=%d, %4.1f%%\n",      //
                     worstTsize, worstKSize, worstRSize, worstPercentage);      //
    Printf("\n\"Legend: K=key size, T=target Bus size, *=non-key fields of cINDX (address pointers, etc.)\"\n\n");//
    if (!m_ctlP->rowSizedB && rr < HOWMANY(RowSizes)) goto top;                 //
   } //cMath::Sam_mq...

//Entry point
void DisplayMath(SAM_CONTROLS *ctlP)
   {cMath math(ctlP);
    switch (ctlP->math)
       {case 1: math.Sam_mc();     break;
        case 2: math.Sam_md();     break;
        case 3: math.Sam_me();     break;
        case 4: math.Sam_mq(true); break;
        case 5: math.Sam_mq(false);break;
       }
    } //DisplayMath...
