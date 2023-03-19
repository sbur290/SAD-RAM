/* file: generateVerilog.cpp : Jan 2023

 Generates test data arrays for inclusion in Verilog simulation or loading to FPGA.
 The arrays translate the indexbase created for this sort run into equivalent hITEMs
 in the FPGA/hardware implementation. Host addresses are mapped to BRAM addresses,
 pointers and counters are adjusted to the sizes used in the FPGA emulation.
 hBOOK, hPAGE, and hINDX are the hardware equivalents of cBOOK, cPAGE, and cINDX
 with identical field names, but different units or bit sizes.

 Default rowSize is 256 unless overridden with /rowSize <rowSize> on command line
 eg, 'sam <rcdCount> /genVerilog'

 The following files are generated:
   params.data   - run time parameters for FPGA emulation; see structure m_params.
       This file contains debugging/testing directives and the location of various major
       cITEMs in the blockRAM.
   blockRam.data - raw hex data to be loaded into FPGA block RAM using $readmem(...).
       Each row is cSadram::m_rowsize bytes long and occupies one (very long) word of BRAM.
       The rows contain hBOOK, hPAGE, and hINDX data.
           bram[0] contains the configuration vector for the entire row of cells.
           bram[1] contains the first bUsed cBOOK records.
           Each hBOOK record points to an array of cPAGE[]s.
           Each hPAGE record points to an array of cINDX[]s,
           Finally cINDX points to the user data record.
           NOTE: INDX.data isused differently in software/FPGA/hardware implementation:
                 - software: data is the (virtual) address of the user record.
                 - FPGA:     data is the relative address of the user record; user data is not
                             store in the FPGA, and user data references must read the data from disk.
                 - hardware: data is the absolute address of the user record.
       The pointers in hBOOK and hPAGE are translated into the proper bram addresses,
       A single blockram row encodes the configuration bytes for the entire sequency-array.
           cBOOK[] & cPAGE[] configuration is encoded in bits [3:2] (hPAGE and hBOOK are identical).
           cINDX[]s          configuration is encoded in bits [1:0].
           The configuration byte are adjusted to the actual keysize (cSadram::m_keySize)
           and aligned based upon the target bus size (cSadram::m_targetbus_sz).
       The index data for hBOOK, hPAGE, and hINDX is padded and aligned so that the key
           fields are always aligned with the target bus. The structures are rounded to be an
           exact multiple of parameter TARGETBUS_SZ in samDefines.sv.
   blockRam.txt  - human readable description of blockRam.data.
   blockRam.bin  - Binary copy of blockRam.data; used for verification only.
   user.data     - dump of data generated for sort, ie., the 'user data'.
   user.cmds     -
   bulkScan.cmds - file of scan(indx) commands replicating keys generated during sort.

   Two environment variables must be specified to sam.exe:
      - 'verilogSimulationDir' specifying the behavioural simulation directory of the Verilog project,
                               eg., c:\svn\sam\ver-15\sadram\sadram.sim\sim_1\behav\xsim
      - 'verilogSourceDir'    specifying the source directory of the Verilog project,
                               eg., c:\svn\sam\ver-15\sadram\sadram.srcs\sim_1\new
   NOTE: in .bat files this is repesented as %verilogSimulationDir% and %verilogSourceDir%.
   Data files are written to the simulation directory.
   .sv  files are written to the source directory.
   Parameters for GenerateVerilog() are extracted from the file samDefines.sv in the source directory,
   ie., getenv('verilogSourceDir') and verified against the parameters specified to sam.exe.
   For example, 'sam /rowSize <value>'; value must equal the ROW_SZ in %verilogSourceDir%\samDefines.sv.

 cITEMs are mapped to BRAM in four blocks: a configuration block, an hBOOK block,
 an hPAGE block, and an hINDX block.
 The configuration block comprises a single row with the configuration bits
 for hPAGE/hBOOK in bits [7:4] and hINDX in bits [3:0] of a single byte.
 cITEM[]s are packed so that the lower array (hBOOK.page1 or cPAGE.x1P) is
 always as full as possible, and the upper array (hBOOK.page2 or cPAGE.x2P)
 has any remaining (possibly zero) entries. This is critical to the proper
 operation of key insert.
 Representative layout for rcdCount == 333 is shown below. This particular
 layout comprises: 1 configuration, 2 cBOOKs, 4 cPAGE[]s, and 28 cINDX[]s.
 +--------+-------------------+---------+-----------------+---------+
 | BRAM   |                   |#hPAGE[]s|                 |#hPAGE[]s|
 |address |                   |   or    |                 |   or    |
 |        |                   |#hINDX[]s|                 |#hINDX[]s|
 +--------+--------------------\-------/-------------------\--------/
 |  0     | configuration       \     /                     \      / configuration block
 +--------+---------------------+-----+---------------------+------+
 |  1     | book[0-1]           |     |                     |      |         hBOOK block
 +--------+---------------------+-----+---------------------+------+
 |  2,  3 | book[0].page1 = 4   |  10 | book[0].page2 = 26  |   4  |         hPAGE block
 +--------+---------------------+-----+---------------------+------+
 |  4,  5 | book[0].page[0].x1P |  16 | book[0].page[0].x2P |  15  | \
 |  6,  7 | book[0].page[1].x1P |  16 | book[0].page[1].x2P |  13  |  \
 |  8,  9 | book[0].page[2].x1P |  16 | book[0].page[2].x2P |   7  |   \
 | 10, 11 | book[0].page[3].x1P |  16 | book[0].page[3].x2P |  10  |    \
 | 12, 13 | book[0].page[4].x1P |  16 | book[0].page[4].x2P |  12  |     \
 | 14, 15 | book[0].page[5].x1P |  16 | book[0].page[5].x2P |  15  |      \
 | 16, 17 | book[0].page[6].x1P |  16 | book[0].page[6].x2P |  13  |       \ hINDX block
 | 18, 19 | book[0].page[7].x1P |  16 | book[0].page[7].x2P |   2  |       /
 | 20, 21 | book[0].page[8].x1P |  16 | book[0].page[8].x2P |   1  |      /
 | 22, 23 | book[0].page[9].x1P |  16 | book[0].page[9].x2P |  10  |     /
 | 24, 25 | book[1].page[0].x1P |  16 | book[1].page[0].x2P |   3  |    /
 | 26, 27 | book[1].page[1].x1P |  16 | book[1].page[1].x2P |   1  |   /
 | 28, 29 | book[1].page[2].x1P |  16 | book[1].page[2].x2P |   4  |  /
 | 30, 31 | book[1].page[3].x1P |  16 | book[1].page[3].x2P |   3  | /
 +--------+---------------------+-----+---------------------+------+
*/

#include "generateVerilog.h"

#define cBadr(buk)               g_sP->Badr(buk)                                //
#define cBPadr(buk, page)        g_sP->BPadr(buk, page)                         //
#define cBPIadr(buk, page, inx)  g_sP->BPIadr(buk, page, inx)                   //
#define Printx      Printf(CONDITIONAL_CRLF                                     //
#define Printy      if (m_verboseB) Printf                                      //
#define TAB         Printf("\t")                                                //
extern uint32_t     g_rcdSize, g_keySize;                                       //
extern bool         g_doubleBookB;                                              //true=format of indexbase uses book.page1P & page2P
extern FILE        *g_printFileP;                                               //file used by Printf(). duplicate printf output to this file.
extern cBugIndex   *g_bugP;                                                     //
extern cSadram     *g_sP;                                                       //pointer to parent class
uint8_t            *g_bramP;                                                    //globally visible pointer to bram image
extern uint32_t Anumber(char *pp, char **ppP);                                  //
extern void Printf(char const *fmtP,...);                                       //
#define p m_params                                                              //i am one lazy SOB

#pragma pack(push, 1)
typedef struct                                                                  //array of host addresses:
   {void    *hostAdrP;                                                          //corresponding BRAM adr is
   } sMAP;
#pragma pack(pop)

cGenerateVerilog::cGenerateVerilog(void) //class constructor
    {m_itemName[ITEM_CFG ] = "CFG ";
     m_itemName[ITEM_INDX] = "INDX";
     m_itemName[ITEM_PAGE] = "PAGE";
     m_itemName[ITEM_BOOK] = "BOOK";
    } //cGenerateVerilog::cGenerateVerilog...

