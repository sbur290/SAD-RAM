#include "sim-15.h"

static void BUGOUT(char const *fmtP,...)
    {va_list arg;                                                               //
     char    buf[512];                                                          //
     va_start(arg, fmtP);                                                       //
     vsnprintf(buf, sizeof(buf)-1, fmtP, arg);                                  //
     va_end(arg);                                                               //
     buf[sizeof(buf)-1] = 0;                                                    //
     OutputDebugStringA(buf);                                                   //
    }

void ShowThis(char const *nameP, uint64_t val, int line)
    {char buf[512]; int len; static int lastLine;
     if (line > (lastLine+1)) OutputDebugStringA("\n");
     lastLine = line;
     memset(buf, ' ', sizeof(buf));
              snprintf(buf, sizeof(buf), "%s", nameP);
     buf[strlen(buf)] = ' '; len = 32;
     if (val >= 1000000000000ull) snprintf(&buf[len], sizeof(buf)-len, " = %lldT(0x%llX)\n", val/1000000000000ull, val); else
     if (val >= 1000000000ull)    snprintf(&buf[len], sizeof(buf)-len, " = %lldG(0x%llX)\n", val/1000000000ull, val); else
     if (val >= 1000000ull)       snprintf(&buf[len], sizeof(buf)-len, " = %lldM(0x%llX)\n", val/1000000ull, val);else
                                  snprintf(&buf[len], sizeof(buf)-len, " = %lld(0x%llX)\n", val, val);
     buf[sizeof(buf)-1] = 0;
     OutputDebugStringA(buf);
    } //ShowThis...

