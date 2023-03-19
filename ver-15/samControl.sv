/*Sam State machine,  Sep 17, 2022 18:16:50, rev 13.
 Control module for SADRAM Verilog implementation.
 Target: XC7Z020 (nominal)
 Tool:   Vivado 2021.2
 Docs:   UG835, UG900
*/

`include "samDefines.sv"

module SamControl
   (input  wire                rsta,                                            //+0 asserted for one cycle at startup
    input  wire                clk,                                             //+1
    input  wire [15:0]         clkCount,                                        //+2
    input  wire                samGo,                                           //+3
    output reg                 samIdleO,                                        //+4 true when SamControl is idle
    input  wire [TGT_BITS-1:0] notTarget                                        //+5 target key to scan for
   );                                                                           //

logic [4:0]                    bugLevel  = 0;                                   //
logic                          bugSummary= 0;                                   //
reg [BRAM_ADR_BITS-1:0]        mAddr=0;                                         //address
reg [TGT_BITS-1:0]             target, nextTarget;                              //
OPCODE                         seqOp, samOp;                                    //
logic[SC_STATE_BITS-1:0]       state      = SC_START;                           //state of samController
logic[1:0]                     wrytGo     = WR_IDLE;                            //
reg [TGT_BITS-1:0]             wrytData;                                        //
reg [BRAM_ADR_BITS-1:0]        theAddr;                                         //address
reg [4:0]                      samStatus  = 0;                                  //
`define                        samStatus_qrdy samStatus[0]                      //[0] =  key available in input que
`define                        samStatus_c    samStatus[1]                      //[1] =  carry or full from OP_SCIN
`define                        samStatus_nc   samStatus[2]                      //[2] = ~carry(from arithmetic op)
`define                        samStatus_z    samStatus[3]                      //[3] =  zero       "
`define                        samStatus_nz   samStatus[4]                      //[4] = ~zero       "
SYSTEM_STAT                    sysStat;                                         //
DEBUG_STAT                     bugStat;                                         //
reg                            seqwryt  = 0;                                    //
reg [4:0]                      opWait   = 0;                                    //
`define IF_OPWAIT(n, stmt)  opWait <= opWait / 2; if (opWait == 0) begin opWait <= n; stmt; end
wire                           readGo   =(seqOp.g.act             == OP_READ)|| //activated by OP_READ
//                                         samOp.u16[`OP_SHORT_BITS]== OP_RDF  || //about to start SC_RDF	needed ???
                                        `IsRdfState(state);                     //[4] signals SC_RDF<i>
wire[TGT_BITS-1:0]             wordOut;                                         //from ReadWord module
wire[ROW_BITS-1:0]             bramIn;                                          //dram row to block RAM
reg [ROW_BITS-1:0]             bramOut, seqIn;                                  //dram row from block RAM
wire[ROW_BITS-1:0]             seqOut;                                          //row to/from sequencer
reg [TGT_BITS-1:0]             alignment;                                       //
wire [GROUP_CNT-1:0]           grpMask;                                         //
wire                           rowFull;                                         //signals row full on OP_SCIN
reg  [GROUP_CNT-1:0]           saveGrpMask, insPt;                              //
wire [TGT_BITS:0]              arithOut;                                        //NOTE: extra bit
wire                           carryOut;                                        //
                                                                                //
reg  [TGT_BITS-1:0]            fieldIn;                                         //input  to   RWfield
reg  [TGT_BITS-1:0]            fieldOut;                                        //field read by RWfield
wire [2*TGT_BITS-1:0]          structOut;                                       //structure output from RWfield
reg                            fieldGo;                                         //starts ReadWriteField module

wire [2:0]                     breg      = samOp.ind.breg;                      //
wire [2:0]                     areg      = samOp.ind.areg;                      //
wire [4:0]                     cond      = samOp.go.cond;                       //only for OP_GO_T or OP_GO_F
wire [4:0]                     act5      = samOp.g.act;                         //action bits for most cases except go, ldi, and r2r
wire [2:0]                     act3      = samOp.go.act;                        //action bits for OP_GO_T and OP_GO_F
wire [BRAM_ADR_BITS-1:0]       samAdr    = samOp.g.adr[BRAM_ADR_BITS-1:0];      //
wire [4:0]                     subOp     = samOp.arith.subOp;                   //
wire                           takeJmp   = (((samStatus & cond) != cond) ==     //evaluate condition
                                                            samOp.go.act[2]) && //separate GO_T and GO_F ([2] == 1)
                                           `IsGoOp(samOp);                      //
