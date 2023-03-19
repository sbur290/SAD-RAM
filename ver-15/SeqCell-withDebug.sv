/* SequencerCell.sv, Sep 19,2022 11:47:23, rev 11
 Target: XC7Z020 (nominal)
 Tool:   Vivado 2021.2

 The cell is replicated for every byte across the DRAM row buffer.
 The basic operation is to compare the target byte against the byte read from DRAM
 and insert target at the insertion point in the BRAM buffer.
    - cells to the left  of the insertion point: do nothing
    - cell  at              the insertion point: load kreg from target (ie do insert)
    - cells to the right of the insertion point: load kreg from cell to the left (ie right shift)
 Behaviour depends upon the opcode and the configuration byte.
 The critical code is the computation of cmpO:
//WARNING: generated code#1
┌──────────┬───────┬───────┬───────┬────────────────────────────────────────────────────────────┐
│ Config   │ cmpI  │thisCmp│ RSTLO │            Comment                                         │
│          │gtr equ│gtr equ│gtr equ│                                                            │
├──────────┼───┬───┼───┬───┼───┬───┼────────────────────────────────────────────────────────────┤
│          │ 0 │ 0 │d/c│d/c│ 0 │ 0 │                                                            │
│hCFG_IDLE │ 0 │ 1 │d/c│d/c│ 0 │ 1 │ cmpI duplicated to cmpO regardless of thisCmp result       │
│          │ 1 │ 0 │d/c│d/c│ 1 │ 0 │                                                            │
│          │ 1 │ 1 │d/c│d/c│ 1 │ 1 │                                                            │
├──────────┼───┼───┼───┼───┼───┼───┼────────────────────────────────────────────────────────────┤
│          │d/c│d/c│ 0 │ 0 │ 0 │ 0 │                                                            │
│hCFG_FRST │d/c│d/c│ 0 │ 1 │ 0 │ 1 │ cmpI is ignored.                                           │
│          │d/c│d/c│ 1 │ 0 │ 1 │ 0 │ cmpO == thisCmp regardless of cmpI                         │
│          │d/c│d/c│ 1 │ 1 │ 1 │ 1 │                                                            │
├──────────┼───┼───┼───┼───┼───┼───┼────────────────────────────────────────────────────────────┤
│          │ 0 │ 0 │ 0 │ 0 │ 0 │ 0 │ \                                                          │
│          │ 0 │ 0 │ 0 │ 1 │ 0 │ 0 │  \ target < kreg from cell to the left.                    │
│          │ 0 │ 0 │ 1 │ 0 │ 0 │ 0 │  / cmpO == cmpI regardless of thisCmp                      │
│          │ 0 │ 0 │ 1 │ 1 │ 0 │ 0 │ /                                                          │
│          │ 0 │ 1 │ 0 │ 0 │ 0 │ 0 │ \                                                          │
│hCFG_MIDL │ 0 │ 1 │ 0 │ 1 │ 0 │ 1 │  \ target == kreg from cell to the left.                   │
│   or     │ 0 │ 1 │ 1 │ 0 │ 1 │ 0 │  / cmpO == thisCmp comparison                              │
│hCFG_LAST │ 0 │ 1 │ 1 │ 1 │ 1 │ 1 │ /                                                          │
│          │ 1 │ 0 │ 0 │ 0 │ 1 │ 0 │ \                                                          │
│          │ 1 │ 0 │ 0 │ 1 │ 1 │ 0 │  \ target > kreg from cell to the left.                    │
│          │ 1 │ 0 │ 1 │ 0 │ 1 │ 0 │  / cmpO == cmpI regardless of thisCmp                      │
│          │ 1 │ 0 │ 1 │ 1 │ 1 │ 0 │ /                                                          │
├──────────┼───┼───┼───┼───┼───┼───┼────────────────────────────────────────────────────────────┤
│          │ 1 │ 1 │d/c│d/c│ 1 │ 1 │ Cell inactive; pass inactive to cell to the right          │
└──────────┴───┴───┴───┴───┴───┴───┴────────────────────────────────────────────────────────────┘
//END WARNING: generated code#1
*/

`include "samDefines.sv"

module SequencerCell
#(CELL_NO=9999)                                                                 //
   (input  wire                     clk,                                        //+0
    input  wire [15:0]              clkCount,                                   //+1
    input  wire [`OP_SHORT_BITS-1]  seqOp,                                      //+2
    input  wire [CELL_SIZE-1:0]     target,                                     //+3
    input  wire `COMPARE            cmpI,   output wire `COMPARE     cmpO,      //+4,5  compare from left neighbour
    output wire `COMPARE            rsltO,  input  wire `COMPARE     rsltI,     //+6,7  compare from right neighbour
    input  wire [CELL_SIZE-1:0]     byteI ///,  output reg [CELL_SIZE-1:0] byteO//+8,9  interface to DRAM
   );                                                                           //
                                                                                //
