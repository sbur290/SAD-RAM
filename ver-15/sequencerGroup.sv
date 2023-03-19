/*Create Date: Jan 28, 2023, rev 13
  Module Name: sequencerGroup
  The group is an array of TARGETBUS_SIZE cells laid out in a diagonal fashion
  to intersect the DRAM sense lines and the target bus. In the canonical case
  in which the key size is equal to target bus  size the group returns a single
  COMPARE - namely {target == kReg, target < kreg}.
  The group is the largest target that can be compared in a single cycle.
  Target Devices:
  Tool Versions:
  Description:

//WARNING: generated code#1
 A sequencer group is an array of sequencerCells (0 thru n-1), where n == TARGETBUS_SIZE.
            ┌───cell#0────┐                                                                       ╓─next group──┐
conn[0]─────┼►prevI nextO─┼─►─────────────────────────────────────────────────────────────────────╟►prevI nextO─┼─►
target[0]───┼►────────────┼─►─────────────────────────────────────────────────target[0]───────────╟►────────────┼─►
compare[0]──┼─►cmpI       │             ┌───cell#1────┐                                           ║             │
            │     cmpO────┼┐  conn[1]───┼►prevI nextO─┼─►─────────────────────────────────────────╟►prevI nextO─┼─►
            │             ││  target[1]─┼►────────────┼─►─────────────────────target[1]───────────╟►────────────┼─►
            └────┬───┬────┘└─►compare1──┼─►cmpI       │                           ┌───cell#n-1──┐ ║             │
                 ↑   ↓                  │     cmpO────┼─┐            conn[n-1]────┼►prevI nextO─┼─╟►prevI nextO─┼─►
                 │   │                  └────┬───┬────┘ │            target[n-1]──┼►────────────┼─╟►────────────┼─►
                 │   │                       ↑   ↓      └►compare2...compare[n-1]─┼─► cmpI      │ ╙─────────────┘
                 │   │                       │   │                                └───┬───┬─────┘
      dramI[0]──►┘   └───►dataO[0] dramI[1]─►┘   └──►dataO[1]            dramI[n-1]──►┘   └──►dataO[n-1]

//END WARNING: generated code#1
* The hardware organization of the group is rigidly defined by the parameter n=TARGETBUS_SIZE.        *
* Groups are replicated across an entire row of memory (DRAM).                                        *
* The software structure is overlaid on this hardware and organized into hITEMs. Each hITEM comprises *
* some binary information (adrL, adrH, pageL, pageH, count, total, & stop bit) followed by a user key.*
* Since the size of the user key is unknown at design time the size of an hITEM is 'variable'.        *
*                                                                                                     *
* hITEMs must align their key fields exactly with the target bus; this requires alignment bytes in    *
* front of each ITEM. For example assuming keySize == 7 and TARGETBUS_SIZE == 8:                      *
* - an hINDX      (align=3 bytes + hINDX      =14 bytes) requires two   groups of 8 cells.            *
* - an hPAGE/hBOOK(align=3 bytes + hPAGE/hBOOK=21 bytes) requires three groups of 8 cells.            *
*                                                                                                     *
* The last bit of the binary field(ie., the high order bit in the group immediately preceding the key)*
* signals that this hITEM is the last in the row.                                                     */

`include "samDefines.sv"

module SequencerGroup
  #(GROUP_NO=0)                                                                 //
   (input  wire                   clk,                                          //+0
    input  wire[15:0]             clkCount,                                     //+1
    input  wire OPCODE            seqOp,                                        //+2
    input  wire[TGT_BITS-1:0]     target,                                       //+3
    input  wire                   stopHere,                                     //+4    stop bits
    input  wire                   stopB4,                                       //+5       "
    output wire                   stopO,                                        //+6       "
    output reg                    rowFullO,                                     //+7    row is full   
    input  wire `COMPARE          grpRsltI,                                     //+8    compare result from group on right
    output wire `COMPARE          grpRsltO,                                     //+9    result of this compare sent to group on left
    output reg                    overTgt,                                      //10
    input  wire                   grpMask,                                      //11   
    input  wire                   insertPoint,                                  //12
    input  wire[TGT_BITS-1:0]     prevI,   output wire[TGT_BITS-1:0]nextO,      //13,14 this group is at the insertion point
    input  wire[TGT_BITS-1:0]     dramI,   output wire[TGT_BITS-1:0]dramO       //15,16 group to group bus
   );                                                                           //

genvar                            cellN;                                        //cell number within group
wire `COMPARE                     compare[TARGETBUS_SIZE+1],                    //compare results passed cell[n]->cell[n+1]
                                  cmpRslt[TARGETBUS_SIZE+1];                    //compare returns passed cell[n]<-cell[n+1]
assign                            compare[0] = 2'b10;                           //presume initial equality in cmp chain
assign                            grpRsltO   = compare[TARGETBUS_SIZE];         //harvest result at extreme end of cmp chain.
assign                            stopO      = stopHere || stopB4;              //past valid group
reg                               scanPlus1  = 0;                               //
reg   [7:0]                       konfig;                                       //from OP_CFG_G
reg   [GROUP_ADR_BITS-1:0]        insPt;                                        //
reg                               lastBinGrp = 0;                               //contains stop bit for last hITEM

