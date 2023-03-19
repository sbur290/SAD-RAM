#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <fcntl.h>
#include <crtdbg.h>
#include <opName.h>
#include <C3_errors.h>

cOpName::cOpName(cSamError *errP, const char *msgsP, int msgSize) 
   {m_errP          = errP;                                                     //
    m_messagesP     = (char*)msgsP;                                             //
    m_messageSize   = msgSize;                                                  //
    m_messageNum    = 0;                                                        //
   } //cOpName::cOpName...

cOpName::~cOpName()
   {free(m_messagesP); m_messagesP = NULL;
   } //cOpName::~cOpName..

//Lookup field name as used in [$reg].fieldName
int cOpName::FieldName(CC textP, int fldNum, CC *namePP)
   {static const char *names[] =                                                //
      {"",     "data", "stop",                                                  //0-2  raw and indx fields
       "P1",   "P2",   "count", "total", "stop", //preempted by names[2]        //3-7  page/book fields
       "key0", "key1", "key2",  "key3",  "key4", "key5", "key6", "key7"};       //8-15 key fields
   if (fldNum >= 0)                                                             //called to map num->name
      {*namePP = fldNum < HOWMANY(names) ? names[fldNum] : NULL;  return 0;}    //          "
   for (int ii=1; ii < HOWMANY(names); ii++)                                    //called to map name->num
       if (strnicmp(names[ii], textP, strlen(names[ii])) == 0) return ii;       //
    return m_errP->LogError(ERR_6915,"", textP);                                //6915 = Field specified is invalid
   } //cOpName::FieldName...

char *cOpName::ConditionNames(uint16_t cc)
   {static char buf[15];
    buf[0] = 0;
    if ((cc & COND_QRDY) != 0)        {strcpy(buf, "qrdy "); cc &= ~COND_QRDY;}	
    if ((cc & COND_FULL) == COND_FULL){strcat(buf, "full "); cc &= ~COND_FULL;}	//uses c & ~C simultaneously
    if ((cc & COND_CC)   != 0)         strcat(buf, "C ");   //condition bit c1= carry
    if ((cc & COND_NC)   != 0)         strcat(buf, "~C ");  //condition bit c2=~carry
    if ((cc & COND_ZZ)   != 0)         strcat(buf, "Z ");   //condition bit c3= zero
    if ((cc & COND_NZ)   != 0)         strcat(buf, "~Z ");  //condition bit c4=~zero
    if (buf[strlen(buf)-1] == ' ') buf[strlen(buf)-1] = 0;  //ridiculous
    return buf;                                             //
   } //cOpName::ConditionNames...

//return messageNum for specified (msgP, len)
int cOpName::StoreMessage(const char *msgP, int len)
   {int num=0; char *pp;                                                        //
    for (pp=m_messagesP; pp && (int)(pp-m_messagesP) < m_messageSize; num++)    //lookup msg
       {if (strncmp(pp, msgP, len) == 0 && pp[len] == 0) return num;            //found
        pp += strlen(pp)+1;                                                     //nextmessage
       }                                                                        //
   //add (msgP, len) to m_messagesP                                             //not found
   m_messagesP = (char*)realloc(m_messagesP, m_messageSize+len+1);              //
   strncpy(&m_messagesP[m_messageSize], msgP, len);                             //add to m_messagesP
   m_messagesP[(m_messageSize += len+1)-1] = 0;                                 //
   return m_messageNum++;                                                       //
  } //cOpName::StoreMessage..

//Return pointer to message #msgNum or NULL if invalid
const char *cOpName::FindMessage(int msgNum)
   {const char *pp;
    if (!(pp=m_messagesP)) return NULL;
    for (; ; msgNum--) 
         if (msgNum == 0) return pp; else pp += strlen(pp)+1;
    return NULL;
   } //cOpName::FindMessage...