reg  [CELL_SIZE-1:0]          kreg;                                             //internal cell.value
reg  [CFG_BITS-1:0]           konfig;                                           //configuration of this cell.
reg  [1:0]                    cmpO_r   = 0;                                     //latched results
reg                           scanPlus1= 0;

//The geometry of hITEM changes with the hITEM stored in the BRAM row.          //
//item1st and itemLast signal the cell is first or last in an hITEM.            //
//These are latched when OP_ISET_BIT is asserted in anticipation of the next op.//
//This to save propagation delays in the daisy chain of compare functions.      //
reg                           item1st = 0, itemLast = 0;                        //first/last cells in upcoming row
always @(posedge clk)                                                           //
   begin                                                                        //
   if (seqOp[`OP_SET_TYPE] == 1 && seqOp[`OP_INX_TYPE] == 1)                    //
         begin item1st  <= konfig[`hCFG_INDX_BITS] == hCFG_FRST;                //
               itemLast <= konfig[`hCFG_INDX_BITS] == hCFG_LAST;                //
         end                                                                    //
   if (seqOp[`OP_SET_TYPE] == 1 && seqOp[`OP_INX_TYPE] == 0)                    //
         begin item1st  <= konfig[`hCFG_PAGE_BITS] == hCFG_FRST;                //
               itemLast <= konfig[`hCFG_PAGE_BITS] == hCFG_LAST;                //
         end
   end

//Very first operation: load cell configuration register
always @(posedge clk)                                                           //
    if (seqOp[OP_CFG_BIT]) konfig <= byteI;                                     //

//OP_SCAN: -------------------------------------------------------------------- //
//NOTE: rsltO is not valid until one clock after the comparison.                //
//cmpO is at the end of a long combinatorial chain so it is latched into cmpO_r.//
wire `COMPARE thisCmp = {target == byteI, target > byteI};                      //results of local comparison
assign        cmpO    = (item1st || cmpI[1] == 1) ? thisCmp : cmpI;             //1st in bank or cmpI.eq means use local compare.
assign        rsltO   = itemLast                  ? cmpO_r  : rsltI;            //daisy chain cmp results back to head of group.
always @(posedge clk)                                                           //
    if (seqOp[OP_SCAN_BIT]) begin cmpO_r <= cmpO; kreg <= byteI; end            //update cell.kreg from byteI


//`ifdef XILINX_SIMULATOR //{-----------------------------------------------------//<*>
////Debugging: $display state of cell: dram byte, target byte, cmpI and cmpO.     //<*>
//reg [1:0] bugCfg;                                                               //<*>
//reg `COMPARE cmpiR, thisR;                                                      //<*>
//always @(posedge clk)                                                           //<*>
//    if (seqOp[OP_SCAN_BIT])                                                     //<*>
//        begin
//        thisR <= thisCmp; cmpiR <= cmpI;                                        //<*>
//        bugCfg <= (seqOp[`OP_INX_TYPE] == ITEM_INDX) ? konfig[1:0] : konfig[3:2];//<*>
//    end                                                                         //<*>
//                                                                                //<*>
//reg [`OP_FULL_BITS-1:0] lastOp;                                                 //<*>
//always @(posedge clk)                                                           //<*>
//    if ((seqOp & (OP_SCAN | OP_READ | OP_WRYT)) != 0) lastOp <= seqOp;          //<*>
//wire [1:0]bugLevel = seqOp[`OP_BUG_LVL];                                       //<*>
//always @(posedge clk)                                                           //<*>
//    begin                                                                       //<*>
////if (CELL_NO <= 15)                                                            //<*>
//    if (bugLevel == 3 && seqOp[OP_SET_BIT] != 0)                                //<*>
//        $write("#checkC:%X,%2X,%1X,%1X,%1X,%1X,%X,%X,%X,%X,%X\n",               //<*>
//               clkCount, lastOp, CELL_NO, item1st, itemLast, bugCfg,            //<*>
//               (lastOp[OP_READ_BIT] ?byteI :target),kreg, cmpiR, thisR, cmpO_r);//<*>
//    end                                                                         //<*>
//`endif //XILINX_SIMULATOR... --------------------------------------------------}//<*>

endmodule
