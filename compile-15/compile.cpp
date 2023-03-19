/*File: compile.cpp. Version 13, Jan 2023.
 Compile an ascii source file into object code for FPGA/hardware. 
 Input is in register transfer format, meaning that each opcode and supporting
 parameters are specified in the style of a standard assembler plus some
 simple language extensions such as if, for, and while. 
 Syntax is: 
    statement      == opcode plus supporting parameters terminated with ';'
    statement-List == {statement; statement;... }
    if (statement) statement-List [else statement-List]
    for (reg=expression; reg < expression; reg++) statement-List;
        (where reg is a register name #1 thru #7)
    while (expression) statement-List
    do statement-List while (expression)
 For example:
    bug 3;                                  //set bug level
    bug=3;                                  //set bug level
    row = 15;                               //set DRAM row of interest
    scan(indx) "7C299999";                  //scan DRAM row comprising hINDX[] for key <= "7C299999"
    if (full) {bug 3; read[0:10]; bug = 0;} //if said row is full execute {...}
 
 A powerful preprocessor is also implemented using the #define keyword.
 Said preprocessor implements C++ like #defines plus:
   - variable parameter, eg #define mac(a,b,...), then use mac(1,2,3,4)
   - multi-line block macros, 
   - named parameter, eg mac(.param1=1, .param2=5)
   - #for.... #endfor. This can be used in lieu of the for statement to
         avoid using a register. #for will replicate code multiple times
 See comments in PreProcessor.cpp, or documentation for a thorough explanation.

 The opcodes generated are 16-bits wide and are structured as follows:
┌───────────────────────────────────────────────────────── OPCODE TABLE ───────────────────────────────────────────────┬───┐
├───┬───┬───┬───┬───┬───┬─┬──OPCODE─┬───┬───┬───┬───┬───┬───┬───╥─────────┬────────────────────────────────────────────┼───┤
| 15| 14| 13| 12| 11| 10| 9 | 8 ║ 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 ║ OPNAME  |          MEANING                           |   |
├───┴───┴───┴───┴───┴───┴───┴───╫───┴───┴───┴───┴───┴───┴───┴───╫─────────┼────────────────────────────────────────────┼───┤
|                               ║<-------- shortOp ------------>║         |                                            |   |
├───┬───┬─ address bits ┬───┬───╫───┬───┬───╥───┬─action bits───╟─────────┼────────────────────────────────────────────┼───┤
|a10| a9| a8| a7| a6| a5| a4| a3| a2| a1| a0║ 0 | 0 | 0 | 0 | 0 ║ OP_CALL | push pc, goto {a10:a0}                     |(1)|
├───┴───┴───╫───┼───┼───┼───┬───╫─reg bits──╫───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┼───┤
|           ║ 0 |   |   |   |   ║           ║ 0 | 1 | 0 | 0 | 0 ║ OP_RDF  | field={f3:f0}; $breg <= [$areg].field      |   |
|  $areg    ║nu | f3| f2| f1| f0║  $breg    ╟───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┤(5)|
|           ║ 0 |   |   |   |   ║           ║ 1 | 0 | 0 | 0 | 0 ║ OP_WRF  | field={f3:f0}; [$areg].field <= $breg      |   |
├───┬───┬───╫───┼───┼───┼───┼───╫───┬───┬───╫───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┼───┤
| f2| f1| f0║ l4| l3| l2| l1| l0║ 0 | 0 | 0 ║ 1 | 1 | 0 | 0 | 0 ║ OP_BUG  | if(f2)bugLevel <= {l4:l0}, f2==0 adhoc cmds|(6)|
├───┼───┼───╟───┼───┼───┼───┼───╟───┼───┼───╟───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┼───┤
| 0 | 0 | 0 ║ 0 | 0 | 0 | 0 | 0 ║ 0 | 0 | 0 ║ 1 | 1 | 0 | 0 | 0 ║ OP_STOP | $stop simulation (OP_BUG with {f2:f0} == 0)|   |
├───┼───┼───╫───┼───┼───┼───┼───╫───┼───┼───╟───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┼───┤
| a7| a6| a5| a4| a3| a2| a1| a0║ c4| c3| c2| c1| c0║ 0 | 0 | 1 ║ OP_GO_T | if(condition & status)==condition goto adr |(3)|
|  relative destination address ║     condition     ║ 1 | 0 | 1 ║ OP_GO_F | if(condition & status)!=condition goto adr |   |
├───┬───┬───╥───┬───┬───┬───┬───╫───┴───┴───╥───┼───╫───┼───┼───╫─────────┼────────────────────────────────────────────┼───┤
|   $areg   ║     subOp         ║   $breg   ║ 0 | 0 | 0 | 1 | 0 ║ OP_ARITH| $breg <= $areg <subOp> $breg or unary ops  |(4)|
├───┬───┬───╫───┬───┬───┬───┬───╫───┬───┬───╫───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┼───┤
| i7| i6| i5| i4| i3| i2| i1| i0║   $breg   ║ 0 | 0 | 1 | 1 | 0 ║ OP_RI   | $breg <= {n'b0,i7:i0}                      |   |
├───┼───┼───╫───┼───┼───┼───┼───╫───┼───┼───╟───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┼───┤
| 0 | 0 | 0 ║ 0 | 0 | 0 | 0 | 0 ║ 0 | 0 | 0 ║ 0 | 1 | 0 | 1 | 0 ║ OP_RET  | return from call                           |   |
├───┼───┼───╫───┼───┼───┼───┼───╫───┼───┼───╟───┼───┼───┼───┼───╟─────────┼────────────────────────────────────────────┼───┤
| 0 | 0 | 0 ║ 0 | 0 | 0 | 0 | 0 ║ 1 | 0 | 0 ║ 0 | 1 | 0 | 1 | 0 ║ OP_CFG_G| CFG(group); group.konfig<=dramI            |   |
| 0 | 0 | 0 ║ 0 | 0 | 0 | 0 | 0 ║ 1 | 0 | 1 ║ 0 | 1 | 0 | 1 | 0 ║ OP_CFG_C| CFG(cell);  cell. konfig<=dramI            |   |
├───┼───┼───╫───┼───┼───┼───┼───╫───┼───┼───╟───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┼───┤
| R | A | - ║ 0 | 0 | 0 | 0 | 0 ║   $breg   ║ 0 | 1 | 1 | 1 | 0 ║OP_REPREG| repeat nxt op, breg = repititions          |   |
├───┼───┼───╫───┼───┼───┬───┬───╫───┼───┼───╟───┼───┼───┼───┼───╟─────────┼────────────────────────────────────────────┼───┤
| R | A | - ║   repeatCount-1   ║ 0 | 0 | 0 ║ 1 | 0 | 0 | 1 | 0 ║OP_REPEAT| repeat nxt op, R:reg ++/--, A:adr ++/--    |   |
├───┴───┴───╫───┴───┴───┴───┴───╫───┼───┼───╫───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┼───┤
| a7| a6| a5| a4| a3| a2| a1| a0║ 0 | 0 | 0 ║ 1 | 0 | 1 | 1 | 0 ║ OP_CROWI| currentRow <= {a7:a0}                      |   |
├───┴───┴───╫───┬───┬───┬───┬───╫───┼───┼───╫───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┼───┤
|   $areg   ║ 0 | 0 | 0 | 0 | 0 ║ 0 | 0 | 0 ║ 1 | 1 | 0 | 1 | 0 ║ OP_CROW | currentRow <= $areg                        |   |
├───┬───┬───╟───┼───┼───┼───┼───╟───┼───┼───╟───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┼───┤
| a7| a6| a5| a4| a3| a2| a1| a0║ v2| v1| v0║ 1 | 1 | 1 | 1 | 0 ║ OP_PRINT| $display($reg0)  a7:a0 = message # in host |(7)|
├───┼───┼───├───┼───┼───┼───┼───╫───┼───┼───╫───┼───┼───╫───┼───╫─────────┼────────────────────────────────────────────┼───┤
|i13|i12|i11|i10| i9| i8| i7| i6| i5| i4| i3| i2| i1| i0║ 1 | 1 ║ OP_LDI  | $reg0 <= ($reg0 << 14) + i13:i0            |   |
├───┼───┼───├───┼───┼───┼───┼───╫───┴───┴───╫───┬───┼───╨───┼───╫─────────┼────────────────────────────────────────────┼───┤
|   |   |   |   |   |   |   |   ║           ║ 0 | 0 | 1 | 0 | 0 ║ OP_READ | $breg <= curRow[a7:a0]                     |   |
| a7| a6| a5| a4| a3| a2| a1| a0║           ╟───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┼───┤
|   |   |   |   |   |   |   |   ║           ║ 0 | 1 | 1 | 0 | 0 ║ OP_WRYT | curRow[a7:a0 <= $breg                      |   |
├───┴───┴───┴───┴───┴───╢───┼───╢           ╟───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┼───┤
|     Reserved for      ║ row   ║   $breg   ║ 1 | 0 | 1 | 0 | 0 ║ OP_SCAN | Scan for $breg; rowType: 0=PAGE,1=INDX     |   |
|  multi-key support    ║  type ║           ╟───┼───┼───┼───┼───╫─────────┼────────────────────────────────────────────┤(2)|
|                       ║       ║           ║ 1 | 1 | 1 | 0 | 0 ║ OP_SCIN | Scan and insert $breg into rowType         |   |
└───────────────────────╨───┴───╨───────────╨───┴───┴───┴───┴───╨─────────┴────────────────────────────────────────────┴───┘
 Blank and x fields are not assigned, but should be set to zero; 
 NOTE (1): Address needs to be large enough to address each word in a DRAM row.
    For targetBus == 8 bytes (word size = 64-bits) and rowSize = 256 bytes each row contains 256/8 = 32 words;
    This requires an address field = [n=12:8].  2^(n+1-8) = 2^5 = 32
 NOTE (2): rowType: 0=PAGE, 1=INDX; other values reserved for multi-key support.
 NOTE (3): condition bits: c0=qrdy, c1=carry, c2=~carry, c3=zero, c4=~zero, c1&c2 row full(OP_SCAN)
 NOTE (4): subOp: 0x00=ADD, 1=ADC, 2=SUB, 3=SBB, 4=CMP, 5=XOR, 6=OR, 7=AND, 0x08=XSUB, 9=XSBB; operators in samArithmetic.sv
                  0x0A=INC, B=DEC, C=SHL, D=SHR, E=RCL, F=RCR;                                 operators        "
                  0x10=R2R, 11=XCHG, 12=OPS_XTOS, 13=POP, 14=PUSH, 15=PUSH_CURROW;             operators in samControls.sv 
                  0x16=STC, 17=CLC, 18=STZ, 19=CLZ, 1A thru 1F                                                  "
 NOTE (5): field number defines the size and location of a specific field in the 
           hINDX, hPAGE, or hBOOK structure addressed by regD.
 NOTE (6): flags: f2     == 1 sets buglevel to [i4:i0]
                              Refer to ERR_2727 in (c3_errors.cpp) for an explanation of bug levels.
 NOTE (7): variant {v2:v0} : 0 = print, 1=$expect, 2=$actual. 3-7= not used   
                 {f1:f0} == 0 stop
                            1 dump BRAM rows hPAGE[] format
                            2 dump BRAM rows in raw
                            3 dump BRAM rows hINDX[] format
┌───────── OPCODES by NUMBER ───────────────────────────────┐
├──────┬──────╥──────┬────────╥──────┬───────╥───────┬──────┤
| 0x00 | call ║ 0x08 | rdf    ║ 0x10 | wdf   ║  0x18 | bug  |
| 0x01 | goto ║ 0x09 | goto   ║ 0x11 | goto  ║  0x19 | goto |
| 0x02 | arith║ 0x0a | ret/cfg║ 0x12 | repeat║  0x1a | crow |
| 0x03 | ldi  ║ 0x0b | ldi    ║ 0x13 | ldi   ║  0x1b | ldi  |
├──────┼──────╫──────┼────────╫──────┼───────╢───────┼──────┤  
| 0x04 | read ║ 0x0c | wryt   ║ 0x14 | scan  ║  0x1c | scin |
| 0x05 | goto ║ 0x0d | goto   ║ 0x15 | goto  ║  0x1d | goto |
| 0x06 | ri   ║ 0x0e | repreg ║ 0x16 | crowi ║  0x1e | print|
| 0x07 | ldi  ║ 0x0f | ldi    ║ 0x17 | ldi   ║  0x1f | ldi  |
└──────┴──────╨──────┴────────╨──────┴───────╨───────┴──────┘
*/

#include "compile.h"

#undef  Error     //undo definition in compile-15.h
#undef  ErrorN    //undo definition in compile-15.h
#define Error(erC, param)           _Error(erC, GetContext(), param, __FILE__, __LINE__, __FUNCTION__)
#define ErrorS(erC, context, param) _Error(erC, context, param,      __FILE__, __LINE__, __FUNCTION__)
#define ErrorA(erC, atom)           _ErrorA(erC, atom,               __FILE__, __LINE__, __FUNCTION__)
#define ErrorN(erC, p1)             _ErrorN(erC, p1,                 __FILE__, __LINE__, __FUNCTION__)

cSamError g_err;
FILE *g_printFileP;

//push down list of break/continues at beginning of loop statement and
//prepare for a new batch of break's. Popup at or near the end of loop 
//statement, fixup jmps, & restore member variables from locals.
#define PUSH_BREAKS_AND_CONTINUES()                             \
    int *breaksP     = m_breaksP,    breaks      = m_breaks,    \
        *continuesP  = m_continuesP, continues   = m_continues; \
         m_breaksP   = NULL;         m_breaks    = 0;           \
         m_continuesP= NULL;         m_continues = 0; 
#define FixBreaks(pc)    FixThese(pc, true, breaksP,    breaks)                 //patch up current breaks;    pop previous breaks
#define FixContinues(pc) FixThese(pc, false,continuesP, continues)              //patch up current continues; pop previous continues
#define UNRESOLVED 0 // 0xD0D0                                                  //the DODO bird
#define REG0       0
#define REG1       1
#define STEPR      1
#define STEPA      1

//Opcode bits for OPCODE_TBL: A_ = allow this subfield, X_ = expect this subfield
enum
   {X_ENVIRONMENT=1, //
    X_DO,            //
    X_IF,            //
    X_FOR,           //
    X_FORZ,          //
    X_PRINT,         //
    X_ONE,           //OP_RET, _STOP, _BREAK
    X_WHILE,         //
    X_DW,            //
    X_XCHG,          //XCHG $a,$b                                           eg xchg $0, $1 
    X_REG_LIST,      //expecting list of registers                          eg push $0, $1, $2
    X_GOTO,          //expecting label                                      eg goto xxx
    X_PSEUDO,        //pseudo OP                                            eg $shoCode
    X_BUG,           //assign value to $bug,                                eg $bug = 3
    X_ARITH,         //requires register,                                   eg ADD #3
    X_UNOP,          //requires register,                                   eg ADD #3
    X_BRAKE,         //break or continue within a loop                      eg break or continue
    X_CFG,           //                                                     eg cfg(cell)
    X_SCAN,          //expect (rowType) or (cell/group) on opcode,          eg scan(indx), cfg(cell)
    X_SHIFT,         //SHL and SHR, allow <#bits>                           eg SHL; or SHL 3;
    X_REG,           //expecting = target                                   eg $reg = <expession>; or $reg++
    X_XTOS,          //XTOS $reg                                            eg xtos $1
    X_STORE,         //expect [row:word] possibly with .fileName = $reg/lit see AssignMem for details
    A_PAIR,          //allow adr pair [wordStart:wordEnd] addresses,        eg write[15:17] = 99;
    X_ALLOC,         //
    X_VBL_ASSIGN,    //
    X_COMPILE_STOP,  //
   };
    
static OPCODE_TBL  
  OpcodeTbl[] =                                                                 //
      {//name           act                    ctrl         },                  //
       {"$0",     OP_ARITH+(OPS_R2R<<8),       X_REG        },                  //$0 = reg or lit
       {"$1",     OP_ARITH+(OPS_R2R<<8)+0x100, X_REG        },                  //$1 = reg or lit (reg1 is pc)
       {"$2",     OP_ARITH+(OPS_R2R<<8)+0x200, X_REG        },                  //$2 = reg or lit
       {"$3",     OP_ARITH+(OPS_R2R<<8)+0x300, X_REG        },                  //$3 = reg or lit
       {"$4",     OP_ARITH+(OPS_R2R<<8)+0x400, X_REG        },                  //$4 = reg or lit
       {"$5",     OP_ARITH+(OPS_R2R<<8)+0x500, X_REG        },                  //$5 = reg or lit
       {"$6",     OP_ARITH+(OPS_R2R<<8)+0x600, X_REG        },                  //$6 = reg or lit
       {"$7",     OP_ARITH+(OPS_R2R<<8)+0x700, X_REG        },                  //$7 = reg or lit
       {"$CURROW",      OP_CROW,               X_REG        },                  //$curRow = reg or literal
       {"$SHOCODE",     0,                     X_PSEUDO     },                  //debugging
       {"$SAFEREG0",    1,                     X_PSEUDO     },                  //
       {"EXEC",         1,                     X_PSEUDO     },                  //
       {"$BUG",         OP_BUG+0x8000,         X_BUG        },                  //set bug level (f2==1)
       {"$StopCompile", 0,                     X_COMPILE_STOP},
       {"$BUG_PAGE",    OP_BUG+0x2000,         X_ONE        },                  //f2 == 0, f1:f0 = 1
       {"$BUG_RAW",     OP_BUG+0x4000,         X_ONE        },                  //f2 == 0, f1:f0 = 2
       {"$BUG_INDX",    OP_BUG+0x6000,         X_ONE        },                  //f2 == 0, f1:f0 = 3
       {"ALLOCATE",     0,                     X_ALLOC      },                  //
       {"BREAK",        1,                     X_BRAKE      },                  //break from for, while, or do loop
       {"CONTINUE",     0,                     X_BRAKE      },                  //continue   for, while, or do loop
       {"NOOP",         OP_NOOP,               X_ONE        },                  //
       {"CFG",          OP_CFG_G,              X_CFG        },                  //(cell) or (group)
       {"SCIN",         OP_SCIN,               X_SCAN       },                  //(indx), (page), or (book)
       {"SCAN",         OP_SCAN,               X_SCAN       },                  //         "
       {"STOP",         OP_STOP,               X_ONE        },                  //
       {"GOTO",         OP_GO_T,               X_GOTO       },                  //
       {"GO",           OP_GO_T,               X_GOTO       },                  //
       {"CALL",         OP_CALL,               X_GOTO       },                  //
       {"[",            OP_WRYT,               X_STORE      },                  //
       {"$ENVIRONMENT", 0,                     X_ENVIRONMENT},                  //
       {"DO",           0,                     X_DO,        },                  //
       {"IF",           0,                     X_IF,        },                  //
       {"FOR",          0,                     X_FOR,       },                  //0 = standard for
       {"FORZ",         1,                     X_FORZ,      },                  //1 = non-std  for
       {"PRINT",        0,                     X_PRINT,     },                  //
       {"$EXPECT",      1,                     X_PRINT      },                  //
       {"$ACTUAL",      2,                     X_PRINT      },                  //
       {"$STRING",      3,                     X_PRINT,     },                  //
       {"RET",          OP_RET,                X_ONE,       },                  //
       {"WHILE",        0,                     X_WHILE,     },                  //
       {"DW",           0,                     X_DW,        },                  //      "
       {"ADD",          OPS_ADD,               X_ARITH      },                  //binary opcodes
       {"ADC",          OPS_ADC,               X_ARITH      },                  //      "
       {"SUB",          OPS_SUB,               X_ARITH      },                  //      "
       {"SBB",          OPS_SBB,               X_ARITH      },                  //      "
       {"CMP",          OPS_CMP,               X_ARITH      },                  //      "
       {"XOR",          OPS_XOR,               X_ARITH      },                  //      "
       {"OR",           OPS_OR,                X_ARITH      },                  //      "
       {"AND",          OPS_AND,               X_ARITH      },                  //      "
       {"INC",          OPS_INC,               X_UNOP       },                  //unary opcodes
       {"DEC",          OPS_DEC,               X_UNOP       },                  //      "
       {"SHL",          OPS_SHL,               X_SHIFT      },                  //      "
       {"SHR",          OPS_SHR,               X_SHIFT      },                  //      "
       {"RCL",          OPS_RCL,               X_SHIFT      },                  //      "
       {"RCR",          OPS_RCR,               X_SHIFT      },                  //      "
       {"STC",  OP_ARITH + (OPS_STC<<8),       X_ONE        },
       {"CLC",  OP_ARITH + (OPS_CLC<<8),       X_ONE        },
       {"STZ",  OP_ARITH + (OPS_STZ<<8),       X_ONE        },
       {"CLZ",  OP_ARITH + (OPS_CLZ<<8),       X_ONE        },
       {"PUSH",         OPS_PUSH,              X_REG_LIST   },                  //inline opcodes
       {"POP",          OPS_POP,               X_REG_LIST   },                  //      "
       {"XCHG",         OP_ARITH,              X_XCHG       },                  //      "
       {"XTOS",         OP_ARITH,              X_XTOS       },                  //      "
       {NULL,           0,                     0,           }                   //stopper
      }; //OpcodeTbl...              