//Allocate bufferand read file into said buffer; append a goose egg.
int cGenerateVerilog::ReadAllFile(const char *fileNameP, char **dataPP)
   {int      size, ii;
    char    *bufP;
    FILE    *fileP=fopen(fileNameP, "rb");

    if (!fileP)                       return 0;
    fseek(fileP, 0, SEEK_END); size = ftell(fileP);
    bufP = (char*) calloc(size+4, 1);
    if (bufP == NULL) {fclose(fileP); return 0;}
    fseek(fileP, 0, SEEK_SET); ii = (int)fread(bufP, size,1, fileP);
    fclose(fileP);
    *dataPP = bufP;
    return size;
   } //cGenerateVerilog::ReadAllFile...

//Get critical BRAM parameters from samDefines.sv in sources directory.
//Verify critical values against the C++ #defines/variables.
bool cGenerateVerilog::GetVerilogParams(SAM_CONTROLS *ctlP)
   {char        *verilogSrcP=NULL, samDefines[_MAX_PATH], buf[100];             //
    int          ii;                                                            //
    uint32_t     valu;                                                          //
    bool         okB=true;                                                      //
    typedef struct {void *valuP; const char *nameP; uint32_t shouldbe;} TBL;    //
    TBL *aP, arch[] =                                                           //
        {//valueP           name                shouldbe                        //
         {&m_bramRows,     "BRAM_ROWS",         0                },             //+1 howmany rows in BRAM
         {&m_rowBytes,     "ROW_BYTES",         m_rowBytes        },             //+2 rowsizes in samDefins must be equal to
         {&m_targetBusSz,  "TARGETBUS_SIZE",    ctlP->targetBusSz},             //+3 size of target bus in FPGA implementation
         {NULL,            "hCFG_INDX_BITS",    hCFG_INDX_BITS   },             //1:0 bits of configuration byte //used when op.item == hINDX
         {NULL,            "hCFG_PAGE_BITS",    hCFG_PAGE_BITS   },             //3:2              "             //used when op.item == hPAGE/hBOOK
         {NULL,            "hCFG_INDX_1STK",    hCFG_INDX_1STK   },             //ie., bit[4]      "             //
         {NULL,            "hCFG_INDX_LASTB",   hCFG_INDX_LASTB  },             //ie., bit[5]      "             //
         {NULL,            "hCFG_PAGE_1STK",    hCFG_PAGE_1STK   },             //ie., bit[6]      "             //
         {NULL,            "hCFG_PAGE_LASTB",   hCFG_PAGE_LASTB  },             //ie., bit[7]      "             //
         {NULL,            "hUSER_ADR_BITS",    hUSER_ADR_BITS   },             //+4 verified, but define used hereafter
         {NULL,            "hROW_ADR_BITS",     hROW_ADR_BITS    },             //+5               "
         {NULL,            "hCOUNT_BITS",       hCOUNT_BITS      },             //+6               "
         {NULL,            "hTOTAL_BITS",       hTOTAL_BITS      },             //+7               "
//       {NULL,            "hPAGE_PAD_BITS",    hPAGE_PAD_BITS   },             //+8               "
//       {NULL,            "hBOOK_PAD_BITS",    hBOOK_PAD_BITS   },             //+9               "
         {NULL,            "hCFG_IDLE",         hCFG_IDLE        },             //10               "
         {NULL,            "hCFG_FRST",         hCFG_FRST        },             //11               "
         {NULL,            "hCFG_MIDL",         hCFG_MIDL        },             //12               "
         {NULL,            "hCFG_LAST",         hCFG_LAST        },             //13               "
        };                                                                      //
                                                                                //
    SNPRINTF(samDefines), "%s\\%s", getenv("verilogSourceDir"),"samDefines.sv");//
    if (ReadAllFile(samDefines, &verilogSrcP) == 0)                             //
        {m_err.PrintError(ERR_0003, "", samDefines); return false;}             //
                                                                                //
    for (ii=0, aP=arch; ii < HOWMANY(arch); ii++, aP++)                         //
      {valu = GetParam(verilogSrcP, aP->nameP);                                 //
       if (aP->valuP) *((uint32_t*)aP->valuP) = valu;                           //
       if (aP->shouldbe == valu || aP->shouldbe == 0) continue;                 // == 0 means no local value to compare
       SNPRINTF(buf), "%s != local %s\n", aP->nameP, aP->nameP);                //
       ShowError(buf); okB = false;                                             //
      }                                                                         //
    FREE(verilogSrcP);                                                          //
    return okB;                                                                 //
   } //cGenerateVerilog::GetVerilogParams...

//Generate the promised files
bool cGenerateVerilog::Generate(SAM_CONTROLS *ctlP)
   {const char *pp;                                                             //
    uint32_t    bug=g_bugLevel;                                                 //
    bool        okB=false;                                                      //true when everything is successful
                                                                                //
    m_ctlP          = ctlP;                                                     //
    m_bramP         = NULL;                                                     //
    m_extentP       = NULL;                                                     //
    memset(&m_params, 0, sizeof(m_params));                                     //
    m_verboseB      = ctlP->verboseB || g_bugLevel >= 2;                        //copy from other structures ease of use.
    m_keySize       = g_sP->m_keySize;                                          //           "
    m_rowBytes       = g_sP->m_rowBytes;                                          //           "
    memmove(m_baseSz, g_sP->m_baseSz, sizeof(m_baseSz));                        //           "
    memmove(m_sizes,  g_sP->m_sizes,  sizeof(m_sizes));                         //           "
    memmove(m_align,  g_sP->m_align,  sizeof(m_align));                         //           "
    memmove(m_padSz,  g_sP->m_padSz,  sizeof(m_padSz));                         //           "
    memmove(m_perRow, g_sP->m_perRow, sizeof(m_perRow));                        //           "
                                                                                //
    if (!GetVerilogParams(ctlP))                                  return false; //fails because something is the wrong size
                                                                                //
    //some files live in simulation drctry; others in source directory          //
    //Either way, the environment variables must be defined                     //
    if (!getenv(pp="verilogSourceDir") || !getenv(pp="verilogSimulationDir"))   //source/simulation directory of current .sv project
       {m_err.PrintError(ERR_9007, "", pp); okB = false;              goto xit;}//
                                                                                //
    if (!g_doubleBookB) ReformatBook();                                         //Convert to double pages/cBOOK (page1P and page2P)
    m_bramP    = g_bramP = (uint8_t*) calloc(m_rowBytes+8, m_bramRows);         //image of blockRam.bin; +8 for lazy programmers
    m_extentP  =           (uint32_t*)calloc(m_bramRows, sizeof(uint32_t));     // m_bramRows from samDefines.sv,
    //Create text description file *******************************              //
    Printf("Creating data files for Verilog simulation using sim.exe rev %d\n", SAM_VERSION);
    if (!CreateAfile("blockRam.txt", true))                           goto xit; //
    if (!ComputeSizes())                                              goto xit; //
    GenerateParamsStruct();                                                     //-\.
    GenerateCfg();                                                              //  \.   accumulate
    GenerateBook();                                                             //   \.  binary
    GeneratePage();                                                             //   |. structues
    GenerateIndx();                                                             //   /. in m_bramP
  //RecomputeLoKeys();                                                          //  /.  (aka g_bramP)
    GenerateAdrPattern();                                                       //-/.   ...
    if (m_verboseB) DisplayMisc();                                              //display only; call early to get into .txt file
    if (m_verboseB) DisplayBlockram();                                          //synopsis for .txt file
    CloseTheFile();                                                             //close blockRam.txt
                                                                                //
    okB          = GenFileParams_txt();                                         //descriptive data
    if (okB) okB = GenFileUserData();                                           //
    if (okB) okB = GenFileBulkScan();                                           //
    if (okB) okB = GenFileBlockRam_data();                                      //Finally write out all generated data
    if (okB) okB = GenFileReadWord_sv();                                        //Generate readWord.sv, writeWord.sv 
    if (okB) okB = GenFileWriteWord_sv();                                       //   and groupSmear.sv
    if (okB) okB = GenFileGroupSmear_sv();                                      //
//  if (okB)                                                                    //
//    {cBugSim bugSim(m_params);                                                //
//     okB = VerifyHbase();                                                     //
//     if (false)                                                               //
//        {cBugSim bugSim(m_params);                                            //
//         g_sP->PushBook((cBOOK *)&m_bramP[p.b1st * m_rowBytes], p.bUsed);      //-+   this displays bram the
//         g_bugLevel = 3;                                                      //  \  bram version
//         bugSim.Bug("Check hBOOKs",cGenerateVerilog::ShowAdr);                //  /  of the indexbase
//         g_sP->PopBook();                                                     //-+   Restore original indexbase
//    }   }                                                                     //
xit:FREE(m_bramP);    m_bramP    = NULL;                                        //
    FREE(m_extentP);  m_extentP  = NULL;                                        //
    return okB;                                                                 //
   } //cGenerateVerilog::Generate...

