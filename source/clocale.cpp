/////////////////////////////////////////////////////////////////////////////
// substs standard insensitive string compares for ANSI compatibility
//
#include <wchar.h>
#include "clocale.h"

/////////////////////////////////////////////////////////////////////////////

bool CLocale::cl_isdigit(wchar_t c)
{
	return iswdigit(c) != 0;
};
bool CLocale::cl_isword(wchar_t c)
{
	return (iswalpha(c) != 0) || (c == L'_');
}
bool CLocale::cl_isuppercase(wchar_t c)
{
	return iswupper(c) != 0;
};
bool CLocale::cl_islowercase(wchar_t c)
{
	return iswlower(c) != 0;
};
wchar_t CLocale::cl_lowcase(wchar_t c)
{
	return towlower(c);
};
wchar_t CLocale::cl_upcase(wchar_t c)
{
	return towupper(c);
};

int CLocale::cl_stricmp(wchar_t *c1, wchar_t *c2)
{
	return _wcsicmp(c1, c2);
};
int CLocale::cl_strnicmp(wchar_t *c1, wchar_t *c2, size_t len)
{
	return _wcsnicmp(c1, c2, len);
};
