static inline wchar_t *StrLower (wchar_t *str)
{
	FSF.LStrlwr (str);
	return (str);
}

static bool strMatch
(
	const wchar_t	*s,
	const wchar_t	*pattern,
	const wchar_t	*prefix,
	const wchar_t	*suffix,
	size_t	nb,
	intptr_t		bounds[][2] = nullptr,
	const intptr_t	regn[] = nullptr
)
{
	bool    ret = false;
	size_t  l = wcslen (prefix) + wcslen (pattern) + wcslen (suffix);
	wchar_t	*slashPattern = (wchar_t *) malloc ((l + 1) * sizeof(wchar_t));

	if (slashPattern)
	{
		wcscat (lstrcat (lstrcpy (slashPattern, prefix), pattern), suffix);

		RegExpSearch rs;
		rs.Text = s;
		rs.Position = 0;
		rs.Length = wcslen(s);
		rs.Match = new RegExpMatch[nb + 1];
		rs.Count = nb + 1;
		rs.Reserved = nullptr;
		ret = ::Info.RegExpControl(RegExpHandle, RECTL_COMPILE, 0, slashPattern) &&
			::Info.RegExpControl(RegExpHandle, RECTL_MATCHEX, 0, &rs);
		if (ret)
		{
			if (bounds && regn)
			{
				for (size_t i = 0; i < nb; i++)
				{
					bounds[i][0] = rs.Match[regn[i]].start;
					bounds[i][1] = rs.Match[regn[i]].end;
				}
			}
		}
		delete[] rs.Match;

		free (slashPattern);
	}

	return (ret);
}

bool wcscmpi2 (wchar_t *szStr1, wchar_t *szStr2)
{
	size_t len = wcslen (szStr2);
	for (size_t i = 0; i < len; i++)
	{
		if (!szStr1[i] || (((szStr1[i] ^ szStr2[i]) & 0xDF) != 0)) return ((i + 1) != 0);
	}

	return (0);
}

static inline bool isCharSpace (wchar_t c)
{
	return c == L' ' ||
	       c == L'\t' ||
	       c == L'\r' ||
	       c == L'\n';
}

static const wchar_t *skipSpaces (const wchar_t * &line)
{
	while (*line && isCharSpace (*line)) line++;
	return (line);
}

static const wchar_t *getWord (const wchar_t * &line, wchar_t *kwd)
{
	wchar_t	*k = kwd;
	if (*skipSpaces (line) == L'\"')
	{
		while (*++line)
		{
			if (*line == L'\\' && line[1] == L'\"')
				*k++ = *++line;
			else if (*line == L'\"')
			{
				if (*skipSpaces (++line) != L'\"') break;
			}
			else
				*k++ = *line;
		}

		*k = 0;
	}
	else
	{
		while (*line && !isCharSpace (*line)) *k++ = *line++;
		*k = 0;
	}

	return (kwd);
}

wchar_t *makeSubstr(size_t n, const wchar_t *origStr, intptr_t bounds[][2], size_t nBounds)
{
	wchar_t	*s = nullptr;

	if (origStr && bounds && n < nBounds)
	{
    	intptr_t len = bounds[n][1] - bounds[n][0] + 1;
		s = (wchar_t *) malloc ((len + 1) * sizeof(wchar_t));
		if (s)
		{
			wcsncpy (s, origStr + bounds[n][0], len + 1);
			s[len] = 0;
		}

		SysLog (L"n=%d [%d:%d] '%s' -> '%s'\n", n, bounds[n][0], bounds[n][1], origStr, s);
	}

	return (s);
}

static wchar_t *FirstNonSpace (const wchar_t *s)
{
	wchar_t	*i = (wchar_t *) s;
	while (*i)
	{
		if (!isCharSpace (*i)) return (i);
		i++;
	}

	return (nullptr);
}
