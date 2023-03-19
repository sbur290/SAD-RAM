#include <stdarg.h>
#include <malloc.h>
#include "sam-15.h"
#include "samApp.h"
#include "samApp.h"
#include "samBug.h"
#include "sam-15.h"
#include "..\include-15\c3_errors.h"
#include <samParams.h>
#include <hStructures.h>

#undef  Error     //undo definition in sam-15.h
#undef  ErrorN    //undo definition in sam-15.h
#define Error(erC, context, param) g_sP->_Error(erC,  context, param, __FILE__, __LINE__, __FUNCTION__)
#define ErrorN(erC, p1)            g_sP->_ErrorN(erC, p1,             __FILE__, __LINE__, __FUNCTION__)

class cGenerateVerilog
   {private:
    uint32_t      m_targetBusSz,                                                //size of target bus
                  m_bramRows,                                                   //extracted from samDefines.sv
                  m_rowBytes, m_keySize,                                        //copies from cSadram
                  m_baseSz[4], m_sizes[4], m_align[4], m_padSz[4], m_perRow[4]; //          "
    char          m_fileName[_MAX_PATH];                                        //
    uint32_t     *m_extentP;                                                    //bytes used in each row
    uint8_t      *m_bramP;                                                      //image of blockRam.bin in memory
    sPARAMS       m_params;                                                     //parameters for sim.exe
    SAM_CONTROLS *m_ctlP;                                                       //backlinks to cSadram
    bool          m_verboseB;                                                   //copied fomr cSadram
    public:                                                                     //
    cGenerateVerilog              ();                                           //constructor
   ~cGenerateVerilog              () {}                                         //destructor
    static void ShowAdr           (void *adrP);                                 //custom display routine for g_bugP->Bug()
    bool     Generate             (SAM_CONTROLS *ctlP);                         //Generate all data blocks
    private:                                                                    //
    const char    *m_itemName[4];                                               //
    cSamError      m_err;                                                       //error reporting function
    bool     ComputeSizes         (void);                                       //
    void     CloseTheFile         (bool showB=true);                            //Close said file
    bool     CreateAfile          (const char *fileOnlyP, bool simDirB=false);  //Create an output file.
    void     DisplayBlockram      (void);                                       //
    void     DisplayMisc          (void);                                       //
    void     DisplayParams        (void);                                       //
    uint32_t DivUp                (uint32_t a, uint32_t b) {return (a+b-1)/b;}  //
    uint32_t GenerateCfg          (void);                                       //
    void     GenerateBook         (void);                                       //
    void     GenerateIndx         (void);                                       //
    void     GeneratePage         (void);                                       //
    void     GenerateAdrPattern   (void);                                       //
    void     GenerateParamsStruct (void);                                       //
    bool     GenFileBlockRam_data (void);                                       //
    bool     GenFileBulkScan      (void);                                       //
    bool     GenFileGroupSmear_sv (void);                                       //
    bool     GenFileParams_txt    (void);                                       //
    bool     GenFileReadWord_sv   (void);                                       //
    bool     GenFileUserData      (void);                                       //
    bool     GenFileWriteWord_sv  (void);                                       //
    bool     GetVerilogParams     (SAM_CONTROLS *ctlP);                         //get parameters from samDefines.sv
    int      GetParam             (const char *srcP, const char *paramNameP);   //read parameter from samDefines.sv
    hBOOK   *hBadr                (uint32_t buk);                               //
    hPAGE   *hBPadr               (uint32_t buk, uint32_t page);                //
    hINDX   *hBPIadr              (uint32_t buk, uint32_t page, uint32_t inx);  //
    void     MoveKey              (uint8_t *d, uint8_t *s);                     //
    int      PrintHex             (uint8_t *u8P,int ii,int sz,bool adrB=false); //
    void     PrintKey             (uint8_t *keyP);                              //
    int      ReadAllFile          (const char *fileNameP, char **dataPP);       //
    void     RecomputeLoKeys      (void);                                       //
    void     ReformatBook         (void);                                       //
    void     ShowItem             (uint8_t *u8P, eITEM_TYPE type);              //
    bool     ShowError            (const char *msgP);                           //
    bool     VerifyHbase          (void);                                       //
   }; //class cGenerateVerilog...

