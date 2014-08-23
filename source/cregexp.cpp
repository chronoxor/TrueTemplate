//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  You can use, modify, distribute this code or any other part
//  of colorer library in sources or in binaries only according
//  to Colorer License (see /doc/license.txt for more information).
//

//  Updated by Alexander Nazarenko AKA /CorWin
#include "cregexp.h"
#include "winmem.h"

int GetNumber(int *str, int s, int e)
{
int r = 1, num = 0;
  if (e < s) return -1;
  for(int i = e-1; i >= s; i--){
    if (str[i] > L'9' || str[i] < L'0') return -1;
    num += (str[i] - 0x30)*r;
    r *= 10;
  };
  return num;
};
int GetHex(wchar_t c)
{
  c = CLocale::cl_lowcase(c);
  c -= L'\x30';
  if (c >= L'\x31' && c <= L'\x36') c -= L'\x27';
  else if (c < L'\x00' || c > L'\x9') return -1;
  return c;
};

/////////////////////////////////////////////////////////////////////////////
//
SRegInfo::SRegInfo()
{
  next = prev = parent = 0;
  un.param = 0;
  op = ReEmpty;
  param0 = param1 = 0;
};
SRegInfo::~SRegInfo()
{
  if (next) delete next;
  if (un.param)
    switch(op){
      case ReEnum:
      case ReNEnum:
        delete un.charclass;
        break;
      default:
        if (op > ReBlockOps && op < ReSymbolOps || op == ReBrackets)
          delete un.param;
        break;
    };
};

////////////////////////////////////////////////////////////////////////////
// regexp class
CRegExp::CRegExp()
{
  Info = 0;
  Exprn = 0;
  NoMoves = false;
  Error = EERROR;
  FirstChar = 0;
  CurMatch = 0;
};
CRegExp::CRegExp(wchar_t *Text)
{
  Info = 0;
  Exprn = 0;
  NoMoves = false;
  Error = EERROR;
  FirstChar = 0;
  CurMatch = 0;
  if (Text) SetExpr(Text);
};
CRegExp::~CRegExp()
{
  if (Info) delete Info;
};

bool CRegExp::SetExpr(wchar_t *Expr)
{
  if (!this) return false;
  Error = EERROR;
  CurMatch = 0;
  Error = SetExprLow(Expr);
  return Error == EOK;
};
bool CRegExp::isok()
{
  return Error == EOK;
};
EError CRegExp::geterror()
{
  return Error;
};

