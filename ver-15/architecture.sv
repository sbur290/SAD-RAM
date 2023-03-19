//Sadram Architectural parameters
//basic terminology
// 10 bits  =    10**3    one  kilobyte
// 20 bits  =    10**6    one  megabyte
// 30 bits  =    10**9    one  gigabyte
// 32 bits  =  4*10**9    four gigabytes
// 40 bits  =    10**12   one  terabyte
// 46 bits  = 140*10**12  140  terabytes
//basic parameters               value                                  //units    range
parameter DRAM_ADR_BITS       =   46;                                   //bits 
parameter MAX_DRAM_SIZE       =  2 ** DRAM_ADR_BITS;                    //140Terabytes
parameter PGM_ADR_BITS        =   16;                                   //         fixed sizeof(SamPU opcode)
parameter MAX_ROW_BYTES       = 8192;                                   //
parameter MIN_RCD_SZ          =  256;                                   //bytes
parameter MIN_RCD_ADR_BITS    = $clog2(MIN_RCD_SZ);                     //bits     8
parameter MIN_KEY_BYTES       =    4;                                   //bytes    
parameter MAX_KEY_BYTES       =   64;                                   //bytes    
                                                                        //
parameter ROW_BYTES           =  256;                                   //bytes    256, 512, 1024, 2048, 4096, 8192
parameter ROW_BITS            =  8*ROW_BYTES;                           //bits     =2048
parameter KEY_BYTES           =    8;                                   //bytes    2 thru 64
parameter KEY_BITS            =  8*KEY_BYTES;                           //bits  
parameter BRAM_ROWS           =   32;                                   //
parameter BRAM_ADR_BITS       =   $clog2(BRAM_ROWS);                    //==5      
parameter CELL_SIZE           =    8;                                   //         width of each cell
parameter TARGETBUS_SIZE      =    8;                                   //cells    8, 16
parameter TGT_BITS            =  TARGETBUS_SIZE * CELL_SIZE;            //==64     assume full sized target
parameter GROUP_CNT           =  ROW_BYTES / TARGETBUS_SIZE;            //==32     cells in diagonal group
parameter MAX_GROUP_CNT       =  MAX_ROW_BYTES / TARGETBUS_SIZE;        //==1024   cells in diagonal group
parameter GROUP_ADR_BITS      =  $clog2(GROUP_CNT);                     //==10     bits reqd to entire row of groups
                                                                        //
parameter hUSER_ADR_BITS      =  DRAM_ADR_BITS;                         //bits=48  bits reqd to adr user records
parameter hUSER_ADR_BYTES     = (hUSER_ADR_BITS+7)/8;                   //bytes=6              
parameter hROW_ADR_BITS       =  DRAM_ADR_BITS - $clog2(ROW_BYTES);     //==38     bits required to address all 256byte rows
                                                                        //
parameter MIN_hINDX_BYTES     = hUSER_ADR_BYTES + MIN_KEY_BYTES;        //min and max size of hINDX + key
parameter MAX_hINDX_BYTES     = hUSER_ADR_BYTES + MAX_KEY_BYTES;        //
parameter hINDX_BYTES         = hUSER_ADR_BYTES + KEY_BYTES;            //
                                                                        //
parameter hCOUNT_BITS         =  11;    //iterative determination       //bits reqd to count cPAGE/cBOOK entries on each row

parameter MIN_hPAGE_BYTES     = (2*hROW_ADR_BITS+8*MIN_KEY_BYTES+hCOUNT_BITS+7)/8; //min and max size of hPAGE/hBOOK + key
parameter MAX_hPAGE_BYTES     = (2*hROW_ADR_BITS+8*MAX_KEY_BYTES+hCOUNT_BITS+7)/8; //
parameter hPAGE_BYTES         = (2*hROW_ADR_BITS+8*KEY_BYTES+hCOUNT_BITS+7)/8; //
                                                                        //
//number of hITEMS can fit on a row; hINDX is smaller than hPAGE/hBOOK  //
parameter MAX_hITEMS_PER_ROW  = MAX_ROW_BYTES / MIN_hINDX_BYTES;        //==1024
parameter MIN_hITEMS_PER_ROW  = MAX_ROW_BYTES / MAX_hPAGE_BYTES;        //==118

parameter MAX_RECORD_COUNT    = 2 ** (DRAM_ADR_BITS - MIN_RCD_ADR_BITS);//==2 teraRecords (2*10^12)
parameter MAX_hINDX_COUNT     = MAX_RECORD_COUNT;                       //exactly the same - duh 
parameter MAX_hINDX_ROWS      = 2*(MAX_hINDX_COUNT/MIN_hITEMS_PER_ROW); //==41 gigaRows (37*10^9)
//now recompute MAX_hCOUNT_BITS which should be less than hCOUNT_BITS
parameter MAX_hCOUNT_BITS     = $clog2(MAX_hITEMS_PER_ROW);             //== 10 max bits reqd to count cPAGE/cBOOK entries on each row
parameter hINDX_ADR_BITS      = $clog2(MAX_RECORD_COUNT);               //
parameter hTOTAL_BITS         = 40;//>=log2(MAX_RECORD_COUNT)           //      can count 2^51 record ~= 2*10^15 records

parameter CFG_BITS            = 8;                                      //bits - presupposed one byte    
