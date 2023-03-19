//WARNING: generated code#1
//Module to write a single word to the blockRAM.

`include "samDefines.sv"

module WriteWord
   (input  wire                         clk,                                    //+0
    input  wire [1:0]                   wrytGo,                                 //+1 2'b01=prepare, 2'b10=doit
    input  wire [ROW_BITS-1:0]          rowIn,                                  //+2 one row of DRAM
    input  wire [BRAM_ADR_BITS-1:0]     wdAdr,                                  //+3 word adr within row
    input  wire [TGT_BITS-1:0]          target,                                 //+4 word to write
    output reg  [ROW_BITS-1:0]          rowOut                                  //+6 modified row
   );                                                                           //

`define MM(n) n: rowOut[(n+1)*TGT_BITS-1:n*TGT_BITS] <= target                  //
always @(posedge clk)                                                           //
    begin                                                                       //
    if (wrytGo == WR_PREPARE) rowOut <= rowIn;                                  //capture rowin
    if (wrytGo == WR_UPDATE)                                                    //patch rowin[wdAdr]
      case (wdAdr) //dial in specified word                                     //
         `MM( 0); `MM( 1); `MM( 2); `MM( 3); `MM( 4); `MM( 5); `MM( 6); `MM( 7);//
         `MM( 8); `MM( 9); `MM(10); `MM(11); `MM(12); `MM(13); `MM(14); `MM(15);//
         `MM(16); `MM(17); `MM(18); `MM(19); `MM(20); `MM(21); `MM(22); `MM(23);//
         `MM(24); `MM(25); `MM(26); `MM(27); `MM(28); `MM(29); `MM(30); `MM(31);//
         endcase                                                                //
    // (wrytGo == WR_COMMIT)                                                    //samControl updates BRAM
    end                                                                         //
endmodule //WriteWord...
//END WARNING: generated code#1
