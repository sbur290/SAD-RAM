//parameters to firmware emulation
#ifndef SAM_PARAMS_H_INCLUDED_
#define SAM_PARAMS_H_INCLUDED_
typedef struct 
   {uint16_t recordSize,                                        //+0 
             cfgAdr,                                            //+1 BRAM address of config vector
             iSize, iPerRow, iAlign, iUsed, i1st,               //+2-6  hINDX geometry
             pSize, pPerRow, pAlign, pUsed, p1st,               //+7-11 hPAGE geometry
             bSize, bPerRow, bAlign, bUsed, b1st,               //12-16 hBOOK geometry 
             fre1st,                                            //17 first unused BRAM row
             testAdr,                                           //18 debugging
             stopAtLine;                                        //19 stop simulation at this opcode address. n/u
   } sPARAMS;                                                   //count = 20
#endif SAM_PARAMS_H_INCLUDED_