EError CRegExp::SetExprLow(wchar_t *Expr)
{
int  EnterBr = 0, EnterGr = 0, EnterFg = 0;
int  pos, tmp, i, j, s = 0;
bool Ok = false;
int  Len = 0;

  while (Expr[Len]) Len++;
  if (!Len) return EERROR;

  if (Info) delete Info;
  Info = new SRegInfo;
  Exprn = new int[Len];

  endchange = startchange = false;
  NoCase = false;
  Extend = false;
  SingleLine = false;
  if (Expr && Expr[0] == L'/') s++;
  else return ESYNTAX;

  for (i = Len; i > 0 && !Ok;i--)
    if (Expr[i] == L'/'){
      Len = i-s;
      Ok = true;
      for (j = i+1; Expr[j]; j++){
        if (Expr[j] == L'i') NoCase = true;
        if (Expr[j] == L'x') Extend = true;
        if (Expr[j] == L's') SingleLine = true;
      };
    };
  if (!Ok) return ESYNTAX;

  // precompiling
  for (j = 0, pos = 0; j < Len; j++,pos++){
    if (Extend && (Expr[j+s] == L' ' || Expr[j+s] == L'\n'|| Expr[j+s] == L'\r')){
      pos--;
      continue;
    };
    Exprn[pos] = (int)Expr[j+s];
    if (Expr[j+s] == L'\\'){
      switch (Expr[j+s+1]){
        case L'd':
          Exprn[pos] = ReDigit;
          break;
        case L'D':
          Exprn[pos] = ReNDigit;
          break;
        case L'w':
          Exprn[pos] = ReWordSymb;
          break;
        case L'W':
          Exprn[pos] = ReNWordSymb;
          break;
        case L's':
          Exprn[pos] = ReWSpace;
          break;
        case L'S':
          Exprn[pos] = ReNWSpace;
          break;
        case L'u':
          Exprn[pos] = ReUCase;
          break;
        case L'l':
          Exprn[pos] = ReNUCase;
          break;
        case L't':
          Exprn[pos] = L'\t';
          break;
        case L'n':
          Exprn[pos] = L'\n';
          break;
        case L'r':
          Exprn[pos] = L'\r';
          break;
        case L'b':
          Exprn[pos] = ReWBound;
          break;
        case L'B':
          Exprn[pos] = ReNWBound;
          break;
        case L'c':
          Exprn[pos] = RePreNW;
          break;
#ifdef COLORERMODE
        case L'm':
          Exprn[pos] = ReStart;
          break;
        case L'M':
          Exprn[pos] = ReEnd;
          break;
        case L'y':
          tmp = GetHex(Expr[j+s+2]);
          if (tmp == -1) return ESYNTAX;
          Exprn[pos] = ReBkTrace + tmp;
          j++;
          break;
        case L'Y':
          tmp = GetHex(Expr[j+s+2]);
          if (tmp == -1) return ESYNTAX;
          Exprn[pos] = ReBkTraceN + tmp;
          j++;
          break;
#endif
        case L'x':
          tmp = GetHex(Expr[j+s+2]);
          if (tmp == -1 || GetHex(Expr[j+s+3]) == -1) return ESYNTAX;
          tmp = (tmp<<4) + GetHex(Expr[j+s+3]);
          Exprn[pos] = tmp;
          j += 2;
          break;
        default:
          tmp = GetHex(Expr[j+s+1]);
          if (tmp != -1){
            Exprn[pos] = ReBkBrack + tmp;
            break;
          }else
            Exprn[pos] = Expr[j+s+1];
          break;
      };
      j++;
      continue;
    };
    if (Expr[j+s] == L']'){
      Exprn[pos] = ReEnumE;
      if (EnterFg || !EnterGr) return EBRACKETS;
      EnterGr--;
    };
    if (Expr[j+s] == L'-' && EnterGr) Exprn[pos] = ReFrToEnum;

    if (EnterGr) continue;

    if (Expr[j+s] == L'[' && Expr[j+s+1] == L'^'){
      Exprn[pos] = ReNEnumS;
      if (EnterFg) return EBRACKETS;
      EnterGr++;
      j++;
      continue;
    };
    if (Expr[j+s] == L'*' && Expr[j+s+1] == L'?'){
      Exprn[pos] = ReNGMul;
      j++;
      continue;
    };
    if (Expr[j+s] == L'+' && Expr[j+s+1] == L'?'){
      Exprn[pos] = ReNGPlus;
      j++;
      continue;
    };
    if (Expr[j+s] == L'?' && Expr[j+s+1] == L'?'){
      Exprn[pos] = ReNGQuest;
      j++;
      continue;
    };
    if (Expr[j+s] == L'?' && Expr[j+s+1] == L'#' &&
        Expr[j+s+2]>=L'0' && Expr[j+s+2]<=L'9'){
      Exprn[pos] = ReBehind+Expr[j+s+2]-0x30;
      j+=2;
      continue;
    };
    if (Expr[j+s] == L'?' && Expr[j+s+1] == L'~' &&
        Expr[j+s+2]>=L'0' && Expr[j+s+2]<=L'9'){
      Exprn[pos] = ReNBehind+Expr[j+s+2]-0x30;
      j+=2;
      continue;
    };
    if (Expr[j+s] == L'?' && Expr[j+s+1] == L'='){
      Exprn[pos] = ReAhead;
      j++;
      continue;
    };
    if (Expr[j+s] == L'?' && Expr[j+s+1] == L'!'){
      Exprn[pos] = ReNAhead;
      j++;
      continue;
    };

    if (Expr[j+s] == L'('){
      Exprn[pos] = ReLBrack;
      if (EnterFg) return EBRACKETS;
      EnterBr++;
    };
    if (Expr[j+s] == L')'){
      Exprn[pos] = ReRBrack;
      if (!EnterBr || EnterFg) return EBRACKETS;
      EnterBr--;
    };
    if (Expr[j+s] == L'['){
      Exprn[pos] = ReEnumS;
      if (EnterFg) return EBRACKETS;
      EnterGr++;
    };
    if (Expr[j+s] == L'{'){
      Exprn[pos] = ReRangeS;
      if (EnterFg) return EBRACKETS;
      EnterFg++;
    };
    if (Expr[j+s] == L'}' && Expr[j+s+1] == L'?'){
      Exprn[pos] = ReNGRangeE;
      if (!EnterFg) return EBRACKETS;
      EnterFg--;
      j++;
      continue;
    };
    if (Expr[j+s] == L'}'){
      Exprn[pos] = ReRangeE;
      if (!EnterFg) return EBRACKETS;
      EnterFg--;
    };

    if (Expr[j+s] == L'^') Exprn[pos] = ReSoL;
#ifdef COLORERMODE
    if (Expr[j+s] == L'~') Exprn[pos] = ReSoScheme;
#endif
    if (Expr[j+s] == L'$') Exprn[pos] = ReEoL;
    if (Expr[j+s] == L'.') Exprn[pos] = ReAnyChr;
    if (Expr[j+s] == L'*') Exprn[pos] = ReMul;
    if (Expr[j+s] == L'+') Exprn[pos] = RePlus;
    if (Expr[j+s] == L'?') Exprn[pos] = ReQuest;
    if (Expr[j+s] == L'|') Exprn[pos] = ReOr;
  };

  if (EnterGr || EnterBr || EnterFg) return EBRACKETS;


  // making tree structure
  Info->op = ReBrackets;
  Info->un.param = new SRegInfo;
  Info->un.param->parent = Info;
  Info->s = CurMatch++;
  EError err = SetStructs(Info->un.param, 0, pos);

  delete Exprn;
  if (err) return err;
  Optimize();
  return EOK;
};


