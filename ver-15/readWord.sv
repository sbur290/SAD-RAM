//WARNING: generated code#1
//Module to extract a single 64-bit word from the dramI (dram row).
`include "samDefines.sv"

module ReadWord
   (input  wire                         clk,                                    //+0 
    input  wire                         readGo,                                 //+1
    input  wire [ROW_BITS-1:0]          dramI,                                  //+2 one row of DRAM
    input  wire [BRAM_ADR_BITS-1:0]     address,                                //+3
    output wire [TGT_BITS-1:0]          wordOut                                 //+4
   );                                                                           //

reg [TGT_BITS-1:0]theWord; assign wordOut = theWord;                            //
`define GroupBits(n)   (n+1) * TGT_BITS-1 : (n) * TGT_BITS                      //partition of dram for each sequencerGroup

always @(posedge clk)                                                           //
   if (readGo)                                                                  //
      case (address)                                                            //
          0: theWord <= dramI[`GroupBits( 0)];
          1: theWord <= dramI[`GroupBits( 1)];
          2: theWord <= dramI[`GroupBits( 2)];
          3: theWord <= dramI[`GroupBits( 3)];
          4: theWord <= dramI[`GroupBits( 4)];
          5: theWord <= dramI[`GroupBits( 5)];
          6: theWord <= dramI[`GroupBits( 6)];
          7: theWord <= dramI[`GroupBits( 7)];
          8: theWord <= dramI[`GroupBits( 8)];
          9: theWord <= dramI[`GroupBits( 9)];
         10: theWord <= dramI[`GroupBits(10)];
         11: theWord <= dramI[`GroupBits(11)];
         12: theWord <= dramI[`GroupBits(12)];
         13: theWord <= dramI[`GroupBits(13)];
         14: theWord <= dramI[`GroupBits(14)];
         15: theWord <= dramI[`GroupBits(15)];
         16: theWord <= dramI[`GroupBits(16)];
         17: theWord <= dramI[`GroupBits(17)];
         18: theWord <= dramI[`GroupBits(18)];
         19: theWord <= dramI[`GroupBits(19)];
         20: theWord <= dramI[`GroupBits(20)];
         21: theWord <= dramI[`GroupBits(21)];
         22: theWord <= dramI[`GroupBits(22)];
         23: theWord <= dramI[`GroupBits(23)];
         24: theWord <= dramI[`GroupBits(24)];
         25: theWord <= dramI[`GroupBits(25)];
         26: theWord <= dramI[`GroupBits(26)];
         27: theWord <= dramI[`GroupBits(27)];
         28: theWord <= dramI[`GroupBits(28)];
         29: theWord <= dramI[`GroupBits(29)];
         30: theWord <= dramI[`GroupBits(30)];
         31: theWord <= dramI[`GroupBits(31)];
      endcase                                                                   //
endmodule //ReadWord...                                                         //
//END WARNING: generated code#1                                                 //
