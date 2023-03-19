//File hStructures.h: Oct, 2022.
//C++ representation of Verilog structures hINDX, hPAGE, and hBOOK.

#ifndef H_STRUCTURES_DEFINED
#define H_STRUCTURES_DEFINED

#include "c3_types.h"

// Verilog structures corresponding to cINDX, cPAGE, and cBOOK as implemented in FPGA/hardware:
#define hUSER_ADR_BITS      63                                                  //bits reqd to adr of user data
#define STOP_BIT             1                                                  //max data file size = 2^47, i.e., 6 terabytes
#define hROW_ADR_BITS       38                                                  //bits reqd to address cPAGE[]s & cBOOK[]s rows
#define hCOUNT_BITS         11                                                  //bits reqd to count cPAGE/cBOOK entries
#define hTOTAL_BITS         40                                                  //bits will count 140T records
#define hCFG_INDX_BITS      1     //bits of configuration byte                  //used when op.item == hINDX
#define hCFG_PAGE_BITS      3     //             "                              //used when op.item == hPAGE/hBOOK
#define hCFG_INDX_1STK      4                                                   //ie., bit[4]
#define hCFG_INDX_LASTB     5                                                   //ie., bit[5]
#define hCFG_PAGE_1STK      6                                                   //ie., bit[6]
#define hCFG_PAGE_LASTB     7                                                   //ie., bit[7]

#pragma pack (push, 1)
typedef struct
    {int rowSize, targetBusSz;
    } sHDW_PARAMS;

//DO NOT USE sizeof(hINDX), sizeof(hPAGE), or sizeof(hBOOK)
extern uint32_t hINDX_size,   hPAGE_size,   hBOOK_size,                         //sizes with user key (variable length)
                hINDX_padSz,  hPAGE_padSz,  hBOOK_padSz,                        //sizes rounded to targetBus size
                hINDX_perRow, hPAGE_perRow, hBOOK_perRow,                       //number of padded records on a row
                hINDX_align,  hPAGE_align,  hBOOK_align;                        //lead bytes required to align key with targetBus

//NOTE: Structures used by hardware are beyond the capability of C++ to accurately represent.
//Following representation is correct, except the .i sub-structure rounds beyond its proper size.
//This does not matter so long as the .k structure aligns _key to the proper byte boundary.
//This representation, relies on the keysize >= 2 bytes.
// 20 bits = 10^6  one megabyte
// 30 bits = 10^9  one gigabyte,   32 bits = 4 gigabytes
// 40 bits = 10^12 one terrabyte,  48 bits = 260 terrabytes
// 50 bits = 10^50 one petabyte
#define hINDX_BASE  (hUSER_ADR_BITS+STOP_BIT+7)/8                               //=6     size of binary prefix to key
#define hPAGE_BASE  (hTOTAL_BITS+STOP_BIT+2*hROW_ADR_BITS+hCOUNT_BITS+7)/8      //=14    also is the offset of the key
#define hBOOK_BASE  hPAGE_BASE                                                  //=14    within the structure
typedef struct  //hINDX                                                         //
    {uint64_t data : hUSER_ADR_BITS,  //[62:0]     word[0]                      //reladr of underlying record 
              stop : STOP_BIT;        //[63]       word[0]                      //
   //char     key[n];                 //[64+..]    word[1+]                     //actual key; n=2 thru 8
    } hINDX;                                                                    //

/*typedef struct packed                                                           //
    {logic [31:0]                p1,        //bits[ 31:  0]   word[0]           //row address of page1 (low 32 bits)
                                 p2;        //bits[ 63: 32]   word[0]           //row address of page2 (low 32 bits)
     logic                       stop;      //bits[127]       word[1]           //total # records in this and preceding pages
     logic [5:0]                 p1_hi,     //bits[126:121]   word[1]           //row address of page1 (high 6 bits)
                                 p2_hi;     //bits[120:115]   word[1]           //row address of page2 (high 6 bits)
     logic [hCOUNT_BITS-1:0]     count;     //bits[114:104]   word[1]           //number of entries on this page
     logic [hTOTAL_BITS-1:0]     total;     //bits[103: 64]   word[1]           //total # elements in this and preceding pages
    } hPAGE_BOOK; */
typedef struct //hPAGE                                                          
    {uint32_t  p2, p1;                      //32+6                              //low bits of row address
     uint64_t  total : 40,                  //40 stored MSB first               //total # elements in this and preceding pages
               kount : 11,                  //11                                //number of entries on this page
               p2_hi : 6,                   // 6                                //hi bits of row address
               p1_hi : 6,                   // 6                                //            "
               stop  : 1;                   // 1                                //
   //char     key[n];                       //                                  //actual key; n=2 thru 8
    } hPAGE;                                                                    //
typedef hPAGE       hBOOK;                                                      //exactly the same structure as cPAGE

inline uint64_t HiLo40(uint64_t u64)
   {uint8_t *u8P=(uint8_t*)&u64, u8[5], r8[5];
    memmove(u8, &u64, 5);
    r8[0] = u8[4]; r8[1] = u8[3]; r8[2] = u8[2]; r8[3] = u8[1]; r8[4] = u8[0];
    memmove(&u64, r8, 5);
    return u64;
   } //HiLo40...

inline uint64_t GetP1   (hPAGE *itemP) {return (((uint64_t)itemP->p1) << 8) + (((uint64_t)itemP->p1_hi) << 40);}
inline uint64_t GetP2   (hPAGE *itemP) {return (((uint64_t)itemP->p2) << 8) + (((uint64_t)itemP->p2_hi) << 40);}
inline uint64_t GetTotal(hPAGE *itemP) {return HiLo40(itemP->total);}

inline void PutP1   (hPAGE *itemP, uint64_t val) {itemP->p1 = (uint32_t)(val >> 8); itemP->p1_hi = (uint32_t)(val >> 40);}
inline void PutP2   (hPAGE *itemP, uint64_t val) {itemP->p2 = (uint32_t)(val >> 8); itemP->p2_hi = (uint32_t)(val >> 40);}
inline void PutTotal(hPAGE *itemP, uint64_t val) {itemP->total = HiLo40(val);}
#pragma pack(pop)
#endif //H_STRUCTURES_DEFINED...

//end of file...