void CRegExp::Optimize()
{
PRegInfo Next = Info;
  FirstChar = 0;
  while(Next){
    if (Next->op == ReBrackets){
      Next = Next->un.param;
      continue;
    };
    if (Next->op == ReSymb){
      if (Next->un.symb & 0xFF00  &&  Next->un.symb != ReSoL  &&  Next->un.symb != ReWBound)
        break;
      FirstChar = Next->un.symb;
      break;
    };
    break;
  };
};

EError CRegExp::SetStructs(PRegInfo &re, int start, int end)
{
PRegInfo Next, temp;
int comma, st, en, ng, i, j, k, brnum;
bool Add;

  if (end - start < 0) return EERROR;
  Next = re;
  for(i = start; i < end; i++){
    Add = false;
    // Ops
    if (Exprn[i] > ReBlockOps && Exprn[i] < ReSymbolOps){
      Next->un.param = 0;
      Next->s = 0;
      Next->op = (EOps)Exprn[i];
      Add = true;
    };
    // {n,m}
    if (Exprn[i] == ReRangeS){
      st = i;
      en = -1;
      comma = -1;
      ng = 0;
      for (j = i;j < end;j++){
        if (Exprn[j] == ReNGRangeE){
          en = j;
          ng = 1;
          break;
        };
        if (Exprn[j] == ReRangeE){
          en = j;
          break;
        };
        if (Exprn[j] == L',')
          comma = j;
      };
      if (en == -1) return EBRACKETS;
      if (comma == -1) comma = en;
      Next->s = GetNumber(Exprn, st+1, comma);
      if (comma != en)
        Next->e = GetNumber(Exprn, comma+1, en);
      else
        Next->e = Next->s;
      Next->un.param = 0;
      Next->op = ng ? ReNGRangeNM : ReRangeNM;
      if (en - comma == 1){
        Next->e = -1;
        Next->op = ng?ReNGRangeN:ReRangeN;
      };
      i=j;
      Add = true;
    };
    // ( ... )
    if (Exprn[i] == ReLBrack){
      brnum = 1;
      for (j = i+1;j < end;j++){
        if (Exprn[j] == ReLBrack) brnum++;
        if (Exprn[j] == ReRBrack) brnum--;
        if (!brnum) break;
      };
      if (brnum) return EBRACKETS;
      Next->op = ReBrackets;
      Next->un.param = new SRegInfo;
      Next->un.param->parent = Next;
      Next->s = CurMatch;
      if (CurMatch < MATCHESNUM)
        CurMatch++;
      EError er = SetStructs(Next->un.param, i+1, j);
      if (er) return er;
      Add = true;
      i = j;
    };
#ifdef COLORERMODE
    if ((Exprn[i]&0xFF00) == ReBkTrace){
      Next->op = ReBkTrace;
      Next->un.symb = Exprn[i]&0xFF;
      Add = true;
    };
    if ((Exprn[i]&0xFF00) == ReBkTraceN){
      Next->op = ReBkTraceN;
      Next->un.symb = Exprn[i]&0xFF;
      Add = true;
    };
#endif
    if ((Exprn[i]&0xFF00) == ReBkBrack){
      Next->op = ReBkBrack;
      Next->un.symb = Exprn[i]&0xFF;
      Add = true;
    };
    if ((Exprn[i]&0xFF00) == ReBehind){
      Next->op = ReBehind;
      Next->s = Exprn[i]&0xFF;
      Add = true;
    };
    if ((Exprn[i]&0xFF00) == ReNBehind){
      Next->op = ReNBehind;
      Next->s = Exprn[i]&0xFF;
      Add = true;
    };
    // Chars
    if (Exprn[i] >= ReAnyChr && Exprn[i] < ReChrLast  ||  Exprn[i] < 0x100){
      Next->op = ReSymb;
      Next->un.symb = Exprn[i];
      Add = true;
    };
    // [] [^]
    if (Exprn[i] == ReEnumS || Exprn[i] == ReNEnumS){
      Next->op = (Exprn[i] == ReEnumS) ? ReEnum : ReNEnum;
      for (j = i+1; j < end; j++){
        if (Exprn[j] == ReEnumE)
          break;
      };
      if (j == end) return EBRACKETS;
      Next->un.charclass = new SCharData;

      for(k = 0; k < 8; k++)
        Next->un.charclass->IArr[k] = 0x0;

      for (j = i+1;Exprn[j] != ReEnumE;j++){
        if (Exprn[j+1] == ReFrToEnum){
          for (i = (Exprn[j]&0xFF); i < (Exprn[j+2]&0xFF);i++)
			if (i < 256)
			  Next->un.charclass->setbit((unsigned char)(i&0xFF));
          j++;
          continue;
        };
        switch(Exprn[j]){
          case ReDigit:
            for (k = 0x30;k < 0x40;k++)
              if ((k < 256) && CLocale::cl_isdigit(k))
                Next->un.charclass->setbit((unsigned char)(k));
            break;
          case ReNDigit:
            for (k = 0x30;k < 0x40;k++)
              if ((k < 256) && !CLocale::cl_isdigit(k))
                Next->un.charclass->setbit((unsigned char)(k));
            break;
          case ReWordSymb:
            for (k = 0;k < 256;k++)
              if ((k < 256) && CLocale::cl_isword(k))
                Next->un.charclass->setbit((unsigned char)(k));
            break;
          case ReNWordSymb:
            for (k = 0;k < 256;k++)
              if ((k < 256) && !CLocale::cl_isword(k))
                Next->un.charclass->setbit((unsigned char)(k));
            break;
          case ReWSpace:
            Next->un.charclass->setbit('\x20');
            // tab also!
            Next->un.charclass->setbit('\x09');
            Next->un.charclass->setbit('\x0A');
            Next->un.charclass->setbit('\x0D');
            break;
          case ReNWSpace:
            for(k = 0; k < 8; k++)
              Next->un.charclass->IArr[k] = 0xFFFFFFFF;
            Next->un.charclass->clearbit('\x20');
            Next->un.charclass->clearbit('\x09');
            Next->un.charclass->clearbit('\x0A');
            Next->un.charclass->clearbit('\x0D');
            break;
          default:
            if ((Exprn[j] < 256) && !(Exprn[j]&0xFF00))
              Next->un.charclass->setbit((unsigned char)(Exprn[j]&0xFF));
            break;
        };
      };
      Add = true;
      i = j;
    };
    // next element
    if (Add && i != end-1){
      Next->next = new SRegInfo;
      Next->next->parent = Next->parent;
      Next->next->prev = Next;
      Next = Next->next;
    };
  };

  // operators fixes
  for(Next = re; Next; Next = Next->next){
    // deletes temporary operators...
    if (Next->op == RePlus){
      Next->op = ReRangeN;
      Next->s = 1;
    };
    if (Next->op == ReNGPlus){
      Next->op = ReNGRangeN;
      Next->s = 1;
    };
    if (Next->op == ReMul){
      Next->op = ReRangeN;
      Next->s = 0;
    };
    if (Next->op == ReNGMul){
      Next->op = ReNGRangeN;
      Next->s = 0;
    };
    if (Next->op == ReQuest){
      Next->op = ReRangeNM;
      Next->s = 0;
      Next->e = 1;
    };
    if (Next->op == ReNGQuest){
      Next->op = ReNGRangeNM;
      Next->s = 0;
      Next->e = 1;
    };
    // adds empty alternative
    while (Next->op == ReOr){
      temp = new SRegInfo;
      temp->parent = Next->parent;
      // |foo|bar
      if (!Next->prev){
        temp->next = Next;
        Next->prev = temp;
        continue;
      };
      // foo||bar
      if (Next->next && Next->next->op == ReOr){
        temp->prev = Next;
        temp->next = Next->next;
        if (Next->next) Next->next->prev = temp;
        Next->next = temp;
        continue;
      };
      // foo|bar|
      if (!Next->next){
        temp->prev = Next;
        temp->next = 0;
        Next->next = temp;
        continue;
      };
      // foo|bar|*
      if (Next->next->op > ReBlockOps && Next->next->op < ReSymbolOps){
        temp->prev = Next;
        temp->next = Next->next;
        Next->next->prev = temp;
        Next->next = temp;
        continue;
      };
      delete temp;
      break;
    };
  };

  // op's generating...
  Next = re;
  while(Next){
    if (Next->op > ReBlockOps && Next->op < ReSymbolOps){
      if (!Next->prev) return EOP;
      if (!Next->prev->prev)
        re = Next;
      else
        Next->prev->prev->next = Next;
      Next->prev->parent = Next;
      Next->prev->next = 0;
      Next->un.param = Next->prev;
      Next->prev = Next->prev->prev;
    };
    Next = Next->next;
  };
  return EOK;
};


