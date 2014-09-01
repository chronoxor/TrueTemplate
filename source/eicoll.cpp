////////////////////////////////////////////////////////////////
// If this code works, it was written by Alexander Nazarenko.
// If not, I don't know who wrote it.

#include "winmem.h"
#include "eicoll.h"

TEInfo::TEInfo(intptr_t aID, intptr_t aLang, const wchar_t *aFn, bool aNewFile)
{
  ID = aID;
  lang = aLang;
  newFile = aNewFile;
  wcscpy(fn, aFn);
}

size_t TEICollection::insert(intptr_t aID, intptr_t aLang, const wchar_t *aFn, bool aNewFile)
{
  return TCollection::insert(new TEInfo(aID, aLang, aFn, aNewFile));
}

static int find_ID(void *it, void *data)
{
  return ((TEInfo*)it)->ID == *((intptr_t*)data);
}

TEICollection::TEICollection(size_t aLimit, size_t aDelta) :
  TCollection(aLimit, aDelta, NULL)
{
}

ptrdiff_t TEICollection::findID(intptr_t aID)
{
  return findIndex(find_ID, &aID);
}

ptrdiff_t TEICollection::findLang(intptr_t aID)
{
  TEInfo *i = (TEInfo*)find(find_ID, &aID);
  if ( i )
    return i->lang;
  return -1;
}

const wchar_t *TEICollection::findFile(intptr_t aID)
{
  TEInfo *i = (TEInfo*)find(find_ID, &aID);
  if ( i )
    return i->fn;
  return NULL;
}

size_t TEICollection::removeID(intptr_t aID)
{
  ptrdiff_t i = findIndex(find_ID, &aID);
  if ( i != -1 )
    return remove(i);
  return getCount();
}