//Return address of hbook[buk].
//Structure is not linealy addressed since block of cBOOK_hperRow * hBOOK fill
//a single row and there is a hiccup as it crosses onto the next row.
hBOOK *cGenerateVerilog::hBadr(uint32_t buk)
   {uint8_t *rowP;
    rowP = &m_bramP[(p.b1st + buk / hBOOK_perRow) * m_rowBytes];                //which row
    return (hBOOK*)(rowP + hBOOK_padSz * (buk % hBOOK_perRow) + hBOOK_align);   //hBOOK within row
   } //cGenerateVerilog::hBadr...

//Return address of hbook[buk].hpages[page]; structure is not linealy addressed
//since no hPAGE structure will cross a row boundary.
hPAGE *cGenerateVerilog::hBPadr(uint32_t buk, uint32_t page)
   {uint8_t *rowP;
    rowP = &m_bramP[(p.p1st + buk + page/hPAGE_perRow) * m_rowBytes];           //which row
    return (hPAGE*)(rowP + hPAGE_padSz * (page % hPAGE_perRow) + hPAGE_align);  //hPAGE within row
   } //cGenerateVerilog::BPadr...

hINDX *cGenerateVerilog::hBPIadr(uint32_t buk, uint32_t page, uint32_t inx)
   {hPAGE   *hpageP = hBPadr(buk, page);
    uint32_t inxAdr = (uint32_t)(inx >= cINDX_perRow ? hpageP->X2P : hpageP->X1P);
    return (hINDX*)&m_bramP[inxAdr*m_rowBytes + (inx % hINDX_perRow) * hINDX_padSz + hINDX_align];
   } //cGenerateVerilog::hBPIadr...

bool g_changesB=false;
void cGenerateVerilog::MoveKey(uint8_t *d, uint8_t *s)
    {if (g_sP->CompareKey(d,s,m_keySize) != 0) g_changesB = true;
     memmove(d,s,m_keySize);
    } //cGenerateVerilog::MoveKey...

//Calculate count and locn of hBOOK[]s hPAGE[]s & hINDX[]s.
//The size and other characteristics of the sequencers is specified to the
//FPGA/hdw by m_params(=p). For simplicity of genVerilog we adopt the perRow
//values from the current software values.
//The FPGA/hardware implementation in which the size of the hITEMs is smaller
//allow a larger number of cITEMs to be stored in a row.
bool cGenerateVerilog::ComputeSizes(void)
   {char        erBuf[100];                                                     //
    p.iSize   = hINDX_padSz;  p.pSize   = hPAGE_padSz;  p.bSize   = hBOOK_padSz;//
    p.iPerRow = hINDX_perRow; p.pPerRow = hPAGE_perRow; p.bPerRow = hBOOK_perRow; 
    p.iAlign  = hINDX_align;  p.pAlign  = hPAGE_align;  p.bAlign  = hBOOK_align;//
    p.iUsed   = DivUp(g_sP->m_rcdCount, hINDX_perRow);                          //
    p.pUsed   = DivUp(p.iUsed,        2*hPAGE_perRow);                          //
    p.bUsed   = DivUp(p.pUsed,        2*hBOOK_perRow);                          //to allow thismany hBOOKs.
    p.b1st    = 1;                                                              //bram[1] = first hBOOK[].
    p.p1st    = p.b1st + p.bUsed;                                               //first hPAGE will reside here.
    p.i1st    = p.p1st + p.pUsed;                                               //first hINDX will reside here.
    p.fre1st  = p.i1st + p.iUsed;                                               //
    if (p.fre1st < m_bramRows) return true;                                     //
    SNPRINTF(erBuf), "Rows reqd for indexbase(=%d) exceed available BRAM (=%d rows)",   //
                                      p.fre1st+1,                 m_bramRows);  //
    ShowError(erBuf);                                                           //
    return false;                                                               //
   } //cGenerateVerilog::ComputeSizes...

/*Generate the config byte for each sequencerCell/group for a specific keySize
 and wit due respect to the sizeof(hINDX) and sizeof(cPAGE).
 Configuration bytes fill an entire DRAM row and apply to all cells across the row.
 The opcodes vary in time, the configuration bytes are fixed at configuration time
 but vary by the physical position of the cell/group within an array.
 Even with the same opcode, each cITEM (cINDX, cPAGE, or cBOOK), the cell will
 behave differently because of different config bytes.
 The configuration byte is laid out as follows:
   [1:0] = hINDX       first and last bits.
   [3:2] = hPAGE/hBOOK first and last bits (these two structures are identical).
   [5:4] = group configuration bits when scanning an hINDX array.
   [7:6] = group configuration bits when scanning an hPAGE/hBOOK array.

 Each cITEM structure has 'binary stuff' in the beginning followed by the key.
     eg., struct cINDX = {dataRelAdr:47, stop:1, key[k]};
          dataRelAdr & stop are the binary stuff. The binary stuff is of fixed
 size but the key size depends upon the user specification. this makes for some
 very delicate hardware design and is the reason for the configuration vector.
 The key must be aligned with the target bus for the sequencer to work properly,
     ie., key.byte[0] must align with target[0].
 This requires alignment bytes in front of each cITEM.
 These are coded as hCFG_IDLE bits. The pattern is:
       <alignment>       <-binary->     <------------------- key ------------------>
      {align*ccfg_idle,  bin*ccfg_idle, key=ccfg_1st, (keySz-2)*ccfg_mid, ccfg_last}.
 The pattern is repeated m_perRow[item] times thru the row.

 The upper nibble of the configuration bytes controls the diagonal group.
 Configuration bits are only required every targetbus_sz cells.
 Group configuration bits are present in the first byte of a group:
   config == 2'b00      the group is not active (for example if the group is
                        hovering over binary/alignment bytes)
   config == 2'b01      the group is the first group in a sequencer-array
   config == 2'b10      the group is the last  group in a sequencer-array
   config == 2'b11      the group is active but neither the first nor last group.

 For example, with a target_bus_sz = 8 cells and KEY_SZ == 8, a cINDX
 item has key=8, align=2, and bin=6.
 The data and config rows are therefore formatted as follows:
  0123456789ABCDEF, 0123456789ABCDEF, 0123456789ABCDEF, ...  adr in row
  xxBBBBBBKKKKKKKK, xxBBBBBBKKKKKKKK, xxBBBBBBKKKKKKKK, ...  cINDX data
  AAIIIIIIFMMMMMML, AAIIIIIIFMMMMMML, AAIIIIIIFMMMMMML, ...  cINFX config
 Notice that the key (KKKKKKKK) always begin on byte 8
 where: A=alignment byte,   I=hCFG_IDLE, F=CFG_1st, M=CFG_MID, L=CFG_LAST,
        B=binary head byte, x=dont/care, K=key byte
 Each config byte has one of four values: idle, 1st, mid, or last.
 These are packed into a single byte: [3:0]=INDX, [7:4]=PAGE/BOOK.
 The 'for (item=' loop accumulates these packed value across the dram row.
 NOTE: not checked for multiplexed scans.
*/
//OR in new cfg bits at u8P * num bytes, return updated pointer.
static inline uint8_t *Encode(uint8_t *u8P, int hilo, uint8_t cfg, int num=1)
    {while (--num >= 0) *u8P++ += (cfg << hilo); return u8P;}

