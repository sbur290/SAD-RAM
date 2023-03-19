/*Module to read/write binary fields within hINDX, hPAGE, or hBOOK structures.
   fieldNum = specific field (FLD_PAGE_P1, FLD_STOP, etc).
   structIn = 2*64bit words representing the binary part of an hITEM:
                  hPAGE_BOOK structure (occupying all 128 bits)
               or hINDX      structure (high 64 bits)
   fieldIn  = selector for specific field to update
 For read:  structOut <= field[fieldNum] extracted from hITEM.
 for write: structOut <= structure with field[fieldNum] updated.
 fieldGo is asserted for one clock.

 total field is stored MS-byte first and must align on TARGETBUS_SIZE.
 p1, and p2 fields are normalized to a valid DRAM address by adding 8 zero bits.
     These fields address the base of a row and so must be a multiple of 
     rowSize bytes, ie. the bottom 8 bits must be zero. This is not stored in
     the hPAGE/hBOOK structure, but added/removed here.
 count field is stored in native format
 stop bit is located in the last bit of the structure ('highest significant bit)
     immediately preceding the key field(s). The is the same as hINDX.
*/

`include "samDefines.sv"

module ReadWriteField
   (input  wire                      rsta,       //+0
    input  wire                      clk,        //+1
    input  wire                      fieldGo,    //+2     assert for one clock
    input  wire                      wryt,       //+3     write field versus read field
    input  wire  [3:0]               fieldNum,   //+4     field selector
    input  wire  [2*TGT_BITS-1:0]    structIn,   //+5     hITEM binary input
    input  wire  [  TGT_BITS-1:0]    fieldIn,    //+6     
    output wire  [2*TGT_BITS-1:0]    structOut,  //+7     hITEM binary output
    output reg   [  TGT_BITS-1:0]    fieldOut    //+8     
   );

typedef union packed
   {hPAGE_BOOK                                         pg;
    struct packed {logic [TGT_BITS-1  :0] nu; hINDX h;}inx;  
    struct packed {logic [2*TGT_BITS-1:0] nu;         }w;
   }hSTRUCTURE;
hSTRUCTURE hIn;  assign hIn.w     = structIn;
hSTRUCTURE hOut; assign structOut = hOut.w;
reg        wrytP1=0;

always_ff @(posedge clk)
  if (rsta) wrytP1 <= 0; else                                                                     //
  begin                                                                                           //
  if (fieldGo && wryt)                                                                            //write field
     begin hOut <= hIn; wrytP1 <= 1; end                                                          //
  if (fieldGo && !wryt)                                                                           //read field
     begin                                                                                        //
     case (fieldNum)                                                                              //
         FLD_DATA:      fieldOut <= hIn.inx.h.dataAdr;                                            //fields of hINDX
         FLD_STOP:      fieldOut <= hIn.inx.h.stop;                                               //fields of hINDX/hPAGE/hBOOK
         FLD_P1:        fieldOut <={hIn.pg.p1_hi, hIn.pg.p1, 8'h0};                               //fields of hPAGE
         FLD_P2:        fieldOut <={hIn.pg.p2_hi, hIn.pg.p2, 8'h0};                               //     "
         FLD_COUNT:     fieldOut <= hIn.pg.count;                                                 //     "
         FLD_TOTAL:     fieldOut <={hIn.pg.total[7:0], hIn.pg.total[15:8], hIn.pg.total[23:16], hIn.pg.total[31:24], hIn.pg.total[39:32]};
     endcase                                                                                      //
     end //(fieldGo && !wryt)...                                                                  //
  //write field, one cycle later                                                                  //  
  if (wrytP1)                                                                                     //
     begin                                                                                        //
     case (fieldNum)                                                                              //
         FLD_DATA:      hOut.inx.h.dataAdr <= fieldIn[hUSER_ADR_BITS-1:0];                        //fields of hINDX
         FLD_STOP:      hOut.inx.h.stop    <= fieldIn[0];                                         //fields of hINDX
         FLD_P1:  begin hOut.pg.p1         <= fieldIn[39:8]; hOut.pg.p1_hi <= fieldIn[45:40]; end //fields of hPAGE
         FLD_P2:  begin hOut.pg.p2         <= fieldIn[39:8]; hOut.pg.p2_hi <= fieldIn[45:40]; end //     "
         FLD_COUNT:     hOut.pg.count      <= fieldIn[hCOUNT_BITS-1:0];                           //fields of hPAGE
         FLD_TOTAL:     hOut.pg.total      <= {fieldIn[7:0], fieldIn[15:8], fieldIn[23:16], fieldIn[31:24], fieldIn[39:32]};
     endcase                                                                                      //
     wrytP1 <= 0;                                                                                 //
     end //if (wrytP1)...                                                                         //
  end //(!rsta)...

endmodule