////////////////////////////////////////////////////////////////////////////
// parsing

bool CRegExp::CheckSymb(int Symb, wchar_t **toParse)
{
  switch(Symb){
    case ReAnyChr:
      if (*toParse >= End) return false;
      if (!SingleLine && (**toParse == L'\n' || **toParse == L'\r')) return false;
      (*toParse)++;
      return true;
    case ReSoL:
      return (Start == *toParse);
    case ReEoL:
      return (End == *toParse);
    case ReDigit:
      if (*toParse >= End || !CLocale::cl_isdigit(**toParse)) return false;
      (*toParse)++;
      return true;
    case ReNDigit:
      if (*toParse >= End || CLocale::cl_isdigit(**toParse)) return false;
      (*toParse)++;
      return true;
    case ReWordSymb:
      if (*toParse >= End || !CLocale::cl_isword(**toParse)) return false;
      (*toParse)++;
      return true;
    case ReNWordSymb:
      if (*toParse >= End || CLocale::cl_isword(**toParse)) return false;
      (*toParse)++;
      return true;
    case ReWSpace:
      if (*toParse >= End ||
          !(**toParse == 0x20 || **toParse == L'\t' || **toParse == L'\r' || **toParse == L'\n')) return false;
      (*toParse)++;
      return true;
    case ReNWSpace:
      if (*toParse >= End ||
          (**toParse == 0x20 || **toParse == L'\t' || **toParse == L'\r' || **toParse == L'\n')) return false;
      (*toParse)++;
      return true;
    case ReUCase:
      if (*toParse >= End || !CLocale::cl_isuppercase(**toParse)) return false;
      (*toParse)++;
      return true;
    case ReNUCase:
      if (*toParse >= End || !CLocale::cl_islowercase(**toParse)) return false;
      (*toParse)++;
      return true;
    case ReWBound:
      if (*toParse >= End) return true;
      return CLocale::cl_isword(**toParse) && (*toParse == Start || !CLocale::cl_isword(*(*toParse - 1)));
    case ReNWBound:
      if (*toParse >= End) return true;
      return !CLocale::cl_isword(**toParse) && CLocale::cl_isword(*(*toParse - 1));
    case RePreNW:
      if (*toParse >= End) return true;
      return (*toParse == Start || !CLocale::cl_isword(*(*toParse-1)));
#ifdef COLORERMODE
    case ReSoScheme:
      return (SchemeStart == *toParse);
    case ReStart:
      Matches->s[0] = (int)(*toParse - Start);
      startchange = true;
      return true;
    case ReEnd:
      Matches->e[0] = (int)(*toParse - Start);
      endchange = true;
      return true;
#endif
    default:
      if ((Symb & 0xFF00) || *toParse >= End) return false;
      if (NoCase){
        if (CLocale::cl_lowcase(**toParse) != CLocale::cl_lowcase(Symb))
          return false;
      }else
        if (**toParse != Symb) return false;
      (*toParse)++;
      return true;
  };
}