//Generate configuraiton vector
//showB displays the results, false just does the work.
uint32_t cGenerateVerilog::GenerateCfg(void)
   {int       item, perRow, align, cfgRow=0, hilo=0, grpsPerItem;               //
    uint8_t  *cfgP= &m_bramP[cfgRow * m_rowBytes], *u8P;                        //
#define FIRSTK(what)  (h##what##_size+h##what##_align)-m_targetBusSz            //group# of first key field in row
#define LASTB(what)  FIRSTK(what)+(h##what##_perRow-1)*h##what##_padSz-m_targetBusSz //group# of last word of last group in row
                                                                                //
    m_extentP[p.cfgAdr=cfgRow] = m_rowBytes;                                    //entire row will be filled
    Printy("-------- bram[%d] = Sequencer Configuration --------\n",   cfgRow); //location of configuration vector in bram
//Cell konfig ------------------------------------------------------------------//
    for (item=ITEM_INDX; item <= ITEM_PAGE /*not TYPE_BOOK*/; item++, hilo+=2)  //ie cINDX, cPAGE/cBOOK packed into one
        {Printy("%s=%d:",  item == ITEM_INDX ? "cINDX_size      "               //
                                             : "cBOOK/cPAGE_size", m_sizes[item]); 
         grpsPerItem = m_padSz[item] / m_targetBusSz;                           //groups required for this item
         align = m_align[item];                                                 //
            {Printy("{align=%d*0x%02X,",   align,          hCFG_IDLE);          //
             Printy( "bin=%2d*0x%02X,",    m_baseSz[item], hCFG_IDLE);          //
             Printy(     "key=0x%02X,",                    hCFG_FRST);          //
             Printy( "%2d*0x%02X,",        m_keySize-2,    hCFG_MIDL);          //
             Printy(    " 0x%02X}",                        hCFG_LAST);          //
             Printy(" repeated %2d times in bits[%d:%d]\n",                     //
                                m_perRow[item], hilo+1, hilo);                  //
            }                                                                   //
         //Generate actual data for INDX, PAGE or BOOK configuration.           //
         for (u8P=cfgP, perRow=m_perRow[item]; --perRow >= 0; )                 //
             {u8P = Encode(u8P, hilo, hCFG_IDLE, align+m_baseSz[item]);         //align+binary header; aligns key on targetBus edge (target_bus_sz bytes)
              u8P = Encode(u8P, hilo, hCFG_FRST);                               //first byte of key
              u8P = Encode(u8P, hilo, hCFG_MIDL, m_keySize-2);                  //middle bytes of key (possibly zero)
              u8P = Encode(u8P, hilo, hCFG_LAST);                               //last byte of key
             }                                                                  //
       } //item=ITEM_INDX...                                                    // (interesting because it contains the stop bit :)
//Group konfig -----------------------------------------------------------------//
     //Group konfig: rowType bits are identical to the cell konfig, ie.         //
     //`hCFG_INDX_BITS (1:0 bits) and `hCFG_PAGE_BITS ([3:2] bits respectively. //
     //These are used to generate 'overTgt' in sequencerGroup.sv signalling that// 
     //the group is located over the key portion of the hITEM.                  //
     //                                                                         //
     //The following lines designate the first & last groups in the upper nibble//
     //of the configuration byte, ie.,                                          //
     //    hCFG_INDX_1STK (bit[4]),   hCFG_INDX_LASTB (bit[5])                  //
     //    hCFG_PAGE_1STK (bit[6]), & hCFG_PAGE_LASTB (bit[7]).                 //
     //Only the last designation is actually used - to detect the group         //
     //containing the stop bit in the binary section of hITEM.                  //
     cfgP[FIRSTK(INDX)] |= 1 << hCFG_INDX_1STK;                                 //first  hINDX
     cfgP[LASTB( INDX)] |= 1 << hCFG_INDX_LASTB;                                //last-1 hINDX
     cfgP[FIRSTK(PAGE)] |= 1 << hCFG_PAGE_1STK;                                 //first  hPAGE
     cfgP[LASTB( PAGE)] |= 1 << hCFG_PAGE_LASTB;                                //last-1 hPAGE
     //Finally print out the configuration vector                               //
     if (m_verboseB)                                                            //
        {PrintHex(cfgP, 0, m_rowBytes, true);                                   //
         Printf("\n");                                                          //
        }                                                                       //
    return cfgRow+1;                                                            //next available row
#undef Encode                                                                   //
   } //cGenerateVerilog::GenerateCfg...

//Copy cBOOK entries to BRAM beginning at p.b1st. Compact cBOOK.page1,
//and change hBOOK.page1 & page2 to the BRAM address (row).
void cGenerateVerilog::GenerateBook(void)
   {uint32_t dbuk, sbuk, align, count, eSize, row, page, bUsed=g_sP->m_bUsed;   //
    cBOOK   *bookP;                                                             //source structure
    hBOOK   *hBookP;                                                            //target structure
    Printy("-------- bram[%d]: hBOOK block --------\n", p.b1st);                //
    row          = p.b1st-1;                                                    //
    page         = p.p1st;                                                      //
    align        = m_align[ITEM_BOOK];                                          //
    eSize        = align + hBOOK_size + m_keySize;                              //
    for (dbuk=count=sbuk=0; sbuk < bUsed; sbuk++, dbuk++)                       //
        {if ((dbuk % hBOOK_perRow) == 0)                                        //will overflow
            {memset(&m_bramP[++row *m_rowBytes], 'B', align);                    //signature
             m_extentP[row] = hBOOK_align+Min(bUsed, hBOOK_perRow)*hBOOK_padSz  //
                                             - (hBOOK_padSz - hBOOK_size);      //
            }                                                                   //
         bookP        = g_sP->Badr(sbuk);                                       //
         count       += bookP->count;                                           //
         hBookP       = hBadr(dbuk);                                            //good spot to build an hBOOK
         hBookP->X1P  = page++;                                                 //-+   copy
         hBookP->X2P  = page++;                                                 //  \   and
         hBookP->Total= bookP->total;                                           //   \   translate
         hBookP->Kount= bookP->count;                                           //   /    all
         hBookP->Stop = sbuk == (g_sP->m_bUsed-1);                              //  /      fields
         MoveKey(hBookP->LoKey, bookP->loKey);                                  //-+        from cBOOK
         Printf("  b[%d]=", dbuk);                                              //
         ShowItem((uint8_t*)hBadr(dbuk), ITEM_BOOK);                            //
        }                                                                       //
     Printy("\n");                                                              //
    } //cGenerateVerilog::GenerateBook...

//Copy each cBOOK::page1/2[] to BRAM image entry by entry filling lower page first.
void cGenerateVerilog::GeneratePage(void)
   {uint32_t    buk, page, pageCount, align, eSize, inx, row;                   //
    cBOOK      *bukP;                                                           //
    cPAGE      *pageP;                                                          //
    hPAGE      *hPageP;                                                         //
                                                                                //
    Printy("-------- bram[%d] hPAGE block --------\n", p.p1st);                 //
    row              = p.p1st;                                                  //
    inx              = p.i1st;                                                  //where hINDX's will go after hPAGEs are generated
    align            = m_align[ITEM_PAGE];                                      //
    eSize            = align + hPAGE_size + m_keySize;                          //
    for (buk=page=0; buk < g_sP->m_bUsed; buk++, row++)                         //
        {//Copy&xlat cBOOK page to hBOOK page, pausing @dpage=perRow to ++row   //
         pageCount = (bukP=cBadr(buk))->count;                                  //
         Printy("bram[%d]\thbook[%d] has %d cPAGEs\n", row, buk, pageCount);    //
         for (page=0; page < pageCount; page++)                                 //
             {pageP        = cBPadr(buk, page);                                 //source
              Printf("  bp[%d.%2d]=", buk, page);                               //
              if (page == 0 || page == hPAGE_perRow)                            //
                 {memset(&m_bramP[row*m_rowBytes], 'P', align);                  //signature
                  m_extentP[row++] =                                            //
                      hPAGE_align+Min(hPAGE_perRow,pageCount-page)*hPAGE_padSz  //
                                             - (hPAGE_padSz - hPAGE_size);      //
                 }                                                              //
              hPageP       = hBPadr(buk, page);                                 //destination
              hPageP->Total= pageP->total;                                      //--+ copy
              hPageP->Stop = (page==(bukP->count-1));                           //   \ and
              hPageP->X1P  = inx++;                                             //    \ translate
              hPageP->X2P  = inx++;                                             //    /  all Fields
              hPageP->Kount= pageP->count;                                      //   /    from cPAGE
              MoveKey(hPageP->LoKey, pageP->loKey);                             //--+                                                         //
              ShowItem((uint8_t*)hPageP, ITEM_PAGE);                            //
        }    }                                                                  //next available row
    Printy("\n");                                                               //
   } //cGenerateVerilog::GeneratePage...