char *cOpName::Show(int pc, OPCODE op, OPCODE nxt, const char *labelP, bool knownB)
    {uint16_t     op16=op.u16, act5 = op.g.act, adr = op.g.adr, uu,             //
                  bReg=op.arith.breg, aReg = op.arith.areg, subOp=op.arith.subOp;//
     #define      OUTBUF   snprintf(buf, sizeof(buf)-1,                         //
     #define      SCC static const char                                         //
     SCC *regs[] ={"$0",     "$1",    "$2",    "$3",    "$4",    "$5",          "$6",     "$7"    }; //
     SCC *sc[]   ={"PAGE",   "INDX",  "C2??",  "C3??",  "C4??",  "C5??",        "C6??",   "C7??",    //room for 16 user
                   "C8??",   "C9??",  "CA??",  "CB??",  "CC??",  "CD??",        "CE??",   "CF??"  }; //defined hITEMs
     SCC *arith[]={"ADD",    "ADC",   "SUB",   "SBB",   "CMP",   "XOR",         "OR",     "AND",     //0-7 binary ops
                   "SUBX",   "SBBX",  "INC",   "DEC",   "SHL",   "SHR",         "RCL",    "RCR",     //8-15 unary ops
                   "R2R",    "XCHG",  "XTOS,", "POP",   "PUSH",  "PUSH $currow","STC",    "CLC",     //16-23
                   "STZ",    "CLZ",   "A26??", "A27??", "A28??", "A29??",       "A30??",  "A31??" }; //24-31
     SCC *bugs[] ={"", "_PAGE", "_RAW", "_INDX"};                               //
     const  char *rw="WRYT", *si="SCAN", *rP=NULL, *aP, *msgP, *subP,           //
                 *dataRegP=regs[op.ind.breg], *addrRegP=regs[op.ind.areg];      //
     bool         goT;                                                          //
     int          offset=0, hiBit=0, len;                                       //
     static char  buf[99];                                                      //OUTBUFput buffer 
                                                                                //
     if((uu=HOWMANY(arith)) != 32) rP = "HOWMANY(arith)!=32";                   //
     if((uu=HOWMANY(regs))  !=  8) rP = "HOWMANY(regs)!= 8";                    //
     if((uu=HOWMANY(sc))    != 16) rP = "HOWMANY(sc)!=16";                      //
     if (rP) {m_errP->LogError(ERR_9995, "",  rP); exit(1);}                    //9995 = Internal software error
     buf[0] = buf[sizeof(buf)-1] = 0;                                           //
     switch (act5)                                                              //
       {default:      OUTBUF "op=0x%04X??", op16);                      break;  // ??
        case OP_CALL: if (knownB) OUTBUF "CALL %s@(%d)",labelP,op.call.callAdr);//
                      else        OUTBUF "CALL %s <unresolved>",labelP);break;  //
        case OP_WRF: case OP_RDF:                                               //