inline bool IsOp(const char *nameP, IATOM aa) 
   {return istrlen(nameP) == aa.len && strnicmp(aa.textP, nameP, aa.len) == 0;}

inline char *rTrim(char *bufP)
   {int len; for (len=istrlen(bufP); len > 0 && bufP[--len] == ' ';) bufP[len] = 0; return bufP;}

char *cCompile::strdupl(const char *srcP, int len) //== -1
   {char *destP;                                                                //
    if (len < 0) len = (int)strlen(srcP);                                       //
    destP = (char*)calloc(len+1, 1);                                            //
    if (destP == NULL) {Error(ERR_0005, srcP); exit(1);}                        //memory allocation failed
    memmove(destP, srcP, len);                                                  //
    destP[len] = 0;                                                             //
    return destP;                                                               //
   } //cCompile::strdupl..

//cCompile class contructor
cCompile::cCompile(CC srcNameP, CC includeDirP, bool bugEmitB, bool patchDrvB)  //input files/dir
   {m_errCount     = 0;                                                         //
    m_alwaysMessageBoxB = true;                                                 //for errors
    m_bugEmitB     = bugEmitB;                                                  //
    m_patchDrvB    = patchDrvB;                                                 //
    m_environment  = ENV_NOT_SET;                                               //
    BuildFileNames((char*)srcNameP);                                            //
    m_objFileP     = m_microcode;                                               //
    m_msgFileP     = m_msgFile;                                                 //
    m_lineMapP     = m_lineMap;                                                 //
    m_symbolTblP   = m_symbolTbl;                                               //
    //::cAtomize opens the source file and include file(s), expands the macros  //
    //and prepares a line map of all source lines so that the input can be read //
    //atom by atom using m_azP->GetAtom().                                      //
    m_azP          = new cAtomize(srcNameP, includeDirP, m_capFile);            //
    m_breaks       = m_continues  = m_loopDepth = m_longestLabel = 0;           //count of breaks/continues
    m_breaksP      = m_continuesP = NULL;                                       //list of break/continues in for, while, or do loops
    m_vblsP        = NULL;                                                      //variables introduces with allocate statement
    m_vblCount     = 0;                                                         //count of same
    m_max.u16      = -1;                                                        //max of opcode fields
    m_pgmSize      = m_pgmAvail = 0;                                            //
    m_codeP        = NULL;                                                      //
    m_opNameP      = new cOpName(&g_err, NULL, 0);                              //starting new messages block
   } //cCompile::cCompile...

//Class destructor
cCompile::~cCompile()
   {for (int pc=0; pc < m_pgmSize; pc++) 
        {free((void*)m_codeP[pc].tgtLabelP); free((void*)m_codeP[pc].hereP);}   //
    free(m_codeP);     m_codeP     = NULL;                                      //
    free(m_breaksP);   m_breaksP   = NULL;                                      //
    free(m_continuesP);m_continuesP= NULL;                                      //
    while (--m_vblCount >= 0) free(m_vblsP[m_vblCount].nameP);                  //
    free(m_vblsP);     m_vblsP     = NULL;                                      //
    delete m_azP;      m_azP       = NULL;                                      //
    delete m_opNameP;  m_opNameP   = NULL;                                      //
   } //cCompile::~cCompile...

//Perform first pass thru code and generate opcodes. The zeroeth pass has already
//been performed (ie., file read, line demarcation, and macro expansion).
//Place labels, and generate output 'hex file', message file and linemap file
int cCompile::CompileProgram(void)
   {int    erC;                                                                 //
    if ((erC=m_azP->m_lines) < 0)                                               //
       {//routines below the level of cAtomize do not publish errors - they     //
        //only call LogError(). Must call Error(0) to publicize.                //
        Error(0, ""); return erC;                                               //not 'return Error(0)'
       }                                                                        //  (if user says no

    if ((m_pgmSize=StatementList  (0, ';'))        < 0) return m_pgmSize;       //pass1 of compiler
    if ((erC=ResolveGotos         ())              < 0) return erC;             //insert proper addresses for jumps
    if ((erC=PatchSamDefines      (m_objFileP))    < 0) return erC;             //patch samDefines.sv (hrgt convenience)
    if ((erC=GenerateMicrocodeFile(m_objFileP))    < 0) return erC;             //  <programName>.microcode
    if ((erC=GenerateMsgFile      (m_msgFileP))    < 0) return erC;             //  <programName>.msgFile
    if ((erC=GenerateLinemapFile  (m_lineMapP))    < 0) return erC;             //  <programName>.lineMap
    if ((erC=GenerateSymbolTable  (m_symbolTblP))  < 0) return erC;             //  <programName>.symbolTbl
    return m_environment;                                                       //
   } //cCompile::CompileProgram...

//Compile one or more statements until we encounter one terminated with stopper
//stopper  = ';' typically, but '}', ',' and ')' are also possible.
//oneShotB = true to compile a single statement as called from ForStmt or IfStmt.
int cCompile::StatementList(int pc, char stopper, bool oneShotB)
   {int           erC=0, ii, startPc, reg, pcPlz=130, linePlz=-7;               //
    int64_t       result=0;                                                     //
    uint32_t      u32, rhsVal=99;                                               //
    IATOM         aa;                                                           //
    char         *pp, *labelP=NULL;                                             //
    bool          bugEmitB, bb;                                                 //
    OPCODE_TBL   *otP=NULL, vbl;                                                //
    sCODE_BLOB   *bP, blob;                                                     //
                                                                                //
    vbl.act = 0; vbl.ctrl = X_VBL_ASSIGN; vbl.nameP = NULL;                     //
    while ((aa=Get()).type != GC_NULL)                                          //repeat to end of file
       {m_pgmSize = startPc = pc; reg = -1; m_stepableB = m_rowOvrB = false;    //
        m_ref     = m_a.ref;                                                    //prevails thru this opcode generation
        #ifdef _DEBUG                                                           //
//          _CrtCheckMemory();                                                  //
        #endif                                                                  //
        if (IsChar(';'))     continue;                                          //ignore repeated ';'
        if (IsChar(stopper)) return pc;                                         //StatementList eats up stopper
        CheckCodeSize(pc);                                                      //
        if (aa.type != GC_NAME && !IsChar('['))    return ErrorA(ERR_3000, aa); //3000 = must be label or opcode
        bP = &m_codeP[pc];                                                      //
        if (Is(':') && stopper > 0 && !IsRegister(aa))                          //opcode is labelled, ie <name>:
            {pp = strdupl(aa.textP, aa.len);                                    //
             if (WhereisLabel(pp) >= 0)            return ErrorA(ERR_3001, aa); //3001 = duplicate label
             bP->hereP = pp;                                                    //
             m_longestLabel = Max(m_longestLabel, aa.len);                      //for listing
             if ((aa=Get()).type != GC_NAME && !IsChar('['))                    //
                                                   return ErrorA(ERR_3000, aa); //3000 = missing name
             m_lastCurRow = -1;                                                 //
            }                                                                   //
        else Backup(m_a);                                                       //no label
        //lookup opcode                                                         //
        for (otP=OpcodeTbl; otP->nameP; otP++) if (IsOp(otP->nameP, aa)) break; //
        if  (otP->nameP == NULL)                                                //
            {if ((ii=LookupVbl(aa.textP, aa.len)) < 0)                          //
                return ErrorA(ERR_2522, aa);                                    //2522 = Unknown opcode in asm stmt
             otP = &vbl;                                                        //
            }                                                                   //
        bP->ref      = m_ref   = aa.ref;                                        //
        bP->ref.pc   = pc;                                                      //
        bP->op.u16   = otP->act;                                                //initial setting, tweaked as we move along
        bP->isBreakB = bP->isCodeAdrB  = bP->isDwB = bP->isPcB =                //
                       bP->isContinueB = bP->isLongJmpB = false;                //
        blob         = *bP;                                                     //
        m_loAdr      = m_hiAdr         = 0;                                     //for READ[lo:hi] or wryt[lo:hi]
        bugEmitB     = m_bugEmitB;                                              //saved in case routine clobber
        if (m_safeRegB=!Is('!')) {Backup(m_a); m_safeRegB = m_safeRegDefaultB;} //opcode! means overwrite $reg if needed
        if (pc == pcPlz || m_ref.lineNum == linePlz)                            //stop on this pc/line
            bugEmitB = true;                                                    //<<< debugging point <<<
        switch (otP->ctrl)                                                      //
          {case X_COMPILE_STOP: if (!Is(';')) Backup(m_a); continue;            //<--- debugging break point <<---
           case 0:                                  return ErrorA(ERR_2522, aa);//
           case X_ONE: pc++; break;                                             //opcode from table is complete
           case X_ENVIRONMENT:                                                  //
            if ((erC=SetEnvironment())                < 0) return erC;continue; //
           case X_ARITH: case X_UNOP:                                           //arithmetics except inline ops (pop, push, xchg) or shifts
            if ((pc=Arithmetic(pc, otP))              < 0) return pc; break;    //
           case X_SHIFT:                                                        //shift opcodes
            if ((pc=ShiftStmt(pc, (uint8_t)otP->act)) < 0) return pc; break;    //
           case X_VBL_ASSIGN:                                                   //
            if ((pc=AssignMem(pc, ii))                < 0) return pc; break;    //
           case X_STORE:                                                        //[adr] = reg
            if ((pc=AssignMem(pc, -1))                < 0) return pc; break;    //
           case X_REG:                                                          //$reg = something 
            if ((pc=RegAssignment(pc, aa, otP->act))  < 0) return pc; break;    //
           case X_CFG:                                                          //expecting (cell) or (group)
            if ((pc=Qualified(pc, true))              < 0) return pc; break;    //
           case X_DW:   if((pc=DwStmt(pc))            < 0) return pc; break;    //
           case X_DO:   if((pc=DoStmt(pc))            < 0) return pc; continue; //
           case X_IF:   if((pc=IfStmt(pc))            < 0) return pc; continue; //
           case X_FORZ:                                                         //
           case X_FOR:  if((pc=ForStmt(pc, aa))       < 0) return pc; continue; //
           case X_WHILE:if((pc=WhileStmt(pc))         < 0) return pc; continue; //
           case X_PRINT:if((pc=PrintStmt(pc,otP->act))< 0) return pc; break;    //
           case X_ALLOC:if((erC=AllocateBram())       < 0) return erC;break;    //
           case X_BRAKE:if ((pc=BreakStmt(pc, otP->act, &labelP)) < 0)          //
                                                           return pc; break;    //
           case X_SCAN:                                                         //expecting (rowType)
            //scin and scan allow target operand, eg. scan(indx) "abcdefgh"     //
            //plain 'scan(indx)' will use $reg0 as target                       // 
            if ((pc=Qualified(pc, false)) < 0)             return pc;           //
            bb = Is(';'); Backup(m_a);                                          //
            if (!bb)                                                            //
               {blob     = *bP;                                                 //save opcode generated thus far
                pc       = LongStringOrLiteral(pc-1, m_a);                      //    
                *(bP     = &m_codeP[pc]) = blob;                                //restore opcode
               }                                                                //
            pc++; break;                                                        //
           case X_BUG:                                                          //
            if (Is('='))                                                        //
               {if ((erC=ConstantExpression(&result)) < 0) return erC;          //$bug = constant
                bP->op.bug.level = result;                                      //  sets debug level
                pc++;                                                           //
               }                                                                //
            else                                                                //
               {if (!IsWord("break")) Backup(m_a);                              //$bug or $bug break;
                pc = GenerateOp(pc, OP_BUG, 0, 1, 3);                           //   is just a software break point
               }                                                                //
            break;                                                              //       
           case X_PSEUDO:                                                       //
            switch (otP->act)                                                   //
               {case 0: case 1:                                                 //
                  bb = true;                                                    //$showCode;
                  if (Is('='))                                                  //$shoCode <number>;
                     {if (!IsNumber(&u32))    return ErrorA(ERR_1002, m_a);     //1002 = invalid number
                      bb = (u32 != 0);                                          //
                     }                                                          //
                  else Backup(m_a);                                             //
                  if (otP->act ==0)  m_bugEmitB = bugEmitB = bb;                //$shoCode or $shoCode = t/f
                  else               m_safeRegDefaultB     = bb;                //
                  break;                                                        //
               } //switch (op.u16)...                                           //
            continue;                                                           //
           case X_GOTO:                                                         //
            pc++;                                                               //
            if (Is('('))                                                        //
                {if (!Is("long") || !Is(')')) return ErrorA(ERR_3000, m_a);     //3000 = missing name
                 pc = GenerateOp(pc, UNRESOLVED);                               //
                 bP->isLongJmpB = true;                                         //modify properties of jmp instruction
                 (bP+1)->isCodeAdrB = (bP+1)->isDwB = true; (bP+1)->isLongJmpB; //modify properties of adr word
                 Get();                                                         //now the target label
                }                                                               //
            if (m_a.type != GC_NAME)         return ErrorA(ERR_3000, m_a);      //goto: analyze label
            m_longestLabel = Max(m_longestLabel, m_a.len);                      //for listing
            bP->tgtLabelP  = labelP = strdupl(m_a.textP, m_a.len);              //save label name for BugEmit()
            break;                                                              //
                                                                                //
           case X_REG_LIST:                                                     //expecting a register list
             if (Is("$currow") && IsOp("push", aa))                             //
                {pc = GenerateOp(pc, OP_ARITH, OPS_PUSH_CURROW); break;}        //
             else Backup(m_a);                                                  // 
             blob.op.u16 = OP_ARITH; blob.op.arith.subOp = otP->act;            //variant of arithmetic opcode ??
             do {if (!IsRegister(Get(), &reg)) return ErrorA(ERR_2738, m_a);    //2727 = Register not found following arithmetic opcode
                    {*bP = blob; bP++->op.arith.breg = reg; pc++;               //
                     blob.hereP = NULL;                                         //label lost after 1st iteration
                }   }                                                           //
             while (Is(','));                                                   //
             Backup(m_a);                                                       //was not a comma after all
             if (blob.op.arith.act   == OP_ARITH &&                             //
                (blob.op.arith.subOp == OPS_PUSH || blob.op.arith.subOp == OPS_POP))
                pc = CollapsePops(startPc, pc);                                 //
             break;                                                             //
           case X_XCHG:                                                         //expecting two registers
             if (!IsRegister(Get(), &reg)) return ErrorA(ERR_2738, m_a);        //2738 = Register not found following arithmetic opcode
             bP->op.arith.subOp = OPS_XCHG; bP->op.arith.breg = reg;            //
             if (!Is(','))                 return ErrorA(ERR_3017, m_a);        //3017 = missing comma
             if (!IsRegister(Get(), &reg)) return ErrorA(ERR_2738, m_a);        //2738 = Register not found following arithmetic opcode
             bP->op.arith.areg  = reg;                                          //
             pc++; break;                                                       //
           case X_XTOS:                                                         //expecting one register
             if (!IsRegister(Get(), &reg)) return ErrorA(ERR_2738, m_a);        //2738 = Register not found following arithmetic opcode
             bP->op.arith.subOp = OPS_XTOS; bP->op.arith.areg = reg;            //
             pc++; break;                                                       //
        } //switch (otP->ctrl)...                                               //
        m_codeP[startPc].ref.srcOffset = m_ref.srcOffset;                       //always point to initial src address
        m_bugEmitB = bugEmitB;                                                  //restore after possible screwup by one of the routines
        BugEmit(startPc, pc-1, NULL, labelP);                                   //
        //check for closing semicolon                                           //
        if (Is(','))         continue;                                          //eg, for ($0=1, $1=2;...
        if (IsChar(';'))    {if (oneShotB) break; continue;}                    //
        if (IsChar(stopper)) break;                                             //
        return ErrorA(ERR_3003, m_a);                                           //
       } //while (aa=...                                                        //
    return pc;                                                                  //
   } //cCompile::StatementList...

//statement; or {statementlist}
int cCompile::CompoundStatement(int pc)
   {if (IsChar('{')) return StatementList(pc, '}', false);
    Backup(m_a);     return StatementList(pc, ';', true);
   } //cCompile::CompoundStatement...

//arithmetics except inline ops (pop, push, xchg) or shifts
int cCompile::Arithmetic(int pc, OPCODE_TBL *otP)
   {int reg; sCODE_BLOB blob, *bP=&m_codeP[pc];                         //
    //expecting 'dec/inc $reg', '$reg, $reg' or '$reg, literal'         //
    if (!IsRegister(Get(), &reg))          return ErrorA(ERR_2738, m_a);//2738= Register not found following arithmetic opcode
    bP->op.u16         = OP_ARITH;                                      //
    bP->op.arith.subOp = otP->act;                                      //add, adc, sub, etc
    bP->op.arith.areg  = reg;                                           //destination register
    if (otP->ctrl == X_UNOP) {bP->op.arith.breg = reg; return pc+1;}    //unary operator: sreg == dreg
    if (!Is(','))                          return ErrorA(ERR_3017, m_a);//3007 = Missing comma 
    if (IsRegister(Get(), &reg)) {bP->op.arith.breg = reg; return pc+1;}//<arith> $rega, $regb
    if (reg == 0)                          return ErrorA(ERR_7359, m_a);//7359 = reg$0 invalid in this context
    Backup(m_a); blob = *bP;                                            //
    pc = SafeReg(pc, OPS_PUSH, REG0);                                   //
    if ((pc=BuildLiteral(pc, false, REG0)) < 0) return pc;              //$0 used to build literal
    m_codeP[pc++] = blob;                                               //restore opcode with $0 as $sreg
    pc = SafeReg(pc, OPS_POP, REG0);                                    //
    return pc;
   } //cCompile::Arithmetic...