wire                           longJmp   = act3 == OP_GO_T && samOp.go.relAdr == 0; //act[2] == 0 is GO_T
wire                           isCall    = act5 == OP_CALL;                     //
wire                           isRet     = (samOp.u16 == OP_RET);               //in the context of OP_GO
wire                           arithGo   = (act5 == OP_ARITH)  &&               //OP_ARITH from opcode 
                                           (state == SC_DO_OP) &&               //when executing the opcode
                                           !subOp[4];                           //ie excluding OPS_PUSH, _POP, _XCHG, _STEP, and _XTOS
reg                            stepA, stepR, dirn, repeating=0;                 //step adr, step reg, dirn of OP_REPEAT/RPTR
reg [5:0]                      repete = 0;                                      //one bit larger than needed
                                                                                //
(*ram_style="block"*)   reg[ROW_BITS-1:0] bram[BRAM_ROWS-1:0];                  //block RAM
(*ram_style="register"*)reg[TGT_BITS-1:0] samRegs[7:0];                         //
(*ram_style="register"*)reg[`OP_FULL_BITS]microCode[MICROCODE_SIZE-1:0];        //16-bits wide

reg  [BRAM_ADR_BITS-1:0]            curRow, saveRow;                            //
reg  [PGM_ADR_BITS-1:0]             pc     = 0;                                 //
wire signed [PGM_ADR_BITS-1:0]      signedPc= pc;                               //signed to handle +/- relAdr
reg  [TGT_BITS-1:0]                 tempReg;                                    //temp for OPS_R2R
reg  [TGT_BITS-1:0]                 stak[63:0];                                 //
reg  [7:0]                          sp=0;                                       //
                                                                                //
initial                                                                         //
    begin                                                                       //
    $readmemh("blockram.data", bram,      0, BRAM_ROWS-1);                      //read in blockRam created by genVerilog
    $readmemh(`NAMEOF_MICROCODE,microCode,0, MICROCODE_SIZE-1);                 //read in micro code
    samRegs[0] = 101010;samRegs[1] = 111;  samRegs[2] = 222;  samRegs[3] = 333; //
    samRegs[4] = 444;   samRegs[5] = 555;  samRegs[6] = 666;  samRegs[7] = 777; //
    end                                                                         //

//-------------- Supporting modules --------------------------------------------//
SequencerArray aray(clk, clkCount, seqOp, target, seqIn, seqOut, grpMask, rowFull);//
WriteWord      Wryt(clk, wrytGo, bramOut, theAddr, wrytData, bramIn);           //write target to bram[address_bits]
ReadWord       Reed(clk, readGo, bramOut, theAddr, wordOut);                    //read word from bram[address_bits]
SamArithmetic  Arith(rsta, clk, arithGo, subOp,                                 //operation
                                   samRegs[areg], samRegs[breg],                //inputs: $breg = $bReg <op> $areg
                                   arithOut, carryOut);                         //output