/*konfig contains the configuration byte for this group.                        *
* The control bits are specified by the software in the light of:               *
*     - keysize,                                                                *
*     - sizeof(hITEM) being processed,                                          *
*     - the position of the group in the array.                                 *
* konfig is captured on OP_CFG_G and remains unchanged thereafter.              *
* overTgt  is DYNAMIC and signals that the group is situated over a key field,  *
* ie, not alignment/binary bits. It changes depending on op.rowType[0].         *
*                                                                               *
* For example: with keySize == 8 and targetBus_sz == 8, hINDXs occupy 2 groups  *
* (128 bits) and hPAGE/hBOOK occupy 3 groups (192 bits). Based on this geometry *
* the hINDX settings for konfig are:                                            *
*   - konfig[1]          is  (overTgt, first),                                  *
*   - konfig[3,5,7,9...] are (overTgt),                                         *
*   - konfig[30]         is  (overTgt, last)                                    *
*   - all other konfig[] elements are inactive.                                 *
* and the hPAGE/hBOOK settings for konfig are:                                  *
*   - konfig[2]          is  (overTgt, first),                                  *
*   - konfig[5,8,11...]  are (overTgt),                                         *
*   - konfig[29]         is  (overTgt, last)                                    *
*   - konfig[30,31]      are dead, ie. they do not participate in comparisons.  *
*   - all other konfig[] elements are inactive (overTgt == 0).                  */

//Assemble the five control lines to sequenceCell.                              //
CELL_CTRL cellCtrl;                                                             //Control lines for each cell
    assign cellCtrl.cfg        =  (seqOp.u16[8:0] == OP_CFG_C);                 //do config operation
    assign cellCtrl.isScan     = `IsScanOp(seqOp);                              //ie OP_SCAN or OP_SCIN
    assign cellCtrl.rowTypeInx =  seqOp.sc.rowType[0];                          //rowType of SCAN is hINDX
    assign cellCtrl.doShift    = !grpMask;                                      //do shift operation
    assign cellCtrl.insPt      =  insertPoint;                                  //cell is at insertion point
                                                                                //
`define SelectByte(n) (n+1)*CELL_SIZE-1 : (n)*CELL_SIZE                         //selects byte for each cell
for (cellN=0; cellN < TARGETBUS_SIZE; cellN++)                                  //
    begin                                                                       //
    SequencerCell#(GROUP_NO * TARGETBUS_SIZE + cellN)                           //
       celli(clk, cellCtrl, target[`SelectByte(cellN)],                         //+0,1,2
           compare[cellN],              compare[cellN+1],                       //+3,4  compare propagating right
           cmpRslt[cellN],              cmpRslt[cellN+1],                       //+5,6  compare result propagating left
           prevI  [`SelectByte(cellN)], nextO  [`SelectByte(cellN)],            //+7,8  prevI/nextO
           dramI  [`SelectByte(cellN)], dramO  [`SelectByte(cellN)]);           //+9,10
    end //for (cellN ...                                                        //

//Capture group config bits on OP_CFG_G                                         //
always @(posedge clk) if (seqOp.u16[8:0] == OP_CFG_G) konfig <= dramI[7:0];     //capture 1st config byte of each group
                                                                                //
//Set overTgt & lastBinGrp for the actual cITEMs in the DRAM row.               //
//Select configuration based upon rowType (1=hINDX[] array, 0=hPAGE[] array)    //
always @(posedge clk)                                                           //
   if (`IsScanOp(seqOp))                                                        //ie OP_SCAN or OP_SCIN
       begin                                                                    //
       overTgt   <= seqOp.sc.rowType[0] ? konfig[`hCFG_INDX_BITS] != hCFG_IDLE  //
                                        : konfig[`hCFG_PAGE_BITS] != hCFG_IDLE; //
       lastBinGrp<= seqOp.sc.rowType[0] ? konfig[`hCFG_INDX_LASTB]              //
                                        : konfig[`hCFG_PAGE_LASTB];             //
       end                                                                      //

always @(posedge clk)
    rowFullO <= lastBinGrp && (dramI[`SelectByte(TARGETBUS_SIZE-1)] & 8'h80)!=0;//0x80 bit = stop bit

always @(posedge clk)                                                           //
    begin                                                                       //
    scanPlus1 = `IsScanOp(seqOp);                                               //
    if (scanPlus1)                                                              //
       begin                                                                    //
       if (GROUP_NO == 0) insPt <= 0; else                                      //
       if (insPt == 0)    insPt <= !grpRsltI[0] && grpRsltO[0];                 //
       end                                                                      //
    end                                                                         //

`ifdef XILINX_SIMULATOR //{-----------------------------------------------------//<*>
OPCODE               lastOp;                                                    //<*>
reg  [2:0]           bugLevel;                                                  //<*>
                                                                                //<*>
always @(posedge clk) if (`IsRdWrScOp(seqOp)) lastOp   <= seqOp;                //<*>interesting opcodes
                                                                                //<*>
always @(posedge clk) if (`IsBugOp(seqOp))    bugLevel <= seqOp.bug.level[2:0]; //<*>more bits available
                                                                                //<*>
//Output results of one hITEM comparison. sim.exe/emu.exe formats for display.  //<*>
always @(posedge clk)                                                           //<*>
   if (bugLevel >= 6 && `IsBugOp(seqOp) && `IsScanOp(lastOp) && overTgt)        //<*>
       $write("#checkG:%X,%04X,%X,%X, %03X,%X,%X,%X\n",                         //<*>
                       clkCount, lastOp, target, dramI,                         //<*> 1,2,3,4
                       GROUP_NO, grpRsltI, grpRsltO, stopO);                    //<*> 5,6,7,8

`endif //XILINX_SIMULATOR ... -------------------------------------------------}//<*>

endmodule