/*Analyse literal expression between '[' and ']'. Variants include:
 On entry m_codeP[pc] has a valid opcode; the .adr field is completed, 
 [adr]                          m_loAdr = m_hiAdr = adr
 [adr1:adr2]                    m_loAdr = adr1, m_hiAdr = adr2
 m_lodAdr and m_hiAdr are used by MultiOp to create OP_REPEAT(opcode).
 [row@adr] or [@row:adr1:adr2]  generate $curRow = row, then handle [adr1:adr2]
On entry cursor is already stepped over '[' and the next atom is not a $reg.    */
int cCompile::Address(int pc)                                                   //
   {int64_t    result, rowOvr;                                                  //
    int        erC, sReg=0;                                                     //
    bool       rowOvrB=false;                                                   //
    sCODE_BLOB blob = m_codeP[pc];                                              //
                                                                                //
    Backup(m_a);                                                                //
    if ((erC=ConstantExpression(&result)) < 0)                    return erC;   //
    if (!Is('@')) Backup(m_a);                                                  //no row overrride
    else {rowOvr = result; rowOvrB = true;                                      //
          if ((erC=ConstantExpression(&result)) < 0)              return erC;   // number@number is row override
         } //Is('@')...                                                         //
    m_hiAdr = m_loAdr = (int)result;                                            //
    if (!Is(':')) Backup(m_a);                                                  //
    else{//Adr:adr or possibly row@adr:adr or even @row@adr:@row:adr            //
         //In the later case row override must be the same for both hi and loAdr//
         if ((erC=ConstantExpression(&result)) < 0)               return erC;   //
         if (!Is('@')) Backup(m_a);                                             //
         else{if (!rowOvrB || result != rowOvr)                   goto ovrErr;  //
              if ((erC=ConstantExpression(&result)) < 0)          return erC;   //
             } //I('@')...                                                      //
         m_hiAdr = (int)result;                                                 //
        } //Is(':')...                                                          //
    if (rowOvrB && m_lastCurRow != rowOvr)                                      //
       {if (rowOvr <= m_max.g.adr)                                              //
             pc = GenerateOp(pc, OP_CROWI, (int)rowOvr);                        //row# fits in adr field
        else pc = GenerateOp(IntegerLiteral(pc, rowOvr, sReg), OP_CROW, sReg);  //$reg = literal (m_rowOvr)
        m_lastCurRow = (int) rowOvr;                                            //
       }                                                                        //
    blob.op.g.adr = m_loAdr;                                                    //
    m_codeP[pc]   = blob;                                                       //
    return pc;                                                                  //
ovrErr: return Error(ERR_2735, "");                                             //2735 = Invalid row override.
   } //cCompile::Address...

//Muliple opcodes using OP_REPEAT. Proximal source is [loAdr:hiAdr] and the
//generated opcode (baseOp) is already stored at code[pc]. 
int cCompile::MultiOp(int pc, OPCODE baseOp)
   {int  reg=baseOp.g.breg, repeat; sCODE_BLOB *bP=&m_codeP[pc];                //
    if (m_loAdr == m_hiAdr) return pc+1;                                        //
    bP->op.u16         = OP_REPEAT;                                             //first the repeat meta-op
    bP->op.rpt.count   = m_hiAdr < m_loAdr ? (repeat = m_loAdr - m_hiAdr)       //
                                           : (repeat = m_hiAdr - m_loAdr);      //
    if (repeat >= 32) return Error(ERR_2734, "");                               //2734 = repeat is too big
    bP  ->op.rpt.stepA = 1;                                                     //
    bP  ->op.rpt.stepR = m_stepableB ? 0 : 1;                                   //
    bP  ->op.rpt.bkwd  = m_loAdr > m_hiAdr;                                     //step fwd/bkwd, eg read [0:10] versus read[10:0]
    bP++->ref          = m_ref;                                                 //
 // bP  ->lineNum      = m_lineNum;                                             //
 // bP  ->fileNum      = m_fileNum;                                             //
 // bP++->srcOffset    = m_srcOffset;     pc++;                                 //
    bP  ->op           = baseOp;                                                //duplicate opcode, but...
    bP  ->op.g.adr     = m_loAdr;                                               //starting word address
    bP  ->op.g.breg    = reg;                                                   //starting reg address
    bP->ref            = m_ref;                                                 //
//  bP  ->fileNum      = m_fileNumber;                                          //
//  bP  ->lineNum      = m_lineNumber;                                          //
//  bP  ->srcOffset    = m_srcOffset;                                           //
    bP  ->hereP        = NULL;                                                  //label valid on first op, NULL thereafter
    return pc+1;                                                                //
   } //cCompile::MultiOp...

//DW value; Allows number in Verilog and a variety of others formats.
//eg  DW 33'h12345, 12345, 0b101011010, 0x12345, 123, etc.
//Using the Verilog format yeilds precise control over the number of bits generated.
int cCompile::DwStmt(int pc)
   {int        bits;                                                            //
    sCODE_BLOB blob=m_codeP[pc];                                                //
    uint64_t   dw;                                                              //
    IATOM      aa;                                                              //
    char      *pp, *qq;                                                         //
    do {aa = Get(); pp = strdupl(aa.textP, aa.len);                             //do NOT collapse to LOCAL_COPY(Get())
        if (aa.type != GC_INT)                          goto err;               //   (loop repeatedly re-allocates pp)
        dw = Anumber(pp, &qq, &bits);                                           //
        if ((bits != 64 && (1ull << bits) < dw) || *qq) goto err;               //declared size is too small for value
        do {blob.op.u16   = (uint16_t) dw;                                      //
            blob.isDwB    = true;                                               //prvents ResolveGotos getting too excited
            m_codeP[pc++] = blob;                                               //
            blob.tgtLabelP= blob.hereP = NULL;                                  //only first opcode is labelled
            dw          >>= 16;                                                 //
           } while ((bits-=16) > 0);                                            //
         free(pp);                                                              //
        } while (Is(','));                                                      //
    Backup(m_a);                                                                //
    return pc;                                                                  //
err:free(pp); return ErrorA(ERR_1002, m_a);                                     //1002 = invalid number
   } //cCompile::DwStmt...

//Syntax is [SHL | SHR | RCL | RCR] $reg [,number | $register]
//if ',number' is specified generate OP_REPEAT <number>; OP_<shift> 
//otherwise                 generate OP_REPREG $breg;    OP_<shift>
int cCompile::ShiftStmt(int pc, uint8_t shiftOp)
    {int         reg, breg;                                                     //
     sCODE_BLOB *bP, blob=m_codeP[pc];                                          //set fields of blob like hereP, etc
     uint32_t    u32;                                                           //
     if (!IsRegister(Get(), &reg))          return ErrorA(ERR_2738, m_a);       //2738= Register not found following arithmetic opcode
     if (!Is(',')) Backup(m_a);                                                 //
     //shift $reg, $reg                                                         //
     if (IsRegister(Get(), &breg))                                              //
        {pc   = GenerateOp(pc, OP_REPREG,  breg);                               //
         return GenerateOp(pc, OP_ARITH, shiftOp, reg);                         //
        }                                                                       //
     Backup(m_a);                                                               //
     //shift $reg, literal                                                      //
     if (!IsNumber(&u32)) {Backup(m_a); u32 = 1;}                               //SHR $reg; implies shr $reg,1
     if (u32 == 0) return pc;                                                   //'shl 0' - fine with me
     if (u32 >= CELL_SZ * TARGETBUS_SIZE)   return ErrorA(ERR_2728, m_a);       //2728 = Shift amount was not found or is invalid
     blob.op.u16         = OP_ARITH;                                            //
     blob.op.arith.subOp = shiftOp;                                             //
     blob.op.arith.breg  = blob.op.arith.areg = reg;                            //
     bP                  = &m_codeP[pc];                                        //
     if (u32 == 1) *bP = blob; else                                             //repeat == 1, no OP_REPEAT
        {if (u32 != 2) {bP->op.u16 = OP_REPEAT; bP->op.rpt.count = u32-1;}         //SHL > 2, use OP_REPEAT
         *(++bP) = blob; bP->hereP = NULL; pc++;                                //SHL ==2, just repeat opcode
        }                                                                       //
     return pc+1;                                                               //
    } //cCompile::Shifter...

int cCompile::_ErrorA(int erC, IATOM aa, const char *fileP, int line, const char *fncP)
   {char context[256], *pp=LOCAL_COPY(aa);                                      //
    SNPRINTF(context), "line %03d:", m_ref.lineNum);                                //
    CopySource(&context[strlen(context)], sizeof(context)-istrlen(context));    //
    return _Error(erC, context, pp, fileP, line, fncP);                         //
   } //cCompile::ErrorA...

/*┌──condition─bits───┬────╥action bits╥──────────────────┬────────────────────────┐
* | c4 | c3 | c2 | c1 | c0 ║ 0 | 0 | 1 ║ OP_GO_T (3'b001) | jmp on true  condition |
* | c4 | c3 | c2 | c1 | c0 ║ 1 | 0 | 1 ║ OP_GO_F (3'b101) | jmp on false condition |
* └────┴op.go.cond────┴────╨─op.go.act─╨──────────────────┴────────────────────────┘
* the condition bits (c[i]) are evaluated against a status register as follows:
* if '([c4:c0] & status) == [c4:c0]' and actionBits == OP_GO_T take the jump 
* if '([c4:c0] & status) != [c4:c0]' and actionBits == OP_GO_F take the jump 
* or more succinctly ([c4:c0] & status) != [c4:c0]) == OP_GO.act[2]
* [c3:c0] == 5'b0 always succeeds and OP_GO_T is therefore an unconditional goto
* [c3:c0] == 5'b0 always succeeds and OP_GO_F is the official NOOP.
* 
* if, and while are followed by an (expression) which generate OP_GO_F(condition).,
* The else branch generates additional unconditional OP_GO_T({c4:c0} == 5'b0).
* do {<statement list>} (expression) generates an OP_GO_T(condition) (jmp on true).
* The compiler optimizes if() goto.
*/
//Following are used by test condition in ForStmt for $loopReg <relop> <limit>.
//Code generated is $reg0 <= limit; cmp $reg0 $loopReg
typedef struct {const char *txtP, *wordP; int op, cond;} sRELOP;

static const sRELOP Relops[] = 
    {{"<",  "LT",   OP_GO_T, COND_NC+COND_NZ},   //+0 go_t(Z || C) == go_f(~Z && ~C)
     {"<=", "LEQ",  OP_GO_F, COND_CC        },   //+1 go_f(C)   
     {">",  "GT",   OP_GO_T, COND_CC        },   //+2 go_t(C)   
     {">=", "GEQ",  OP_GO_F, COND_NC+COND_NZ},   //+3 go_t(C || Z))== go_f(~C && ~Z)
     {"==", "EQ",   OP_GO_T, COND_ZZ        },   //+4 go_t(Z)   
     {"!=", "NEQ",  OP_GO_T, COND_NZ        },   //+5 go_t(~Z)
     //non comparisons cases (note: '!=' can never match hereafter)
     {"!=", "C",    OP_GO_F, COND_CC        },   //+6 go_f(C)
     {"!=", "~C",   OP_GO_F, COND_NC        },   //+7 go_f(NC)
     {"!=", "Z",    OP_GO_F, COND_ZZ        },   //+8 go_f(Z)
     {"!=", "~Z",   OP_GO_F, COND_NZ        },   //+9 go_f(NZ)
     {"!=", "FULL", OP_GO_F, COND_FULL      },   //10 go_t(full)
     {"!=", "!FULL",OP_GO_T, COND_FULL      },   //11 go_f(full)
     {"!=", "~FULL",OP_GO_F, COND_FULL      },   //12 go_f(full)
     {"!=", "QRDY", OP_GO_T, COND_QRDY      },   //13 go_t(qrdy)
     {"!=", "!QRDY",OP_GO_F, COND_QRDY      },   //14 go_f(qrdy)
     {"!=", "~QRDY",OP_GO_F, COND_QRDY      },   //15 go_f(qrdy)
     {"!=", "1",    OP_GO_F, 0              },   //16 go_f(true)
     {"!=", "0",    OP_GO_T, 0              }};  //17 go_t(true)

//NOTE (4): condition bits: c0=qRdy, c1=carry, c2=~carry, c3=zero, c4=~zero
//                          c3=1 & c4=1  signal row full on OP_SCIN
//                          c0           target ready in input que 
//On entry initial '(' has already been scanned.
//Generate code to jump on failure of the expression between ().
//This applies directly to if() and while() but is dead wrong for do{...} ()
//which must invert the sense of the switch.
int cCompile::Conditional(int pc, bool *ifTruP)
   {int         ii, startPc=pc;                                                 //
    *ifTruP = false;                                                            //
    if ((ii=SimpleCondition(Get())) >= 0)                                       //
       {if (*ifTruP=Relops[ii].wordP[0] == '1') pc = pc;                        // (true): nothing-to-do
        else {pc = GenerateOp(pc, Relops[ii].op, Relops[ii].cond, 0, 0);        //
              BugEmit(startPc, pc-1, NULL);                                     //
       }     }                                                                  //
    else                                                                        //
       {Backup(m_a); pc = CompileExpression(pc, m_safeRegB); InvertJmp(pc-1);} //
    if (!Is(')'))                                 return ErrorA(ERR_7184, m_a); //7184 = missing or unbalanced parentheses
    return pc;                                                                  //
   } //cCompile::Conditional

int cCompile::IfStmt(int pc)
   {int erC, jIf, jElse; bool ifTruB;                                           //ie., if (true)
    if (!Is('(')) return Error(ERR_3007, "");                                   //3007 = missing '('
    if ((erC=Conditional(jIf=pc++, &ifTruB)) < 0) return erC;                   //expression following 'if (
    //optimize 'if (condition) goto label;'                                     //
    if (Is("break"))                                                            //if (condition) goto label/break;
       {m_breaksP = (int*)realloc(m_breaksP, (m_breaks+1)*sizeof(int));         //
        m_breaksP[m_breaks++] = pc-1;                                           //
        InvertJmp(pc-1);                                                        //
        if (!Is("else")) Backup(m_a);                                           // ... break; else. Who cares ?
        return pc;                                                              //
       }                                                                        //
    if (IsWord("goto"))                                                         //if (condition) goto label/break;
       {Backup(m_a);                                                            //         "
        if ((erC=pc=StatementList(pc-1, ';', true)) < 0) return erC;            //true = one statement
        if ((m_codeP[jIf].op.u16 ^= (OP_GO_T ^ OP_GO_F)) == OP_GO_F)            //flip sense of goto in StatementList
           {pc--; if (m_bugEmitB) Bugout("%03d: Dead opcode at removed\n", pc);}//if (false) - be serious 
        else                                                                    //
           BugEmit(pc-1, pc-1, "*change sense of goto", m_codeP[jIf].tgtLabelP);//display modified opcode
        if (!Is("else")) Backup(m_a);                                           //ignore else clause 'if () goto; else...' <who cares>
        return pc;                                                              //
       }                                                                        //
    if ((pc=CompoundStatement(pc)) < 0)                              return pc; //
    if (!Is("else")) {FixGoto(jIf, pc,"*jmp around if"); Backup(m_a);return pc;}//jump around if statement
    //has else branch                                                           //
    EmitOp(jElse=pc++, OP_GO_T, 0);                                             //placeholder for jump around else statement
    FixGoto(jIf, jElse+1, "*jmp around if");                                    //jump around if statement+1
    Get();                                                                      //
    if ((pc=CompoundStatement(pc)) < 0)                              return pc; //
    FixGoto(jElse, pc, "*jmp around else");                                     //
    return pc;                                                                  //
  } //cCompile::IfStmt...

int cCompile::WhileStmt(int pc)
   {bool        emptyStmtB = false, ifTruB=false;                               //while statement is empty
    int         erC, jWhile, top=pc;                                            //
    PUSH_BREAKS_AND_CONTINUES(); m_loopDepth++;                                 //
                                                                                //
    if (!Is('('))                               return ErrorA(ERR_3007, m_a);   //3007 = missing '('
    if ((pc=Conditional(pc, &ifTruB)) < 0)      return pc;                      //evaluate (condition) following 'while'
    jWhile = pc-1;                                                              //save location of GO_T/F
    if (!Is('{'))                               return ErrorA(ERR_2520, m_a);   //2520 = missing '{' 
    if (!(emptyStmtB=Is('}')))                                                  //while (...) {}
       {Backup(m_a); if ((erC=pc=StatementList(pc, '}')) < 0) return erC;}      //    {<statement>}
    //If the while statement is empty and the conditional comprises a single    //
    //conditional goto, then the GO_F can be replaced by GO_T back to itself.   //
    //The only possible case for this is while(!qrdy) {} since this is the only //
    //condition under which the while condition could change without warning.   //
    if (emptyStmtB && (pc - jWhile) == 1 && IsGoOp(m_codeP[jWhile].op))         //
         {m_codeP[jWhile].op.go.relAdr = -1;                                    //loop on itself.
          BugEmit(jWhile, jWhile, "*tight loop", NULL, true);                   //bug change of conditional
         }                                                                      //
    else                                                                        //
    if (ifTruB)                                                                 //
       {pc = GenerateOp(pc, OP_GO_T);                                           //while(true) 
        FixGoto(pc-1, top, "jmp top of while");                                 //
       }                                                                        //
    else                                                                        //
//     {EmitOp(pc++, m_codeP[jWhile].op.u16 ^ (OP_GO_T ^ OP_GO_F),top+1,true);  //jump back to top on reverse of while condition
       {pc = GenerateOp(pc, OP_GO_T, top);
        FixGoto(jWhile, pc, "*exit while");                                     //exit from while loop
       }                                                                        //
    if ((erC=FixContinues(top)) < 0) return erC;                                //could jump to pc-1 if it is closer
    if ((erC=FixBreaks(pc))     < 0) return erC;                                //
    m_loopDepth--;                                                              //
    return pc;                                                                  //
  } //cCompile::WhileStmt...

