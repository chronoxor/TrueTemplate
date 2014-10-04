////////////////////////////////////////////////////////////////
// If this code works, it was written by Alexander Nazarenko.
// If not, I don't know who wrote it.

#include "winmem.h"
#include "eicoll.h"

size_t TEICollection::insert(intptr_t aID, intptr_t aLang, const wchar_t *aFn, bool aNewFile)
{
	return TCollection<TEInfo>::insert(new TEInfo(aID, aLang, aFn, aNewFile));
}

static bool find_ID(const TEInfo *it, void *data)
{
	return it->ID == *((intptr_t*)data);
}

ptrdiff_t TEICollection::findID(intptr_t aID)
{
	return findIndex(find_ID, &aID);
}

ptrdiff_t TEICollection::findLang(intptr_t aID)
{
	TEInfo *i = find(find_ID, &aID);
	if (i)
		return i->lang;
	return -1;
}

const wchar_t *TEICollection::findFile(intptr_t aID)
{
	TEInfo *i = find(find_ID, &aID);
	if (i)
		return i->fn;
	return nullptr;
}

size_t TEICollection::removeID(intptr_t aID)
{
	ptrdiff_t i = findIndex(find_ID, &aID);
	if (i != -1)
		return remove(i);
	return getCount();
}
