#ifndef _OPCODE_H_INCLUDED
#define _OPCODE_H_INCLUDED
#include <C3_always.h>
#include <C3_errors.h>

#define BRAM_ADR_BITS 5

#pragma pack (push, 1)
typedef struct                                                                  //samStatus or op_go.condition bits
   {uint8_t qrdy:1,                                                             //key available in input que
            cc:1, nc:1,                                                         //carry and ~carry
            zz:1, nz:1;                                                         //zero  and ~zero
   } CONDITION_CODES;                                                           //

#define OP_CALL       0x00                                                      //set pc <= target+1
#define OP_BUG        0x18                                                      //output bug info, set bugLevel
#define OP_ARITH      0x02                                                      //
#define OP_RI         0x06                                                      //+dst*32 + src*4
#define OP_RET        0x0A                                                      //
#define OP_CFG_G      0x8A                                                      //configure groups
#define OP_CFG_C      0xAA                                                      //configure cells
#define OP_REPREG     0x0E                                                      //repeat, count = $preg
#define OP_REPEAT     0x12                                                      //repeat fixed number of times
#define OP_CROWI      0x16                                                      //set current row from op.s.adr bits
#define OP_CROW       0x1A                                                      //set current row
#define OP_PRINT      0x1E                                                      //
#define OP_GO_T       0x01                                                      //set pc <= target+1
#define OP_GO_F       0x05                                                      //set pc <= target+1
#define OP_NOOP       0x05                                                      //
#define OP_STOP     0x0018                                                      //
#define OP_RDF        0x08                                                      //
#define OP_WRF        0x10                                                      //
#define OP_LDI        0x03                                                      //
#define OP_READ       0x04                                                      //read  word at row[address]
#define OP_WRYT       0x0C                                                      //write word at row[address]
#define OP_SCAN       0x14                                                      //scan DRAM row
#define OP_SCIN       0x1C                                                      //scan, right shift & insert DRAM row
//Sub operations of OP_ARITH
#define OPS_ADD       0x00                         // binary ops                //-+
#define OPS_ADC       0x01                         //     "                     //  \.
#define OPS_SUB       0x02                         //     "                     //   \.
#define OPS_SBB       0x03                         //     "                     //    \.
#define OPS_CMP       0x04                         //     "                     //     \.
#define OPS_XOR       0x05                         //     "                     //      \.
#define OPS_OR        0x06                         //     "                     //       \.
#define OPS_AND       0x07                         //     "                     //        \. 
#define OPS_XSUB      0x08                         //     "                     //        /.Operators implemented
#define OPS_XSBB      0x09                         //     "                     //       /. in samArithmetic.sv
#define OPS_INC       0x0A //-------------------------unary ops ----------------//      /. 
#define OPS_DEC       0x0B                         //     "                     //     /.  
#define OPS_SHL       0x0C                         //     "  shift by one bit   //    /. 
#define OPS_SHR       0x0D                         //     "         "           //   /.
#define OPS_RCL       0x0E                         //     "         "           //  /. 
#define OPS_RCR       0x0F                         //     "         "           //-+.
#define OPS_R2R       0x10 //-------------------------binary ops ---------------//-+\.    
#define OPS_XCHG      0x11                         //                           //   |. Binary Operators
#define OPS_XTOS      0x12                         //                           //-+/.  (implemented inline)
#define OPS_POP       0x13 //-------------------------unary ops ----------------//-+\.  Unary Operators
#define OPS_PUSH      0x14                         //                           //- |.  (implemented inline)
#define OPS_PUSH_CURROW 0x15                       //                           //-+/
#define OPS_STC       0x16 //-------------------------nonary ops ---------------//-\.
#define OPS_CLC       0x17                         //                           //  \.  Nonary Operators
#define OPS_STZ       0x18                         //                           //  /.  (implemented inline)
#define OPS_CLZ       0x19                         //                           //+/.

//bits of goto(condition)
#define COND_QRDY 0x01 //input que is ready
#define COND_CC   0x02 //condition bit c1= carry
#define COND_NC   0x04 //condition bit c2=~carry
#define COND_ZZ   0x08 //condition bit c3= zero
#define COND_NZ   0x10 //condition bit c4=~zero
#define COND_FULL 0x06 //C + ~C = row full on OP_SCIN

#define REG_ACCUM   0                                                           //
#define MAX_REGS    8                                                           //$0 thru $7