int cCompile::DoStmt(int pc)
   {int erC, top=pc, errN=ERR_3007; bool ifTruB, emptyStmtB=false;              //3007 = missing '('
    PUSH_BREAKS_AND_CONTINUES(); m_loopDepth++;                                 //
    if (!Is('{'))                                return ErrorA(ERR_2520, m_a);  //520 = missing '{' 
    if (Is('}')) emptyStmtB = true; else                                        //do {} while(...)
       {Backup(m_a); if ((erC=pc=StatementList(pc, '}')) < 0) return erC;}      //do {<statements>}
    if (!Is("while") || !Is('('))                goto err;                      //3007 = missing '('
    if ((erC=FixContinues(pc)) < 0)              return erC;                    //
    if ((erC=Conditional(pc++, &ifTruB)) < 0)    return pc;                     //(expression) following 'do {...} while ('
    m_codeP[pc-1].op.go.relAdr = top - pc; InvertJmp(pc-1);                     //
    if (ifTruB) m_codeP[pc-1].op.shortOp = OP_GO_T;                             //
    BugEmit(pc-1, pc-1, "top of do{}");                                         //bug change of conditional
    if (!Is(';'))                                goto err;                      //3007 = missing '('
    if ((erC=FixBreaks(pc)) < 0)                 return erC;                    //
    m_loopDepth--;                                                              //
    return pc;                                                                  //
err:return ErrorA(errN, m_a);                                                   //
   } //cCompile::DoStmt...

//Syntax is: for (initialStatement(s); expression; incrementStatement(s)) {body}
//eg. for ($reg=init; $reg < limit; $reg++) {....}       param forz == 0
//or  forz($reg=init; $reg < limit; $reg++) {....}       param forz == 1
//forz(...) will execute at least once and clobbers reg$0 unless safeReg0 is set.
//for(...)  may execute zero times and preserves reg$0 at the expense of 
//          1 more op in the initilization and 2 more ops in the loop
int cCompile::ForStmt(int pc, IATOM aa)
   {bool        forz=aa.len == 4;                                               //forx versus for    
    int         start, jj, testCnt, incCnt, forzAdr, szBlob=sizeof(sCODE_BLOB); //
    sCODE_BLOB *testCodeP, *incCodeP;                                           //save test and inc code
    bool        bugEmitB = m_bugEmitB;                                          //
    PUSH_BREAKS_AND_CONTINUES(); m_loopDepth++;                                 //
    if (!Is('('))                                return ErrorA(ERR_3007, m_a);  //3007 = missing '('
//initialStatement                                                              //
    if (!Is(';'))                                                               //empty initial statement
       {Backup(m_a);                                                            //
        if ((pc=StatementList(pc, ';', true)) < 0) return pc;                   //true = one statement only
       }                                                                        //   (possibly with commas)
    m_bugEmitB = false;                                                         //supress during test and step
//test condition: expression                                                    //
    if (Is(';')) {testCodeP = NULL; testCnt = 0;}                               //empty test, eg. for($1=0;;$1++)
    else {Backup(m_a); start = pc;                                              //
          if ((pc=CompileExpression(pc, !forz || m_safeRegB)) < 0) return pc;  //
          testCodeP           = (sCODE_BLOB*)alloca((testCnt=pc-start)*szBlob); //
          memmove(testCodeP, &m_codeP[pc=start], testCnt * szBlob);             //save and back out test code
          if (!Is(';'))                          return ErrorA(ERR_3007, m_a);  //3007 = missing ';'
         }                                                                      //
//incrementStatement                                                            //
    if (Is(')')) {incCodeP = NULL; incCnt = 0;}                                 //empty increment, eg. for (%1=0; $1<5;)
    else                                                                        //
       {Backup(m_a);                                                            //
        if ((pc=StatementList(start=pc, ')')) < 0)   return pc;                 //
        incCodeP   = (sCODE_BLOB*)alloca((incCnt=pc-start)*szBlob);             //
        memmove(incCodeP, &m_codeP[pc=start], incCnt * szBlob);                 //save and back out increment code
       }                                                                        //
    if (forz &&                                                                 //
        (jj=SimplifiedForz(testCodeP,testCnt,incCodeP,incCnt)) > 0)testCnt = jj;//
    m_bugEmitB = bugEmitB;                                                      //
//body of for statement, ie {statementList}                                     //
    if (!Is('{'))                                return ErrorA(ERR_2520, m_a);  //2520 = missing '{'
    forzAdr = pc;                                                               //
    if (!forz) pc = GenerateOp(pc, OP_GO_T, 0);                                 //initial jump around {body} to conditional
    start = pc;                                                                 //start of looped code
    if ((pc=StatementList(pc, '}')) < 0)         return pc;                     //
//end of loop processing                                                        //
    if ((jj=FixContinues(pc)) < 0) return jj;                                   //
    memmove(&m_codeP[pc], incCodeP, incCnt * szBlob);                           //emit increment code
    pc = pc + incCnt;                                                           //step over increment code
    if (!forz) m_codeP[forzAdr].op.go.relAdr = pc-(forzAdr+1);                  //relative address
//Copy testCodeP to output and patch unresolved goto's fromCompileExpression(). //
    for (jj=0; jj < testCnt; jj++, pc++)                                        //
       {if (IsGoOp((m_codeP[pc]=testCodeP[jj]).op))                             //
                                        m_codeP[pc].op.go.relAdr = start-(pc+1);//
       }                                                                        //
    BugEmit(forzAdr, pc-1);                                                     //
    if ((jj=FixBreaks(pc)) < 0) return jj;                                      //
    m_loopDepth--;                                                              //
    return pc;                                                                  //
   } //cCompile::ForStmt...

int cCompile::SimplifiedForz(sCODE_BLOB *testCodeP, int testCnt, sCODE_BLOB *incCodeP, int incCnt) 
/*The source statements:
   'forz($reg=value; $reg >= 0; $reg--) {....} would normally generate:
 initialization:
    001:02B6 $reg5 <= value
loopcode:
    002:ABB8       OP_DEC $reg5                     //incCodeP[0]
    003:0006       $reg0 <= 0x0                     //testCodeP[0]
    004:04B8       OP_CMP $reg5                     //testCodeP[1]
    005:FC11       OP_GO_T(C) loop                  //testCodeP[2] $reg >= 0;
or      FC81       OP_GO_T(~Z)loop                  //testCodeP[2] $reg != 0;
The loop code can be improved to:
    002:ABB8 loop: OP_DEC $reg5                     //incCodeP[0]
    003:0111       OP_GO_T(~Z) loop                 //testCodeP[0]
a net saving of two instructions and leaving reg$0 unchanged.
NOTE: If this optimization technique becomes widely used, then it
      will be converted to a table driven function :)
*/
   {OPCODE op=incCodeP[0].op; int sreg=op.arith.breg, dreg=op.arith.areg, adr;
    if (incCnt != 1 || testCnt != 3) return 0;                                  //
    if (!IsArithOp(op, OPS_DEC) || sreg != dreg)           return 0;            //
    op = testCodeP[0].op; adr = op.ri.imm;                                      //
    if (!IsRegImmOp(op) || op.arith.breg != 0 || adr != 0) return 0;            //$reg0 <= 0x0
    op = testCodeP[1].op;                                                       //
    if (!IsArithOp(op, OPS_CMP) ||                                              //
        op.arith.breg  != sreg  || op.arith.areg != 0)     return 0;            //
    op = testCodeP[2].op;                                                       //
    if (!IsGoOp(op) || (op.go.cond != COND_CC &&                                //$reg >= 0;
                        op.go.cond != COND_NZ))            return 0;            //$reg != 0;
    //ok recook that testCode                                                   //
    op.go.cond      = COND_NZ;                                                  //
    testCodeP[0].op = op;                                                       //
    return 1;                                                                   //size of improved code
   } //SimplifiedForz...

//Break or continue statements; generate jump out from loop and log address in
//m_breaksP or m_continuesP. These are cleaned up on exit from loop stmt.
int cCompile::BreakStmt(int pc, int isBreak, char **labelPP)                    //
   {int  *listP=isBreak ? m_breaksP : m_continuesP,                             //
          count=isBreak ? m_breaks  : m_continues;                              //
    if (m_loopDepth == 0) return ErrorA(ERR_2737, m_a);                         //2737 Illegal placement of break/continue.
    *labelPP                  = strdupl(isBreak ? "break" : "continue");        //save label for BugEmit()
    listP                     = (int*)realloc(listP, (count+1)*sizeof(int));    //
    pc                        = GenerateOp(pc, OP_GO_T, 0);                     //just an ordinary rel jmp
    m_codeP[pc-1].tgtLabelP   = *labelPP;                                       //
    m_codeP[pc-1].isBreakB    = (isBreak != 0);                                 //
    m_codeP[pc-1].isContinueB = (isBreak == 0);                                 //
    listP[count++]            = pc-1;                                           //
    if (isBreak){m_breaksP    = listP; m_breaks    = count;}                    //
    else        {m_continuesP = listP; m_continues = count;}                    //
    return pc;                                                                  //
   } //cCompile::BreakStmt...

//Called at the end of ForStmt, WhileStmt, and DoStmt to fix break/continue addresses
//Substitutes all goto's generated when a 'break/continue' was encountered
//then frees m_breaksP/m_continuesP. 
//The caller (eg ForStmt) calls PUSH_BREAKS_AND_CONTINUES on entry, then calls
//FixBreaks/FixContinues at the appropriate spots.
//Flush cycles thru (m_breaksP, m_breaks) or (m_continuesP, m_continues) fixing 
//addresses, then restores m_breaks/m_continues to the values that prevailed at 
//the start of the calling function (eg ForStmt).
int cCompile::FixThese(int pc, bool isBreakB, int *prevP, int prevCnt)
   {int ii, relAdr, *listP, count;                                              //
    m_nastiesP = NULL; m_nastyCnt = 0;                                          //
    if (isBreakB) {listP = m_breaksP;    count = m_breaks;   }                  //
    else          {listP = m_continuesP; count = m_continues;}                  //
    while (count > 0)                                                           //
        {ii = listP[--count];                                                   //
         if ((relAdr = pc-(ii+1)) > 0x7F)                                       //will need a long jmp
            {m_nastiesP=(int*)realloc(m_nastiesP,(m_nastyCnt+1)*sizeof(int));   //
             m_nastiesP[m_nastyCnt++] = ii;                                     //
            }                                                                   //
         else m_codeP[ii].op.g.adr = relAdr;                                    //relative address
         BugEmit(ii, ii, "*", isBreakB ? "break;" : "continue;", true);         //
        }                                                                       //
    free(listP);                                                                //m_breaksP or m_continuesP
    if (isBreakB) {m_breaksP    = prevP; m_breaks    = prevCnt;}                //POP breaks
    else          {m_continuesP = prevP; m_continues = prevCnt;}                //POP continues
    return ResolveNasties();                                                    //
   } //cCompile::FixThese...

//nasties are relative jumps that will no longer work because too much code
//has been inserted between the op_go and the target of the go. 
//This can and should be fixed but is not so in this revision.
int cCompile::ResolveNasties(void)
   {if (m_nastyCnt == 0) return 0;
    free(m_nastiesP); m_nastyCnt = 0;
    return Error(ERR_9998, "nasties");
   } //cCompile::ResolveNasties...

//Allocate various variables in specified row; syntax is:
//allocate [row#] = 
//   {uint64_t vbl1,
//             vbl2,
//             vbl3,...
//   };
//Only 64 bit variables are supported at the moment.
int cCompile::AllocateBram(void)
   {int64_t row, wordAdr; int erC;
    if (!Is('[')) return ErrorA(ERR_0001,m_a);
    if ((erC=ConstantExpression(&row)) < 0) return erC;
    if (!Is(']') || !Is('=') || !Is('{'))   return ErrorA(ERR_0001, m_a);
    for (wordAdr=0; !Is('}'); wordAdr++)
        {if (IsWord("uint64_t")) Get();
         if (m_a.type != GC_NAME)           return ErrorA(ERR_0001, m_a);
         m_vblsP = (sVARIABLE*)realloc(m_vblsP, (m_vblCount+1)*sizeof(sVARIABLE));
         m_vblsP[m_vblCount  ].nameP   = strdupl(m_a.textP, m_a.len);
         m_vblsP[m_vblCount  ].row     = row;
         m_vblsP[m_vblCount  ].nameLen = m_a.len;
         m_vblsP[m_vblCount++].wordAdr = wordAdr;
         if (!Is(',') && !IsChar(';'))      return ErrorA(ERR_0001, m_a);
        }
    return 0;
   } //cCompile::AllocateBram...

int cCompile::LookupVbl(const char *nameP, int nameLen)
   {for (int ii=m_vblCount; --ii >= 0;)
        if (m_vblsP[ii].nameLen == nameLen && 
            strncmp(m_vblsP[ii].nameP, nameP, nameLen) == 0) return ii;
    return -1;
   } //cCompile::LookupVbl...
    
//print   "string1" "string1" "string3" ...
//string  "string1" "string1" "string3" ...
//$expect zero of more "strings". Generate "#expect:" "string1" "string2" ... and append \n
//$actual zero of more "strings". Generate "#actual:" "string1" "string2" ... and append \n
//If no string parameters are provided in the $expect/$actual cases then no \n
//is appended and a print "\n" must be used to terminate the $actual/$expect.
//   (this allows multiple print statements within the $expect/$actual ..... \n code
int cCompile::PrintStmt(int pc, int caze) 
   {int msgnum, len=8; char *stringP; bool catB=false;                          //
    if (caze == 2) stringP = (char*)"#actual:"; else                            //$actual "..."
    if (caze == 1) stringP = (char*)"#expect:"; else                            //$expect "...."
       {if (Get().type != GC_STRING) return ErrorA(ERR_7180, m_a);              //0 & 3 print and string
        stringP = (char*)m_a.textP; len = m_a.len;                              //
       }
    //concatenate multiple strings                                              //
    for (; Get().type == GC_STRING; catB=true)                                  //
        {if (!catB) stringP = strdupl(stringP, len);                            //first time
         stringP = (char*)realloc(stringP, 2+(len+=m_a.len));                   //make space; 2 eh !
         strncat(stringP, m_a.textP, m_a.len);                                  //append next string
        }                                                                       //
    Backup(m_a);                                                                //
    if ((caze == 1 || caze == 2) && catB) {strcat(stringP, "\n"); len++;}       //$expect/$actual; ok: 2+(len... :)        
    msgnum = m_opNameP->StoreMessage(stringP, len);                             //
    if (catB) free(stringP);                                                    //
    if (caze != 3) pc = GenerateOp(pc, OP_PRINT+(caze << 5), msgnum);           //3 == $string
    return pc;                                                                  //
   } //cCompile::PrintStmt...

//OP_SCAN/OP_SCIN   are qualified with (indx) or (page) or (book)    configB = false
//OP_CFG            is  qualified with (cell) or (group)             configB = true
//cell and page/book return 0x100 (op.g.breg or op.sc.rowType == 1).
int cCompile::Qualified(int pc, bool configB)
   {IATOM   aa;                                                                 //
    OPCODE *opP=&m_codeP[pc].op;                                                //
    if (!Is('('))                                 return ErrorA(ERR_3007, m_a); //3007 = Missing open parenthesis
    //opcode qualifier, (cell), (group), (indx), (page), or (book)              //
    //eg. cfg(cell), scin(indx) <target>. set = 1, type from qualifier.         // 
    if ((aa=Get()).type != GC_NAME)               return ErrorA(ERR_3000, m_a); //3007 = Missing name
    if (!Is(')'))                                 return ErrorA(ERR_7184, m_a); //7184 = Missing close parenthesis ')' in an expression
    m_a = aa;                                                                   //makes IsWord() work properly
    if (configB)                                                                //
       {if (IsWord("cell")) {opP->g.breg = 1;     return pc+1;}                 //
        if (IsWord("group"))                      return pc+1;                  //
       }                                                                        //
    else                                                                        //
       {if (IsWord("indx")) {opP->sc.rowType = 1; return pc+1;}                 //indx/page bit in OP_CFG
        if (IsWord("page"))                       return pc+1;                  //book & page have identical structures
        if (IsWord("book"))                       return pc+1;                  //                 "
       }                                                                        //
    return ErrorA(ERR_2731, m_a);                                               //2731 = Missing (cell) or (group)
   } //cCompile::Qualified...

IATOM cCompile::Get(bool probeB) 
   {while ((m_a=m_azP->GetAtom(probeB)).type == GC_COMMENT) {}
    return m_a;
   } //cCompile::Get...

void cCompile::Backup(IATOM aa) {m_azP->Backup(aa);}

