#ifndef __CONSOLE_H__
#define __CONSOLE_H__
#include "winmem.h"
#include "mystring.h"
#include "tcoll.h"

class TOutputLine : public TCollectionItem
{
public:
	String line;
	TOutputLine(const wchar_t *aLine)
		: line(aLine)
	{
	}
};

extern DWORD ExecConsoleApp(const wchar_t*, const wchar_t*, TCollection*, bool, bool);

#endif