void cGenerateVerilog::GenerateIndx(void)
   {uint32_t  inx, page, buk, inxCount, paddedSz, perRow, align, row;            //
    uintptr_t dataRelAdr;                                                       //
    uint8_t  *u8P;                                                              //
    cBOOK    *bookP;                                                            //
    cPAGE    *pageP;                                                            //
    hINDX    *hIndxP;                                                           //
    cINDX    *inxP;                                                             //
    char      note[64];                                                         //
    const char*fmtP="row=%d, available hardware rows=%d", *x1P="x1P";           //
                                                                                //
    row              = p.i1st;                                                  //
    Printy("-------- bram[%d] hINDX block --------\n", row);                    //
    paddedSz         = m_padSz[ITEM_INDX];                                      //
    perRow           = m_perRow[ITEM_INDX];                                     //
    align            = m_align[ITEM_INDX];                                      //
    for (buk=0; buk < g_sP->m_bUsed; buk++)                                     //
        {//NOTE: # of cINDXs in each row may differ from the current indexbase  //
         //because the hdw structures are smaller and x1P must be filled before //
         //x2P may be used (requirement of key insertion).                      //
         for (page=0, bookP=cBadr(buk); page < bookP->count; page++)            //for each page in book.page1/2[]
             {inxCount = (pageP=cBPadr(buk, page))->count;                      //
              for (inx=0; inx < inxCount; inx++)                                //
                  {u8P          = &m_bramP[row * m_rowBytes] +                   //
                                 (inx % hINDX_perRow) * (hINDX_size + align);   //
                   if (inx == 0 || inx == hINDX_perRow)                         //
                      {if (((int)row) >= m_bramRows)                            //
                          {SNPRINTF(note), fmtP, row, m_bramRows);              //
                           #ifdef _DEBUG
                             Assert(((int)row) < m_bramRows, note);             //
                           #endif
                          }                                                     //
                       Printy("bram[%d] bp[%d.%d].%s\n", row, buk, page, x1P);  //
                       x1P = "x2P"; BUGSET(u8P, 'X', align);                    //
                       m_extentP[row] = hINDX_align+                            //
                                    Min(inxCount, hINDX_perRow)*hINDX_padSz     //
                                             - (hINDX_padSz - hINDX_size);      //
                      }                                                         //
                   Printy("  bpi[%d.%d.%02d]=", buk, page, inx);                //
                   inxP         = g_sP->BPIadr(buk, page, inx);                 //
                   dataRelAdr   = (uintptr_t)(inxP->dataP-(uint8_t*)g_dataP);   //relativize address
                   hIndxP       = (hINDX*)(u8P += align);                       //
                   hIndxP->Data = dataRelAdr;                                   // \ copy and translate
                   hIndxP->Stop = inx == (inxCount-1);                          //  |  all Fields
                   MoveKey(hIndxP->Key, inxP->key);                             // /     from cINDX
                   u8P          = hIndxP->Key + m_keySize;                      //
                   ShowItem((uint8_t*)hIndxP, ITEM_INDX);                       //
                   if ((inx % cINDX_perRow) == (cINDX_perRow-1)) row++;         //
         }    }   }                                                             //
       } //cGenerateVerilog::GenerateIndx...

//Rebuild loKey fields in hBOOK and hPAGE. Would only be necessary if
//hPAGEs or hINDXs have been packed diffeently.
//void cGenerateVerilog::RecomputeLoKeys(void)
//   {uint32_t buk, page;                                                         //
//    hBOOK   *hBukP;                                                             //
//    g_changesB = false;                                                         //used experimentally only
//    for (buk=0; buk < p.bUsed; buk++)                                           //
//       {for (hBukP=hBadr(buk), page=0; page < hBukP->Kount; page++)             //
//            MoveKey(hBPadr(buk, page)->LoKey, hBPIadr(buk, page, 0)->Key);      //
//        MoveKey(hBukP->LoKey, hBPadr(buk,0)->LoKey);                            //
//       }
//  } //cGenerateVerilog::RecomputeLoKeys...

//Generate address pattern (00 01 02 03 04 05,...) for the rest of BRAM
void cGenerateVerilog::GenerateAdrPattern(void)
   {int      ii, sz, row=p.fre1st;                                              //
    uint8_t *u8P=&m_bramP[row*m_rowBytes];                                       //
    Printy("\n-------- bram[%d-%d]\tunused; filled with address pattern"        //
                      "(00 01 02...) --------\n\n", row, m_bramRows-1);         //
    for (ii=0, sz=(m_bramRows-row)*m_rowBytes; sz-- > 0;) *u8P++ = ii++;         //
   } //cGenerateVerilog::GenerateAdrPattern...

//Verilog code to generate this is so clumsy it is easier to generate in C++.
bool cGenerateVerilog::GenFileReadWord_sv(void)
   {int grpCount = m_rowBytes / m_targetBusSz;                                   //
    FILE *fileP;                                                                //
    SNPRINTF(m_fileName), "%s\\readWord.sv", getenv("VerilogSourceDir"));       //    
    if (!(fileP=fopen(m_fileName, "wb")))                                       //
       {Printf("*** Error creating %s\n", m_fileName); return false;}           //
    #define gen fprintf(fileP,                                                  //
     gen "//WARNING: generated code#1\n");
     gen "//Module to extract a single 64-bit word from the dramI (dram row).\n");
     gen "`include \"samDefines.sv\"\n");
     gen "\n");
     gen "module ReadWord\n");
     gen "   (input  wire                         clk,                                    //+0 \n");
     gen "    input  wire                         readGo,                                 //+1\n");
     gen "    input  wire [ROW_BITS-1:0]          dramI,                                  //+2 one row of DRAM\n");
     gen "    input  wire [BRAM_ADR_BITS-1:0]     address,                                //+3\n");
     gen "    output wire [TGT_BITS-1:0]          wordOut                                 //+4\n");
     gen "   );                                                                           //\n\n");
     gen "reg [TGT_BITS-1:0]theWord; assign wordOut = theWord;                            //\n");
     gen "`define GroupBits(n)   (n+1) * TGT_BITS-1 : (n) * TGT_BITS                      //partition of dram for each sequencerGroup\n\n");
     gen "always @(posedge clk)                                                           //\n");
     gen "   if (readGo)                                                                  //\n");
     gen "      case (address)                                                            //\n");
     for (int ii=0; ii < grpCount; ii++) {gen "         %2d: theWord <= dramI[`GroupBits(%2d)];\n", ii, ii);}
     gen "      endcase                                                                   //\n");
     gen "endmodule //ReadWord...                                                         //\n");
     gen "//END WARNING: generated code#1                                                 //\n");
    CloseTheFile();
    return true;
    #undef gen
   } //cGenerateVerilog::GenFileReadWord_sv...

//Code to generate this is so clumsy in Verilog it is easier to generate in C++.
bool cGenerateVerilog::GenFileWriteWord_sv(void)
   {int grpCount = m_rowBytes / m_targetBusSz;                                   //
    FILE *fileP;                                                                //
    SNPRINTF(m_fileName), "%s\\writeWord.sv", getenv("VerilogSourceDir"));      //
    if (!(fileP=fopen(m_fileName, "wb")))                                       //
                                {Error(ERR_7147, "", m_fileName); return false;}//
    #define gen fprintf(fileP,
    gen "//WARNING: generated code#1\n");
    gen "//Module to write a single word to the blockRAM.\n\n");
    gen "`include \"samDefines.sv\"\n\n");
    gen "module WriteWord\n");
    gen "   (input  wire                         clk,                                    //+0\n");
    gen "    input  wire [1:0]                   wrytGo,                                 //+1 WR_PREPARE=prepare, WR_PREPARE=insert target into row\n");
    gen "    input  wire [ROW_BITS-1:0]          rowIn,                                  //+2 one row of DRAM\n");
    gen "    input  wire [BRAM_ADR_BITS-1:0]     wdAdr,                                  //+3 word adr within row\n");
    gen "    input  wire [TGT_BITS-1:0]          target,                                 //+4 word to write\n");
    gen "    output reg  [ROW_BITS-1:0]          rowOut                                  //+6 modified row\n");
    gen "   );                                                                           //\n\n");
    gen "`define MM(n) n: rowOut[(n+1)*TGT_BITS-1:n*TGT_BITS] <= target                  //\n");
    gen "always @(posedge clk) \n");                                                     //\n");
    gen "    begin                                                                       //\n");
    gen "    if (wrytGo == WR_PREPARE) rowOut <= rowIn;                                  //capture rowin\n");
    gen "    if (wrytGo == WR_INSERT)                                                    //patch rowin[wdAdr]\n");
    gen "      case (wdAdr) //dial in specified word                                     //\n         ");
    for (int ii=0; ii < grpCount; ii++) gen "`MM(%2d);%s", ii, (ii&7) == 7 ? "//\n         " : " ");
    gen          "endcase                                                                //\n");
    gen "    // (wrytGo == WR_COMMIT)                                                    //samConrol updates BRAM\n");
    gen "    end                                                                         //\n");
    gen "endmodule //WriteWord...\n");
    gen "//END WARNING: generated code#1\n");
    CloseTheFile();
    return true;
    #undef gen
   } //cGenerateVerilog::GenFileWriteWord_sv...

