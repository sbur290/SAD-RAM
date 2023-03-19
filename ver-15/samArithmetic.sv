/* DumpIndexbase.sv, Sep 29,2022, rev 9
Module to perform arithmetic operations as specified in opcode:
+--+--+--+--+--+--+--+--+--+--+--+---+---+---+---+---+
|  dreg  |  subOp[4:0]  |  sreg  | 1 | 1 | 0 | 0 | 0 |
+--+--+--+--+-----------+--+--+--+---+---+---+---+---+
subcodes 0=ADD, 1=ADC, 2=SUB, 3=SBB, 4=CMP, 5=XOR, 6=OR, 7=AND,      binary
      0x08=nu,  9=nu,  A=INC, B=DEC, C=SHL, D=SHR, E=RCL, F=RCR,     operators
ie, opcodes with subOp[4] == 0 (not inline ops).

for binary opcodes (add, adc, etc):
    left = $reg0 and rite = $sreg; wordO <= left <op> rite
for unary opcodes (inx, shl, etc)
    wordO <= <op>(rite)
The result (wordO is stored in $dreg by the caller.
*/

`include "samDefines.sv"

module SamArithmetic
   (input  wire                         rsta,                                   //+0 asserted for one cycle
    input  wire                         clk,                                    //+1 
    input  wire                         go,                                     //+2 initiate operation (one clock)
    input  wire[4:0]                    subOp,                                  //+3 
    input  wire[TGT_BITS-1:0]           left,   //== samRegs[nReg]              //+4 
    input  wire[TGT_BITS-1:0]           rite,   //== samRegs[sReg]              //+5 
    output reg [TGT_BITS:0]             wordO,                                  //+6 word result (extra bit)
    output wire                         carryO
   );                                                                           //

reg       ccc=0, goP1;                                                          //
assign carryO = (subOp == OPS_SHR || subOp == OPS_RCR) ? rite[0]          :     //
                (subOp == OPS_SHL || subOp == OPS_RCL) ? rite[TGT_BITS-1] :     //
                                                         wordO[TGT_BITS];       //one cycle late

always @(posedge clk)                                                           //
   begin arithmetic:                                                            //
   if (rsta) begin wordO <= 0; goP1 <= 0; end                                   //
   else                                                                         //
       begin                                                                    //
       goP1 <= go;                                                              //
       if (go)                                                                  //
           begin                                                                //
         //$display("SamArithmetic: OP(%1d), left=%X, rite=%X, carry=", subOp, left, rite, ccc);
           case (subOp)                                                         //
             OPS_ADD: wordO <= left + rite;                                     // 4'h0
             OPS_ADC: wordO <= left + rite + ccc;                               // 4'h1
             OPS_CMP,                                                           // 4'h3 caller ignores wordO
             OPS_SUB: wordO <= left - rite;                                     // 4'h2
             OPS_SBB: wordO <= left - rite - ccc;                               // 4'h4
             OPS_XOR: wordO <= left ^ rite;                                     // 4'h5
             OPS_OR:  wordO <= left | rite;                                     // 4'h6
             OPS_AND: wordO <= left & rite;                                     // 4'h7
             OPS_XSUB:wordO <= rite - left;                                     // 4'h8
             OPS_XSBB:wordO <= rite - left - ccc;                               // 4'h9
             OPS_INC: wordO <= rite+1;                                          // 4'hA
             OPS_DEC: wordO <= rite-1;                                          // 4'hB
             OPS_SHL: wordO <= {rite,1'b0};                                     // 4'hC
             OPS_SHR: wordO <= {1'b0,rite} >> 1;                                // 4'hD
             OPS_RCL: wordO <= {rite,ccc };                                     // 4'hE
             OPS_RCR: wordO <= {ccc, rite} >> 1;                                // 4'hF
           endcase                                                              //
           end //if (go)...                                                     //
       if (goP1) ccc <= carryO;                                                 //capture carry one cycle later
   end                                                                          //
   end //always...                                                              //

endmodule //SamArithmetic...