bool CRegExp::LowParse(PRegInfo re, PRegInfo prev, wchar_t *toParse)
{
int i, sv;
bool leftenter = true;

  if (!re){
    re = prev->parent;
    leftenter = false;
  };

  while(re){
    switch(re->op){
      case ReEmpty:
        break;
      case ReBrackets:
        if (leftenter){
          re->param0 = (int)(toParse - Start);
          re = re->un.param;
          leftenter = true;
          continue;
        };
        if (re->s >= MATCHESNUM) break;
        if (re->s || !startchange)
          Matches->s[re->s] = re->param0;
        if (re->s || !endchange)
          Matches->e[re->s] = (int)(toParse - Start);
        break;
      case ReSymb:
        if (!CheckSymb(re->un.symb, &toParse)) return false;
        break;
      case ReEnum:
        if (toParse >= End) return false;
        if ((*toParse < 256) && !re->un.charclass->getbit((unsigned char)*toParse)) return false;
        toParse++;
        break;
      case ReNEnum:
        if (toParse >= End) return false;
        if ((*toParse < 256) && re->un.charclass->getbit((unsigned char)*toParse)) return false;
        toParse++;
        break;
#ifdef COLORERMODE
      case ReBkTrace:
        if (!bkstr | !bktrace) return false;
        sv = re->un.symb;
        for (i = bktrace->s[sv]; i < bktrace->e[sv]; i++){
          if (*toParse != bkstr[i] || End == toParse) return false;
          toParse++;
        };
        break;
      case ReBkTraceN:
        if (!bkstr | !bktrace) return false;
        sv = re->un.symb;
        for (i = bktrace->s[sv]; i < bktrace->e[sv]; i++){
          if (CLocale::cl_lowcase(*toParse) != CLocale::cl_lowcase(bkstr[i]) || End == toParse) return false;
          toParse++;
        };
        break;
#endif
      case ReBkBrack:
        sv = re->un.symb;
        if (Matches->s[sv] == -1 || Matches->e[sv] == -1) return false;
        for (i = Matches->s[sv]; i < Matches->e[sv]; i++){
          if (*toParse != Start[i] || End == toParse) return false;
          toParse++;
        };
        break;
      case ReAhead:
        if (!leftenter) return true;
        if (!LowParse(re->un.param, 0, toParse)) return false;
        break;
      case ReNAhead:
        if (!leftenter) return true;
        if (LowParse(re->un.param, 0, toParse)) return false;
        break;
      case ReBehind:
        if (!leftenter) return true;
        if (!LowParse(re->un.param, 0, toParse - re->s)) return false;
        break;
      case ReNBehind:
        if (!leftenter) return true;
        if (LowParse(re->un.param, 0, toParse - re->s)) return false;
        break;

      case ReOr:
        if (!leftenter){
          while (re && re->op == ReOr)
            re = re->next;
          break;
        };
        if (LowParse(re->un.param, 0, toParse)) return true;
        break;

      case ReRangeN:
        // first enter into op
        if (leftenter){
          re->param0 = re->s;
          re->oldParse = 0;
        };
        if (!re->param0 && re->oldParse == toParse) break;
        re->oldParse = toParse;
        // making branch
        if (!re->param0){
          if (!LowParse(re->un.param, 0, toParse)){
            if (LowParse(re->next, re, toParse))
              return true;
          }else return true;
          return false;
        };
        // go into
        if (re->param0) re->param0--;
        re = re->un.param;
        leftenter = true;
        continue;
      case ReRangeNM:
        if (leftenter){
          re->param0 = re->s;
          re->param1 = re->e - re->s;
          re->oldParse = 0;
        };
        //
        if (!re->param0){
          if (re->param1) re->param1--;
          else return LowParse(re->next, re, toParse);
          if (LowParse(re->un.param, 0, toParse)) return true;
          if (LowParse(re->next, re, toParse)) return true;
          re->param1++;
          return false;
        };
        if (re->param0) re->param0--;
        re = re->un.param;
        leftenter = true;
        continue;
      case ReNGRangeN:
        if (leftenter){
          re->param0 = re->s;
          re->oldParse = 0;
        };
        if (!re->param0 && re->oldParse == toParse) break;
        re->oldParse = toParse;
        //
        if (!re->param0 && LowParse(re->next, re, toParse)) return true;
        if (re->param0) re->param0--;
        re = re->un.param;
        leftenter = true;
        continue;
      case ReNGRangeNM:
        if (leftenter){
          re->param0 = re->s;
          re->param1 = re->e - re->s;
          re->oldParse = 0;
        };
        //
        if (!re->param0){
          if (re->param1) re->param1--;
          else return LowParse(re->next, re, toParse);
          if (LowParse(re->next, re, toParse)) return true;
          if (LowParse(re->un.param, 0, toParse))
            return true;
          else{
            re->param1++;
            return false;
          };
        };
        if (re->param0) re->param0--;
        re = re->un.param;
        leftenter = true;
        continue;
    };
    if (!re->next){
      re = re->parent;
      leftenter = false;
    }else{
      re = re->next;
      leftenter = true;
    };
  };
  return true;
};

