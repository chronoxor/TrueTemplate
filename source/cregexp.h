//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  You can use, modify, distribute this code or any other part
//  of colorer library in sources or in binaries only according
//  to Colorer License (see /doc/license.txt for more information).
//
#ifndef __CREGEXP__
#define __CREGEXP__
#include "clocale.h"

// with this define class uses extended command set for
// colorer compatibility mode
// if you undef it, it will compile stantard set for
// regexp compatibility mode
#define COLORERMODE

#define MATCHESNUM 0x10

enum EOps
{
  ReBlockOps = 0x100000,
  ReMul,              // *
  RePlus,             // +
  ReQuest,            // ?
  ReNGMul,            // *?
  ReNGPlus,           // +?
  ReNGQuest,          // ??
  ReRangeN,           // {n,}
  ReRangeNM,          // {n,m}
  ReNGRangeN,         // {n,}?
  ReNGRangeNM,        // {n,m}?
  ReOr,               // |
  ReBehind  = 0x110000, // ?#n
  ReNBehind = 0x120000, // ?~n
  ReAhead   = 0x130000, // ?=
  ReNAhead  = 0x140000, // ?!

  ReSymbolOps = 0x200000,
  ReEmpty,
  ReSymb,             // a b c ...
//  ReMetaSymb,         // \W \s \d ...
  ReEnum,             // []
  ReNEnum,            // [^]
  ReBrackets,         // (...)
#ifdef COLORERMODE
  ReBkTrace = 0x210000, // \yN
  ReBkTraceN= 0x220000, // \YN
#endif
  ReBkBrack = 0x230000 // \N
};

enum ESymbols
{
  ReAnyChr = 0x400000,  // .
  ReSoL,              // ^
#ifdef COLORERMODE
  ReSoScheme,         // ~
#endif
  ReEoL,              // $
  ReDigit,            // \d
  ReNDigit,           // \D
  ReWordSymb,         // \w
  ReNWordSymb,        // \W
  ReWSpace,           // \s - space, tab, cr, lf
  ReNWSpace,          // \S
  ReUCase,            // \u
  ReNUCase ,          // \l
  ReWBound,           // \b
  ReNWBound,          // \B
  RePreNW,            // \c
#ifdef COLORERMODE
  ReStart,            // \m
  ReEnd,              // \M
#endif

  ReChrLast,
  ReChr    = 0x0      // Char in Lower Byte
};
enum ETempSymb
{
  ReTemp = 0x600000,
  ReLBrack, ReRBrack,
  ReEnumS, ReEnumE, ReNEnumS,
  ReRangeS, ReRangeE, ReNGRangeE, ReFrToEnum
};
enum EError
{
  EOK = 0, EERROR, ESYNTAX, EBRACKETS, EOP
};

typedef struct SRegInfo
{
  SRegInfo();
  ~SRegInfo();

  EOps op;
  union{
    SRegInfo *param;
    int symb;
    PCharData charclass;
  }un;
  int s, e;
  int param0, param1;
  wchar_t *oldParse;
  SRegInfo *parent;
  SRegInfo *next;
  SRegInfo *prev;
} *PRegInfo;

typedef struct SMatches
{
  int s[MATCHESNUM];
  int e[MATCHESNUM];
  int CurMatch;
} *PMatches;

typedef class CRegExp
{
  bool NoCase, Extend, NoMoves, SingleLine;
//  wchar_t *toParse, *StartPos;
  wchar_t *End, *Start;
#ifdef COLORERMODE
  wchar_t *SchemeStart;
#endif
  PRegInfo Info;
  int *Exprn;
  int FirstChar;
  bool startchange, endchange;
  EError Error;
#ifdef COLORERMODE
  PMatches bktrace;
  wchar_t *bkstr;
#endif

  EError SetExprLow(wchar_t *Expr);
  EError SetStructs(PRegInfo &Info, int st, int end);
  void Optimize();
  bool CheckSymb(int Symb, wchar_t **toParse);
  bool LowParse(PRegInfo re, PRegInfo prev, wchar_t *toParse);
  bool QuickCheck(wchar_t *toParse);
  bool ParseRe(wchar_t *Str);
  PMatches Matches;
  int CurMatch;
public:
  CRegExp();
  CRegExp(wchar_t *Text);
  ~CRegExp();

  bool isok();
  EError geterror();
  bool SetNoMoves(bool Moves);
#ifdef COLORERMODE
  bool SetBkTrace(wchar_t *str, PMatches trace);
  bool GetBkTrace(wchar_t **str, PMatches *trace);
  bool getfirstchar(PCharData cd);
#endif
  bool SetExpr(wchar_t *Expr);
  bool Parse(wchar_t *str, PMatches mtch);
  bool Parse(wchar_t *str, wchar_t *sol, wchar_t *eol, PMatches mtch, wchar_t *soscheme = 0, int moves = -1);
} *PRegExp;

#endif