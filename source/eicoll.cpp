////////////////////////////////////////////////////////////////
// If this code works, it was written by Alexander Nazarenko.
// If not, I don't know who wrote it.

#include "winmem.h"
#include "eicoll.h"

TEInfo::TEInfo(int aID, int aLang, const wchar_t *aFn, int aNewFile)
{
  ID = aID;
  lang = aLang;
  newFile = aNewFile;
  wcscpy(fn, aFn);
}

unsigned TEICollection::insert(int aID, int aLang, const wchar_t *aFn, int aNewFile)
{
  return TCollection::insert(new TEInfo(aID, aLang, aFn, aNewFile));
}

static int find_ID(void *it, void *data)
{
  return ((TEInfo*)it)->ID == *((int*)data);
}

TEICollection::TEICollection(unsigned aLimit, unsigned aDelta) :
  TCollection(aLimit, aDelta, NULL)
{
}

int TEICollection::findID(int aID)
{
  return findIndex(find_ID, &aID);
}

int TEICollection::findLang(int aID)
{
  TEInfo *i = (TEInfo*)find(find_ID, &aID);
  if ( i )
    return i->lang;
  return -1;
}

const wchar_t *TEICollection::findFile(int aID)
{
  TEInfo *i = (TEInfo*)find(find_ID, &aID);
  if ( i )
    return i->fn;
  return NULL;
}

unsigned TEICollection::removeID(int aID)
{
  int i = findIndex(find_ID, &aID);
  if ( i != -1 )
    return remove(i);
  return getCount();
}
