static void parseExec (TExec *tmpe, const wchar_t *name, const wchar_t *value)
{
	if (!FSF.LStricmp (name, L"Save"))
	{
		if (!FSF.LStricmp (value, L"Current"))
			tmpe->saveType = saCurrent;
		else if (!FSF.LStricmp (value, L"All"))
			tmpe->saveType = saAll;
		else
			tmpe->saveType = saNone;
	}
	else if (!FSF.LStricmp (name, L"CD"))
	{
		if (!FSF.LStricmp (value, L"File"))
			tmpe->cd = cdFile;
		else if (!FSF.LStricmp (value, L"Base"))
			tmpe->cd = cdBase;
		else
			tmpe->cd = cdNone;
	}
	else if (!FSF.LStricmp (name, L"Jump"))
	{
		if (!FSF.LStricmp (value, L"Smart"))
			tmpe->jumpType = jtSmart;
		else if (!FSF.LStricmp (value, L"Menu"))
			tmpe->jumpType = jtMenu;
		else if (!FSF.LStricmp (value, L"First"))
			tmpe->jumpType = jtFirst;
		else
			tmpe->jumpType = jtNone;
	}
	else if (!FSF.LStricmp (name, L"Echo"))
		tmpe->echo = FSF.atoi (value) ? true : false;
	else if (!FSF.LStricmp (name, L"Title"))
		tmpe->title = value;
	else if (!FSF.LStricmp (name, L"Enable"))
	{
		if (value[0] == L'*' && value[1] == L'\\')
		{
			tmpe->enable = value;
			tmpe->searchBase = true;
		}
		else
			tmpe->enable = value + 2;
	}
	else if (!FSF.LStricmp (name, L"Disable"))
		tmpe->disable = value;
	else if (!FSF.LStricmp (name, L"Compiler"))
		tmpe->compiler = value;
}

static void parseCompiler (TCompiler *tmpc, const wchar_t *name, const wchar_t *value)
{
	if (!FSF.LStricmp (name, L"Error"))
		tmpc->err = value;
	else if (!FSF.LStricmp (name, L"Line"))
		tmpc->line = FSF.atoi (value);
	else if (!FSF.LStricmp (name, L"File"))
		tmpc->fileMatch = FSF.atoi (value);
	else if (!FSF.LStricmp (name, L"Col"))
	{
		wchar_t	*v = (wchar_t *) value;
		if (*value == L'?')
		{
			v++;
			tmpc->searchCol = true;
		}

		tmpc->col = FSF.atoi (v);
	}
}

static wchar_t *parseItem (TCollection *dc, wchar_t * &line, wchar_t *kwd, wchar_t *value)
{
	if (IsCharAlpha (*skipSpaces (line)))
	{
		wchar_t	*k = kwd;
		while (*line && IsCharAlpha (*line)) *k++ = *line++;
		*k = *value = 0;
		if (*skipSpaces (line) == L'=')
		{
			line++;
			if (!dc)
			{
				getWord (line, value);
			}
			else
			{
				wchar_t	buf[MAX_STR_LEN], *pv = value;
				getWord (line, buf);
				for (wchar_t *pch = buf; *pch;)
				{
					if (L'&' != *pch)
					{
						*pv++ = *pch++;
					}
					else
					{
						++pch;						//skip '&'
						wchar_t	name[MAX_STR_LEN], *pname = name;
						while (*pch && *pch != L';') *pname++ = *pch++;
						*pname = 0;

						//if (!*pch) complain about entity reference syntax;
						if (*pch) ++pch;	//skip
															///';
															///'
						for (size_t i = 0; i < dc->getCount (); ++i)
						{
							TDefine *d = (TDefine *) ((*dc)[i]);
							if (0 == wcscmp (d->name, name))
							{
								for (const wchar_t *pdv = d->value; *pdv;) *pv++ = *pdv++;
								break;
							}
						}

						//if (i == dc->getCount()) complain about undefined entity;
					}
				}

				*pv = 0;
			}
		}

		return (line);
	}

	return (NULL);
}

static wchar_t *getItem (wchar_t * &line, wchar_t *kwd, bool &group)
{
	group = true;

	bool	coment = true;
	while (coment)
	{
		if (*skipSpaces (line) == L'<')
		{
			if (line[1] == L'!')
			{
				if (line[2] == L'-')
				{
					if (line[3] == L'-')
					{
						line += 4;
						coment = true;
						while (*line)
						{
							if (line[0] == L'-')
								if (line[1] == L'-')
									if (line[2] == L'>')
									{
										line += 3;
										break;
									}

							line++;
						}
					}
					else
						coment = false;
				}
				else
					coment = false;
			}
			else
				coment = false;
		}
		else
			coment = false;
	}

	if (*skipSpaces (line) == L'<')
	{
		wchar_t	*stLine = line;
		wchar_t	*k = kwd;
		line++;
		if (*line && (*line == L'/')) *k++ = *line++;
		while (*line && IsCharAlpha (*line)) *k++ = *line++;
		*k = 0;
		k = --line;

		int inQuote = 0;
		while (*++line)
		{
			if (inQuote)
			{
				if (*line == L'\\' && line[1] == L'\"')
					line++;
				else if (*line == L'\"')
					inQuote = 0;
			}
			else
			{
				if (*line == L'>')
				{
					if (line > stLine)
					{
						--line;
						if (*line == L'/')
						{
							*line = 0;
							group = false;
						}

						line++;
					}

					*line = 0;
					line++;
					break;
				}
				else if (*line == L'\"')
					inQuote = 1;
			}
		}

		return (k);
	}

	return (NULL);
}

static void findSectionInXML (wchar_t * &p)
{
	bool	group;
	wchar_t	name[MAX_STR_LEN];
	while ((getItem (p, name, group)) != NULL)
		if (group && !FSF.LStricmp (name, L"TrueTpl")) break;
}

static wchar_t *GetXmlParam (const wchar_t **szParam)
{
	static wchar_t szString[_MAX_PATH + 1];
	const wchar_t	*p = *szParam;
	if (*(++p) == L'\'')
	{
		wchar_t	*o = szString;
		while (*++p)
		{
			if ((*p == L'\\') && ((p[1] == L'\\') || (p[1] == L'\'')))
				p++;
			else if (*p == L'\'')
				break;
			if (*p) *o++ = *p;
		}

		*o = 0;
		*szParam = p;
		return (szString);
	}
	else
		return (NULL);
}
