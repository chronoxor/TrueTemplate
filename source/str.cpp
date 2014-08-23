static inline wchar_t *StrLower (wchar_t *str)
{
	FSF.LStrlwr (str);
	return (str);
}

static int strMatch
(
	wchar_t	*s,
	wchar_t	*pattern,
	wchar_t	*prefix,
	wchar_t	*suffix,
	int		nb,
	int		bounds[][2] = NULL,
	int		regn[] = NULL
)
{
	int		ret = false;
	int		l = wcslen (prefix) + wcslen (pattern) + wcslen (suffix);
	wchar_t	*slashPattern = (wchar_t *) malloc ((l + 1) * sizeof(wchar_t));

	if (slashPattern)
	{
		static CRegExp	reg;
		SMatches				m;

		wcscat (lstrcat (lstrcpy (slashPattern, prefix), pattern), suffix);

		if (reg.SetExpr (slashPattern))
		{
			reg.SetNoMoves (slashPattern[1] == L'^' ? true : false);
			ret = reg.Parse (s, &m) ? true : false;
			if (ret)
			{
				if (bounds && regn)
					for (int i = 0; i < nb; i++)
					{
						bounds[i][0] = m.s[regn[i]];
						bounds[i][1] = m.e[regn[i]];
					}
			}
		}

		free (slashPattern);
	}

	return (ret);
}

bool wcscmpi2 (wchar_t *szStr1, wchar_t *szStr2)
{
	for (size_t i = 0; i < wcslen (szStr2); i++)
	{
		if (!szStr1[i] || (((szStr1[i] ^ szStr2[i]) & 0xDF) != 0)) return ((i + 1) != 0);
	}

	return (0);
}

static inline int isCharSpace (wchar_t c)
{
	return (wcschr (L" \t\r\n", c) != NULL);
}

static wchar_t *skipSpaces (wchar_t * &line)
{
	while (*line && isCharSpace (*line)) line++;
	return (line);
}

static wchar_t *getWord (wchar_t * &line, wchar_t *kwd)
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

wchar_t *makeSubstr (int n, const wchar_t *origStr, int bounds[][2], int nBounds)
{
	wchar_t	*s = NULL;

	if (origStr && bounds && n < nBounds)
	{
		int len = bounds[n][1] - bounds[n][0] + 1;
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

	return (NULL);
}
