#ifndef __EICOLL_H
#define __EICOLL_H

#include "tcoll.h"

struct TEInfo
{
  int ID, lang, newFile;
  wchar_t fn[260];
  TEInfo() { ID = lang = -1; *fn = 0; newFile = 0; }
  TEInfo(int, int, const wchar_t*, int);
};

class TEICollection: public TCollection
{
  public:
    TEICollection(unsigned = 0, unsigned = 5);
    unsigned insert(int, int, const wchar_t*, int);
    unsigned removeID(int);
    int findID(int);
    int findLang(int);
    const wchar_t *findFile(int);
};
#endif
