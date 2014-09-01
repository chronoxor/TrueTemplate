#ifndef __EICOLL_H
#define __EICOLL_H

#include "tcoll.h"

struct TEInfo
{
  intptr_t ID, lang;
  bool newFile;
  wchar_t fn[260];
  TEInfo() { ID = lang = -1; *fn = L'\0'; newFile = false; }
  TEInfo(intptr_t, intptr_t, const wchar_t*, bool);
};

class TEICollection: public TCollection
{
  public:
    TEICollection(size_t = 0, size_t = 5);
    size_t insert(intptr_t, intptr_t, const wchar_t*, bool);
    size_t removeID(intptr_t);
    ptrdiff_t findID(intptr_t);
    ptrdiff_t findLang(intptr_t);
    const wchar_t *findFile(intptr_t);
};
#endif
