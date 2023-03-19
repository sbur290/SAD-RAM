/*sequencerArray.sv create Date: Sep 21, 2022, rev 9
  The sequencerArray comprises an array of sequencerGroups. Each group
  performs a comparison of m_tagetBusSz dramI bytes input versus the target.
  Target Devices:
  Tool Versions: Vivado #2019.2
*/

`include "samDefines.sv"

module SequencerArray
   (input  wire                         clk,                                    //+0
    input  wire [15:0]                  clkCount,                               //+1 debugging only
    input  wire OPCODE                  seqOp,                                  //+2
    input  wire [TGT_BITS-1:0]          target,                                 //+3
    input  wire [ROW_BITS-1:0]          dramI,                                  //+4 one row of DRAM
    output wire [ROW_BITS-1:0]          dramO,                                  //+5
    output reg  [GROUP_CNT-1:0]         grpMask,                                //+6
    output wire                         rowFullO                                //+7
   );                                                                           //

genvar  grpN;                                                                   //
//dramZ = dramI + a stop bit to accommodate the 'for (grpN' loop below.         //
wire  [ROW_BITS:0]                   dramZ    = {dramI, 1'b0};                  //to accommodate stop bit calculation
logic [(GROUP_CNT+1) * TGT_BITS-1:0] pnBus;                                     //group to group bus
wire  [GROUP_CNT-1:0]                stop;                                      //group to group stop bit
wire  `COMPARE                       grpRslt[GROUP_CNT+1];                      //group to group compare results
assign                               grpRslt[GROUP_CNT] = 2'b00;                //finally result tied to the door knob
OPCODE                               lastOp;                                    //
wire [GROUP_CNT-1:0]                 overTgt;                                   //=1 if group is over the key
wire [GROUP_CNT-1:0]                 rowFull;                                   //=1 if group is over the key
assign                               rowFullO = |rowFull;                       //long or of rowFull from each group

`define GroupBits(n)   (n+1) * TGT_BITS-1 : (n) * TGT_BITS                      //partition of dram for each sequencerGroup

//Instantiate the sequencerGroups                                               //
    SequencerGroup #(0)                                                         //
      groupi(clk, clkCount, seqOp, target,                                      //+0,1,2,3, target of search
             1'b0, 1'b0, stop[0], rowFull[0],                                   //+4=stopHere, 5=stopB4, 6=stopOut, +7=rowFull
             2'b00,                                                             //+8
             grpRslt[0],                                                        //+9    grpCmp[n+1]=output from diagonal group
             overTgt[0],                                                        //10
             grpMask[0],                                                        //11
             1'b0,                                                              //12
             pnBus[`GroupBits(0)], pnBus[`GroupBits(1)],                        //13,14 group to group bus
             dramI[`GroupBits(0)], dramO[`GroupBits(0)]);                       //15,16
for (grpN=1; grpN < GROUP_CNT; grpN++)                                          //
    SequencerGroup #(grpN)                                                      //
      groupi(clk, clkCount, seqOp, target,                                      //+0,1,2,3, target of search
             dramI[(grpN)*TGT_BITS-1], stop[grpN-1], stop[grpN], rowFull[grpN], //+4,5,6,7, stop bit, 5=stopB4, 6=stopOut
             grpRslt[grpN-1],                                                   //+8    daisy chain back through cITEM
             grpRslt[grpN],                                                     //+9    grpCmp[n+1]=output from diagonal group
             overTgt[grpN],                                                     //10
             grpMask[grpN],                                                     //11    input for insert operation
            !grpMask[grpN] && grpMask[grpN-1],                                  //12    insertpoint
             pnBus[`GroupBits(grpN)], pnBus[`GroupBits(grpN+1)],                //13,14 group to group bus
             dramI[`GroupBits(grpN)], dramO[`GroupBits(grpN)]);                 //15,16
                                                                                //
always @(posedge clk) if (`IsRdWrScOp(seqOp)) lastOp <= seqOp;                  //

reg scanPlus1, scanPlus2;
always @(posedge clk) begin scanPlus2 <= scanPlus1; scanPlus1 <= `IsScanOp(seqOp); end

//Smear grpMask bits over the entire item. The depends on the sizeof(hITEM) and keySize
GroupSmear smear(clk, scanPlus2, `IsScanOp(lastOp), lastOp.sc.rowType[0], overTgt, grpRslt, stop, grpMask);//

endmodule