bool CRegExp::QuickCheck(wchar_t *toParse)
{
  switch(FirstChar){
    case ReSoL:
      if (toParse != Start) return false;
      return true;
#ifdef COLORERMODE
    case ReSoScheme:
      if (toParse != SchemeStart) return false;
      return true;
#endif
    case ReWBound:
      return CLocale::cl_isword(*toParse) && (toParse == Start || !CLocale::cl_isword(*(toParse-1)));
    default:
      if (NoCase){
        if (CLocale::cl_lowcase(*toParse) != CLocale::cl_lowcase(FirstChar)) return false;
      }else
        if (*toParse != FirstChar) return false;
      return true;
  };
};

bool CRegExp::ParseRe(wchar_t *Str)
{
  if (Error) return false;

  wchar_t *toParse = Str;

  if (NoMoves && FirstChar && !QuickCheck(toParse))
    return false;

  for (int i = 0; i < CurMatch; i++)
    Matches->s[i] = Matches->e[i] = -1;
  Matches->CurMatch = CurMatch;

  do{
    if (!LowParse(Info, 0, toParse)){
      if (NoMoves) return false;
    }else
      return true;
    toParse = ++Str;
  }while(toParse < End+1);
  return false;
};

bool CRegExp::Parse(wchar_t *str, wchar_t *sol, wchar_t *eol, PMatches mtch, wchar_t *soscheme, int moves)
{
//  if (!this) return false;
  bool nms = NoMoves;
  if (moves != -1) NoMoves = (moves != 0);
  Start = sol;
#ifdef COLORERMODE
  SchemeStart = soscheme;
#endif
  End = eol;
  Matches = mtch;
  bool result =  ParseRe(str);
  NoMoves = nms;
  return result;
};

