uint32_t          cBOOK_size,   cPAGE_size,   cINDX_size,                       //Static fields: Do not want these
                  cBOOK_padSz,  cPAGE_padSz,  cINDX_padSz,                      //Static fields: Do not want these
                  cBOOK_perRow, cPAGE_perRow, cINDX_perRow,                     //in every instance of structure.
                  hBOOK_size,   hPAGE_size,   hINDX_size,                       //sizes of hardware structures
                  hBOOK_padSz,  hPAGE_padSz,  hINDX_padSz,                      //sizes of hardware structures
                  hBOOK_perRow, hPAGE_perRow, hINDX_perRow,                     //           "
                  hINDX_align=2,hPAGE_align=2,hBOOK_align=6;                    //manual computations for targetBus == 8

//Compute the padded size of cITEMS and the alignment.
//Unused cells at beginning of a row require to align first byte of
//the key with target[0]. The cITEMs are then spaced out so that the
//second and subsequent banks will also be properly aligned.
//<hITEM>_BASE is the size of the hardware structure without the user key.
//Calculate number of bytes required to align the key part of an item with targetBus[0].
//This is required so that compare will work.
int whatClass::Align2TargetBus(int posn, int itemBytes)
    {int ii;                                                       //
     for (ii=0; ((ii+posn+itemBytes) % m_targetBusSz) != 0;)ii++;  //boy is that lazy coding
     return ii;                                                    //
    } //whatClass::Align2TargetBus...

//round size up to size of target bus
uint32_t whatClass::RoundupToTargetbus(uint32_t size)
   {return ((size + m_targetBusSz-1)/m_targetBusSz) * m_targetBusSz;
   } //whatClass::RoundupToTargetbus...

int whatClass::ComputeGeometry(int keySize)
   {int ii; const char *pp=NULL;                                                //
    m_baseSz[ITEM_CFG ] = 0;          m_baseSz[ITEM_INDX] = hINDX_BASE;         //
    m_baseSz[ITEM_PAGE] = hPAGE_BASE; m_baseSz[ITEM_BOOK] = hBOOK_BASE;         //sizes without key
    //unused cells are reqd at the start of a row to align key[0] with target[0]//
                  m_align[ITEM_CFG]  = 0;                                       //
    hBOOK_align = m_align[ITEM_BOOK] = Align2TargetBus(0, hBOOK_BASE);          //initial alignment for hBOOK
    hPAGE_align = m_align[ITEM_PAGE] = Align2TargetBus(0, hPAGE_BASE);          //           "      for hPAGE
    hINDX_align = m_align[ITEM_INDX] = Align2TargetBus(0, hINDX_BASE);          //           "      for hINDX
    for (ii=ITEM_INDX; ii <= ITEM_BOOK; ii++)                                   //roundup size (including key)
        {m_sizes[ii]  = m_baseSz[ii] + keySize;                                 //
         m_padSz[ii]  = RoundupToTargetbus(m_sizes[ii]);                        //
         m_perRow[ii] = m_rowBytes / m_padSz[ii];                               //
        }                                                                       //
    //Store these constants in global variables                                 //
    hINDX_perRow = m_perRow[ITEM_INDX]; hINDX_padSz = m_padSz[ITEM_INDX]; hINDX_size = m_sizes[ITEM_INDX];//
    hPAGE_perRow = m_perRow[ITEM_PAGE]; hPAGE_padSz = m_padSz[ITEM_PAGE]; hPAGE_size = m_sizes[ITEM_PAGE];//
    hBOOK_perRow = m_perRow[ITEM_BOOK]; hBOOK_padSz = m_padSz[ITEM_BOOK]; hBOOK_size = m_sizes[ITEM_BOOK];//
    //Calculate geometry of cINDX's (software simulation)                       //
    cINDX_size            = sizeof(cINDX) + keySize-1;                          //actual size of cINDX
    cINDX_padSz           = RoundupToTargetbus(cINDX_size);                     //
    cINDX_perRow          = Min(cINDX_perRow, m_rowBytes / cINDX_size);          //minimum of hardware or software requirements
    //Calculate geometry of cPAGE's                                             //
    cPAGE_size            = sizeof(cPAGE) + keySize-1;                          //actual size of cPAGE;
    cPAGE_padSz           = RoundupToTargetbus(cPAGE_size);                     //
    cPAGE_perRow          = Min(cPAGE_perRow, m_rowBytes / cPAGE_size);          //minimum of hardware or software requirements
    //Calculate geometry of cBOOK's                                             //
    cBOOK_size            = (uint32_t)sizeof(cBOOK)-1 + keySize;                //sz including actual key; cBOOK_size for operator[]
    cBOOK_padSz           = RoundupToTargetbus(cBOOK_size);                     //
    cBOOK_perRow          = Min(cBOOK_perRow, m_rowBytes / cBOOK_size);          //minimum of hardware or software requirements
                                                                                //
    hINDX_size            = sizeof(cINDX) + keySize-1;                          //actual size of cINDX
    hINDX_padSz           = RoundupToTargetbus(hINDX_size);                     //
    hINDX_perRow          = Min(hINDX_perRow, m_rowBytes / hINDX_size);         //minimum of hardware or software requirements
    //Calculate geometry of cPAGE's                                             //
    hPAGE_size            = sizeof(cPAGE) + keySize-1;                          //actual size of cPAGE;
    hPAGE_padSz           = RoundupToTargetbus(hPAGE_size);                     //
    hPAGE_perRow          = Min(hPAGE_perRow, m_rowBytes / hPAGE_size);         //minimum of hardware or software requirements
    //Calculate geometry of cBOOK's                                             //
    hBOOK_size            = (uint32_t)sizeof(hBOOK)-1 + keySize;                //sz including actual key; cBOOK_size for operator[]
    hBOOK_padSz           = RoundupToTargetbus(hBOOK_size);                     //
    hBOOK_perRow          = Min(hBOOK_perRow, m_rowBytes / hBOOK_size);         //minimum of hardware or software requirements

    if (pp) return Error(ERR_9995, "", pp);                                     //
    return 0;                                                                   //
   } //whatClass::ComputeHbits...

