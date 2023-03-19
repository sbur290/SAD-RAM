//WARNING: generated code#1
/*Smear grpMask from the groups situated over the keys to preceding grps supporting this hITEM.
The ideas is to get grpMask dense with '<' condition so that the shift (when it comes)
will engage all groups at or beyond the insertion point.

For example: with keysize == 8, targetBus == 8,
    group size    == targetbus Size
    sizeof(hINDX) == 6+keySize=14 bytes, which need 2 groups.
    So when the group identified by overTgt reports that the target > dramI that conclusion
    needs to be promulgated to the group immediately before the overTgt group, ie,
       grpMask[grpN] <= grpRslt[grpN+1][0]; //[0] = COMPARE.gt
    For the group situated over the key, it is a little easire:
       grpMask[grpN] <= grpRslt[grpN][0];   //[0] = COMPARE.gt
For hPAGE/hBOOK the same situation prevails, however, size(hPAGE) == 14+keySize = 3 groups.
    Accordingly, when off-target:
       grpMask[grpN]   <= grpRslt[grpN+2][0];
       grpMask[grpN+1] <= grpRslt[grpN+2][0];
    this requires a more hairy gorilla, namely:
       grpMask[grpN]   <= grpRslt[(grpN/3)*3 + 2][0];
    and when on-target, the familiar:
       grpMask[grpN] <= grpRslt[grpN][0];
*/

`include "samDefines.sv"

module GroupSmear //its a kind of cheese :)
   (input  wire                         clk,                                            //+0
    input  wire                         smearGo,                                        //+1 1=prepare, 2=doit
    input  wire                         setBit,                                         //+2 1=set grpMask
    input  wire                         inxBit,                                         //+3 1=hPAGE/hBOOK, 0=hINDX
    input  wire [GROUP_CNT-1:0]         overTgt,                                        //+4 group[grpN] is situated over a key
    input  wire `COMPARE                grpRslt[GROUP_CNT+1],                           //+5 NOTE: extra location
    input  wire [GROUP_CNT-1:0]         stop,                                           //+6
    output reg  [GROUP_CNT-1:0]         grpMask                                         //+7
   );                                                                                   //

genvar grpN;
wire [GROUP_CNT:0]                      stopX={1'b1, stop};                             //
for (grpN=0; grpN < GROUP_CNT; grpN++)                                                  //
  always @(posedge clk)                                                                 //
    if (smearGo && setBit)                                                              //
       grpMask[grpN] <= overTgt[grpN] ? grpRslt[grpN]        [0] & !stopX[grpN]  :      //on target
                        inxBit        ? grpRslt[grpN+1]      [0] & !stopX[grpN+1]:      //two groups/item
                                        grpRslt[(grpN/3)*3+2][0] & !stopX[(grpN/3)*3+2];//three groups/item
endmodule //GroupSmear...

//END WARNING: generated code#1