//Code to generate this is so clumsy in Verilog it is easier to generate in C++.
//Code to smear bits of grpMask across the entire hITEM. See generated comment for detail.
bool cGenerateVerilog::GenFileGroupSmear_sv(void)
   {FILE *fileP;                                                                //
    SNPRINTF(m_fileName), "%s\\groupSmear.sv", getenv("VerilogSourceDir"));     //
    if (!(fileP=fopen(m_fileName, "wb")))                                       //
       {Printf("*** Error creating %s\n", m_fileName); return false;}           //
    #define gen fprintf(fileP,
    gen "//WARNING: generated code#1\n");
    gen "/*Smear grpMask from the groups situated over the keys to preceding grps supporting this hITEM.\n");
    gen "The ideas is to get grpMask dense with '<' condition so that the shift (when it comes)\n");
    gen "will engage all groups at or beyond the insertion point.\n");
    gen "\n");
    gen "For example: with keysize == 8, targetBus == 8,\n");
    gen "    group size    == targetbus Size\n");
    gen "    sizeof(hINDX) == 6+keySize=14 bytes, which need 2 groups.\n");
    gen "    So when the group identified by overTgt reports that the target > dramI that conclusion\n");
    gen "    needs to be promulgated to the group immediately before the overTgt group, ie,\n");
    gen "       grpMask[grpN] <= grpRslt[grpN+1][0]; //[0] = COMPARE.gt\n");
    gen "    For the group situated over the key, it is a little easire:\n");
    gen "       grpMask[grpN] <= grpRslt[grpN][0];   //[0] = COMPARE.gt\n");
    gen "For hPAGE/hBOOK the same situation prevails, however, size(hPAGE) == 14+keySize = 3 groups.\n");
    gen "    Accordingly, when off-target:\n");
    gen "       grpMask[grpN]   <= grpRslt[grpN+2][0];\n");
    gen "       grpMask[grpN+1] <= grpRslt[grpN+2][0];\n");
    gen "    this requires a more hairy gorilla, namely:\n");
    gen "       grpMask[grpN]   <= grpRslt[(grpN/3)*3 + 2][0];\n");
    gen "    and when on-target, the familiar:\n");
    gen "       grpMask[grpN] <= grpRslt[grpN][0];\n");
    gen "*/\n");
    gen "\n");
    gen "`include \"samDefines.sv\"\n");
    gen "\n");
    gen "module GroupSmear //its a kind of cheese :)\n");
    gen "   (input  wire                         clk,                                            //+0\n");
    gen "    input  wire                         smearGo,                                        //+1 1=prepare, 2=doit\n");
    gen "    input  wire                         setBit,                                         //+2 1=set grpMask\n");
    gen "    input  wire                         inxBit,                                         //+3 1=hPAGE/hBOOK, 0=hINDX\n");
    gen "    input  wire [GROUP_CNT-1:0]         overTgt,                                        //+4 group[grpN] is situated over a key\n");
    gen "    input  wire `COMPARE                grpRslt[GROUP_CNT+1],                           //+5 NOTE: extra location\n");
    gen "    input  wire [GROUP_CNT-1:0]         stop,                                           //+6\n");
    gen "    output reg  [GROUP_CNT-1:0]         grpMask                                         //+7\n");
    gen "   );                                                                                   //\n");
    gen "\n");
    gen "genvar grpN;\n");
    gen "wire [GROUP_CNT:0]                      stopX={1'b1, stop};                             //\n");
    gen "for (grpN=0; grpN < GROUP_CNT; grpN++)                                                  //\n");
    gen "  always @(posedge clk)                                                                 //\n");
    gen "    if (smearGo && setBit)                                                              //\n");
    gen "       grpMask[grpN] <= overTgt[grpN] ? grpRslt[grpN]        [0] & !stopX[grpN]  :      //on target\n");
    gen "                        inxBit        ? grpRslt[grpN+1]      [0] & !stopX[grpN+1]:      //two groups/item\n");
    gen "                                        grpRslt[(grpN/3)*3+2][0] & !stopX[(grpN/3)*3+2];//three groups/item\n");
    gen "endmodule //GroupSmear...\n");
    gen "\n");
    gen "//END WARNING: generated code#1\n");
    CloseTheFile();
    return true;
    #undef gen
   } //cGenerateVerilog::GenFileGroupSmear_sv...

//Generate the ascii files blockRam.data, and firstRow.data (first row of blockRam.data)
//These files are read by $readmemh in the Vivado simulation.
//Generate blockRam.bin for sim.exe; this is the raw data generated by cSadram.
//Byte order is MSB throughout SAM.
bool cGenerateVerilog::GenFileBlockRam_data(void)
   {uint8_t   *u8P;                                                             //
    uint32_t   bramAdr, ii, min=Min(32, m_rowBytes);                             //
    if (!CreateAfile("blockRam.data", true))                      return false; //
    for (bramAdr=0; bramAdr < m_bramRows; bramAdr++)                            //
        {u8P = &m_bramP[bramAdr * m_rowBytes];                                   //
         for (ii=m_rowBytes; ii != 0; )                                          //generate hex in
             fprintf(g_printFileP, "%02X", u8P[--ii] & 0xFF);                   //  MSB first order
         fprintf(g_printFileP, "\n");                                           //double lf
        }                                                                       //
    CloseTheFile();                                                             //
                                                                                //
    if (!CreateAfile("blockRam.bin", true))                       return false; //
    fwrite(m_bramP, m_rowBytes, m_bramRows, g_printFileP);                       //
    CloseTheFile();                                                             //
#if 0                                                                           //
    if (!CreateAfile("firstRow.data", true))                      return false; //
    for (u8P=m_bramP, ii=m_rowBytes; ii != 0; )                                  //generate hex in
        fprintf(g_printFileP, "%02X", u8P[--ii] & 0xFF);                        //  MSB first order
    fprintf(g_printFileP, "\n");                                                //double lf
    CloseTheFile();                                                             //
#endif
    return true;                                                                //
   } //cGenerateVerilog::GenFileBlockRam_data...

void cGenerateVerilog::DisplayBlockram(void)
   {uint8_t   *u8P;                                                             //
    uint32_t   bramAdr, ii, type, align;                                        //
    char       ch;                                                              //
    Printf("-------- BRAM content: alignment('-') and unused('..') bytes=0x00," //
           " B=hBOOK, C=hPAGE, X=hINDX, and F=configuration. --------\n");      //
    for (bramAdr=0; bramAdr < p.fre1st; bramAdr++)                              //
        {if (bramAdr == m_params.cfgAdr){ch = 'F'; type = ITEM_CFG; }   else    //
         if (bramAdr < p.p1st)          {ch = 'B'; type = ITEM_BOOK;}   else    //
         if (bramAdr < p.i1st)          {ch = 'P'; type = ITEM_PAGE;}   else    //
                                        {ch = 'X'; type = ITEM_INDX;};          //
         u8P   = &m_bramP[bramAdr * m_rowBytes];                                //
         align = m_align[type];                                                 //
         Printf("bram[%02d]%c:",  bramAdr, ch);                                 //
         for (ii=0; ii < m_rowBytes; ii++)                                      //
             {if (ii < m_align[type] && bramAdr != 0) Printf("- ");        else //
              if (ii >= m_extentP[bramAdr]) Printf("..");                  else //
              if (type == ITEM_CFG)         Printf("%02X", u8P[ii] & 0xFF);else //
              if ((ii % m_padSz[type]) < align) Printf("- ");        else   //
                                            Printf("%02X", u8P[ii] & 0xFF);     //
              if (ii == (m_rowBytes-1)) {Printf("\n"); break;}                   //
              if ((ii&63) == 63){Printf("\n");                                  //
                                 if (ii >= m_extentP[bramAdr]) break;           //
                                 Printf("    +%04X:",  ii+1);                   //
                                 continue;                                      //
                                }                                               //
              if ((ii&15) == 15) Printf("  ");                            else  //
              if ((ii&7)  ==  7) Printf(" ");                             else  //
              if ((ii&3)  ==  3) Printf("_");                                   //
        }    }                                                                  //
    Printf("bram[%02d-%02d]: address pattern.\n\n", bramAdr, m_bramRows-1);     //
   } //cGenerateVerilog::DisplayBlockramData