//build name of microcode, message, linemap, and capture files.
//NOTE: this fully qualified name is not what is stored in samControl.sv
//  since Vivado can't manage a fully qualified name but insists that the
//  file be in the 'current directory', ie., genenv("VerilogSimulationDir")
int cCompile::BuildFileNames(char *srcFileNameP)
   {const char *pp, *qq; 
    int         ii;                                                             //
    #define CreateSubordinateFileName(name, first)                              \
        if (!first) strncpy(m_##name, m_microcode, sizeof(m_##name) - 1);       \
        strncpy(&m_##name[ii], "." #name, sizeof(m_##name) - ii-1);             //
                                                                                //
    m_srcFileNameP = srcFileNameP;                                              //
    for (pp=qq=m_srcFileNameP; pp=strpbrk(qq, "\\/");) qq=pp+1;                 //find last '\' or '/'
    SNPRINTF(m_microcode),"%s\\%s", getenv("VerilogSimulationDir"), qq);        //
    if (!(pp=strrchr(m_microcode, '.')))return Error(ERR_7296, m_microcode);    //7296 = Invalid filename '%s'
    ii = (int)(pp - m_microcode);                                               //
    CreateSubordinateFileName(microcode,   true);                               //
    CreateSubordinateFileName(msgFile,     false);                              //
    CreateSubordinateFileName(lineMap,     false);                              //
    CreateSubordinateFileName(symbolTbl,   false);                              //
//  if (g_printFileP == NULL)                                                   //user has overriden cap file name
//     {CreateSubordinateFileName(capFile, false); g_printFileP = m_capFile;}   //
#undef CreateSubordinateFileName
    return 0;
   } //cCompile::BuildFileNames...

//Get a number allowing:
// - Verilog format (eg 8'h55), 
// - C++ format (eg 0x123)
// - binary (eg 0b0100010), 
// - k/m/b suffixes (10^3, 10^6, and 10^9).
//For example: 123, 0x456, 19'h123_456, 7'b0000_0001, 100k, 100m, 100b
uint64_t cCompile::Anumber(const char *ppc, char **ppP, int *bitCountP)
   {uint64_t  value;
    int       bitCount;                                                         //
    char     *pp=(char*)ppc;                                                    //
    if (!bitCountP) bitCountP = &bitCount;                                      //someplace to put it
    *bitCountP = 32;                                                            //
    pp += strspn(pp, " ");                                                      //
    if (pp[0] == '0' && pp[1] == 'x') value = _strtoui64(pp+2, &pp, 16);        //
    else                              value = _strtoui64(pp,   &pp, 10);        //
    if (*pp == '\'')                                                            //eg 31'h913
       {*bitCountP = (int) value;                                               //
        if (*(++pp) == 'h') value = _strtoui64(pp+1, &pp, 16); else             //Verilog representation
        if (*(  pp) == 'b') value = _strtoui64(pp+1, &pp,  2); else             //
        if (*(  pp) == 'd') value = _strtoui64(pp+1, &pp, 10); else             //
                            value = _strtoui64(pp+0, &pp, 10);                  //
      }                                                                         //
    else
      {//bitcount not specified Verilog style. guess at bit count: 
       //value <= 255 bitCount 8, <= 65535 bitCount 16, >= 2**32 bitCount 64
       if (value <=   255)         *bitCountP = 8; else
       if (value <= 65535)         *bitCountP =16; else
       if (value > 0x100000000ull) *bitCountP =64; 
      }
    if ((*pp | 0x20) == 'k') {value *= 1000;          pp++;} else               // * 1000 kilo
    if ((*pp | 0x20) == 'm') {value *= 1000000;       pp++;} else               // * 1000,000 millions
    if ((*pp | 0x20) == 'b') {value *= 1000000000;    pp++;} //else             // * 1000,000,000 billions
//  if ((*pp | 0x20) == 't') {value *= 1000000000000; pp++;}                    // * 1000,000,000,000 trillions
    if (ppP) *ppP = pp;                                                         //
    return value;                                                               //
   } //CompilerWrap::Anumber...
   
//Compile offset represented as +expression or -expression into OP_WRF or OP_RDF opcodes.
int cCompile::SignedOffset(OPCODE *opP)
#if 1
   {return Error(ERR_0001, "");}
#else
   {bool plusB; int64_t result; int erC;                                        //
    uint16_t mask = (1 << (m_simP->m_bitsInOffsetField-1));                     //
    if ((plusB=Is('+')) || IsChar('-'))                                         //
       {if ((erC=ConstantExpression(&result)) < 0) return erC;                  //
        if (plusB) opP->ind.offset = result    & (mask-1);                      //
        else       opP->ind.offset = (-result) & (mask-1) | mask;               //
       }                                                                        //
    else Backup(m_a);                                                           //
    return 0;                                                                   //
   } //cCompile::SignedOffset...
#endif

//Lookup field name as used in [$reg].fieldName
int cCompile::FieldName(IATOM aa, int fldNum, CC *namePP)
   {static const char *names[] =                                                //
      {"",     "data", "stop",                                                  //0-2  raw and indx fields
       "P1",   "P2",   "count", "total", "stop", //preempted by names[2]        //3-7  page/book fields
       "key0", "key1", "key2",  "key3",  "key4", "key5", "key6", "key7"};       //8-15 key fields
   if (fldNum >= 0)                                                             //called to map num->name
      {*namePP = fldNum < HOWMANY(names) ? names[fldNum] : NULL;  return 0;}    //          "
   for (int ii=1; ii < HOWMANY(names); ii++)                                    //called to map name->num
       if (strnicmp(names[ii], aa.textP, strlen(names[ii])) == 0) return ii;    //
    return ErrorA(ERR_6915,aa);                                                 //6915 = Field specified is invalid
   } //cCompile::FieldName...

//Optimize push $n, $n+1, $n+2, etc by using repeat(push $n, reg++) or n-1, n-2,...
int cCompile::CollapsePops(int startPc, int pc)
   {sCODE_BLOB *bP=&m_codeP[startPc];                                           //
    int         ii, jj, count=(pc-startPc), baseReg=bP->op.arith.breg;          // 
    bool        incB, decB;                                                     //
    if (count <= 2) return pc;                                                  //no point in optimizing
    //Determine if registers are in sequence                                    //
    for (ii=startPc, jj=0, decB=incB=true; ii < pc; ii++, jj++)                 //are registers in 
        {incB = incB && (m_codeP[ii].op.arith.breg == (baseReg+jj));            // - ascending  sequence ?
         decB = decB && (m_codeP[ii].op.arith.breg == (baseReg-jj));            // - descending sequence ?
        }                                                                       //
    if (!incB && !decB) return pc;                                              //not in sequence
    bP->op.u16       = OP_REPEAT;                                                  //create OP_REPEAT
    bP->op.rpt.count = count-1;                                                 //       "
    bP->op.rpt.stepR = 1;                                                       //       "
    bP->op.rpt.bkwd  = decB ? 1 : 0;                                            //       "
    //previously generated opcode is almost correct - just the register is wrong//
    m_codeP[startPc+1].op.arith.breg += (decB ? 1 : -1);                        //valid opcode, but wrong register
    return startPc+2;                                                           //ie op_repeat, op_push/pop
   } //cCompile::CollapsePops...

//is next atom == ch
bool cCompile::Is(char ch)
   {bool bb = Get().type == GC_GETX && m_a.textP[0] == ch; return bb;}

bool cCompile::IsChar(char ch)
   {return m_a.type == GC_GETX && m_a.len == 1 && m_a.textP[0] == ch;}

bool cCompile::IsRegister(IATOM aa, int *regP)
   {if (aa.type == GC_NAME && aa.textP[0] == '$' && aa.textP[1] >= '0' && aa.textP[1] <= '7' && aa.len == 2)
       {if (regP) *regP = aa.textP[1] & 7; return true;}
    return false;
   } //cCompile::IsRegister...

bool cCompile::Is(const char *wordP)
   {bool bb=Get().type == GC_NAME && IsWord(wordP); m_ref = m_a.ref; return bb;}
 
bool cCompile::IsWord(const char *wordP)
   {return m_a.type == GC_NAME      &&                  //
           m_a.len == strlen(wordP) &&                  //
           strncmp(m_a.textP, wordP, m_a.len) == 0;     //case sensitive
   } //cCompile::IsWord..

bool cCompile::IsWordNoCase(const char *wordP)
   {return m_a.type == GC_NAME      &&                  //
           m_a.len == strlen(wordP) &&                  //
           strnicmp(m_a.textP, wordP, m_a.len) == 0;    //
   } //cCompile::IsWord..

bool cCompile::IsNoCase(const char *wordP)
   {Get();                                               //
    m_ref = m_a.ref;                                     //
    return m_a.type == GC_NAME       &&                  //
           m_a.len == istrlen(wordP) &&                  //
           strnicmp(m_a.textP, wordP, m_a.len) == 0;     //case insensitive
   } //cCompile::IsNoCase...

bool cCompile::IsNumber(uint32_t *valP)
   {IATOM aa;
    if ((aa=Get()).type == GC_INT)
       {*valP = (int)Anumber(aa.textP); return true;}
    return false;
   } //cCompile::IsNumber...

//Generate one opcode; p2 various secondary field, sreg & dreg as expected.
//adr field is not provided and is set to zero. Caller must patch up adr field if reqd.
int cCompile::GenerateOp(int pc, int opcode, int p2, int sreg, int dreg)
   {sCODE_BLOB *bP=&m_codeP[pc]; OPCODE *opP=&bP->op;                           //
    bP->ref = m_ref;                                                            //
    switch ((opP->u16=opcode) & 31)                                             //
        {case OP_ARITH:                                                         //
             opP->arith.subOp = p2;                                             //p2 = subOp: OPS_ADD, ADC, etc
             opP->arith.breg  = sreg;                                           //
             opP->arith.areg  = dreg;                                           //
             break;                                                             //
         case OP_BUG:                                                           //
             opP->bug.level   = p2;                                             //
             opP->bug.set     = sreg;                                           //
             opP->bug.sho     = dreg;                                           //
             break;                                                             //
         case OP_CROW:                                                          //
             opP->ind.areg    = p2;                                             //
             break;                                                             //
         case OP_CROWI:                                                         //p2 = literal cur row
         case OP_PRINT:                                                         //
             opP->g.adr       = p2;                                             //p2 = msg #
             break;                                                             //
         case OP_GO_T: case OP_GO_F:                                            //
             opP->go.cond     = p2;                                             //
             break;                                                             //
         case OP_LDI:                                                           //
             opP->ldi.imm     = p2;                                             //
             break;                                                             //
         case OP_REPEAT:                                                        //%
             opP->rpt.count   = p2 - 1;                                         //repeat count
             opP->rpt.stepA   = sreg;                                           //inc address
             opP->rpt.stepR   = dreg;                                           //int register
             opP->rpt.bkwd    = sreg < 0 || dreg < 0;                           //direction
             break;                                                             //
          case OP_REPREG:                                                       //
             opP->rptR.breg   = p2;                                             //repeat $reg
             opP->rptR.stepA  = sreg;                                           //inc address
             opP->rptR.stepR  = dreg;                                           //int register
             opP->rptR.bkwd   = sreg < 0 || dreg < 0;                           //direction
             break;                                                             //
         case OP_RI:                                                            //
             opP->ri.breg     = sreg;                                           //
             opP->ri.imm      = p2;                                             //R2R as $reg = literal
             break;                                                             //
        }                                                                       //
    return pc+1;                                                                //
   } //cCompile::GenerateOp...

//Generate one opcode at address == pc. 
//superOp may contain reg, or condition fields as well as the base OP_READ, OP_BUG, etc
void cCompile::EmitOp(int pc, uint16_t superOp, uint32_t adr, bool knownB)
   {sCODE_BLOB *bP=&m_codeP[pc];                                                //
    bP->op.u16    = superOp;                                                    //
    bP->ref.lineNum   = m_ref.lineNum;                                          //
    bP->ref.fileNum   = m_ref.fileNum;                                          //
    bP->ref.srcOffset = m_ref.srcOffset;                                        //
    bP->op.g.adr  = IsGoOp(bP->op) ? adr - (pc+1) : adr;                        //goto's expect relative adr
    BugEmit(pc, pc, NULL, NULL, true);                                          //
   } //cCompile::EmitOp...

void cCompile::EmitOp(int pc, OPCODE op, uint32_t tgt, bool knownB)
   {EmitOp(pc, *(uint16_t*)&op, tgt, knownB);}

//FixGoto address in opcode at pc: 
void cCompile::FixGoto(int pc, uint32_t tgtPc, const char *commentP)
   {m_codeP[pc].op.go.relAdr = tgtPc - (pc+1);                                  //relative address
    if (!m_codeP[tgtPc].hereP) m_codeP[tgtPc].hereP = strdupl(">");     //fancy label 
    BugEmit(pc, pc, commentP, NULL, true);                                      //
   } //cCompile::FixGoto...

//flip sense of condition in goto opcode
void cCompile::InvertJmp(int pc)
   {m_codeP[pc].op.u16 ^= (OP_GO_T ^ OP_GO_F);}

void cCompile::BugEmit(int startPc, int endPc, const char *commentP, const char *labelP, bool knownB)
   {OPCODE op; int pc; bool lastWasLongJmpB=false; char flag= ' ';              //
    if (!m_bugEmitB) return;                                                    //
    if (commentP && *commentP == '*') {flag = '*'; commentP++;}                 //
    for (pc=startPc; pc <= endPc; pc++, commentP=NULL)                          //
       {op = m_codeP[pc].op,                                                    //
        Bugout("%03d:%04X%c", pc, op.u16, flag);                                //
        if (m_codeP[pc].isDwB)                                                  //
           {Bugout("DW 0x%04X", op.u16);                                        //
            if (lastWasLongJmpB)Bugout(" (address of %s)", labelP);             //
           }                                                                    //
        else                   Bugout("%s", OPNAME(pc, labelP, knownB));        //
        if (!(lastWasLongJmpB=IsLongJmp(m_codeP[pc].op) && labelP != NULL))     //
             labelP = NULL;                                                     //
        if (commentP) Bugout(" (%s)\n", commentP); else Bugout("\n");           //
   }   } //cCompile::BugEmit...

void cCompile::Bugout(char const *fmtP,...)
    {va_list arg;                                                               //
     char    buf[512];                                                          //
     va_start(arg, fmtP);                                                       //
     vsnprintf(buf, sizeof(buf)-1, fmtP, arg);                                  //
     va_end(arg);                                                               //
     buf[sizeof(buf)-1] = 0;                                                    //
     OutputDebugStringA(buf);                                                   //
    } //cCompile::Bugout...

//Convert two characters at pp from ascii hex to binary
inline uint32_t Hex2Bin(const char *pp)
   {char hex[3];
    hex[0] = pp[0]; hex[1] = pp[1]; hex[2] = 0;
    return (uint32_t)strtol(hex, NULL, 16);
   } //Hex2Bin(...

//Return location of specified label or return -ve error code. Do not call
//Error() since this might be called to verify that label is not already placed.
int cCompile::WhereisLabel(const char *labelP)
   {sCODE_BLOB *bP; int pc=0; bool foundB;                              //
    for (bP=&m_codeP[pc], foundB=false; pc <= m_pgmSize; pc++, bP++)    //
        if (bP->hereP && strcmp(bP->hereP, labelP) == 0) goto ret;      //names are equal ?
    return -ERR_2602;                                                   //2602 = Unresolved label
ret:return pc;                                                          //
   } //cCompile::WhereisLabel...

//Scan program stored in m_codeP for goto or cjmp codes and search
//replace the address field with the actual location of the label.
int cCompile::ResolveGotos(void)
   {int         startPc, pc, jj;                                                //
    sCODE_BLOB *frP;                                                            //goto opcode
    OPCODE      op, max;                                                        //
    bool        callB;                                                          //
    const char *labelP, *commentP;                                              //
                                                                                //
    max.u16 = -1;                                                               //
    for (pc=0, frP=m_codeP; pc < m_pgmSize; pc++, frP++)                        //
        {startPc = pc; labelP = frP->tgtLabelP;                                 //
         if (frP->isDwB || frP->isBreakB || frP->isContinueB || labelP == NULL) //
                                                continue;                       // DW <value> - nothing further to do
         if (((callB=(op=frP->op).call.act == OP_CALL) || IsGoOp(op)) && labelP)//
            {if ((jj=WhereisLabel(labelP)) < 0) return Error(jj,      labelP);  //jj == 2602 = label not found
             commentP = "*resolve goto";                                        //
             if (callB)                                                         //
                 {if (jj > max.call.callAdr)    return Error(ERR_2740,labelP);  //call address too lage for OP_CALL
                  frP->op.call.callAdr = jj;                                    //abs address in OP_CALL 
                  commentP             = "*resolve call";                       //
                 }                                                              //
             else                                                               //
             if (frP->isLongJmpB) {(++frP)->op.u16 = jj; pc++;}                 //OP_GO_L abs address lives in next word
             else{if (jj <= (pc+0x80) && jj >= (pc-0x7F))                       //relative to pc+1, eh !
                     {if ((frP->op.go.relAdr=jj-(pc+1)) == 0 && op.go.act == OP_GO_T)//within range of relative jump
                         {//prevent go_t(+0) because it conflicts with long jmps//
                          //If it is an unconditional jmp (ie condition == 0)   //
                          //then it can simply be replaced with go_f(0).        //
                          //If it is a conditional jump we must insert a noop.  //
                          if (op.go.cond == 0)                                  //
                               {frP->op.go.act ^= (OP_GO_T ^ OP_GO_F);          //
                                commentP        = "*replace go_t(+0) with noop";//
                               }                                                //
                          else {StretchOp(pc+1);                                //
                                frP                  = &m_codeP[pc];            //m_codeP could be relocated by StretchOp
                                frP++->op.go.relAdr  = 1;                       //
                                m_codeP[++pc].op.u16 = OP_NOOP;                 //
                                commentP = "*insert noop to avoid go_t(+0)";    //
                     }    }    }                                                //
                  else                                                          //
                     {StretchOp(pc+1);                                          //insert space for long jump
                      frP               = &m_codeP[pc++];                       //m_codeP could be relocated by StretchOp
                      frP->op.go.relAdr = 0;                                    //signals absolute jump
                      (++frP)->op.u16   = jj + (jj > pc ? 1 : 0);               //
                      frP->tgtLabelP    = frP->hereP = NULL;                    //
                      frP->isDwB        = frP->isCodeAdrB = true;               //
                      commentP          = "*stretch goto";                      //
                  }  }                                                          //
             BugEmit(startPc, pc, commentP, labelP, true);                      //
             if ((jj=ResolveNasties()) < 0) return jj;                          //
        }   }                                                                   //
    return 0;                                                                   //
   } //cCompile::ResolveGotos...

//Insert an opcode at the specified address.
//All ops above this address will be moved. Then the entire codestream is
//scanned to increment any goto/call/dw addresses into the moved area.
//DWs referring to the code space are marked with the flags isCodeAdrB.
void cCompile::StretchOp(int pc)
   {int ii, tgt, adr; bool changesB; sCODE_BLOB *bP;                        //
    CheckCodeSize(m_pgmSize);                                               //
    m_nastiesP = NULL; m_nastyCnt = 0;                                      //
    for (ii=m_pgmSize++; ii > pc; ii--) m_codeP[ii] = m_codeP[ii-1];        //move subsequent code up one location
    m_codeP[pc].op.u16 = 0;                                                 //
    for (ii=0; ii < m_pgmSize; ii++)                                        //
        {changesB = false;                                                  //
         if ((bP=&m_codeP[ii])->isDwB && !bP->isCodeAdrB) continue;         //nothing-to-do
         if (bP->isCodeAdrB && bP->isPcB)                                   //$pc
            {adr = (bP->op.ri.imm << 14) + (bP+1)->op.ldi.imm +1;           //address smeared over OP_RIMM and OP_LDI) 
             (bP+0)->op.ri.imm = adr >> 14;                                 //reconstruct OP_RIMM
             (bP+1)->op.ldi.imm = adr & ((2<<14)-1);                        //reconstruct OP_LDI
             ii++; changesB = true;                                         //
            }                                                               //
         else                                                               //
         if (bP->isCodeAdrB)                                                //DW or long jump
            {if ((tgt=bP->op.u16) < pc) continue;                           //
             bP->op.u16++;       changesB = true;                           //
            }                                                               //
         else                                                               //
         if (IsShortJmp(bP->op))                                            //
            {if ((tgt=(ii+1)+ bP->op.go.relAdr) < pc) continue;             //tgt=target of jump
             if (ii >= pc && tgt >= pc)               continue;             //tgt and source move together
             if (bP->op.go.relAdr == 0x7F)                                  //oh dear, that jmp will no longer work
                {m_nastiesP = (int*)realloc(m_nastiesP, (++m_nastyCnt) * sizeof(int));
                 m_nastiesP[m_nastyCnt++] = ii;                             //
                 changesB   = false;                                        //
                }                                                           //
             else bP->op.go.relAdr++; changesB = true;                      //
            }                                                               //
         else                                                               //
         if (IsCall(bP->op))                                                //
            {if ((tgt=bP->op.call.callAdr) < pc) continue;                  //tgt=target of call
             bP->op.call.callAdr++;  changesB = true;                       //
            }                                                               //
         if (changesB) BugEmit(ii, ii, "*inc'd for StretchOp");             //
        }                                                                   //
    ResolveNasties();
   } //cCompile::StretchOp...

//Return true if there are no jumps to address this pc. (not used)
bool cCompile::NoJumpsToHere(int pc)
   {int         ii;                                                             //
    sCODE_BLOB *frP;                                                            //goto opcode
    OPCODE      op;                                                             //
    for (ii=0, frP=m_codeP; ii < pc; ii++, frP++)                               //
        {if ((op=frP->op).go.act == OP_GO_T && op.g.adr == (pc)) return false;} //user label
    return true;                                                                //
   } //cCompile::NoJumpsToHere...

//Display code generated by compiler
const char *cCompile::GetTgtLabel(uint32_t adr)
   {if (adr >= (uint32_t)m_pgmSize) return NULL;
    return m_codeP[adr].hereP == NULL ? "" : m_codeP[adr].tgtLabelP; 
   } //cCompile::GetTgtLabel...

const char *cCompile::GetHereLabel(uint32_t adr)
   {if (adr >= (uint32_t)m_pgmSize) return NULL;
    return m_codeP[adr].hereP == NULL ? "" : m_codeP[adr].hereP; 
   } //cCompile::GetHereLabel...

void cCompile::CheckCodeSize(int pc)
   {if (pc < (m_pgmAvail - 128)) return;
    m_codeP = (sCODE_BLOB*)realloc(m_codeP, (m_pgmAvail+=1024) * sizeof(sCODE_BLOB));
    if (pc < 0 || pc >= (m_pgmAvail-32)) {Error(ERR_9995, "Invalid program counter (pc)"); exit(1);}
    memset(&m_codeP[pc], 0, (m_pgmAvail - pc) * sizeof(sCODE_BLOB));
   } //cCompile::CheckCodeSize...

//Get filename from fileNum returns in IATOM
const char *cCompile::FileNameOnly(int fileNum)
   {const char *pp=m_azP->GetFileName(fileNum), *qq=pp;
    while (pp=strpbrk(qq, "\\/")) qq=pp+1; //find last '\' or '/'
    return qq;
   } //cCompile::FileName...

const char *cCompile::FileNameOnly(const char *pp)
   {const char *qq=pp;
    while (qq != NULL && (pp=strpbrk(qq, "\\/"))) qq=pp+1; //find last '\' or '/'
    return qq;
   } //cCompile::FileName...

const char *cCompile::GetSourceLineP(sCODE_BLOB *blobP)
   {char *pp=m_azP->GetSource(blobP->ref.fileNum, blobP->ref.lineNum);
    return pp+strspn(pp, " \t");
   } //cCompile::GetSourceLineP...

const char *cCompile::GetSourceLineP(int pc)
   {return pc < 0 || pc >= m_pgmSize ? "" : GetSourceLineP(&m_codeP[pc]);}
 
int cCompile::GetSourceLineNum(int pc)
   {return pc < 0 || pc >= m_pgmSize ? -1 : m_codeP[pc].ref.lineNum;}

char *cCompile::GetContext(void)
   {static char buf[256];
    SNPRINTF(buf), "%s #%04d: %s", FileNameOnly(m_azP->GetFileName(m_ref.fileNum)),
                  m_ref.lineNum, m_azP->GetSource(m_ref.fileNum, m_ref.lineNum));
    return buf;
   } //cCompile::GetContext...
  