ReadWriteField RWfield(rsta, clk, fieldGo, `IsWrField(samOp), samOp.ind.fieldNum,//0-4 command
                            {wordOut, fieldIn}, samRegs[samOp.ind.breg],        //5-6 inputs
                             structOut,         fieldOut);                      //7-8 outputs

//-------------- BlockRAM control ----------------------------------------------//
always @(posedge clk)                                                           //
    begin bramOut <=               bram[curRow];                                //read data from block RAM
          if (wrytGo == WR_COMMIT) bram[curRow] <= seqwryt ? seqOut : bramIn;   //write data to  block RAM
    end                                                                         //

//--------------------------------------------------------------------------------------//
//This is a state machine controlling the sequencer(s).                                 //
always_ff @(posedge clk)                                                                //
    begin samController:                                                                //
    case (state)                                                                        //
      //-------- Controller idle state -------------------------------------------------//
      SC_START:                                                                         //
         if (rsta)                                                                      //
             begin insPt <= 0; pc <= 0; curRow  <= 0; target <= 0;repeating <= 0;       //
                   sp    <= 0;          fieldGo <= 0;             state <= SC_IDLE;     //
             end //wait for restart                                                     //
      SC_IDLE:                                                                          //
         begin seqOp.u16 <= 0; wrytGo <= WR_IDLE;                                       //
               if (pc >= MICROCODE_SIZE)                                                //
                          begin $display("#checkA: Illegal pc=%d", pc);      $stop; end //                                                         
               if (sp[7]) begin                                                         //
                          $display("##Abort:stack over/underflow,pc=%d, sp=%d",pc,sp);  //
                          $stop;                                                        //
                          end                                                           //
                                                                 state <= SC_GETOP;     //
 /* ---->>>> */                                         `samStatus_qrdy <= clkCount[2]; //artificial ready condition
         //    if (`samStatus_qrdy == 0) samIdle <= 1;
         end //SC_IDLE...                                                               //
      SC_GETOP:                                                                         //
         begin                                                                          //
         if (repeating && !dirn)                                                        //
            begin if (stepA) samOp.g.adr++; if (stepR) samOp.g.breg++; repete--; end    //
         else                                                                           //
         if (repeating &&  dirn)                                                        //
            begin if (stepA) samOp.g.adr--; if (stepR) samOp.g.breg--; repete--; end    //
         else                                                                           //
            begin samOp.u16 <= microCode[pc][15:0]; sysStat.op <= microCode[pc]; end    //
         sysStat.stat <= samStatus;     sysStat.insPt  <= insPt;  sysStat.nu <= 0;      //
         sysStat.sp   <= sp;            sysStat.pc     <= pc;                           //
         bugStat.nu   <= 35'h555555555; bugStat.curRow <= curRow; bugStat.y  <= 0;      //
         bugStat.bugLvl<=bugLevel;    //bugStat.nxtOp  <= microCode[pc+1];              //
                                                                 state <= SC_DO_OP;     //
         end //SC_GETOP...                                                              //
      SC_DO_OP:                                                                         //
         begin seqIn  <= bramOut;                                                       //
         seqwryt      <= 0;                                                             //
         samIdleO     <= 0;                                                             //
         saveRow      <= curRow;                                                        //
         sysStat.op   <= samOp.u16;                                                     //
         repeating    <= repete != 0;                                                   //
         if (bugLevel >= 3 ||                                                           //bug
            (bugLevel >= 1 && samOp.g.act == OP_PRINT) &&                               //    (then its up to sim.exe to figure it out). 
             repete == 0)                                                               //
               $write("#checkE:%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X,%X\n",//
                            clkCount,                                                   // 1
                            {bugStat[63:16], microCode[pc+1]},                          // 2
                            {samOp.u16,      sysStat[47:0]},                            // 3 (sysStat.op not updated yet)
                            target,     wordOut,    arithOut,                           // 4- 5
                            samRegs[0], samRegs[1], samRegs[2], samRegs[3],             // 7-10
                            samRegs[4], samRegs[5], samRegs[6], samRegs[7],             //11-14
                            stak[sp-1], stak[sp-2], stak[sp-3], stak[sp-4]);            //15-18
         case (act5)                                                                    //action[4:0]
           OP_WRF, OP_RDF:                                                              //
                    begin                                                               //
                    if (samOp.ind.fieldNum[3])                                          //is field a key field ?
                       begin theAddr <= samRegs[areg] + samOp.ind.fieldNum[2:0];        //key access; fields following handle
                             if (`IsWrField(samOp))                                     //[4] = write versus read
                                   begin wrytGo    <= WR_PREPARE; state <= SC_WRYT; end //piggyback OP_WRYT (full word)
                             else  begin seqOp.u16 <= OP_READ;    state <= SC_READ; end //piggyback OP_READ (full word)
                       end                                                              //
                    else                                                                //binary field access; 
                       begin theAddr <= samRegs[areg]-1;          state <= SC_RDF1; end //   fields precede handle
                    end                                                                 //   and are partial words
           OP_BUG:  begin                                                               //
                    seqOp <= samOp;                                                     //signal debug in group/cell
                    if (samOp.bug.set) bugLevel <= samOp.bug.level;                     //
                    else                                                                //
                    if (samOp.bug.sho == 0)                                             //ie., samOp.u16 == OP_STOP
                       begin $display("##Endof Simulation @pc=%d", pc); $stop; end      //
                    else                                                                //
                        $write("#checkF:%X,%X,%X,%X,%X\n", clkCount,                    //dump current row
                                    samOp.u16, pc, curRow, bramOut);                    //samOp for sim.exe to determine rowType
                    state <= SC_IDLE;                                                   //
                    end                                                                 //
           OP_CROWI:begin curRow<= samAdr;                        state <= SC_IDLE; end //curRow from op.g.adr
         //OP_CFG_G, OP_CFG_C,                                                          //
           OP_RET:  begin                                                               //
                    if (samOp.g.breg != 0) seqOp <= samOp; else                         //signal group/cell to configure
                       begin {bugLevel, pc} <= stak[sp-1]+1; sp--; end                  //restore bugLevel at OP_CALL
                                                                  state <= SC_IDLE; end //
           OP_REPREG,                                                                   //
           OP_REPEAT:begin                                                              //
                    stepA <= samOp.rpt.stepA;                                           //
                    stepR <= samOp.rpt.stepR;                                           //
                    dirn  <= samOp.rpt.bkwd;                                            //
                    repete<= samOp.g.breg[0] ?samRegs[breg]                             //OP_REPREG count from $breg
                                             :{1'b0, samOp.rpt.count};                  //OP_REPEAT count from immediate
                    state  <= SC_IDLE;                                                  //
                    end                                                                 //
           OP_CROW: begin curRow<= samRegs[areg];                 state <= SC_IDLE; end //
           OP_PRINT:begin /*handled by CheckE() in sim.exe */     state <= SC_IDLE; end //
           OP_RI:   begin samRegs[samOp.ri.breg] <= samOp.ri.imm; state <= SC_IDLE; end // 
           OP_LDI,OP_LDI+ 4,OP_LDI+ 8,OP_LDI+12,OP_LDI+16,OP_LDI+20,OP_LDI+24,OP_LDI+28://
                    begin samRegs[0] <= (samRegs[0] << 14) | samOp.ldi.imm;             //
                                                                  state <= SC_IDLE; end //14 = #bits in ldi.imm 
           OP_WRYT: begin theAddr <= samAdr; wrytData <= samRegs[breg];                 //
                          wrytGo  <= WR_PREPARE;                  state <= SC_WRYT;     //prepare for write word
                    end                                                                 //
           OP_READ: begin theAddr <= samAdr; seqOp <= samOp;      state <= SC_READ; end //seqOp != 0 starts sequencer
           OP_SCIN,                                                                     //
           OP_SCAN: begin seqOp  <= samOp; target <=samRegs[breg];state <= SC_SCAN; end //scan first, then possibly insert
           OP_CALL: begin stak[sp]<= {bugLevel, pc}; sp++; pc <= samOp.call.callAdr;    //pc is low order 16-bits
                          if (bugLevel == 3 || bugLevel == 4) bugLevel <= 2;            //kill opcode display, but keep 
                                                                  state<= SC_IDLE;      //   print and #expect/#actual checking
                    end                                                                 //
           OP_GO_F, OP_GO_F+8, OP_GO_F+16, OP_GO_F+24,                                  //jump on false(condition)
           OP_GO_T, OP_GO_T+8, OP_GO_T+16, OP_GO_T+24:                                  //jump on true (condition)
                    begin if (takeJmp)                                                  //
                             begin if (longJmp) pc <= microCode[pc+1];                  //dont use pc <= ... ? ... : ...
                                   else         pc <= samOp.go.relAdr + signedPc+1;     //need signed arithmetic
                             end                                                        //
                                                                  state <= SC_IDLE;     //
                    end                                                                 //
           OP_ARITH:case(subOp)                                                         //
                      OPS_R2R:                                                          //0x10
                        begin tempReg <= samRegs[areg];           state <= SC_R2R;  end // 
                      OPS_XCHG:                                                         //0x11 XCHG areg, breg
                        begin tempReg       <= samRegs[areg];                           //
                              samRegs[areg] <= samRegs[breg];     state <= SC_R2R;      //
                        end                                                             //
                      OPS_XTOS:                                                         //0x12 XTOS reg,[tos]
                        begin tempReg       <= samRegs[areg];                           //
                              samRegs[areg] <= stak[sp-1];        state <= SC_XTOS;     //
                        end                                                             //
                      OPS_POP:                                                          //0x13
                        begin samRegs[breg] <= stak[sp-1]; sp--;  state <= SC_IDLE; end //
                      OPS_PUSH:                                                         //0x14
                        begin stak[sp] <= samRegs[breg];   sp++;  state <= SC_IDLE; end //
                      OPS_PUSH_CURROW: 
                                   begin stak[sp] <= curRow; sp++;state <= SC_IDLE; end //0x15
                      OPS_STC: begin `samStatus_c <= 1; `samStatus_nc <= 0; state <= SC_IDLE; end //0x16
                      OPS_CLC: begin `samStatus_c <= 0; `samStatus_nc <= 1; state <= SC_IDLE; end //0x17
                      OPS_STZ: begin `samStatus_z <= 1; `samStatus_nz <= 0; state <= SC_IDLE; end //0x18
                      OPS_CLZ: begin `samStatus_z <= 0; `samStatus_nz <= 1; state <= SC_IDLE; end //0x19
                    default: begin                                state <=SC_ARITH1;end //
                    endcase                                                             //
                                                                                        //
           default: begin $write("#checkA: Illegal op: pc= %03d:0x%04X\n",pc,samOp.u16);//
                          $stop;                                                        //
                    end                                                                 //
         endcase //(act5)...                                                            //
         if (!(takeJmp || isCall || isRet || repete != 0 || longJmp)) pc <= pc+1;       //inc pc except for jumps/calls/repeats
         opWait <= 2;                                                                   //
         end //SC_DO_OP...                                                              //

      SC_R2R:  begin samRegs[breg] <= tempReg;                    state <= SC_IDLE; end //second half of R2R/XCHG op
      SC_XTOS: begin stak[sp-1]    <= tempReg;                    state <= SC_IDLE; end //second half of XTOS op
                                                                                        //
      /*RWfield second half; read 64-bit word into fieldIn and wordOut. 
        Present this 128-bit blob to RWfield({wordOut, fieldIn}).
        RWfield performs structOut <= extract/modify(blob).
       'theAddr' points to the lower address of the blob.
        OP_RDF returns the field extracted by RWfield.
        OP_WRF updates the blob to the row buf using:
            WriteWord(wrytGo=WR_UPDATE, theAddr, wrytData).
            The second invocation of WriteWord courses thru SC_WRYT1 and updates the 
            row to memory.
        Most fields are written in their native format, however, the total field is
        inverted to MS-byte first to be compatible with OP_SCAN/OP_SCIN. */ 
      SC_RDF1: begin `IF_OPWAIT(2, fieldIn <= wordOut; theAddr--; state <= SC_RDF2); end//accum {wordOut, fieldIn}
      SC_RDF2: begin `IF_OPWAIT(2, fieldGo <= 1; state <= SC_RDF3); end                 //trigger RWfield{wordOut, fieldIn}
      SC_RDF3: begin                                                                    //
               fieldGo <= 0; opWait <= opWait / 2;                                      //fieldGo asserted for one clock
               if (opWait == 0)                                                         //
                  begin                                                                 //
                  if (`IsWrField(samOp))                                                //write [].field = rhs
                     begin                                                              //
                     wrytData <= structOut[2*TGT_BITS-1:TGT_BITS]; wrytGo <= WR_UPDATE; //Wryt(update hi-part)
                     opWait <= 2;                                 state   <= SC_WDF1;   //inc theAddr for second write
                   //$display("FirstWryt[%1d] <= 0x%X", theAddr, structOut[2*TGT_BITS-1:TGT_BITS]);
                     end                                                                //
                  else                                                                  //
                     begin samRegs[samOp.ind.breg]<= fieldOut;    state <= SC_IDLE; end //read $breg = [$areg].field
                  end                                                                   //
               end                                                                      //
      SC_WDF1: begin                                                                    //
               opWait <= opWait / 2; 
               if (opWait == 1) theAddr++;                                              //inc theAddr for second write
               else
               if (opWait == 0) 
                   begin wrytData <= structOut[TGT_BITS-1:0];                           //Wryt(update lo-part)
                         wrytGo   <= WR_UPDATE;                   state <= SC_WRYT1;    //then OP_WRYT commits to row buffer
                       //$display("SecndWryt[%1d] <= 0x%X", theAddr, structOut[TGT_BITS-1:0]);
                   end                                                                  //
               else      wrytGo   <= WR_IDLE;                                           //
               end                                                                      //
          
      SC_ARITH1://samArithmetic results: $nreg <= ($nreg <subop> $sreg)                 //
               begin                                                                    //
               //$display("SC_ARITH1: arithOut=%X, subOp=%04X, sreg=%1d, areg=%1d", arithOut, samOp.arith.subOp, breg, areg);
               if(samOp.arith.subOp != OPS_CMP) samRegs[areg] <= arithOut[TGT_BITS-1:0];//
               `samStatus_c  <=  carryOut;                                              //  carry bit
               `samStatus_nc <= ~carryOut;                                              // ~carry bit
               `samStatus_z  <=  arithOut[TGT_BITS-1:0] == 0;                           //  zero bit
               `samStatus_nz <=  arithOut[TGT_BITS-1:0] != 0;                           // ~zero bit
               state         <= SC_IDLE;                                                //
               end                                                                      //

      //-------- Read single word        
      SC_READ: begin seqOp.u16 <= 0;                              state <= SC_READ1;end //extra state for OP_READ
      SC_READ1:begin samRegs[breg] <= wordOut;                                          //save word read in destn reg 
                     curRow        <= saveRow;                    state <= SC_IDLE; end //restore for OP_RDF case
                                                                  
      //-------- write single word                                
      SC_WRYT: begin wrytGo <= WR_UPDATE; seqwryt <= 0;           state <= SC_WRYT1;end //dial in wrytData to row
      SC_WRYT1:begin wrytGo <= WR_COMMIT; opWait  <= opWait/2;                          //write blockRAM
                     if (opWait == 0)                                                   //
                         begin curRow <= saveRow;                 state <= SC_IDLE; end //
               end                                                                      //

      //-------- scan operation do row wide compare                                     //bramOut already loaded in SC_IDLE
      SC_SCAN: begin                                                                    // 
               seqOp.sc.act <= 0;                                                       //stop scan operation
              `samStatus_c  <= rowFull;                                                 //get full status; set 'impossible'
              `samStatus_nc <= rowFull;                                                 //  combination to signal full
               state        <= !rowFull &&                                              //do not proceed with insert if row is full
                               seqOp.sc.act[3] ? SC_SCIN : SC_IDLE;                     //seqOp[3] distinguished OP_SCIN from OP_SCAN
               end                                                                      //

      //-------- scan-insert operation.                          
      //OP_SCIN is preceeded by OP_SCAN to setup grpMask. 
      //OP_SCIN then remains active for multiple clock cycles during which 
      //sequencerCell right shifts across multiple groups.
      //For row type == INDX      opWait <= 1 (2**0) allows two   shifts (groups/INDX == 2),
      //For row type == PAGE/BOOK opWait <= 2 (2**1) allows three shifts (groups/PAGE == 3)
      //The number of shifts must be increased for keySize > 8 by one for each 8 bytes <---<<<
      SC_SCIN: begin                                                            //
               opWait <= opWait/2;                                              //wait for scan to complete
               if (opWait==0)                                                   //
                   begin                                                        //
                   opWait      <= samOp.sc.rowType[0] ? 1 : 2;                  //multi-cycle setting (embed in opcode !!)
                   seqOp.sc.act<= OP_SCIN;                                      //initiates insert
                   saveGrpMask <= grpMask;                                      //debugging
                   nextTarget  <= 64'h1111111111111111;                         //arbitrary stuff for binary fields 
                   insPt       <= $clog2(grpMask);        state <= SC_SCIN1;    // 
                   end                                                          //
               end                                                              //
      SC_SCIN1:begin                                                            //
               opWait     <= opWait/2;                                          //
               seqIn      <= seqOut;                                            //Update seqIn after a right shift. 
               target     <= nextTarget;                                        //set value for binary part 
               nextTarget <= nextTarget << 1;                                   //     of hINDX/hPAGE
               if (opWait == 0)                                                 //
                   begin seqOp.u16 <= 0;                                        //stop right shifting in seqCell
                         seqwryt   <= 1;                  state <= SC_WRYT1;    //write seqIn back to BRAM
               end end                                                          //
    endcase //(state)...                                                        //

    end //samController: ...

endmodule //SamControl...