#define IsArithOp(op, sub)  ((op).arith.act == OP_ARITH && (op).arith.subOp == (sub))   //
#define IsRegImmOp(op)      ((op).ri.act    == OP_RI)                           //
#define IsBugOp(op)         ((op).bug.act   == OP_BUG)                          //
#define IsCall(op)          (((op).call.act)== OP_CALL)                         //OP_CALL
#define IsGoOp(op)          (((op).go.act&3)== OP_GO_T)                         //OP_GO_T or OP_GO_F
#define IsShortJmp(op)      (IsGoOp(op) && (op).go.relAdr != 0)                 //short jump
#define IsLongJmp(op)       ((op).go.act    == OP_GO_T && (op).go.relAdr == 0)  //long jump
#define IsRdWrOp(op)        (op.s.act       == OP_READ || op.s.act == OP_WRYT)  //OP_READ or OP_WRYT
#define IsScanOp(op)        (op.s.act       == OP_SCAN || op.s.act == OP_SCIN)  //OP_SCAN or OP_SCIN
#define IsRdWrSc(op)        (((op).s.act & 7) == 4)                             //
#define IsExtendedOp(op)    (((op).arith.subOp & 0x10) != 0)                    //
#define IsWrField(op)       ((op).ind.act[4] == 1)                              //

typedef enum {ENV_NOT_SET=0, ENV_SW, ENV_XSIM, ENV_FPGA, ENV_HW, ENV_COMPILE} eENVIRONMENT;

//Opcode implemented by samControl.sv
typedef union
    {//OP_CALL;
     struct {uint16_t  act     : 5, //0x001F OP_CALL=5'h00
                       callAdr :11; //0xFFE0
            } call;
     //OP_WRF and OP_RDF ;
     struct {uint16_t  act     : 5,   //0x001F OP_RDF = 5'h08, OP_WRF = 5'h10
                       breg    : 3,   //0x00E0 the data register
                       fieldNum: 4,   //0x0F00 field Number: FLD_DATA, FLD_P1, etc
                       nu      : 1,   //0x0100
                       areg    : 3;   //0xE000 address register (handle of structure)
            } ind;
     //OP_RPT;
     struct {uint16_t  act     : 8, //0x00FF OP_RPT = 8'h90
                       count   : 5, //0x1F00 number of repititions
                       bkwd    : 1, //0x2000 decrement reg and/or address
                       stepA   : 1, //0x4000 step address on each iteration
                       stepR   : 1; //0x8000 step register on each iteration
            } rpt;    
     //OP_RPTR
     struct {uint16_t  act     : 5, //0x001F OP_RPTR = 5'h0E
                       breg    : 3, //0x00E0 (number of repititions-1) = $preg
                       z0      : 5, //0x1F00 number of repititions
                       bkwd    : 1, //0x2000 decrement reg and/or address
                       stepA   : 1, //0x4000 step address on each iteration
                       stepR   : 1; //0x8000 step register on each iteration
            } rptR;    
     //OP_BUG;
     struct {uint16_t  act     : 8, //0x00FF OP_BUG = 5'h18
                       level   : 5, //0x1F00 
                       sho     : 2, //0x6000   0 = unused
                                    //         1 = display curRow as hPAGE[]
                                    //         2 = display curRow as raw hex   
                                    //         3 = display curRow as hINDX[]
                       set     : 1; //0x8000 1 = set bugLevel
            } bug;
     //OP_ADD, OP_ADC, etc;
     struct {uint16_t  act     : 5, //0x001F OP_ARITH == 0x18
                       breg    : 3, //0x00E0
                       subOp   : 5, //0x1F00
                       areg    : 3; //0xE000
            } arith;
     //OP_GO_T, OP_GO_F;
     struct {uint16_t  act     : 3, //0x0007 OP_G_T == 0x01, OP_GO_F == 0x05
                       cond    : 5; //0x00F8
             int16_t   relAdr  : 8; //0xFF00
            } go;
     //OP_RI
     struct {uint16_t  act     : 5, //0x001F OP_RI == 0x06
                       breg    : 3, //0x00E0
                       imm     : 8; //0xFF00
            } ri;
     //OP_LDI;
     struct {uint16_t  act     : 2, //0x0003 OP_LDI = 0x03
                       imm     :14; //0xFFFC
            } ldi;
     //OP_SCAN and OP_SCIN;
     struct {uint16_t  act     : 5, //0x001F OP_READ/WRYT/SCAN/SCIN = 0x04, 0x0C, 0x14, 0x1C
                       breg    : 3, //0x00E0
                       rowType : 8; //0xFF00 
            } sc;
     //generic op
     struct {uint16_t  act     : 5, //0x001F 
                       breg    : 3, //0x00E0 
                       adr     : 8; //0xFF00
            } g;
     uint16_t          u16;
     uint8_t           shortOp;
    } OPCODE;