//Copy line of source code and replace "\n" with "\\n"
void cCompile::CopySource(char *bufP, int bufSize) 
   {char *pp = m_azP->GetSource(m_ref.fileNum, m_ref.lineNum);
    pp += strspn(pp, " ");
    while (bufSize-- > 3) 
        if ((*bufP++=*pp++) == '\n') 
           {*(bufP-1) = '\\'; *bufP++ = 'n'; bufSize--;}
    *bufP = 0;
    rTrim(bufP);
   } //cCompile::CopySource...

//Generate the raw hex output for Verilog $readmemh()
//Format is: <hex target>_<hex opcode>  //pc: <opcode explanation> #<source line>
int cCompile::GenerateMicrocodeFile(const char *objNameP)
   {int           erC=0, width, fileN=-1, len, lastLineNum=-1, lastOffset=-1,   //
                  pc, pcPlz=-7, badBongoes=-1, badLine;                         // 
    bool          lastWasLongJmpB=false;                                        //
    char          buf[125];                                                     //
    const char   *pp, *qq, *labelP=NULL;                                        //
    sCODE_BLOB   *bP;                                                           //
    cTIMEVALUE    tv;                                                           //
    OPCODE        op;                                                           //
    FILE         *fileP;                                                        //
    #define gen   if ((len=istrlen(buf)) > sizeof(buf)-25)                      \
                     return Error(ERR_1566, "len > sizeof(buf)-25");            \
                  snprintf(&buf[len], sizeof(buf) - len,                        //
                                                                                //
    if (!(fileP=fopen(objNameP, "wb"))) return Error(ERR_7147, objNameP);       //7147 = unable to create <file>
   //1D05 //008 wrBingo : OP_PRINT "case 0:\n#expect:0x7" line 10 source        //example format
    width = 5 // |      |     |          |       |      |        |              //len _hexOpcode
                 + 5 // |     |          |       |      |        |              //len /pc:"000"
                        + m_longestLabel//|      |      |        |              //elementary my dear Sherlock
                                     + 13 //     |      |        |              //OP_abcde_wxyz
                                          + 40 //       |        |              //[000]
                                             + 2 + TARGETBUS_SIZE;              // quotes + key
    tv.GetGmtv();                                                               //line# text
    fprintf(fileP, "//File %s created %s.\n", objNameP,                         //
                                tv.Format(buf, sizeof(buf), "%d/%m/%4y@%h:%m"));//
    for (pc=0; pc < m_pgmSize; pc++)                                            //
        {op  = (bP = &m_codeP[pc])->op;                                         //
         if (pc == pcPlz)                                                       //
             pc = pc;                                                           //
         #ifdef _DEBUG                                                          //
          //_CrtCheckMemory();                                                  //
         #endif                                                                 //
         if (op.u16 == 0 && !bP->isDwB && badBongoes < 0)                       //0x0000 - most unlikely instruction 
            {badBongoes = pc; badLine = bP->ref.lineNum;}                           //
         if (bP->ref.lineNum == 0) {bP->ref.lineNum = lastLineNum; bP->ref.fileNum = fileN; bP->ref.srcOffset = lastOffset;}//
         if (fileN != bP->ref.fileNum && bP->ref.lineNum != 0)                          //
             fprintf(fileP, "//op //pc  label     : interpretation    line# and"//
                  " source. Source file=%s\n", FileNameOnly(fileN=bP->ref.fileNum));//
         else                                                                   //
         if (lastLineNum != bP->ref.lineNum) fprintf(fileP, "\n");                  //
         memset(buf, ' ', sizeof(buf)); buf[0] = buf[sizeof(buf)-1] = 0;        //
         gen "%04X //%03d ", op.u16, pc);                                       // 
         len = istrlen(buf);                                                    //
         if (bP->hereP) {gen "%s", bP->hereP);}                                 //
         buf[strlen(buf)] = ' '; buf[len+m_longestLabel] = 0;                   //nice if %*s worked eh ?
         if (bP->isDwB)                                                         //
            {gen ": DW 0x%04X", op.u16);                                        //
             if (lastWasLongJmpB) {gen " (address of %s)", labelP);}            //
             lastWasLongJmpB = false;                                           //
            }                                                                   //
         else                                                                   //
            {gen ": %s ", OPNAME(pc, labelP=bP->tgtLabelP, true));              // 
             lastWasLongJmpB = IsLongJmp(bP->op) && labelP != NULL;             //
            }                                                                   //
         buf[len=istrlen(buf)] = ' '; width = max(width, len+1);                //step ww(dont wipe out anything)
         snprintf(&buf[width], sizeof(buf)-width," line %d  ", bP->ref.lineNum);//
         buf[sizeof(buf)-1] = 0;                                                //bullet proofing  
         fwrite(buf, 1, strlen(buf), fileP);                                    //
         if (bP->ref.lineNum != lastLineNum)                                    //
            {for (qq=GetSourceLineP(bP), pp=&qq[strlen(qq)]; *(--pp)==' ';){}   //
             for (len=min(64, 1+(int)(pp-qq)); --len >= 0; qq++)                //
                {if (*qq == '\n')              fprintf(fileP, "\\n"); else      //
                 if (*qq == '\t')              fprintf(fileP, "\\t"); else      //
                 if (*qq < 0x20 || *qq > 0x80) fprintf(fileP, ".");   else      //
                                               fprintf(fileP, "%c", *qq);       //
            }   }                                                               //
         fprintf(fileP, "\n");                                                  //
         lastLineNum = bP->ref.lineNum; lastOffset = bP->ref.srcOffset;         //
         width = min(width,100);                                                //
        }                                                                       //
    fclose(fileP);                                                              //
    if (badBongoes >= 0)                                                        //
       {SNPRINTF(buf), "pc=%03d, line=%04d. For context refer to\n%s",          //
                            badBongoes, badLine, m_objFileP);                   //
        return _Error(ERR_9995, buf,"0x0000 is an unlikely Opcode",             //
                                              __FILE__, __LINE__, __FUNCTION__);//     
       }                                                                        //
    return 0;                                                                   //
    #undef gen                                                                  //
    } //cCompile::GenerateMicrocodeFile...

//mesages file is just a list of strings. SAM generates OP_PRINT.adr == msgNum
int cCompile::GenerateMsgFile(const char *msgFileP)
    {FILE *fileP = fopen(msgFileP, "wb");                                       //
     fwrite(m_opNameP->m_messagesP, m_opNameP->m_messageSize,1, fileP);//
     fclose(fileP);                                                             //
     return 0;                                                                  //
    } //cCompile::GenerateMsgFile...

//Line map comprises series of 6 byte entries:
//pc, file, line, offset. Duplicates are supressed.
int cCompile::GenerateLinemapFile(const char *mapFileP)
    {FILE *fileP=fopen(mapFileP, "wb");                                         //
     int   lastLine=-1, lastFile=-1;                                            //
     char  buf[50];                                                             //
     sSRC_REF ref;                                                              //
     if (m_bugEmitB)OutputDebugStringA("Line map (pc, file#, line#, offset)\n");//
     for (int pc=0; pc < m_pgmSize; pc++)                                       //
         {ref.fileNum   = m_codeP[pc].ref.fileNum;                              //
          ref.lineNum   = m_codeP[pc].ref.lineNum;                              //
          ref.srcOffset = m_codeP[pc].ref.srcOffset;                            //
          ref.pc        = pc;                                                   //
          if (lastLine != ref.lineNum || lastFile != ref.fileNum)               //
             {fwrite(&ref, 1,sizeof(ref), fileP);                               //
              if (m_bugEmitB)                                                   //
                 {SNPRINTF(buf), "%03d: %02d, %04d, %d\n",                      //
                               ref.pc, ref.fileNum, ref.lineNum, ref.srcOffset);//
                  OutputDebugStringA(buf);                                      //
             }   }                                                              //
          lastLine = ref.lineNum; lastFile = ref.fileNum;                       //
         }                                                                      //
     fclose(fileP);                                                             //
     return 0;                                                                  //
    } //cCompile::GenerateLinemapFile...

int cCompile::GenerateSymbolTable(const char *symbolTableFileP)
    {FILE *fileP=fopen(symbolTableFileP, "wb");
     char  buf[50], asciiZ[1]={0};
     if (m_bugEmitB) OutputDebugStringA("//Symbol table (symbol, address)\n");
     for (int pc=0; pc < m_pgmSize; pc++)
         {if (m_codeP[pc].hereP && m_codeP[pc].hereP[0] != '>')
             {fprintf(fileP, "%s\x000x%X\n", m_codeP[pc].hereP);
              fwrite(asciiZ, 1,1, fileP);
              fprintf(fileP, "0x%X\n", pc);
              if (m_bugEmitB) 
                 {SNPRINTF(buf), "%s,0x%X\n", m_codeP[pc].hereP, pc);
                  OutputDebugStringA(buf);
         }   }   }
     fclose(fileP);
     return 0;
    } //cCompile::GenerateSymbolTable...

//Patch line  parameter MICROCODE_SIZE = ###; 
//and   line  `define NAMEOF_MICROCODE 
//in samDefines.sv with actual values
//if !m_patchDrvB go thru the motions but do not actually patch anything.
int cCompile::PatchSamDefines(const char *microCodeNameP)
   {char       *bufP, fileName[_MAX_PATH], *sizeP, *codeP, buf[100];    //
    const char *whoMe="//patched by compile.exe      ";                 //
    int         sz, len, currentSize;                                   // (of microcode)
    bool        changesB=false;                                         //
    FILE       *fileP;                                                  //
                                                                        //
    SNPRINTF(fileName), "%s\\%s", getenv("verilogSourceDir"), "samDefines.sv");
    if ((sz=m_prepP->ReadAllFile(fileName, &bufP)) < 0) return sz;      //
//patcheroo                                                             //
    if ((sizeP=strstr(bufP, "parameter MICROCODE_SIZE")) == NULL)       //
       {free(bufP); return ErrorS(ERR_2720, fileName, sizeP);}          //2720 = Unable to find %s
    if ((codeP=strstr(bufP, "`define NAMEOF_MICROCODE")) == NULL)       //
       {free(bufP); return ErrorS(ERR_2720, fileName, sizeP);}          //2720 = Unable to find %s
    if (!m_patchDrvB) goto xit;                                         //bit late in the day eh ?
//really now, let do it                                                 //
    SNPRINTF(buf),  "parameter MICROCODE_SIZE   = %d; %s", m_pgmSize, whoMe);
    currentSize = (int)strtol(strchr(sizeP, '=')+1, NULL, 10);          //
    if (currentSize < m_pgmSize &&                                      //declared size is larger
        memcmp(sizeP, buf, (len=istrlen(buf))) != 0)                    //file is up-to-date
       {memmove(sizeP, buf, len); changesB = true;}                     //file needs an attitude adjustment
    SNPRINTF(buf),  "`define NAMEOF_MICROCODE \"%s\" %s",               //
                             FileNameOnly(microCodeNameP), whoMe);      //
    if (memcmp(codeP, buf, (len=istrlen(buf))) != 0)                    //file is up-to-date
       {memmove(codeP, buf, len); changesB = true;}                     //file needs an attitude adjustment
    if (changesB)                                                       //
       {fileP = fopen(fileName, "wb");                                  //           "
        fwrite(bufP, sz,1, fileP);                                      //           "
        fclose(fileP);                                                  //           "
       }                                                                //
xit:free(bufP);                                                         //           "
    return 0;                                                           //           "
   } //cCompile::PatchSamDefines...

//Wrapper for AssignReg to handle $reg:$reg and redirect $reg++ etc
int cCompile::RegAssignment(int pc, IATOM lhs, int act)
   {int        regRange= 0, baseReg=0, reg;                                     //
    bool       curRowB = (act == OP_CROW);                                      //
                                                                                //
    if (!curRowB && !IsRegister(lhs, &baseReg)) {m_a = lhs; goto err;}          //
    if (Is(':') && !curRowB)                                                    //
       {if (IsRegister(Get(), &regRange))                                       //
             {regRange = regRange - baseReg; Get();}                            //
        else {Backup(m_a); regRange = 0;}                                       //
       }                                                                        //
    reg = baseReg;
    if (IsChar('='))                                                            //
       {if ((pc=AssignReg(pc, lhs)) < 0 || curRowB) return pc;                  //error or $curRow = value
        if (iabs(regRange) >= 2)                                                //
           {//generate repeat $r<i> = $baseReg, ii == baseReg +/- 1             //
            pc   = GenerateOp(pc, OP_REPEAT, iabs(regRange), 0, STEPR);         //
            if (regRange < 0) m_codeP[pc-1].op.rpt.bkwd = 1;                    //
           }                                                                    //
        if (regRange!= 0)                                                       //
           {if (regRange < 0) reg = baseReg-1; else reg = baseReg+1;            //
            pc   = GenerateOp(pc, OP_ARITH, OPS_R2R, reg, baseReg);             //$base+i = $reg
           }                                                                    //
        return pc;                                                              //
       }                                                                        //
    if (regRange == 0 && m_a.type == GC_GETX)return RegisterPostOp(pc, lhs,m_a);//
err:return ErrorA(ERR_2736, m_a);                                               //2736 = Invalid register
   } //cCompile::RegAssignment...

/*$<reg> = expression; assign value to register. 
 RHS may be another register (OPS_R2R), a literal, or a memory reference [adr].
 Destination register may also be $curRow (in which case curRowB == true).
 Variants of $reg = literal
     $reg = small literal  generates OPS_R2R with op.sreg = op.dreg, op.adr = literal
     $0   = large literal  uses InsertLongLiteral to generate multiple OP_LDI's
     $reg = large literal  if $reg == 0 generate $0 = InsertLongLiteral()
                           otherwise generate xchg $reg, $0 = InsertLongLiteral(), xchg $reg, $0
 Variants of memory reference:
     $reg = [$reg]         generates OP_RDF
     $reg = [adr]          generates OP_READ
     $reg = [adr:adr]      generates OP_REPEAT, OP_READ
     $reg = [row@adr]      generates OPS_R2R(literal), OP_READ
     $reg = [row@adr:adr]  generates OPS_R2R(literal), OP_REPEAT, OP_READ 
   $curRow= [$reg | literal] if (curRowB==true)
 Upon entry: bP->op.u16 = OP_ARITH+OPS_R2R; meaning that op.sreg == op.adr == 0 */
int cCompile::AssignReg(int pc, IATOM dstnAtom)
   {int         dstnReg=0, sReg=-1, fieldNum, vbl;                      //
    sCODE_BLOB *bP=&m_codeP[pc];                                        //
    bool        curRowB=bP->op.u16 == OP_CROW;                          //dstn reg is $curRow
                                                                        //
    m_stepableB = false;                                                //
    if (!curRowB && !IsRegister(dstnAtom, &dstnReg)) goto err;          //LHS = $curRow or dReg
    //Now process RHS                                                   //
    if (Is("$pc"))                                                      //generates $reg = long literal (always)
       {GenerateOp(pc,   OP_RI,  (pc+2) >> 14, dstnReg, dstnReg);       //bits[22:14]
        GenerateOp(pc+1, OP_LDI, (pc+2) & ((2<<14)-1));                 //bits[13:0]
        m_codeP[pc].isPcB = m_codeP[pc].isCodeAdrB = true;              //OP_LDI is a code reference
        return pc+2;                                                    //
       }                                                                //
    if (IsRegister(m_a, &sReg))                                         //
       {//= $reg                                                        //
        if (curRowB) return GenerateOp(pc, OP_CROW, sReg);              //$curRow = $sreg
        return GenerateOp(pc, OP_ARITH, OPS_R2R, dstnReg, sReg);        //
      //bP->op.r2r.areg = sReg; bP->op.r2r.breg = dstnReg; return pc+1; //$breg   = $areg
       }                                                                //
    else                                                                //
    if (IsChar('['))                                                    //$reg = [adr] (memory reference)
       {if (curRowB) goto err;                                          //$curRow = [memory] is a not allowed
        if (IsRegister(Get(), &sReg))                                   //$reg = [$reg]
           {//= [$reg]                                                  //
            bP->op.u16 = OP_RDF;                                        //
            bP->op.ind.breg = dstnReg; bP->op.ind.areg = sReg;          //
       //   if ((erC=SignedOffset(&bP->op)) < 0) return erC;            //obsolete; was [$reg+offset]
            if (!Is(']')) goto err;                                     //
            if (!Is('.')) {Backup(m_a); fieldNum = FLD_RAW;} else       //
            if ((fieldNum=FieldName(Get())) < 0) return fieldNum;       //
            bP->op.ind.fieldNum = fieldNum;                             // 
          //pc++;                                                       //MultiOp() will increment pc
           }                                                            //
        else                                                            //
           {//= [adr]                                                   //
            bP->op.u16   = OP_READ;                                     //replace OP_ARITH with OP_READ
            bP->op.g.breg= dstnReg;                                     //
            if ((pc=Address(pc)) < 0) return pc;                        //
            m_stepableB  = true;                                        //
            if (!Is(']')) goto err;                                     //
       }   }                                                            //
    else
    if (m_a.type == GC_NAME && (vbl=LookupVbl(m_a.textP, m_a.len)) >= 0)//
       {m_loAdr = m_hiAdr = (int) m_vblsP[vbl].wordAdr;                 //
        bP->op.u16   = OP_READ;                                         //replace OP_ARITH with OP_READ
        bP->op.g.breg= dstnReg;                                         //
        bP->op.g.adr = m_loAdr;                                         //
       }                                                                //
    else {//= literal                                                   //
          Backup(m_a);                                                  //
          pc = BuildLiteral(pc, curRowB, dstnReg) - 1;                  //MultiOp will step pc
          m_stepableB = true;                                           //
         }                                                              //
    return MultiOp(pc, bP->op);                                         //
err:return ErrorA(ERR_2736, m_a);                                       //2736 = Expecting a register. 
   } //cCompile::AssignReg...