bool CRegExp::Parse(wchar_t *Str, PMatches Mtch)
{
//  if (!this) return false;
  End = Start = Str;
#ifdef COLORERMODE
  SchemeStart = Str;
#endif
  while(*End) End++;
  Matches = Mtch;
  return ParseRe(Str);
};

bool CRegExp::SetNoMoves(bool Moves)
{
  NoMoves = Moves;
  return true;
};

#ifdef COLORERMODE
bool CRegExp::getfirstchar(PCharData cd)
{
  for(PRegInfo Next = Info; Next; ){
    if (Next->op == RePlus  || Next->op == ReNGPlus || Next->op == ReBrackets){
      Next = Next->un.param;
      continue;
    };
    if (Next->op == ReEnum){
      *cd = *Next->un.charclass;
      return true;
    };
    if (Next->op == ReNEnum){
      *cd = *Next->un.charclass;
      for(int k = 0; k < 8; k++)
        cd->IArr[k] = ~cd->IArr[k];
      return true;
    };
    break;
  };
 return false;
};
bool CRegExp::SetBkTrace(wchar_t *str, PMatches trace)
{
  bktrace = trace;
  bkstr = str;
  return true;
};
bool CRegExp::GetBkTrace(wchar_t **str, PMatches *trace)
{
  *str = bkstr;
  *trace = bktrace;
  return true;
};
#endif
