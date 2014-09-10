#ifndef __EICOLL_H
#define __EICOLL_H

#include "mystring.h"
#include "tcoll.h"

struct TEInfo
{
	intptr_t ID, lang;
	bool newFile;
	String fn;
  
	TEInfo() 
		: ID(-1), lang(-1), newFile(false) 
	{
	}
	
	TEInfo(intptr_t aID, intptr_t aLang, const wchar_t *aFn, bool aNewFile)
		: ID(aID), lang(aLang), newFile(aNewFile), fn(aFn)
	{
	}
};

class TEICollection: public TCollection
{
public:
	TEICollection(size_t = 0, size_t = 5);
	size_t insert(intptr_t, intptr_t, const wchar_t*, bool);
	size_t removeID(intptr_t);
	ptrdiff_t findID(intptr_t);
	ptrdiff_t findLang(intptr_t);
	const String &findFile(intptr_t);
	TEInfo *operator[](size_t i) { return (TEInfo *)at(i); };
};
#endif