/*Store to memory operations.
  expression                       code generated
 ---------------------------+---------------------------------------------------------
 [adr]           = $regData;| op.g.act = OP_WRYT, op.g.adr = adr, op.g.sreg = $regData
 [adr]           = literal; | $0       = literal, then [adr] = $0
 [$regAdr]       = $regData;| op.ind.act = OP_WRF, op.ind.dataReg = $regAdr (address), 
                            | op.ind.regD = $regD (data), op.ind.field = 0
 [$regAdr].field = $regData;| op.ind.act = OP_WRF, op.ind.dataReg = $regA (address), 
                            | op.ind.regD = $regD (data), op.ind.field = field#
 [$regAdr].field = literal; | $0       = literal, then [$regA] = $0
 [row@adr]       = ...      | $curRow = row, the [adr] = ...
 [lo:hi]         = $regData | op.rpt.count = (hi-lo-1) (meaning (1+hi-lo) repititions)
                            | followed by op.g.act = OP_WRYT, op.g.sreg = reg. 
                            | Both the register and op.g.adr are incremented on 
                            | each repitition (m_stepableB = false).   
                            | op.rpt.bkwd = hi < lo and the adr/reg are decremented
 [lo:hi]         = literal  | $0 = literal, op.rpt.count as above, however, 
                            | op.rpt.stepR == 0, op.rpt.stepA == 1 meaning only the 
                            | address field is incremented/decremented (m_stepableB = true).
 vbl < 0 expression was [...] =
 vbl > 0 expression was vbl   = 
*/
int cCompile::AssignMem(int pc, int vbl)
    {int        adrReg, dataReg, fieldNum;                                      //
     sCODE_BLOB blob = m_codeP[pc];                                             //
     OPCODE     op   = blob.op;                                                 //
     bool       indirectB=false;                                                //
     m_stepableB = false;                                                       //
     if (vbl < 0 && IsRegister(Get(), &adrReg))                                 //
        {//[$adrReg] = rhs; or [$adrReg].field = rhs;                           //
         op.u16 = OP_WRF; op.ind.areg = adrReg; indirectB = true;               //
       //if ((erC=SignedOffset(&bP->op)) < 0) return erC;                       //obsolete; was [$reg +/- literal]
         if (!Is(']'))                        return ErrorA(ERR_3005, m_a);     //3005 = missing ']' or '}'
         if (!Is('.')) {Backup(m_a); fieldNum = 0;} else                        // 
         if ((fieldNum=FieldName(Get())) < 0) return fieldNum;                  //
         if (!(m_safeRegB=Is('!'))) Backup(m_a);                               //
         op.ind.fieldNum = fieldNum; blob.op = op; m_codeP[pc] = blob;          // 
        }                                                                       //
     else                                                                       //
        {op.u16 = OP_WRYT; blob.op = op; m_codeP[pc] = blob; m_stepableB= true; //
         if (vbl >= 0) 
            {op.g.adr = m_loAdr = m_hiAdr = (int)m_vblsP[vbl].wordAdr;} //row override ??
         else
            {if (((pc=Address(pc)) < 0)) return pc;                             //
             op.g.adr = m_loAdr;                                                //write address
             if (!Is(']'))               return ErrorA(ERR_3005, m_a);          //3005 = Missing ']'
        }   }                                                                   //
     if (!(m_safeRegB=Is('!'))) Backup(m_a);                                   //
     if (!Is('='))                       return ErrorA(ERR_2729, m_a);          //2729 = missing '='	
     if (IsRegister(Get(), &dataReg))                                           //
        {if (indirectB)                                                         //[$reg] = $reg;
//            {op.ind.dataReg = op.ind.adrReg; op.ind.adrReg = dataReg;         //
              {op.ind.breg = dataReg;                                           //
               m_post = 1;                                                      //post 1 merely pc++; 
              }                                                                 //
         else {op.g.breg = dataReg; op.g.adr = m_loAdr;}                        //[m] = $reg, or [m:n] = $reg;
         m_stepableB = false;                                                   //
        }                                                                       //
     else                                                                       //
        {op          = m_codeP[pc].op;                                          //save partial opcode
         Backup(m_a);
         pc          = LongStringOrLiteral(pc, m_a);                            //
         m_stepableB = true;                                                    // 
        }                                                                       // 
    blob.op = op; m_codeP[pc] = blob;                                           // 
    return MultiOp(pc, op);                                                     //MultoOp increments pc
   } //cCompile::AssignMem...

//Emit code to assign value to register. 
//Upon entry m_codeP[pc] contains the basic opcode
int cCompile::BuildLiteral(int pc, bool curRowB, int dReg)
   {int         erC; 
    int64_t     i64;
    sCODE_BLOB *bP=&m_codeP[pc], blob;                                  //
    if ((erC=ConstantExpression(&i64)) < 0) return erC;                 //$dreg = literal
    if (i64 >= 0 && i64 <= m_max.g.adr)                                 //
       {if (curRowB) return GenerateOp(pc, OP_CROWI, (int)i64);         //replace partial opcode
        bP->op.u16     = OP_RI;                                         //
        bP->op.ri.imm  = i64;                                           //dreg = small literal
        bP->op.ri.breg = dReg;                                          //
        return pc+1;                                                    //
       }                                                                //
    if (dReg != 0)                                                      //must be $reg0 for long literal
       {bP->op.arith.act   = OP_ARITH;                                  //fabricate xchg $0, $dreg
        bP->op.arith.subOp = OPS_XCHG;                                  //
        bP->op.arith.areg  = dReg;                                      //arith.areg == 0 from op.u16 = otP->act
        blob               = *bP++;                                     //
        if ((pc=InsertLongLiteral(++pc, i64)) < 0) return pc;          //$0 = long literal
        blob.tgtLabelP     = blob.hereP = NULL;                         //
        m_codeP[pc]        = blob;                                      //output xchg $0, $dreg
        return pc+1;                                                    //voila - load literal to other than $0
       }                                                                //
    if (i64 <= m_max.g.adr && curRowB)                                  //
                          return GenerateOp(pc, OP_CROWI, (int)i64);    //
    if ((pc=InsertLongLiteral(pc, i64)) < 0) return pc;                //$dreg0 = long literal
    if (!curRowB) return pc;                                            //
    return GenerateOp(pc, OP_CROW, dReg);                               //
   } //cCompile::BuildLiteral...

//register post op: 
//  $reg++, $reg--, reg+=rhs, reg-=rhs, reg&=rhs, reg |=rhs, reg^=rhs
int cCompile::RegisterPostOp(int pc, IATOM aa, IATOM postOp)
   {int         reg, ii;                                                //
    uint32_t    rhs=1;                                                  //
    const char *pp=postOp.textP;                                        //
    static const char *o[] = //2       3        4       5       6       //
             {"++",   "--",   "+=",   "-=",    "&=",   "|=",  "^="};    //valid post ops
    int u[] ={OPS_INC,OPS_DEC,OPS_INC,OPS_DEC, OPS_AND,OPS_OR,OPS_XOR}; //  unary ops
    int b[] ={OPS_ADD,OPS_SUB,OPS_ADD,OPS_XSUB,OPS_AND,OPS_OR,OPS_XOR}; //  binary ops
                                                                        //
    if (postOp.len != 2 || !IsRegister(aa, &reg)) goto err;             //
    for(ii=HOWMANY(o); --ii >= 0;) if(strnicmp(o[ii], pp, 2) == 0)break;//
    if (ii < 0) goto err;                                               //
    if (ii < 2) //++ and -- variants -----------------------------------//
       {//if (reg == REG0) return GenerateOp(pc, OP_ARITH, u[ii], 0,0);   // $0 = rhs
        return GenerateOp(pc, OP_ARITH, u[ii], reg, reg);               // $reg++, $reg--
       }                                                                //
    if (!IsNumber(&rhs)) goto err;                                      //
    if (ii >= 4) //&=, |=, and ^= variants------------------------------//
       {if (reg == REG0)                                                //
            {pc   = GenerateOp(pc, OP_ARITH, OPS_PUSH, REG1);           //push $1
             if (rhs < m_max.g.adr)                                     //
                 pc = GenerateOp(pc, OP_RI, rhs, REG1);                 //$1 = rhs (where rhs < 256)
             else                                                       //
                {pc = GenerateOp(pc, OP_ARITH, OPS_R2R, REG1, REG0);    //$1=$0
                 pc = IntegerLiteral(pc, rhs, REG0);                    //$0 = longLiteral
                }                                                       //
             pc   = GenerateOp(pc, OP_ARITH, b[ii], REG1, REG0);        //$0 = $0 <op> $1
             return GenerateOp(pc, OP_ARITH, OPS_POP, REG1);            //pop $1
            }                                                           //
      //if (reg != REG0)                                                //
            {pc   = GenerateOp(pc, OP_ARITH, OPS_PUSH, REG0);           //push $0
             pc   = IntegerLiteral(pc, rhs, REG0);                      //$0 = literal (short or long)
             pc   = GenerateOp(pc, OP_ARITH, b[ii], REG0, reg);         //$reg = $0 <op> $reg
             return GenerateOp(pc, OP_ARITH, OPS_POP, REG0);            //pop $0
       }    }                                                           //
    //+=, and -= variants ----------------------------------------------//
    switch(rhs)                                                         //
       {case 3: pc =   GenerateOp(pc, OP_ARITH, u[ii], reg, reg);       //inc/dec $reg
        case 2: pc =   GenerateOp(pc, OP_ARITH, u[ii], reg, reg);       //    "
        case 1: return GenerateOp(pc, OP_ARITH, u[ii], reg, reg);       //    "
        case 4: case 5: case 6:                                         //
                pc =   GenerateOp(pc, OP_REPEAT,rhs, 0,0);              // 0,0 = no register/address stepping
                return GenerateOp(pc, OP_ARITH, u[ii], reg, reg);       //inc/dec $reg 
        default:if (reg == REG0)                                        //
                   {pc   = GenerateOp(pc, OP_ARITH, OPS_PUSH,REG1,REG1);//push $1
                    pc   = GenerateOp(pc, OP_ARITH, OPS_R2R, REG1,REG0);//$1=$0
                    pc   = IntegerLiteral(pc, rhs, REG0);               //$reg0 = literal (short or long)
                    pc   = GenerateOp(pc, OP_ARITH, b[ii], REG1, REG0); //$0 = $0 <op> $reg
                    return GenerateOp(pc, OP_ARITH, OPS_POP, REG1);     //pop $1
                   }                                                    //
              //if (reg != REG0)                                        //
                   {pc   = GenerateOp(pc, OP_ARITH, OPS_PUSH, REG0);    //push $0
                    pc   = IntegerLiteral(pc, rhs, REG0);               //$reg0 = literal
                    if (ii == 3) //-=                                   //operands in reverse order: ha ha
                       {pc=GenerateOp(pc, OP_ARITH, b[ii], reg, REG0);  //$0 = $0 <op> $reg
                        pc=GenerateOp(pc, OP_ARITH, OPS_R2R, reg, REG0);//$reg = $0 <op> $reg
                       }
                    else pc=GenerateOp(pc,OP_ARITH, b[ii], REG0, reg);  //$reg = $0 <op> $reg
                    return GenerateOp(pc, OP_ARITH, OPS_POP, REG0);     //pop $0
                   }                                                    //
       }                                                                //
err:return Error(ERR_2736, "");                                         //2736 = register expected 
   } //cCompile::RegisterPostOp...

//if (conditionName), eg if (C) or if (~C), etc
int cCompile::SimpleCondition(IATOM aa) 
    {int  ii=0;                                                         //
     char word[25]={0}, *wordP;                                         // z, c, qrdy, tgt.. what else ?
     if ((aa.textP[0] == '!' || aa.textP[0] == '~') && aa.len == 1)     //invert sense of condition
        {word[0] = '~'; ii = 1; aa = Get();                             //
         if (aa.type != GC_NAME && aa.type != GC_INT) return -ERR_2733; //2733 = Invalid element in an expression (%s).
         strncpy(&word[ii], m_a.textP, min(m_a.len, sizeof(word)-2));   //
         wordP = word;                                                  //   
        }                                                               //
     else wordP = LOCAL_COPY(aa);                                       //
     for (ii=0; ii < HOWMANY(Relops); ii++)                             //must search in forward direction <<<<
         {if (stricmp(wordP, Relops[ii].txtP)  == 0) return ii;         //
          if (stricmp(wordP, Relops[ii].wordP) == 0) return ii;         //
         }                                                              //
     return -ERR_2733;                                                  //2733 = Invalid element in an expression (%s).
    } //cCompile::SimpleCondition...

//CompileExpression generates the code for the comparison in a for statement.
//The resulting code stream includes some unresolved OP_GO's (ie op.go.relAdr == 0). 
//These must be completed by the caller when the actual addresses are known. 
//The sense of these generated goto's is: if (expression == true) goto.
int cCompile::CompileExpression(int pc, bool safeReg0B)
   {int rel, reg, erC, startPc=pc; int64_t result;                              //
    if (!IsRegister(Get(), &reg))                goto err2733;                  //
    if ((rel=SimpleCondition(Get()))      < 0)   goto err2733;                  //rel = relational operator
    if ((erC=ConstantExpression(&result)) < 0)   return erC;                    //
    if (safeReg0B) pc = GenerateOp(pc, OP_ARITH, OPS_PUSH, 0);                  //save $reg0
    pc      = IntegerLiteral(pc, result, 0);                                    //result in $reg0
    pc      = GenerateOp(pc, OP_ARITH, OPS_CMP, reg, 0);                        //
    if (safeReg0B) pc = GenerateOp(pc, OP_ARITH, OPS_POP,  0);                  //restore $reg0
    pc      = GenerateOp(pc, Relops[rel].op, Relops[rel].cond);                 //conditional goto: address unknown
    if (m_bugEmitB) BugEmit(startPc, pc-1);                                     //
    return pc;                                                                  //
err2733: return ErrorA(ERR_2733, m_a);                                          //2733 = Invalid element in an expression (%s).
   } //cCompile::CompileExpression...

/*Set test environment
    $environment software emulation;   0   emulated in software only
    $environment Xilinx simulation;    1   calls xsim.exe; sim.exe monitors results
    $environment FPGA;                 2   load FPGA and monitor execution
    $environment hardware;             3   load hardware 
    $environment compile only;         4   emulated in software only
    $environment macro only;           5   expand macros only
Only one instance of an $environment statement is expected
$environment is ignored when g_skipXsimB=true (a cranky debugging trick) 
to force re-analysis of simulate.log. */
int cCompile::SetEnvironment(void)
   {char *n1P, *n2P; eENVIRONMENT env;                                          //
    #define cmp(w,t)  stricmp(w, t) == 0                                        //
    if (Get().type != GC_NAME)                return Error(ERR_3000, "");       //missing name
    n1P = LOCAL_COPY(m_a);                                                      //
    if (cmp(n1P, "hardware")                             )env = ENV_HW;     else//
    if (cmp(n1P, "FPGA")                                 )env = ENV_FPGA;   else//
       {Get(); n2P = LOCAL_COPY(m_a);                                           //not 'LOCAL_COPY(Get())' please
        if (cmp(n1P, "Xilinx")  && cmp(n2P, "simulation"))env = ENV_XSIM;   else//rest are twofers
        if (cmp(n1P, "compile") && cmp(n2P, "only")      )env = ENV_COMPILE;else//
        if (cmp(n1P, "software")&& cmp(n2P, "emulation") )env = ENV_SW;     else//
                                              return Error(ERR_7176, "");       //invalid environment setting
       }                                                                        //
    if (!Is(';'))                             return ErrorA(ERR_3003, m_a);     //missing ';'
    m_environment = env;                                                        //
    return (int)env;                                                            //
   } //cCompile::SetEnvironment...

//Generate a push/pop around emerging opcodes to safeguard $reg
int cCompile::SafeReg(int pc, int pushPop, int reg)
    {if (m_safeRegB) 
        pc = GenerateOp(pc, OP_ARITH, pushPop, reg); 
     return pc;
    } //cCompile::SafeReg...

/*Generate a long literal, ie one larger than the 8 bits allocated in OPS_R2R
 or 14 bits in OP_LDI. With targetBus_sz == 8 * cell_sz == 8 == 64-bit word
 this can be acheived with:
 OPS_R2R $0,hi-bits; followed by 4 * OP_LDI (loading 14 bits each). 
 The OPS_R2R clears the high order bits of $reg0, so that:
  1-8  bit literals requires one OPS_R2R
  9-22 bit literals require  one OPS_R2R and one   OP_LDI
 23-37 bit literals require  one OPS_R2R and two   OP_LDI's
 38-52 bit literals require  one OPS_R2R and three OP_LDI's
 53-64 bit literals require  one OPS_R2R and four  OP_LDI's	
*/
int cCompile::LongStringOrLiteral(int pc, const IATOM aa)
   {int         erC;                                                            //
    int64_t     result;                                                         //
    if (aa.type == GC_STRING)                                                   //
       {if (aa.len > TARGETBUS_SIZE) return ErrorA(ERR_2732, aa);               //
        result = *(uint64_t*)aa.textP >> (8*(aa.len - TARGETBUS_SIZE));         //
        Get();                                                                  //
       }                                                                        //
    else                                                                        //
    if ((erC=ConstantExpression(&result)) < 0) return erC;                      //
    return InsertLongLiteral(pc, result);
   } //cCompile::LongStringOrLiteral...

int cCompile::IntegerLiteral(int pc, uint64_t u64, int regN)
   {OPCODE *opP=&m_codeP[pc].op;                                                //
    if (u64 >= 256)                                                             //
       {if (regN != 0 ) pc = GenerateOp(pc, OP_ARITH, OPS_XCHG, regN, 0);       //
        pc = InsertLongLiteral(pc, u64);                                        //
        if (regN != 0 ) pc = GenerateOp(pc, OP_ARITH, OPS_XCHG, regN, 0);       //
        return pc;                                                              //
       }                                                                        //
    opP->u16             = OP_RI;                                               //sreg = dreg means $reg = literal
    opP->ri.breg         = regN;                                                //
    opP->ri.imm          = (uint16_t)u64;                                       //
    m_codeP[pc].ref.lineNum  = m_ref.lineNum;                                   //
    m_codeP[pc].ref.fileNum  = m_ref.fileNum;                                   //
    m_codeP[pc].ref.srcOffset= m_ref.srcOffset;                                 //
    return pc+1;                                                                //
   } //cCompile::IntegerLiteral...