typedef struct 
    {uint64_t pc    :16,                    //0x00000000_0000FFFF
              sp    : 8,                    //0x00000000_00FF0000
              insPt : 8,                    //0x00000000_FF000000
              stat  : 5,                    //0x0000001F_00000000
              nu    :11,                    //0x0000FFE0_00000000
              op    :16;                    //0xFFFF0000_00000000
     } sSYSTEM_STAT;
typedef struct 
     {uint64_t nxtOp    :16,                //0x00000000_0000FFFF
               bugLevel : 5,                //0x00000000_001F0000
               y        : 3,                //0x00000000_00E00000
               curRow   : BRAM_ADR_BITS,    //0x00000000_1F000000 (5 bits)
               nu       :(40-BRAM_ADR_BITS);//0xFFFFFFFF_E0000000 (35 bits)
     } sDEBUG_STAT;
#pragma pack(pop)

/*enum     why this enum gets its knickers in a knot is beyond me               //
 {FLD_NU=0,                                                                     //
  FLD_DATA, FLD_STOP,                                                           //1-2 fields of hINDX
  FLD_P1,   FLD_P2,   FLD_COUNT, FLD_TOTAL, FLD_PAGE_STOP, //nu                 //3-7 fields of hPAGE/hBOOK
//key designations must start at 8; this bit is used in samControl to separate key fields from binary fields.
  FLD_KEY0=8,FLD_KEY1,FLD_KEY2,  FLD_KEY3, FLD_KEY4, FLD_KEY5,FLD_KEY6,FLD_KEY7 //12-15
 } FIELD_NUMBERS;                                                               // */
#define FLD_NU      0                                                           //
#define FLD_DATA    1                                                           //fields of hINDX
#define FLD_STOP    2                                                           //same locn in hINDX/hPAGE
#define FLD_P1      3                                                           //fields of hPAGE
#define FLD_P2      4                                                           //       "
#define FLD_COUNT   5                                                           //       "
#define FLD_TOTAL   6                                                           //       "
//FLD_PAGE_STOP     7                                                           //
//key fields must start at 8; [3] bit is used to separate key fields from bin fields.
#define FLD_KEY0    8                                                           //
#define FLD_KEY1    9                                                           //
#define FLD_KEY2   10                                                           //
#define FLD_KEY3   11                                                           //
#define FLD_KEY4   12                                                           //
#define FLD_KEY5   13                                                           //
#define FLD_KEY6   14                                                           //
#define FLD_KEY7   15                                                           //

#define FLD_RAW FLD_KEY0
#define CC const char*
#define CAT_THIS(buf) len = istrlen(buf); buf[sizeof(buf)-1] = 0; snprintf(&buf[len], sizeof(buf)-1-len

//convert ch to displayable format at outP; return # chars in output
inline int DisplayChar(char *outP, char ch)
   {outP[1] = outP[2] = 0;
    if ((*outP++=ch) >= 0x20 && ch < 0x80) return 1;
    *(outP-1) = '\\';
    if (ch == '\n') *outP++ = 'n'; else
    if (ch == '\r') *outP++ = 'r'; else
    if (ch == '\t') *outP++ = 't'; else
    if (ch == '\v') *outP++ = 'v'; else
       {*outP++ = 'x'; sprintf(outP, "%02X", ch); return 4;}
    return 2;
   } //DisplayChar...

class cOpName
   {private:                                                                    //
    cSamError *m_errP;                                                          //
    public:                                                                     //
    char      *m_messagesP;                                                     //
    int        m_messageSize, m_messageNum;                                     //
    cOpName (cSamError *errP, const char *messagesP, int messagesSize);         //
   ~cOpName ();                                                                 //
    char       *ConditionNames(uint16_t cc);                                    //
    int         FieldName     (CC textP, int fldNum, CC *namePP);               //
    int         StoreMessage  (const char *msgP, int len);                      //
    const char *FindMessage   (int msgNum);                                     //
    const char *GetTgtLabel   (uint32_t adr) {return "";}                       //
    char       *Show          (int pc, OPCODE op, OPCODE nxt, const char *labelP=NULL, bool knownB=false);
   }; //class OpName...

#endif //_OPCODE_H_INCLUDED...
//end of file...