static int MyLog2 (uint64_t val) {for (int ii=0; ii < 64; ii++) if ((val >>= 1) == 0) return ii; return 64;}
int cSimDriver::CheckArchitecture(void) 
   {uint64_t cc; int erC=0;
#define parameter(what, val)  uint64_t what=val; ShowThis(#what, what, __LINE__); 
const int huser_adr_bits = hUSER_ADR_BITS;                              //kludge to step around #undef's
const int htotal_bits    = hTOTAL_BITS;                                 //
const int hcount_bits    = hCOUNT_BITS;                                 //
const int hrow_adr_bits  = hROW_ADR_BITS;                               //
const int hpage_bytes    = 0;//hPAGE_BYTES; 
const int hindx_bytes    = 0;//hINDX_BYTES; 
const int bram_adr_bits  = BRAM_ADR_BITS;
#undef hUSER_ADR_BITS                                                   //
#undef hTOTAL_BITS                                                      //
#undef hITEM_ADR_BITS                                                   //
#undef hCOUNT_BITS                                                      //
#undef hUNUSED_BITS                                                     //
#undef hPAGE_PAD_BITS                                                   //
#undef hBOOK_PAD_BITS                                                   //
#undef hROW_ADR_BITS                                                    //
#undef BRAM_ADR_BITS                                                    //
                                                                        //
//Following code is ported from architecture.sv and translated to C.    //
//Generate constants just a Verilog would do and veiry that they are    //
//equal to the values used in the C++ code (see build_architecture.bat).//
                                                                        //
//WARNING: generated code#1                                             //
//Sadram Architectural parameters                                       //
//basic terminology                                                     //
// 10 bits ,   10**3    one  kilobyte                                   //
// 20 bits ,   10**6    one  megabyte                                   //
// 30 bits ,   10**9    one  gigabyte                                   //
// 32 bits , 4*10**9    four gigabytes                                  //
// 40 bits ,   10**12   one  terabyte                                   //
// 46 bits ,140*10**12  140  terabytes                                  //
//basic parameters               value                                  //units    range
parameter(DRAM_ADR_BITS      ,  46);                                    //bits 
parameter(MAX_DRAM_SIZE      , 2ull << DRAM_ADR_BITS);                     //140Terabytes
parameter(PGM_ADR_BITS       ,  16);                                    //         fixed sizeof(SamPU opcode)
parameter(MAX_ROW_BYTES      ,8192);                                    //
parameter(MIN_RCD_SZ         , 256);                                    //bytes
parameter(MIN_RCD_ADR_BITS   ,MyLog2(MIN_RCD_SZ));                      //bits     8
parameter(MIN_KEY_BYTES      ,   4);                                    //bytes    
parameter(MAX_KEY_BYTES      ,  64);                                    //bytes    
                                                                        //
parameter(ROW_BYTES          , 256);                                    //bytes    256, 512, 1024, 2048, 4096, 8192
parameter(ROW_BITS           , 8*ROW_BYTES);                            //bits     =2048
parameter(KEY_BYTES          ,   8);                                    //bytes    2 thru 64
parameter(KEY_BITS           , 8*KEY_BYTES);                            //bits  
parameter(BRAM_ROWS          ,  32);                                    //
parameter(BRAM_ADR_BITS      ,  MyLog2(BRAM_ROWS));                     //==5      
parameter(CELL_SZ            ,   8);                                    //         width of each cell
parameter(TARGETBUS_SIZE     ,   8);                                    //cells    8, 16
parameter(TGT_BITS           , TARGETBUS_SIZE * CELL_SZ);               //==64     assume full sized target
parameter(GROUP_CNT          , ROW_BYTES / TARGETBUS_SIZE);             //==32     cells in diagonal group
parameter(MAX_GROUP_CNT      , MAX_ROW_BYTES / TARGETBUS_SIZE);         //==1024   cells in diagonal group
parameter(GROUP_ADR_BITS     , MyLog2(GROUP_CNT));                      //==10     bits reqd to entire row of groups
                                                                        //
parameter(hUSER_ADR_BITS     , 63);                                     //bits=48  bits reqd to adr user records
parameter(hUSER_ADR_BYTES    ,(hUSER_ADR_BITS+7)/8);                    //bytes=6              
parameter(hROW_ADR_BITS      , DRAM_ADR_BITS - MyLog2(ROW_BYTES));      //bits required to address all 256byte rows
                                                                        //
parameter(MIN_hINDX_BYTES    ,hUSER_ADR_BYTES + MIN_KEY_BYTES);         //min and max size of hINDX + key
parameter(MAX_hINDX_BYTES    ,hUSER_ADR_BYTES + MAX_KEY_BYTES);         //
parameter(hINDX_BYTES        ,hUSER_ADR_BYTES + KEY_BYTES);             //
                                                                        //
parameter(hCOUNT_BITS        , 11);     //iterative determination       //bits reqd to count cPAGE/cBOOK entries on each row

parameter(MIN_hPAGE_BYTES    ,(2*hROW_ADR_BITS+8*MIN_KEY_BYTES+hCOUNT_BITS+7)/8);  //min and max size of hPAGE/hBOOK + key
parameter(MAX_hPAGE_BYTES    ,(2*hROW_ADR_BITS+8*MAX_KEY_BYTES+hCOUNT_BITS+7)/8);  //
parameter(hPAGE_BYTES        ,(2*hROW_ADR_BITS+8*KEY_BYTES+hCOUNT_BITS+7)/8);  //
                                                                        //
//number of hITEMS can fit on a row);  hINDX is smaller than hPAGE/hBOOK  //
parameter(MAX_hITEMS_PER_ROW ,MAX_ROW_BYTES / MIN_hINDX_BYTES);         //==1024
parameter(MIN_hITEMS_PER_ROW ,MAX_ROW_BYTES / MAX_hPAGE_BYTES);         //==118

parameter(MAX_RECORD_COUNT   ,2ull << (DRAM_ADR_BITS - MIN_RCD_ADR_BITS)); //==2 teraRecords (2*10^12)
parameter(MAX_hINDX_COUNT    ,MAX_RECORD_COUNT);                        //exactly the same - duh 
parameter(MAX_hINDX_ROWS     ,2*(MAX_hINDX_COUNT/MIN_hITEMS_PER_ROW));  //==41 gigaRows (37*10^9)
//now recompute MAX_hCOUNT_BITS which should be less than hCOUNT_BITS
parameter(MAX_hCOUNT_BITS    ,MyLog2(MAX_hITEMS_PER_ROW));              //== 10 max bits reqd to count cPAGE/cBOOK entries on each row
parameter(hINDX_ADR_BITS     ,MyLog2(MAX_RECORD_COUNT));                //
parameter(hTOTAL_BITS        ,40); //>=log2(MAX_RECORD_COUNT)           //      can count 2^51 record ~= 2*10^15 records

parameter(CFG_BITS           ,8);                                       //bits - presupposed one byte    
//END WARNING: generated code#1
    if (hCOUNT_BITS < MAX_hCOUNT_BITS) 
       {printf("\x07"); OutputDebugStringA("hCOUNT_BITS < MAX_hCOUNT_BITS\n");}
    if ((cc=2*hROW_ADR_BITS + hTOTAL_BITS + hCOUNT_BITS + STOP_BIT) != 128)    
       {printf("\x07"); ShowThis("Bit size of(hPAGE/hBOOK)", cc, 0);}
#define Check(a,b) if (a != b) {printf("\x07%s = %lld", #b, b); erC = -1;}
    Check(huser_adr_bits, hUSER_ADR_BITS);
    Check(htotal_bits   , hTOTAL_BITS);  
    Check(hcount_bits   , hCOUNT_BITS);  
    Check(hrow_adr_bits , hROW_ADR_BITS);
    Check(m_rowBytes    , ROW_BYTES);    
    Check(bram_adr_bits,  BRAM_ADR_BITS);

    BUGOUT("m_baseSz[]= %2d %2d %2d\n", m_baseSz[1], m_baseSz[2],  m_baseSz[3]);   
    BUGOUT("m_sizes[] = %2d %2d %2d\n", m_sizes [1], m_sizes [2],  m_sizes [3]);
    BUGOUT("m_align[] = %2d %2d %2d\n", m_align [1], m_align [2],  m_align [3]);
    BUGOUT("m_padSz[] = %2d %2d %2d\n", m_padSz [1], m_padSz [2],  m_padSz [3]);
    BUGOUT("m_perRow[]= %2d %2d %2d\n", m_perRow[1], m_perRow[2],  m_perRow[3]);
    return erC;
#undef Check
#undef parameter
   } //cSimDriver::CheckArchitecture...