//Generate upto 64-bit literal in $0. First OP_RI loads & zero fills $0; 
//then up to 4*OP_LDI stuff in another 56 bits.
//This is one gawd awful piece of code.
int cCompile::InsertLongLiteral(int pc, uint64_t u64)
   {int         ii, cc;                                                         //
    uint64_t    a[5], one=1, mask=0x3FFF, b[5];                                 //
    sCODE_BLOB  blob=m_codeP[pc];                                               //capture line, file and label
    b[4] = (u64 >>  0) & mask; b[3] = (u64 >> 14) & mask;                       //break up u64 into 14 bit pieces
    b[2] = (u64 >> 28) & mask; b[1] = (u64 >> 42) & mask;                       //
    b[0] = (u64 >> 56) & 0xFF;                                                  //high 8 bits
    if (u64 >= 256 && b[1] == b[2] && b[3] == b[2] && b[4] == b[2])             //trivial optimization
       {pc          = GenerateOp(pc, OP_RI, (int)b[0]);                         //  (if 14 bit chunks are identical)
        pc          = GenerateOp(pc, OP_REPEAT, 4);                                //
        pc          = GenerateOp(pc, OP_LDI, (int)b[1]);                        //
       }                                                                        //
    else
      {if (u64 < (one << ( 8)))   {a[0] = u64;                                                      cc = 0;} else
       if (u64 < (one << (14)))   {a[0] = 0;    a[1] = b[4];                                        cc = 1;} else
       if (u64 < (one << (14+8))) {a[0] = b[3]; a[1] = b[4];                                        cc = 1;} else
       if (u64 < (one << (28+8))) {a[0] = b[2]; a[1] = b[3]; a[2] = b[4];                           cc = 2;} else
       if (u64 < (one << (42+8))) {a[0] = b[1]; a[1] = b[2]; a[2] = b[3]; a[3] = b[4];              cc = 3;} else
                                  {a[0] = b[0]; a[1] = b[1]; a[2] = b[2]; a[3] = b[3]; a[4] = b[4]; cc = 4;}     
       pc = GenerateOp(pc, OP_RI, a[0] & 0xFF, REG0);                           //
       blob.tgtLabelP = blob.hereP = NULL;                                      //no more labels after first op
       for (ii=1; ii <= cc; ii++)                                               //no more labels after first op
          {m_codeP[pc] = blob; pc = GenerateOp(pc, OP_LDI,(int)a[ii], REG0);}   //
      }                                                                         //
    m_codeP[pc] = blob;                                                         //original opcode stripped of label
    return pc;                                                                  // 
   } //cCompile::InsertLongLiteral...

//ConstantExpression resolves to an integer value at compile time.
//No code is generated by this routine (in contrast to CompileExpression()), 
//but the full complexity of an expression is processed.
int cCompile::ConstantExpression(int64_t *resultP)
   {int     maxList=0, ii, paren, bracket, curly;                               //
    char   *pp;                                                                 //
    IATOM  *aListP = NULL, aa;                                                  //
    for (ii=paren=bracket=curly=0; ; ii++)                                      //scan to end of expression
       {if (Get().len == 1)                                                     // 
          switch(m_a.textP[0])                                                  //
           {case ';': case ':': case ',': goto dun;                             //ie. terminator == ;:])[{ or @
            case ')':if (paren--   <= 0)  goto dun;                  break;     //               "
            case ']':if (bracket-- <= 0)  goto dun;                  break;     //               "
            case '}':if (curly--   <= 0)  goto dun;                  break;     //               "
            case '(': paren++;                                       break;     //               "
            case '[': bracket++;                                     break;     //               "
            case '{': curly++;                                       break;     //               "
            case '@': if ((paren+bracket+curly) != 0) goto err;      goto dun;  //provoke error
           } //switch...                                                        //
        if (ii >= maxList)                                                      //
                 aListP = (IATOM*)realloc(aListP, (maxList+=16)*sizeof(IATOM)); //
        aListP[ii] = m_a;                                                       //
       }                                                                        //
dun:Backup(m_a);                                                                //
    if (ii == 0) goto err;                                                      //nuthing found
    if (ii == 1 && (aa=aListP[0]).type == GC_INT)                               //most common case
       {*resultP = Anumber(aa.textP, &pp);                                      //
        return ((int)(pp-aa.textP) < aa.len) ? ErrorA(ERR_2733, aa) : 0;        //2733 = Invalid element in an expression 
       }                                                                        //
    return EvaluateExpr(aListP, 0, ii-1, resultP);                              //
err:return ErrorA(ERR_2733, m_a);           //2733 = Invalid element in an expression 
   } //cCompile::ConstantExpression...

int cCompile::EvaluateExpr(IATOM *aListP, int minInx, int maxInx, int64_t *resultP)
   {char        *pp, ch;                                                        //
    IATOM        aa;                                                            //
    uint16_t     pair;                                                          //
    int          ii, kk, lparen, si, prec, pi, erC=0;                           //
    int64_t     *stakP=NULL;                                                    //
    bool         operatorNxt, invert=false;                                     //
    const char  *errTextP="", *atomP;                                           //
    struct OPERATORS {int prec, op;} pStak[PRECEDENCE_LEVELS];                  //table of deferred operators
                                                                                //
    if (minInx > maxInx) return Error(ERR_1033, "");                            //nothing in expresion. 1033=Syntax error in #directive
    if ((stakP=(int64_t*)calloc(maxInx-minInx+1, sizeof(int64_t))) == NULL)     //run time stack (gross over-allocation)
        return Error(ERR_0005, "");                                             //0005 = Not enough memory (%s)
    pStak[0].prec = si = 0; pi = 1;                                             //initialize precedence stack
    aa        = aListP[minInx];                                                 //
    ch        = aa.textP[0];                                                    //
    if (aa.type == GC_GETX && (ch == '!' || ch == '~') && aa.len == 1)          //    
       {stakP[si++] = 1; pStak[pi].prec = 1; minInx++; pStak[pi++].op = ch;}    //simulate binary operator
    for (operatorNxt=false; minInx <= maxInx; minInx++)                         //
        {aa    = aListP[minInx];                                                //
         atomP = aa.textP;                                                      //
         if ((pair=aa.type) == GC_GETX)                                         //
                pair = aa.len == 2 ? atomP[0] + (atomP[1] << 8) : atomP[0];     //
         switch (pair)                                                          //
            {default: err2733: free(stakP); return Error(ERR_2733, atomP);      //2733 = Invalid element in an expression 
             case '(': if (operatorNxt) goto err2733;                           //2733 = Invalid element in an expression 
                       for (lparen=0, ii=minInx, kk=-1; ii <= maxInx; ii++)     //scan to matching )
                           {if ((aa=aListP[ii]).type == GC_GETX && aa.len == 1) //
                               {ch = aa.textP[0];                               //  
                                if (ch == '(') {if (lparen++ == 0) kk = ii;}else//save locn of opening '('
                                if (ch == ')') {if (--lparen == 0) break;}  else//found closing parens ')'
                                if (aa.type == GC_NULL) break;                  //
                           }   }                                                //
                       if (lparen != 0) {erC = ERR_7184; goto err;}             //7184 = missing close parenthesis
                       if (erC=EvaluateExpr(aListP, kk+1, ii-1, &stakP[si++]) < 0) goto err;
                       minInx = ii; goto xItem;                                 //skip over items between ()
             case ')': erC = ERR_7184; goto err;                                //7184 = unbalanced parenthesis
             case GC_INT:                                                       //
                       if (operatorNxt) goto err2733;                           //2733 = Invalid element in an expression 
                       stakP[si++] = Anumber(aa.textP, &pp);                    //
                       if ((int)(pp-aa.textP) < aa.len)                         //trash after digits
                          {free(stakP); return Error(ERR_1002, atomP);}         //1002 = invalid number
             xItem:    operatorNxt = true;    continue;                         //
             case '!': invert      = !invert; continue;                         //
             case '||':case '|':                                                //
             case '&&':case '&':                                                //
             case '==':case '=':                                                //
             case '<<':case '>>':                                               //
             case '>': case '<':                                                //
             case '=>':case '=<':                                               //
             case '=!':case '><':                                               //
             case '+': case '-':                                                //
             case '*': case '/':                                                //
             case '%': case '%%':                                               //
             case '**':if (!operatorNxt) goto err2733;                          //2733 = Invalid element in an expression 
                       prec = Precedence(pair);                                 //
                       while (pStak[pi-1].prec >= prec)                         //
                             {si--;                                             //
                              if ((erC=Evaluate(&stakP[si-1], pStak[--pi].op, stakP[si-1], stakP[si])) < 0) goto err;
                             }                                                  //
                       pStak[pi].prec = prec; pStak[pi++].op = pair;            //
                       operatorNxt    = false; continue;                        //
            } //switch sw                                                       //
        } //for (minInx=...                                                     //
     while (--pi > 0)
          {si--; 
           if ((erC=Evaluate(&stakP[si-1], pStak[pi].op, stakP[si-1], stakP[si])) < 0) goto err;
          }
    *resultP = stakP[0];
    if (si != 1) erC = ERR_1033; //something is wrong. Stack should contain one item
err:free(stakP);
    return (erC == 0) ? 0 : ErrorS(erC, errTextP, "");
   } //cCompile::EvaluateExpr...

int cCompile::Precedence(uint32_t op)
   {switch (op)
      {case '=': case '||':                     return 1;
       case '&&':                               return 2;
       case '|':                                return 3;
       case '^':                                return 4;
       case '&':                                return 5;
       case '~':                                return 5;
       case '=!':case '==':                     return 6;
       case '>': case '<': case '=>':case '=<': return 7;
       case '>>':case '<<':                     return 8;
       case '+': case '-':                      return 9;
       case '*': case '/': case '%': case '%%': return 10;
       case '**':                               return 11;
      }
    return 0;
   } //cCompile::Precedence...

//Do a small set of string expressions
int cCompile::EvaluateString(int op, char *leftP, char *riteP)
   {switch (op)
       {default:              return ErrorS(ERR_3031, leftP, riteP);   //Impermissible string operation
        case '==': case '=':  return stricmp(leftP, riteP) == 0;       //equality              (ignoring case)
        case '>':             return stricmp(leftP, riteP) >  0;       //greater than          (ignoring case)
        case '<':             return stricmp(leftP, riteP) <  0;       //less than             (ignoring case)
        case '=>':            return stricmp(leftP, riteP) >= 0;       //greater than or equal (ignoring case)
        case '=<':            return stricmp(leftP, riteP) <= 0;       //less    than or equal (ignoring case)
        case '=!': case '><': return stricmp(leftP, riteP) != 0;       //not equal             (ignoring case)
   }   } //cCompile::EvaluateString...

//Doi a select set of arithmetic operations
int cCompile::Evaluate(int64_t *resultP, int op, int64_t left, int64_t rite)
   {int64_t u64; char opp[3];
    switch (op)
       {case '+':             *resultP = left +  rite; return 0;
        case '-':             *resultP = left -  rite; return 0;
        case '*':             *resultP = left *  rite; return 0;
        case '%':             *resultP = rite==0 ? 0 : left % rite; return 0;
        case '/':             *resultP = rite==0 ? 0 : left / rite; return 0;
        case '==': case '=':  *resultP = left == rite; return 0;
        case '<<':            *resultP = left << rite; return 0;
        case '>>':            *resultP = left >> rite; return 0;
        case '>':             *resultP = left  > rite; return 0;
        case '<':             *resultP = left  < rite; return 0;
        case '=>':            *resultP = left >= rite; return 0;
        case '=<':            *resultP = left <= rite; return 0;
        case '=!': case '><': *resultP = left != rite; return 0;
        case '&':  case '&&': *resultP = left && rite; return 0;
        case '|':  case '||': *resultP = left || rite; return 0;
        case '^':             *resultP = left ^ rite;  return 0;
        case '!':             *resultP = !rite;        return 0;
        case '~':             *resultP = ~rite;        return 0;
        case '**':            for (u64=left; --rite > 0;) u64 *= left; 
                                       *resultP = u64; return 0;
       }
    *(uint16_t*)&opp = op; opp[2] = 0;
    return Error(ERR_7183, opp); //ConstantExpression has invalid syntax.
   } //cCompile::Evaluate...

int cCompile::_ErrorN(int erC, uint32_t p1, CC fileNameP, int line, const char *fncP)
   {char b1[20];
    SNPRINTF(b1), "%d", p1);
    return _Error(erC, "", b1, fileNameP, line, fncP);
   } //cCompile::_ErrorN..

//erC == 0 is a call to just publish an error that has already been logged.
int cCompile::_Error(int erC, CC contextP, CC paramsP, CC fileNameP, int line, const char *fncP)
   {char erBuf[MAX_ERROR_MESSAGE_SIZE+256], fn[_MAX_PATH+10]; int len;          //
    if (erC == 0) erC = g_err.GetLastError();                                   //
    else          erC = g_err.LogError(erC, contextP, paramsP);                 //
    SNPRINTF(fn), "\n%s #%04d, function=%s", (char*)fileNameP, line, fncP);     //
    if (true/*m_alwaysMessageBoxB*/ || g_err.Severity(erC) == XSEVERITY_MESSAGEBOX)     //
       {g_err.ShortError(erC, erBuf, sizeof(erBuf));                            //
        len = istrlen(erBuf);                                                   //
        Printf("%s\x07", erBuf);                                                //beep :(
        strncat(erBuf,"\n\tPress Yes for more information\n"                    //
                        "\tPress No  to ignore error and continue\n"            //
                        "\tPress Cancel to abort further processing",           //
               sizeof(erBuf)-len-1);                                            //
        switch (MessageBoxA(NULL, erBuf, fn,MB_YESNOCANCEL))                    //
             {case IDNO:     return 0;                                          //
              case IDCANCEL: return erC;                                        //
             }                                                                  //
        erBuf[len] = 0;                                                         //strip 'Press yes...'
        g_err.AddContext(fn);                                                   //
        g_err.FullError(erC, erBuf, sizeof(erBuf));                             //
        Printf("\n%s\n", erBuf);                                                //
   //?  if (strpbrk(erBuf, "\r\n")) *strpbrk(erBuf, "\r\n") = 0;                //vas is das ?
        MessageBoxA(NULL, g_err.LocateErrorP(erC), erBuf, MB_OK);               //
        return erC;                                                             //
       }                                                                        //
    g_err.AddContext(fn);                                                       //
    g_err.ShortError(erC, erBuf, sizeof(erBuf));                                //
    Printf("\n%s\n", erBuf);                                                    //
    return erC;                                                                 //
   } //cCompile::_Error...

#include <samVersion.h>
static int Help(const char *a, const char *b)
   {Printf("---------------- SamCompile rev %d, " __DATE__ " ----------------\n", SAM_VERSION);
    Printf("-----------------------------------------------------------------\n");
    Printf("Program to compiler .sam programs into executable code for the samPU.\n");
    Printf("Usage is:\n"  
           "   samCompile <program.sam> [options]\n");
    Printf("Options:\n"
           "   /include \"includeDirectory\" This option overrides the default setting\n"
           "\t\t\t specified with the environment variable 'samInclude'\n"
           "   /macro\t expand macros only.\n"
           "   /capture\t <fileName> specify output file for .macro command.\n");
    Printf("   /patchDrv patch name and size of microcode file into samDefines.sv. This file\n"
           "\t\t\t is located in the directory specified by the\n"
           "\t\t\t environment variable 'VerilogSourceDir', ie,\n\t\t\t %s.\n"
           "   /bugEmit  debug output of opcodes as they are generated.\n\n",
           getenv("VerilogSourceDir"));
    Printf("The compiler produces five output files:\n"
           "   <program>.microcode\t an ascii file in which the binary executable is represented\n"
           "\t\t\t as 16 bit ascii values. Comments '//.....' are ignored.\n"
           "\t\t\t This format is appropriate for loading by Xilinx' xsim.exe.\n"
           "\t\t\t using $readmemh(\"<progam>.microcode\", size, esize, object);\n"  
           "   <program>.bin\t a raw binary representation of the opcodes in <progam>.microcode.\n");
    Printf("   <program>.lineMap\t a binary file in the format {uint16_t pc, fileNum, lineNum};\n"
           "   <program>.msgFile\t messages used by <program>.sam represented as 0x00 delimitted ascii strings.\n"
           "   <program>.symbolTbl\t a binary file in the format {uint16_t pc; char symbolName;}\n");
    Printf("These files are written to the directory specified by the environment variable\n"
           "'VerilogSimulationDir', ie. %s\n",
           getenv("VerilogSimulationDir"));
    Printf("\n`*Examples:\n"
           "    samCompile myprogram.sam\n"
           "    samCompile myprogram.sam /patchDrv\n");
    return 0;
   } //Help...

int main(int argc, char **argv)
   {int         ii, erC=0, keySize=8;                                           //
    bool        expandOnlyB=false, patchDrvB=true, bugEmitB=false;              //
    const char *captureFileP=NULL, *incDirP=getenv("samIncludeDir"),            // 
               *srcName=NULL;                                                   //
    cCompile   *compilerP;                                                      //
    #define IF_ARG(what) if(argv[ii][0] == '/' && stricmp(argv[ii]+1,what) == 0)//
                                                                                //
    if (argc <= 1) return Help(NULL, NULL);                                     //no command line params; coax the user a wee bit
    if (false)                                                                  //
        bugEmitB    = true;                                                     //patch SimulationDriver.sv
    if (false)                                                                  //
        patchDrvB   = false;                                                    //patch SimulationDriver.sv
    if (false)                                                                  //
        expandOnlyB = true;                                                     //
    for (ii=1; ii < argc; ii++)                                                 //
      {IF_ARG("capture")  captureFileP = argv[++ii];                       else //specify capture file
       IF_ARG("include")  incDirP      = argv[++ii];                       else //specify include directory
       IF_ARG("macro")    expandOnlyB  = true;                             else //
       IF_ARG("patchDrv") patchDrvB    = true;                             else //patch 'parameter MICROCODE_SIZE' in SimulationDriver.sv
       if(strncmp(argv[ii], "/h", 2) == 0 || strncmp(argv[ii], "/?", 2) == 0)   //
                                          return Help(argv[ii],argv[ii+1]);else //
       if (srcName == NULL) srcName = argv[ii]; else {erC = ERR_2005; goto err;}//
      }                                                                         //
    if (!srcName) {erC = ERR_2741; goto err;}                                   //2741 missing source file name
                                                                                //
    compilerP            = new cCompile(srcName, incDirP, bugEmitB, patchDrvB); //input files/dir
    if (expandOnlyB) erC = compilerP->PrintProgram();                           //expand macros only
    else             erC = compilerP->CompileProgram();                         //Compile microcode to objFile
    Printf("%s Compiled, error=%d, environment=%d\n",                           //
                           srcName, erC < 0 ? erC : 0, erC);                    //
    delete compilerP;                                                           //
    return erC < 0 ? 255 : erC;                                                 //
    #undef ARG_IS                                                               //
err:cCompile::_Error(erC, NULL, argv[ii], __FILE__, __LINE__, "main");          //
    return 1;                                                                   //
   } //main...

//end of file