//Generate file params.data in verilogSimDir
bool cGenerateVerilog::GenFileParams_txt(void)
   {if (!CreateAfile("params.txt", true))                         return false; //
    Printf("%04X //= recordSize\n",    g_rcdSize       );                       //
    Printf("%04X //= configAdr\n",     m_params.cfgAdr );                       //
    Printf("%04X //= indxSize\n",      m_params.iSize  );                       //
    Printf("%04X //=  indxesPerRow\n", m_params.iPerRow);                       //
    Printf("%04X //=  indxAlign\n",    m_params.iAlign );                       //
    Printf("%04X //=  indxesUsed\n",   m_params.iUsed  );                       //
    Printf("%04X //=  firstIndx\n",    m_params.i1st   );                       //
    Printf("%04X //= pageSize\n",      m_params.pSize  );                       //
    Printf("%04X //=  pagesPerRow\n",  m_params.pPerRow);                       //
    Printf("%04X //=  pageAlign\n",    m_params.pAlign );                       //
    Printf("%04X //=  pagesUsed\n",    m_params.pUsed  );                       //
    Printf("%04X //=  firstPage\n",    m_params.p1st   );                       //
    Printf("%04X //= bookSize\n",      m_params.bSize  );                       //
    Printf("%04X //=  booksPerRow\n",  m_params.bPerRow);                       //
    Printf("%04X //=  bookAlign\n",    m_params.bAlign );                       //
    Printf("%04X //=  bookUsed\n",     m_params.bUsed  );                       //
    Printf("%04X //=  firstBook\n",    m_params.b1st   );                       //
    Printf("%04X //= firstFreeAdr\n",  m_params.fre1st );                       //row address in BRAM
    Printf("%04X //= testAdr\n",       m_params.testAdr);                       //row address in BRAM
    Printf("%04X //= rcdCount\n", m_params.stopAtOpAdr);                        //stop after last record
    CloseTheFile(true);                                           return true;  //
    } //cGenerateVerilog::GenFileParams_txt...

//Write raw data to user.data containing the raw data records created by sort
bool cGenerateVerilog::GenFileUserData(void)
   {if (!CreateAfile("user.data", true))                          return false; //
    fwrite(g_dataP, m_ctlP->rcdCount, g_rcdSize, g_printFileP);                 //
    CloseTheFile(false);                                                        //
    return true;                                                                //
    } //cGenerateVerilog::GenFileUserData...

//Generates bulkScan.cmds file in VerilogSimulationDir
bool cGenerateVerilog::GenFileBulkScan(void)
   {int       ii, jj;                                                           //
    uint8_t  *u8P;                                                              //
    if (!CreateAfile("bulkScan.cmds", true))                      return false; //
    fprintf(g_printFileP, "//File created by GenerateVerilog\n");               // 
    for (ii=0; ii < (int)m_ctlP->rcdCount; ii++)                                //
        {fprintf(g_printFileP, "scan(indx) ");                                  //OP_SCAN, set INDX
         for (jj=0,u8P=RecordAdr(ii)->key; jj < (int)m_keySize; jj++)           //natural order
             fprintf(g_printFileP, "%02X", u8P[jj]);                            //
         fprintf(g_printFileP, ";\n");                                          //
        }                                                                       //
    CloseTheFile();                                                             //
    return true;                                                                //
    } //cGenerateVerilog::GenFileBulkScan...

//Generates params.data file in VerilogSimulationDir
void cGenerateVerilog::GenerateParamsStruct(void)
   {const char *fmtP;                                                           //
    m_params.recordSize  = g_rcdSize;                                           //
    m_params.stopAtOpAdr = g_sP->m_rcdCount;                                    //set stop past last record
  //m_params.pUsed       = already set                                          //
  //m_params.bUsed       = already set                                          //
    if (!m_verboseB) return;                                                    //
    //Boook.page1 and .page2 are used, however, cITEM sizes for FPGA/hardware   //
    //implementation versus hITEMs are different.                               //
    Printf("-------- File params.data --------\n");                             //
    fmtP = "  %s: size = %2d,\tfirst row =%2d,\tperRow =%2d,\trows used =%2d\n";//
    Printf("  recordSize=%d", m_params.recordSize);                             //
    Printf(fmtP, "hBOOK", p.bSize, p.b1st, p.bPerRow, p.bUsed);                 //
    Printf(fmtP, "hPAGE", p.pSize, p.p1st, p.pPerRow, p.pUsed);                 //
    Printf(fmtP, "hINDX", p.iSize, p.i1st, p.iPerRow, p.iUsed);                 //
    Printf("  bramRoms    = %02d,\tfirstFree =%02d\trecords=%d\t\n\n",          //
                               m_bramRows, m_params.fre1st, g_sP->m_rcdCount);  //
   } //cGenerateVerilog::GenerateParamsStruct...

void cGenerateVerilog::DisplayMisc(void)
   {int       ii, rcds;                                                         //
    sRECORD  *rcdP;                                                             //
    Printf("File user.data\n");                                                 //
    for (ii=0; ii < Min(16, rcds=m_ctlP->rcdCount); ii++)                       //
      {rcdP = (sRECORD*) RecordAdr(ii);                                         //
       Printf("rcd[%2d]: txt=%s, key=",ii, rcdP->txt);                          //
       PrintKey(rcdP->key); Printf("\n");                                       //
      }                                                                         //
    Printf("... etc, (%d records).\n\n", rcds);                                 //
    Printf("-------- File bulkScan.cmds --------\n");                           //
    for (ii=0; ii < Min(64, (int)m_ctlP->rcdCount); ii++)                       //
        {rcdP = (sRECORD*) RecordAdr(ii);                                       //
         if ((ii& 3) == 0) Printf("%2d:",  ii); else Printf(", ");              //
         PrintKey(rcdP->key);                                                   //
         Printf("%s", (ii & 3) == 3 ? "\n" : "");                               //
        }                                                                       //
    Printf("%s... etc, (%d records).\n", (ii & 3) != 0 ? "\n" : "", rcds);      //
   } //cGenerateVerilog::DisplayMisc...

bool IsAlpha(char ch) {return ch == '_' || ((ch|0x20) >= 'a' && (ch|0x20) <= 'z');}

//find name = value in srcP
int cGenerateVerilog::GetParam(const char *srcP, const char *paramNameP)
   {const char *pp;                                                             //
    for (pp=srcP; (pp=strstr(pp, paramNameP)) != NULL; pp++)                    //
       if (!IsAlpha(*(pp-1)) && !IsAlpha(pp[strlen(paramNameP)]))               //name not embedded in larger name
           {pp += strlen(paramNameP);                                           //step over name
            pp += strspn(pp, " =\t");                                           //step over whitespace and '='
            if (*pp >= '0' && *pp < '9') return Anumber((char*)pp, NULL);       //grab that value
            if ((pp=strstr(pp, "//==")) == NULL) return 0;                      //kludge for hROW_ADR_BITS
            return Anumber(4+(char*)pp, NULL);
           }                                                                    //
    return 0;                                                                   //
   } //cGenerateVerilog::GetParam...

//Generate data {itemP, cnt} for item of specified type.
//Print user friendly text and gather raw hex for subsequent generation of blockram.data
void cGenerateVerilog::ShowItem(uint8_t *itemP, eITEM_TYPE type)
   {int      ii=0, jj=0, offset, baseSz=m_baseSz[type];                         //
    uint8_t *keyP=itemP + baseSz;                                               //key within cITEM
    if (!m_verboseB) return;                                                    //
    offset  = (int)(((uint8_t*)itemP) - m_bramP);                               //reladr of cITEM in row
    offset &= (m_rowBytes -1);                                                   //mask off adr within row
    Printf(" align %d*0x00",  m_align[type]);                                   //
    Printf("[byte %03d:%03d]:",  offset, (baseSz+m_keySize+offset)-1);          //bytes within row
    Printf("%s = {", m_itemName[type]);                                         //
    PrintHex(itemP, 0, baseSz, false); itemP += baseSz;                         //
    PrintKey(keyP); Printf("}\n");                                              //
   } //cGenerateVerilog::ShowItem...

