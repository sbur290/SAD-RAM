//-------- Sam master include file. Version 13, Jan 28, 2023. --------
`ifndef SAM_DEFINES_INCLUDED                                                    //
`define SAM_DEFINES_INCLUDED                                                    //
`timescale 1ps / 1ps                                                            //
parameter clkPeriod           =    1;                                           //
parameter revision            =   13;                                           //
                                                                                //
//WARNING: generated code#1
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
parameter MAX_DRAM_SIZE       =  2 ** DRAM_ADR_BITS;                    //140 Terabytes
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
parameter BRAM_ROWS           =   64;                                   //
parameter BRAM_ADR_BITS       =   $clog2(BRAM_ROWS);                    //==5      
parameter CELL_SIZE           =    8;                                   //         width of each cell
parameter TARGETBUS_SIZE      =    8;                                   //cells    8, 16
parameter TGT_BITS            =  TARGETBUS_SIZE * CELL_SIZE;            //==64     assume full sized target
parameter GROUP_CNT           =  ROW_BYTES / TARGETBUS_SIZE;            //==32     cells in diagonal group
parameter MAX_GROUP_CNT       =  MAX_ROW_BYTES / TARGETBUS_SIZE;        //==1024   cells in diagonal group
parameter GROUP_ADR_BITS      =  $clog2(GROUP_CNT);                     //==10     bits reqd to entire row of groups
                                                                        //
parameter hUSER_ADR_BITS      =  63;                                    //bits=48  bits reqd to adr user records
parameter hUSER_ADR_BYTES     = (hUSER_ADR_BITS+7)/8;                   //bytes=6              
parameter hROW_ADR_BITS       =  DRAM_ADR_BITS - $clog2(ROW_BYTES);     //== 38    bits required to address all 256byte rows
                                                                        //
parameter MIN_hINDX_BYTES     = hUSER_ADR_BYTES + MIN_KEY_BYTES;        //min and max size of hINDX + key
parameter MAX_hINDX_BYTES     = hUSER_ADR_BYTES + MAX_KEY_BYTES;        //
parameter hINDX_BYTES         = hUSER_ADR_BYTES + KEY_BYTES;            //
                                                                        //
parameter hCOUNT_BITS         =  11;    //iteratively determined        //bits reqd to count cPAGE/cBOOK entries on each row
                                                                        //must be >= MAX_hCOUNT_BITS
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
//END WARNING: generated code#1

parameter ITEM_NU     = 2'h0;                                                   //
parameter ITEM_INDX   = 2'h1;                                                   //
parameter ITEM_PAGE   = 2'h2;                                                   //
parameter ITEM_BOOK   = 2'h3;                                                   //
                                                                                //
`define OP_ACTION_BITS   4:0                                                    //action bits
`define OP_FULL_BITS    15:0                                                    //
`define OP_SHORT_BITS    7:0                                                    //
`define OP_ADR_BITS     15:8                                                    //ie `OP_FULL_BITS-1:8
`define OP_REG_BITS      7:4                                                    //

//Opcode implemented by samControl.sv
typedef union
    {//OP_CALL
     struct packed {logic [10:0] callAdr;   //0xFFE0
                    logic [4:0]  act;       //0x001F OP_CALL == 5'h0
                   } call;
     //OP_WRF and OP_RDF
     struct packed {logic [2:0] areg;       //0xE000 address register (handle of structure)
                    logic       nu;         //0x1000 reserved for expansion of areg
                    logic [3:0] fieldNum;   //0x1E00 field Number: FLD_DATA, FLD_P1, etc
                    logic [2:0] breg;       //0x00E0 the data register
                    logic [4:0] act;        //0x001F = 5'h08 or 5'h10
                   } ind;
     //OP_REPEAT
     struct packed {logic        stepR;     //0x8000 step register on each iteration
                    logic        stepA;     //0x4000 step address  on each iteration
                    logic        bkwd;      //0x2000 decrement reg and/or address
                    logic [4:0]  count;     //0x1F00 (number of repitions-2)
                    logic [7:0]  act;       //0x001F == 8'h90
                   } rpt;    
     //OP_REPREG
     struct packed {logic       stepR;      //0x8000 step register on each iteration
                    logic       stepA;      //0x4000 step address  on each iteration
                    logic       bkwd;       //0x2000 decrement reg and/or address
                    logic [4:0] z0;         //0x1F00 reserved for preg expansion
                    logic [2:0] breg;       //0x00E0 (number of repititions-1) = $breg
                    logic [4:0] act;        //0x001F == 8'h70
                   } rptR;    
     //OP_BUG
     struct packed {logic       set;        //0x8000 1 = set bugLevel from op.level
                                            //       0 = set and
                    logic [1:0] sho;        //0x6000   0 = unused
                                            //         1 = display curRow as hPAGE[]
                                            //         2 = display curRow as raw hex   
                                            //         3 = display curRow as hINDX[]
                    logic [4:0] level;      //0x1F00 b4:b0=level 
                    logic [7:0] act;        //0x00FF OP_BUG = 5'h18
                   } bug;
     //OP_ADD, OP_ADC, etc
     struct packed {logic [2:0] areg;       //0xE000
                    logic [4:0] subOp;      //0x1F00 OPS_ADD, _ADC, etc
                    logic [2:0] breg;       //0x00E0
                    logic [4:0] act;        //0x001F OP_ARITH= 5'h02
                   } arith;
     //OP_GO_T, OP_GO_F
     struct packed {logic signed[7:0]relAdr;//0xFF00
                    logic [4:0] cond;       //0x00F8
                    logic [2:0] act;        //0x0007 
                   } go;
     //OP_RI
     struct packed {logic [7:0] imm;        //0xFF00 
                    logic [2:0] breg;       //0x00E0
                    logic [4:0] act;        //0x001F OP_RI == 5'h06
                   } ri;
     //OP_LDI
     struct packed {logic [13:0]imm;        //0xFFFC
                    logic [1:0] act;        //0x0003 OP_LDI == 3
                   } ldi;
     //OP_READ, OP_WRYT, OP_SCAN and OP_SCIN
     struct packed {logic [7:0] rowType;    //0xFF00 
                    logic [2:0] breg;       //0x00E0
                    logic [4:0] act;        //0x001F == 5'h04, 5'h0C, 5'h14, 5'h1C
                   } sc;
     //Generic opcode
     struct packed {logic [7:0] adr;        //0xFF00 
                    logic [2:0] breg;       //0x00E0
                    logic [4:0] act;        //0x001F
                   } g;
   //logic        [7:0]  notShortOp,shortOp;//0x00FF
     logic        [15:0] u16;               //0xFFFF
    } OPCODE;

//alright wise-arse; you get enum to work
parameter WR_IDLE   = 2'b00;                                                    //
parameter WR_PREPARE= 2'b01;                                                    //WriteWord:  read row to local buffer
parameter WR_UPDATE = 2'b10;                                                    //WriteWord:  insert specified word
parameter WR_COMMIT = 2'b11;                                                    //samControl: write back to blockRam

parameter OP_CALL  =  5'h00;                                                    //push pc onto stack, jmp to adr
parameter OP_BUG   =  5'h18;                                                    //Generate debug info and set bugLevel=[9:8]    
parameter OP_STOP  =16'h0018;                                                   //$stop simulation/hardware pending user ok     
parameter OP_RDF   =  5'h08;                                                    //read field
parameter OP_WRF   =  5'h10;                                                    //write field 
parameter OP_ARITH =  5'h02;                                                    //
parameter OP_RI    =  5'h06;                                                    //reg <= immediate
parameter OP_RET   =  8'h0A;                                                    //
parameter OP_CFG_G =  8'h8A;                                                    //Load group configuration; group.konfig<=dramI 
parameter OP_CFG_C =  8'hAA;                                                    //Load cell  configuration; cell.konfig <=dramI 
parameter OP_REPREG=  5'h0E;                                                    //repeat, count = $preg
parameter OP_REPEAT=  5'h12;                                                    //repeat fixed number of times
parameter OP_CROWI =  5'h16;                                                    //set current row from op.g.adr bits
parameter OP_CROW  =  5'h1A;                                                    //set current row
parameter OP_PRINT =  5'h1E;                                                    //print $reg0; up to 8 7-bit bytes
parameter OP_GO_T  =  5'h01;                                                    //Evaluate (condition & statusReg) = condition  
parameter OP_GO_F  =  5'h05;                                                    //[2] == 1 jump on true, [2] == 0 jump on false destination address = target                  
parameter OP_NOOP  =  5'h05;                                                    //
parameter OP_LDI   =  5'h03;                                                    //acc <= literal
parameter OP_READ  =  5'h04;                                                    //Read row[adr] to reg[N]                       
parameter OP_WRYT  =  5'h0C;                                                    //Write row[adr]fr reg[N]                       
parameter OP_SCAN  =  5'h14;                                                    //Scan for target; row type = 0(PAGE), 1(INDX)  
parameter OP_SCIN  =  5'h1C;                                                    //Right shift & insert target into rowType      
                                                                                
//rameter OP_ARITH =  5'h02;                         //action=5'00010
parameter OPS_ADD  =  5'h00;                         //      "    binary ops    //-+
parameter OPS_ADC  =  5'h01;                         //      "        "         //  \.
parameter OPS_SUB  =  5'h02;                         //      "        "         //   \.
parameter OPS_SBB  =  5'h03;                         //      "        "         //    \.
parameter OPS_CMP  =  5'h04;                         //      "        "         //     \.
parameter OPS_XOR  =  5'h05;                         //      "        "         //      \.
parameter OPS_OR   =  5'h06;                         //      "        "         //       \.
parameter OPS_AND  =  5'h07;                         //      "        "         //        \. 
parameter OPS_XSUB =  5'h08;                         //      "        "         //        /.Operators implemented
parameter OPS_XSBB =  5'h09;                         //      "        "         //       /. in samArithmetic.sv
parameter OPS_INC  =  5'h0A; //---------------------------------- unary ops ----//      /. 
parameter OPS_DEC  =  5'h0B;                         //      "        "         //     /.  
parameter OPS_SHL  =  5'h0C;                         //      "        "         //    /. 
parameter OPS_SHR  =  5'h0D;                         //      "        "         //   /.
parameter OPS_RCL  =  5'h0E;                         //      "        "         //  /. 
parameter OPS_RCR  =  5'h0F;                         //      "        "         //-+.
parameter OPS_R2R  =  5'h10;//----------------------------------- binary ops ---//-+\.                   \.
parameter OPS_XCHG =  5'h11;                         //      "                  //   |. Binary Operators  |.
parameter OPS_XTOS =  5'h12;                         //      "                  //-+/.                    |.
parameter OPS_POP  =  5'h13;//----------------------------------- unary ops ----//-+\.  Unary Operators   |.
parameter OPS_PUSH =  5'h14;                         //      "                  //- |.                    |. implemented
parameter OPS_PUSH_CURROW=5'h15;                     //      "                  //-+/                     |. in-line
parameter OPS_STC  =  5'h16;//----------------------------------- nonary ops ---//+-\.                    |.
parameter OPS_CLC  =  5'h17;                         //      "                  //   \.  Nonary Operators |.
parameter OPS_STZ  =  5'h18;                         //      "                  //   /.                   |.
parameter OPS_CLZ  =  5'h19;                         //      "                  //-+/.                   /.

`define IsBugOp(op)    (op.u16[`OP_SHORT_BITS] == OP_BUG)                       //
`define IsCfgOp(op)    (op.u16[`OP_SHORT_BITS] == OP_CFG_G)                     //ignore [8]==cell/group differentiation
`define IsGoOp(op)     (op.go.act[1:0]  == OP_GO_T)                             //OP_GO_T or OP_GO_F
`define IsRdWrOp(op)   ( op.g.act       == OP_READ || op.g.act == OP_WRYT)      //OP_READ or OP_WRYT
`define IsScanOp(op)   ( op.g.act       == OP_SCAN || op.g.act == OP_SCIN)      //OP_SCAN or OP_SCIN
`define IsRdWrScOp(op) ((op.g.act & 7)  == 4)                                   //read, wryt, scan, or scin
`define IsWrField(op)  (op.ind.act[4] == 1)                                     //
`define IsRdfState(st) (st[4] != 0)                                             //

//States of SAM controller
parameter SC_STATE_BITS = 5;                                                    //
typedef enum                                                                    //
   {SC_START = 0,                                                               //
    SC_IDLE  = 1,                                                               //initialization
    SC_GETOP = 2,                                                               //
    SC_DO_OP = 3,                                                               //
    SC_SCAN  = 4,                                                               //initiate scan
    SC_READ  = 5,                                                               //wait for results
    SC_READ1 = 6,                                                               //
  //SC XCHG  = 7,                                                               //
    SC_R2R   = 8,                                                               //
    SC_WRYT  = 9,                                                               //write target to dram
    SC_WRYT1 =10,                                                               //write target to dram
    SC_SCIN  =11,                                                               //scan and insert
    SC_SCIN1 =12,                                                               //scan and insert
    SC_ARITH1=13,                                                               //
    SC_XTOS  =14,                                                               //
    SC_RDF1  =16,                                                               //must be 16+
    SC_RDF2  =17,                                                               // (used to signal readGo)
    SC_RDF3  =18,                                                               //    "
    SC_WDF1  =19,                                                               //
    SC_END   =99                                                                //
   } SC_CODES;                                                                  //

//seqOp.rowType[0] signals which konfig bits the cell should use
parameter [1:0] hCFG_IDLE   = 2'b00; //cell is in passive pass thru mode     : passes rcmp-I/rcmp-O straight thru
parameter [1:0] hCFG_FRST   = 2'b01; //cell is first cell in a diagonal group: ignores cmp-I, cmp-O from left neighbour
parameter [1:0] hCFG_MIDL   = 2'b10; //cell is neither first nor last        : cmp-O from cmp-I combined with target >= cell.kreg
parameter [1:0] hCFG_LAST   = 2'b11; //cell is last cell in a diagonal group : same as mid, but drive rcmp-O from cmp-O
`define         hCFG_INDX_BITS 1:0     //bits of configuration byte             //used when op.item == hINDX
`define         hCFG_PAGE_BITS 3:2     //             "                         //used when op.item == hPAGE/hBOOK
`define         hCFG_INDX_1STK     4                                            //ie., bit[4]
`define         hCFG_INDX_LASTB    5                                            //ie., bit[5]
`define         hCFG_PAGE_1STK     6                                            //ie., bit[6]
`define         hCFG_PAGE_LASTB    7                                            //ie., bit[7]
//typedef struct {logic gt=0, eq=1;} COMPARE;                                   //results of comparison

typedef struct packed                                                           //
    {logic [15:0]  op;                       //0xFFFF0000_00000000              //
     logic [10:0]  nu;                       //0x0000FFE0_00000000              //
     logic [ 4:0]  stat;                     //0x0000001F_00000000              //samStatus
     logic [ 7:0]  insPt;                    //0x00000000_FF000000              //insertion point
     logic [ 7:0]  sp;                       //0x00000000_00FF0000              //stack pointer
     logic [15:0]  pc;                       //0x00000000_0000FFFF              //program counter
    } SYSTEM_STAT;                                                              //

typedef struct packed                                                           //
    {logic [(39-BRAM_ADR_BITS):0] nu;        //0xFFFFFFFF_E0000000 (35 bits)    //
     logic [ BRAM_ADR_BITS-1  :0] curRow;    //0x00000000_1F000000 (5 bits)     //
     logic [ 2                :0] y;         //0x00000000_00E00000              //
     logic [ 4                :0] bugLvl;    //0x00000000_001F0000              //
     logic [15                :0] nxtOp;     //0x00000000_0000FFFF              //
     } DEBUG_STAT;

typedef struct packed                                                           //five bit control bus for sequencerCell
    {logic rowTypeInx,                                                          //geometry of row being scanner (1=INDX, 0=PAGE)
           isScan,                                                              //do scan
           doShift,                                                             //do shift
           insPt,                                                               //cell is at the insertion point
           cfg;                                                                 //dramI has a configuratin byte
    } CELL_CTRL;                                                                //
                                                                                //
//Miscellaneous                                                                 //
parameter true             = 1;                                                 //
parameter false            = 0;                                                 //
                                                                                //
//Compare results                                                               //
`define COMPARE   [1:0]                                                         //
`define GT(what)  what[0]                                                       //[0] target > kreg
`define EQ(what)  what[1]                                                       //[1] = target == kreg
//function does not work !*! Use this define instead: w[0] is >, w[1] is ==
`define CMP_NAME(w) (w==2'b00 ? "<" : w==2'b10 ? "=" : w==2'b01 ? ">" : "S")    //S=stop (perhaps);

parameter MICROCODE_SIZE   = 477; //patched by compile.exe                      //sizeof object code (units = 16-bit words)
`define NAMEOF_MICROCODE "scin.microcode" //patched by compile.exe              //
                                                                                //
//Structures comprising the indexbase. Sizes depend on keyS. 
parameter keySz            =  8;  //bytes                                       //defined by user at run time.

/*-------- Field access for read/write field opcodes (OP_RDF/OP_WRF) --------
 Syntax is lhs = [$reg].field       read field (OP_RDF)
    or     [$reg].field = rhs,      wryt field (OP_WRF)
 $reg is the 'handle' to an hINDX/hPAGE/hBOOK structure. The handle is actually 
 an address pointing to the first key field of the structure: 
   - binary field are at lower addresses (handle-1, handle-2, etc).
   - key fields are at higher  addresses (handle+0, handle+1, etc). 
 By pointing to the middle of the struct we have a consistent way of addressing 
 either type of structure; in particular the stop bit (indicating the last hITEM 
 in an hITEM[] array) and the key field(s) are in a consistent location relative
 to the handle.
   - hINDX binary part occupies a single 64 bit word preceding the handle.
   - hPAGE or hBOOK binary part occupy two 64 bit words preceding the handle.
     hPAGE and hBOOK are identical.
 Structures must conform to some basic constraints so that OP_SCAN will work correctly:
   1. Actal key (=handle)       must align on TARGETBUS_SIZE (8 byte) boundary.
   2. total field wiothin hPAGE must align on TARGETBUS_SIZE (8 byte) boundary.
   3. total field must be stored in MS-byte first order. This is NOT equivalent
      to storing in hi-lo bit order. Each byte is in lo-hi bit order but the five
      bytes are in hi-lo order.
   4. stop bit must always be the last bit before the key in all structures.
      stop is used as an internal end-of-row signal. 
*/
enum                                                                            //
 {FLD_NU=0,                                                                     //
  FLD_DATA, FLD_STOP,                                                           //1-2 fields of hINDX
  FLD_P1,   FLD_P2,   FLD_COUNT, FLD_TOTAL, FLD_PAGE_STOP,                      //3-7 fields of hPAGE/hBOOK
//keys must start at 8; 0x08 bit distinguishes key versus binary fields.        //
  FLD_KEY0=8,FLD_KEY1,FLD_KEY2,  FLD_KEY3, FLD_KEY4, FLD_KEY5,FLD_KEY6,FLD_KEY7 //8-15
 } FIELD_NUMBERS;                                                               //

typedef struct packed                                                           //
    {logic                       stop;      //bits[63]        word[0]           //set on last hINDX in hINDX[]
     logic [hUSER_ADR_BITS-1:0]  dataAdr;   //bits[62:0]      word[0]           //address of underlying record (lo part)
    } hINDX;                                                                    //
                                                                                //
typedef struct packed                                                           //
    {logic [31:0]                p1,        //bits[ 31:  0]   word[0]           //row address of page1 (low 32 bits)
                                 p2;        //bits[ 63: 32]   word[0]           //row address of page2 (low 32 bits)
     logic                       stop;      //bits[127]       word[1]           //
     logic [5:0]                 p1_hi,     //bits[126:121]   word[1]           //row address of page1 (high 6 bits)
                                 p2_hi;     //bits[120:115]   word[1]           //row address of page2 (high 6 bits)
     logic [hCOUNT_BITS-1:0]     count;     //bits[114:104]   word[1]           //number of entries on this page
     logic [hTOTAL_BITS-1:0]     total;     //bits[103: 64]   word[1]           //total # elements in this and preceding pages
    } hPAGE_BOOK;                                                               //   (MS byte first)

`endif //SAM_DEFINES_INCLUDED...
