/*Simulation Driver.sv, Sep 17, 2022 18:16:50, rev 9.
 Driver for SADRAM Verilog implementation.
 Target: XC7Z020 (nominal)
 Tool:   Vivado 2021.2
*/

`include "samDefines.sv"

module SimulationDriver();

//Ordinary parameter, registers, and wires                                      //
reg                                    clk=0;                                   //simulated clock
reg [TGT_BITS-1:0]                     target    = 0;                           //target to sequencer
reg [1:0]                              targetType;                              //
OPCODE                                 lastOp;
reg [2:0]                              state     = 0;                           //
reg [15:0]                             tgtInx    = 0;                           //not $clog2()-1 please
reg                                    samGo     = 0;                           //
wire                                   samIdle;                                 //asserted when samControl is idle
reg [15:0]                             clkCount  = 0;                           //
reg [3:0]                              samStatus = 0;                           //sizeof op.go.cond
reg                                    rsta      = 0;
reg [15:0]                             rstCount  = 16'h8000;

reg [TGT_BITS-1:0] zbits;                                                       //
genvar ii; for (ii=0; ii < TGT_BITS; ii++) initial zbits[ii] <= 1'bz;           //can you be serious ?

//Following substitutes for an input que; loaded at startup                     //
(*ram_style="block"*)   reg[ROW_BITS-1:0]   cfgVector[0:0];                     //configuration vector (=first row of bram only)

initial
    begin                                                                       //
    $write("\n##Start Simulation; rev=%1d;\n", revision);                       //start message for sim.exe
    end

//Clock generator and clock counter                                             //
always #(clkPeriod) begin clk <= ~clk; if (clk) clkCount++; end                 //

SamControl ssc(rsta, clk, clkCount, samGo, samIdle, target);                    //

//Simulation driver basic state machine
always @(posedge clk)
   case(state)                                                                  //
      //initialization                                                          //
      0: begin                                                                  //
         $write("##Params: keySize=%2d, rowSize=%3d\n", KEY_BYTES, ROW_BYTES);  //
         tgtInx <= 0;                                           state <= 2;     //goto idle
         end                                                                    //
      //Feed commands to sequencer from file userCmds.data.                     //
      2: begin                                                                  // 
         rstCount <= rstCount/2; rsta = rstCount[3];                            //assert rsta for one cycle 
         if (rstCount == 0)                                     state <= 3;     //
         end
      3: if (samIdle) begin samGo <= 1;                         state <= 4; end //samGo != 0 triggers SAM state machine
      4: begin                  if (samIdle == 0)               state <= 5; end //wait for op to start (ie, SAM non-idle)
      5: begin samGo  <= 0;     if (samIdle != 0)               state <= 6; end //wait for op to finish(ie, SAM returns to idle)
      6: begin tgtInx++;                                        state <= 2; end //next opcode
   endcase                                                                      //

endmodule //SimulationDriver...

//end of file