//Generate hex of (u8P[ii], sz) with due care for spaces and newlines.
int cGenerateVerilog::PrintHex(uint8_t *u8P, int ii, int sz, bool adrB)
    {Printf("   ");
     for (int kk=0; kk < sz; kk++)                                              //
        {if (adrB && (kk & 31) == 0) Printf("%03X:",  kk);                      //
         Printf("%02X%s", u8P[ii++] & 0xFF, (kk & 31) == 31 ? "\n   " :         //
                                            (kk & 7)  ==  7 ? "  "     :        //
                                            (kk & 3)  ==  3 ? "_"      : "");   //
        }                                                                       //
     return ii;                                                                 //
    } //cGenerateVerilog::PrintHex...

//Display key, ercognizing that it may be burried with other data.
void cGenerateVerilog::PrintKey(uint8_t *keyP)
   {Printf(" \"");                                                              //
    for (int kk=0; kk < (int)m_keySize; kk++) Printf("%c",*keyP++ & 0xFF);      //
    Printf("\"");                                                               //
   } //cGenerateVerilog::PrintKey...

//Create a file in the verilog source dir
bool cGenerateVerilog::CreateAfile(const char *fileOnlyP, bool simDirB)
   {SNPRINTF(m_fileName), "%s\\%s",                                             //
      getenv(simDirB ? "VerilogSimulationDir" : "VerilogSourceDir"), fileOnlyP);//
    if (!(g_printFileP=fopen(m_fileName, "wb")))                                //
       {Printf("*** Error creating %s\n", m_fileName); return false;}           //
    return true;                                                                //
   } //cGenerateVerilog::CreateAFile...

//Close the aforesaid file
void cGenerateVerilog::CloseTheFile(bool showB)
   {ClosePrintFile();                                                           //
    if (showB) Printy(CONDITIONAL_CRLF "Generated %s\n\n", m_fileName);         //
   } //cGenerateVerilog::CloseTheFile...

bool cGenerateVerilog::ShowError(const char *msgP)
   {ClosePrintFile(); Printf(msgP, m_fileName); return false;}

//NOTE:At present the software only uses cBOOK.page1 (g_doubleBookB = false).
//There are good reasons to keep it this way even tho it differs from the hardware.
//ReformatBook() packs m_bookP into page1/page2 format to be consistent with the
//FPGA/hardware implementation.
//ReformatBook packs pairs of m_bookP entries into a single hBOOK entry:
//  (book[0], book[1]) mapped to (book[0].page1, book[0].page2),
//  (book[2], book[3]) mapped to (book[1].page1, book[1].page2)
//  (book[4], book[5]), etc.
//It also packs page1 full and leaves any overflow in page2
// book[n].page1 is augmented from book[n+1].page1 in order to fill page1.
//The remainder of book[n+1] is stored in book[n/2].page2.
//book[n, n+1].page1 are recycled in the loop.
void cGenerateVerilog::ReformatBook(void)
   {int      sbuk, dbuk, bUsed, ii, rr;                                         //
    cBOOK   *sbukP, *sbuk1P, *dbukP;                                            //arrays of cBOOK entries
    cPAGE   *x1P, *x2P;                                                         //
    cBOOK   *tbookP = (cBOOK*)alloca(cBOOK_size);                               //
                                                                                //
    if (false)                                                                  //
        g_bugP->BugToFile("beforeReformat.samdmp", 2);                          //
    if ((bUsed=g_sP->m_bUsed) <= 1) return;                                     //only one page - nuthing to do
    x1P = (cPAGE*)g_sP->AllocateRow("",0,0);                                    //transitional pages; they get
    x2P = (cPAGE*)g_sP->AllocateRow("",0,0);                                    // rolled through the entire indexbase
    for (sbuk=dbuk=0; sbuk < bUsed; sbuk+=2, dbuk++)                            //two-step the source, single-step the destn
        {dbukP = cBadr(dbuk); sbukP = cBadr(sbuk); sbuk1P= cBadr(sbuk+1);       //point to all players
         tbookP->page1P = x1P; tbookP->page2P = x2P;                            //point to new cPAGE[]s
         memmove(tbookP->loKey, sbukP->loKey, m_keySize);                       //
         memmove(x1P, sbukP->page1P, m_rowBytes);                                //copy (possibly partial) array of cPAGE[]s
         if ((sbuk+1) < bUsed)                                                  //last page is fine just as it is (with x2P==0)
            {//Append sbuk1 to tbook.page1 spilling over into .page2 if reqd.   //
             ii        = cBOOK_perRow - sbukP->count;                           //ii == 0 means dbuk.page1 is full
             rr        = (sbuk1P->count - ii);                                  //rr =  howmany cPAGEs go to page2
             memmove((*x1P)[sbukP->count], sbuk1P->page1P,      ii*cBOOK_size); //append to    tbook.page1
             memmove(x2P,                (*sbuk1P->page1P)[ii], rr*cBOOK_size); //remainder to tbook.page2
             tbookP->count = sbukP->count + sbuk1P->count;                      //refigure the count
             tbookP->total = sbuk1P->total;                                     //total is already accumulated
            }                                                                   //
         else                                                                   //
            {tbookP->count = sbukP->count;                                      //last page: nothing to change
             tbookP->total = sbukP->total;                                      //
            }                                                                   //
         x1P = sbukP->page1P; x2P = sbuk1P->page1P;                             //scavange rows from sbuk[n, n+1]
         memset(sbuk1P,  0, cBOOK_size);                                        //erase un-needed (ie., odd-numbered) cBOOKs
         memmove(dbukP, tbookP, cBOOK_size);                                    //overwrite old cBOOK entry
        } //for (sbuk=...                                                       //
     FREE(x1P); FREE(x2P);                                                      //no cBOOKs gained or lost
     for (ii=dbuk; ii < (int)bUsed; ii++) memset(cBadr(ii), 0, cBOOK_size);     //clear remaining cBOOK entries
     g_sP->m_bUsed = dbuk;                                                      //... and justice for all
     g_doubleBookB = true;                                                      //
     if (false)                                                                 //
         g_bugP->BugToFile("afterReformat.samdmp", 2);                          //
    } //cGenerateVerilog::ReformatBook...

//This static member is required so that ShowAdr can be passed to debugger
void cGenerateVerilog::ShowAdr(void *adrP)
   {Printf("@bram[%02d]", (uint32_t)(((uintptr_t)adrP) - (uintptr_t)g_bramP));}

//Compare indexbase generated in bram against the original - INDX by INDX.
bool cGenerateVerilog::VerifyHbase(void)
   {uint32_t hbuk, cbuk, hpage, cpage, cinx, hinx, pageCount, icount,total=0,ll;//
    cINDX   *cP;    hINDX *hP;                                                  //
    char     buf[2*MAX_KEY_SIZE+50];                                            //
    int      len;                                                               //
    for (cbuk=hbuk=hpage=hinx=0; cbuk < g_sP->m_bUsed; cbuk++)                  //
        {for(cpage=0, pageCount=cBadr(cbuk)->count; cpage < pageCount; cpage++) //
            {icount = cBPadr(cbuk, cpage)->count;                               //
             total += icount;                                                   //
             for (cinx=0; cinx < icount; cinx++)                                //
                 {if (g_sP->CompareKey((hP=hBPIadr(cbuk, cpage, cinx))->Key,    //
                                      (cP=cBPIadr(cbuk, cpage, cinx))->key, m_keySize) == 0)//
                     continue;                                                  //
                  SNPRINTF(buf), "[%d.%d.%d] bram.key = '", cbuk, cpage, cinx); //
                  ll   = Min(sizeof(buf)-(len=istrlen(buf)), m_keySize);        //
                  memmove(&buf[len], hP->Key, ll);                              //
                  len += ll;                                                    //
                  strncpy(&buf[len], "\' != \'", Min(7, sizeof(buf) - len));    //
                  len = istrlen(buf);                                           //
                  memmove(&buf[len], cP->key, Min(sizeof(buf)-len, m_keySize)); //
                  buf[len+m_keySize] = 0;                                       //
                  Printf("**** Key mismatch\n%s'\n", buf);                      //
                  return false;                                                 //
        }   }    }                                                              //
    if (total == g_sP->m_rcdCount)                                return true;  //
    Printf("**** Total # hINDX's does not match rcdCount\n");     return false; //
   } //cGenerateVerilog::VerifyHbase...

//-------- Entry point -----------------------------------------------------
bool GenVerilogFiles(cSadram *samP, SAM_CONTROLS *ctlP)
   {cGenerateVerilog gv;
    if (ctlP->verboseB) Printf("Packing indexbase to 100%%\n");
    if (!g_sP->PackIndexes(100)) return false;
    return gv.Generate(ctlP);
   } //GenVerilogFiles...

//end of file...
