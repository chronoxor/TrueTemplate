#ifndef __CONSOLE_H__
#define __CONSOLE_H__
#include "winmem.h"
#include "mystring.h"
#include "tcoll.h"

struct TOutputLine : TCollectionItem
{
	String line;
	TOutputLine(const wchar_t *aLine)
		: line(aLine)
	{
	}
};

extern DWORD ExecConsoleApp(const wchar_t*, const wchar_t*, TCollection<TOutputLine>*, bool, bool);

#endif