//                    hiBit = 1 << (m_bitsInOffsetField-1);                     //obsolete [$reg+offset]
//                    if ((offset = op.ind.offset) & hiBit)                     //somebody teach this guy abOUTBUF
//                         offset = -(hiBit-(offset&(hiBit-1)));                //2's complement arithmetic -- please
                      FieldName(NULL, op.ind.fieldNum, &rP);                    //
                      if (op.ind.act == OP_WRF)                                 //
                           OUTBUF "OP_WRF [%s].%s = %s", addrRegP, rP,dataRegP);//
                      else OUTBUF "OP_RDF %s = [%s].%s", dataRegP, addrRegP,rP);//
                      break;                                                    //
        case OP_CROWI:OUTBUF "OP_CROWI=%d", adr);                        break; //
        case OP_CFG_G: case OP_CFG_C:                                           //OP_CFG_C == 0x30 !!
        case OP_RET:  if (op.g.breg != 0)                                       // 
                           OUTBUF "OP_CFG(%s)", (op.g.breg&1) ?"cell" :"group");//
                      else OUTBUF "OP_RET");                             break; //
        case OP_REPREG:                                                         //
        case OP_REPEAT:if (act5 == OP_REPREG)                                   //
                            OUTBUF "OP_REPREG $%d", op.rptR.breg);              //rpt count encoded in reg.
                       else OUTBUF "OP_REPEAT %d",  op.rpt.count + 1);          //rpt count encoded as (count-1)
                       rP = op.rpt.bkwd ? ", reg--" : ", reg++";                //
                       aP = op.rpt.bkwd ? ", adr--" : ", adr++";                //
                       len= istrlen(buf);                                       //
                       snprintf(&buf[len], sizeof(buf)-len-1, "%s%s",           //
                                  op.rpt.stepR ? rP :"", op.rpt.stepA ? aP :"");//
                       break;                                                   //
        case OP_CROW:                                                           //
           OUTBUF "OP_CROW=$%d", op.ind.areg);                           break; //
        case OP_BUG:                                                            //
           if (op.bug.set && op.bug.sho == 3)                                   //
                                OUTBUF "$bug break");                else       //
           if (op.bug.set)      OUTBUF "$bugLevel=%d",op.bug.level); else       //
           if (op.bug.sho == 0) OUTBUF "OP_STOP");                   else       //
                                OUTBUF "$BUG%s", bugs[op.bug.sho]);             //
           break;                                                               //
		case OP_PRINT:                                                          //
           //op.g.breg = variations of print cmd: 1=expect, 2=actual, 3-7=nu    //
           if (!(msgP=FindMessage(op.go.relAdr)))                               //
              {OUTBUF "OP_PRINT msg#%d", op.go.relAdr); break;}                 //what !! cannot happen :(
           switch(op.g.breg)                                                    //
             {case 1: OUTBUF "OP_EXPECT \""); msgP += strlen("#expect:"); break;//
              case 2: OUTBUF "OP_ACTUAL \""); msgP += strlen("#actual:"); break;//
              default:OUTBUF "OP_PRINT \"");                              break;//
             }                                                                  //
           for (len=istrlen(buf); (len < sizeof(buf)-2 && *msgP); msgP++)       //
              len += DisplayChar(&buf[len], *msgP);                             //crush out ctrl chars
           buf[len++] = '\"'; buf[len] = 0;                                     //
           break;                                                               //
        case OP_RI: OUTBUF "%s = 0x%X", regs[op.ri.breg], op.ri.imm);    break; // $reg = literal
        case OP_LDI+ 0: case OP_LDI+ 4:case OP_LDI+ 8:case OP_LDI+12:           //
        case OP_LDI+16: case OP_LDI+20:case OP_LDI+24:case OP_LDI+28:           //
             OUTBUF "OP_LDI $0 0x%04X", op.ldi.imm);                     break; //
        case OP_READ: OUTBUF "OP_READ $%d,[%d]", op.g.breg,op.g.adr);    break; //read specifies word address
        case OP_WRYT: OUTBUF "OP_WRYT[%d], $%d", op.g.adr,op.g.breg);    break; //wryt specify word address and register
        case OP_SCIN: si = "SCIN"; //fall thru                                  //
        case OP_SCAN: OUTBUF "OP_%s(%s)", si, sc[op.sc.rowType]);        break; //
        case OP_ARITH:subP = arith[subOp];
                      switch(subOp)                                             //
                        {case OPS_R2R:                                          //
                           OUTBUF "$%d = $%d",    bReg, aReg);            break;//
                         case OPS_XCHG:                                         //
                           OUTBUF "OP_%s $%d,$%d",subP, bReg, aReg);      break;//
                         case OPS_XTOS:                                         //
                           OUTBUF "OP_XTOS $%d", aReg);                   break;//<<< areg ???
                         case OPS_PUSH_CURROW:                                  //
                           OUTBUF "OP_%s", subP);                         break;//
                         case OPS_CMP: //case OPS_TEST:                         //
                           OUTBUF "$%d %s $%d", aReg, subP, bReg);        break;//binary OP
                         default:                                               //
                           if (subOp < OPS_INC)                                 //
                                OUTBUF "$%d = $%d %s $%d", aReg,aReg,subP,bReg);//binary OP
                           else OUTBUF "OP_%s $%d", subP, bReg);                //unary op
                        } //switch (subOp)...                                   //
                      break;                                                    //
        case OP_GO_T: case OP_GO_T+8: case OP_GO_T+16: case OP_GO_T+24:         //
        case OP_GO_F: case OP_GO_F+8: case OP_GO_F+16: case OP_GO_F+24:         //
            if (labelP == NULL) labelP = GetTgtLabel(adr);                      //
            if (labelP == NULL || *labelP == '>') labelP = "";                  //
            goT = op.go.act == OP_GO_T;                                         //
            if (op.go.cond == 0 && !goT && adr == 0) {OUTBUF "NOOP");}          //go_f(0) is noop
            else                                                                //
               {if (op.go.cond == 0 &&  goT)                                    //
                    OUTBUF "OP_GO_%c %s", op.go.relAdr ? 'T' : 'L', labelP);    //go_t(0) is unconditional goto
                else                                                            //
                    OUTBUF "OP_GO_%c(%s) %s", goT ? 'T' : 'F',                  //otherwise % goto
                                 ConditionNames(op.go.cond), labelP);           //
                if (knownB) {adr = IsLongJmp(op) ? nxt.u16 : pc+1+op.go.relAdr; //
                             CAT_THIS(buf), " @(%d)", adr);                     //address claimed to be known
                            }                                                   //
                else        {CAT_THIS(buf), " <unresolved>");}                  //well,.. at least you are an honest fellow
               }                                                                //
            buf[sizeof(buf)-1] = 0;                                             //bullet proofing
       }                                                                        //
     return buf;                                                                //
#undef OUTBUF                                                                   //
#undef SCC                                                                      //
    } //cOpName::Show...