/* SequencerCell.sv, Jan 28,2023@9:53, rev 13
 Target: XC7Z020 (nominal)
 Tool:   Vivado 2021.2

 The cell is replicated for every byte across the DRAM row buffer.
 The basic operations are:
   - compare the target byte against the byte read from DRAM
   - insert target at the insertion point in the BRAM buffer.
     In the performance of this function
       - cells to the left  of the insertion point (cellCtrl.insPt == 0): do nothing
       - cell  at              the insertion point: load byteI from target (ie do insert)
       - cells to the right of the insertion point: load byteI from cell to the left (ie right shift)
 Behaviour depends upon the opcode and the configuration byte.
 The critical code is the computation of cmpO:
//WARNING: generated code#1
┌──────────┬───────┬───────┬───────┬────────────────────────────────────────────────────────────┐
│ Config   │ cmpI  │thisCmp│ rstlO │            Comment                                         │
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
│          │ 0 │ 0 │ 0 │ 1 │ 0 │ 0 │  \ target < byteI from cell to the left.                   │
│          │ 0 │ 0 │ 1 │ 0 │ 0 │ 0 │  / cmpO == cmpI regardless of thisCmp                      │
│          │ 0 │ 0 │ 1 │ 1 │ 0 │ 0 │ /                                                          │
│          │ 0 │ 1 │ 0 │ 0 │ 0 │ 0 │ \                                                          │
│hCFG_MIDL │ 0 │ 1 │ 0 │ 1 │ 0 │ 1 │  \ target == byteI from cell to the left.                  │
│   or     │ 0 │ 1 │ 1 │ 0 │ 1 │ 0 │  / cmpO == thisCmp comparison                              │
│hCFG_LAST │ 0 │ 1 │ 1 │ 1 │ 1 │ 1 │ /                                                          │
│          │ 1 │ 0 │ 0 │ 0 │ 1 │ 0 │ \                                                          │
│          │ 1 │ 0 │ 0 │ 1 │ 1 │ 0 │  \ target > byteI from cell to the left.                   │
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
   (input  wire                  clk,                                           //+0
    input  wire CELL_CTRL        cellCtrl,                                      //+1
    input  wire [CELL_SIZE-1:0]  target,                                        //+2
    input  wire `COMPARE         cmpI,   output wire `COMPARE        cmpO,      //+3,4  compare from left neighbour
    output wire `COMPARE         rsltO,  input  wire `COMPARE        rsltI,     //+5,6  compare from right neighbour
    input  wire [CELL_SIZE-1:0]  prevI,  output wire [CELL_SIZE-1:0] nextO,    //+7,8  bus from cell to cell
    input  wire [CELL_SIZE-1:0]  byteI,  output reg  [CELL_SIZE-1:0] byteO     //+9,10 interface to DRAM
   );                                                                           //
                                                                                //
reg  [CFG_BITS-1:0]              konfig;                                        //configuration of this cell.
reg  [1:0]                       cmpO_r   = 0;                                  //latched results of compare
assign                           nextO    = byteI;                              //
reg                              firstCell = 0, lastCell = 0;                   //first/last cells in group

//Load cell configuration register
always @(posedge clk) if (cellCtrl.cfg) konfig <= byteI;                        //

always @(posedge clk)                                                           //right shift logic
    byteO <= cellCtrl.insPt ? target : cellCtrl.doShift ? prevI : byteI;        //

//The geometry changes with the hITEM stored in the BRAM row.                   //
//firstCell and lastCell latch first or last for this cell in order to save     //
//propagation delays in the daisy chain of compare functions.                   //
always @(posedge clk)                                                           //
   if (cellCtrl.isScan)                                                         //
       begin                                                                    //
       firstCell <= cellCtrl.rowTypeInx ? konfig[`hCFG_INDX_BITS] == hCFG_FRST  //cell is first in group - ignore cmpI
                                        : konfig[`hCFG_PAGE_BITS] == hCFG_FRST; //
       lastCell  <= cellCtrl.rowTypeInx ? konfig[`hCFG_INDX_BITS] == hCFG_LAST  //cell is last in group  - loop cmpO back thru rsltO
                                        : konfig[`hCFG_PAGE_BITS] == hCFG_LAST; //
       end                                                                      //

//OP_SCAN, or OP_SCIN: -------------------------------------------------------- //
//NOTE: rsltO is not valid until one clock after the comparison.                //
//cmpO is at the end of a long combinatorial chain so it is latched into cmpO_r.//
wire `COMPARE thisCmp = {target == byteI, target > byteI};                      //results of local comparison
assign        cmpO    = (firstCell || cmpI[1] == 1) ? thisCmp : cmpI;           //1st in bank or cmpI.eq means use local compare.
assign        rsltO   = lastCell                    ? cmpO_r  : rsltI;          //daisy chain cmp results back to head of group.

endmodule
