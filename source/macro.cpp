struct TMacro
{
	wchar_t	FARKey[MAX_STR_LEN];
	bool	atStartup;
	bool	submenu;
	wchar_t	Name[MAX_REG_LEN];
	wchar_t	Word[MAX_REG_LEN], At[MAX_REG_LEN], MacroText[MAX_STR_LEN];
	wchar_t	before[MAX_REG_LEN], after[MAX_REG_LEN], immChar;
};

static void InitMacro ()
{
	reloadInProcess = true;

	HANDLE			hScreen = Info.SaveScreen (0, 0, -1, -1);
	const wchar_t	*MsgItems[] = { GetMsg (MTitle), GetMsg (MLoading) };
	Info.Message (&MainGuid, &InitMacroGuid, 0, NULL, MsgItems, _countof (MsgItems), 0);

	wchar_t	path[NM];
	*wcsrchr (wcscpy (path, Info.ModuleName), L'\\') = 0;

	wchar_t	*fileBuff = getFile (path, L"true-tpl.xml");
	if (fileBuff)
	{
		wchar_t				*item, *p = fileBuff;
		wchar_t				name[MAX_STR_LEN], value[MAX_STR_LEN];
		TLang				*lng = NULL;
		bool				group;
		TCollection dummyLang;
		langColl = dummyLang;
		findSectionInXML (p);
		while ((item = getItem (p, name, group)) != NULL)
		{
			if (group && !FSF.LStricmp (name, L"/True-Tpl"))
				findSectionInXML (p);
			else if (!group && !FSF.LStricmp (name, L"Include"))
			{
				wchar_t	incFile[NM];
				*incFile = 0;

				TCollection *dc = (lng == NULL) ? NULL : &(lng->defineColl);
				while (parseItem (dc, item, name, value))
					if (!FSF.LStricmp (name, L"File")) wcscpy (incFile, value);
				if (*incFile)
				{
					wchar_t	*incBuff = getFile (path, incFile);
					if (incBuff)
					{
						wchar_t	*newBuff = new wchar_t[wcslen (incBuff) + wcslen (p) + 16];
						if (newBuff)
						{
							wcscat (wcscat (wcscpy (newBuff, incBuff), L"\r\n<TrueTpl>\r\n"), p);
							delete[] fileBuff;
							fileBuff = p = newBuff;
							findSectionInXML (p);
						}

						delete[] incBuff;
					}
				}
			}
			else if (group && !FSF.LStricmp (name, L"/Language"))
				lng = NULL;
			else if (group && !FSF.LStricmp (name, L"Language"))
			{
				lng = new TLang;
				lng->ignoreCase = false;
				lng->setCP = lng->mask[0] = lng->desc[0] = lng->imm[0] = lng->immExp[0] = 0;
				lng->defExec.defaults ();
				while (parseItem (&(lng->defineColl), item, name, value))
				{
					if (!FSF.LStricmp (name, L"File"))
							wcscpy (lng->mask, value);
					else if (!FSF.LStricmp (name, L"Desc"))
						wcscpy (lng->desc, value);
					else if (!FSF.LStricmp (name, L"BlockComment"))
						wcscpy (lng->blockcomment, value);
					else if (!FSF.LStricmp (name, L"CP"))
						lng->setCP = FSF.atoi (value);
					else if (!FSF.LStricmp (name, L"IgnoreCase"))
						lng->ignoreCase = FSF.atoi (value) ? true : false;
					else
						parseExec (&(lng->defExec), name, value);
				}

				if (lng->mask[0])
					langColl.insert (lng);
				else
				{
					delete lng;
					lng = NULL;
				}
			}
			else if (lng != NULL)
			{
				if (!group && !FSF.LStricmp (name, L"Define"))
				{
					TDefine *tmpd = new TDefine (L"", L"");
					while (parseItem (&(lng->defineColl), item, name, value))
					{
						if (!FSF.LStricmp (name, L"Name"))
							wcscpy (tmpd->name, value);
						else if (!FSF.LStricmp (name, L"Value"))
							wcscpy (tmpd->value, value);
					}

					if (tmpd->name[0])
						lng->defineColl.insert (tmpd);
					else
						delete tmpd;
				}
				else if (!group && !FSF.LStricmp (name, L"Navigation"))
				{
					TNavy *tmpn = new TNavy;
					while (parseItem (&(lng->defineColl), item, name, value))
					{
						if (!FSF.LStricmp (name, L"Mask"))
							wcscpy (tmpn->mask, value);
						else if (!FSF.LStricmp (name, L"Pos"))
							tmpn->pos = FSF.atoi (value);
						else if (!FSF.LStricmp (name, L"Path"))
							ExpandEnvironmentStrings (value, tmpn->path, MAX_STR_LEN);
						else if (!FSF.LStricmp (name, L"Suffixes"))
							ExpandEnvironmentStrings (value, tmpn->suffixes, MAX_STR_LEN);
						else if (!FSF.LStricmp (name, L"Rect"))
							FSF.sscanf
								(
									value,
									L"%d%%,%d%%,%d%%,%d%%",
									&tmpn->rect[0],
									&tmpn->rect[1],
									&tmpn->rect[2],
									&tmpn->rect[3]
								);
						else if (!FSF.LStricmp (name, L"Viewer"))
							tmpn->viewer = FSF.atoi (value) ? true : false;
					}

					if (tmpn->mask[0])
						lng->navyColl.insert (tmpn);
					else
						delete tmpn;
				}
				else if (!group && !FSF.LStricmp (name, L"Format"))
				{
					TFormat *tmpf = new TFormat;
					while (parseItem (&(lng->defineColl), item, name, value))
					{
						if (!FSF.LStricmp (name, L"Name"))
							wcscpy (tmpf->name, value);
						else if (!FSF.LStricmp (name, L"Command"))
							ExpandEnvironmentStrings (value, tmpf->comm, MAX_STR_LEN);
						else if (!FSF.LStricmp (name, L"Echo"))
							tmpf->echo = FSF.atoi (value) ? true : false;
					}

					if (tmpf->name[0] && tmpf->comm[0])
						lng->formatColl.insert (tmpf);
					else
						delete tmpf;
				}
				else if (!group && !FSF.LStricmp (name, L"Expand"))
				{
					TMacro	*tmpm = new TMacro;
					wcscpy (tmpm->At, expOnBlank);
					tmpm->immChar = 0;
					tmpm->Name[0] = tmpm->Word[0] = tmpm->before[0] = tmpm->after[0] = tmpm->MacroText[0] = 0;
					wcscpy (tmpm->FARKey, L"");
					tmpm->atStartup = false;
					tmpm->submenu = false;

					int validTag = false;
					while (parseItem (&(lng->defineColl), item, name, value))
					{
						if (!FSF.LStricmp (name, L"Pattern"))
						{
							wcscpy (tmpm->Word, value);
							if (tmpm->Word[0])
							{
								validTag = true;

								wchar_t	*opDelimiter = wcschr (tmpm->Word, L'|');
								if (opDelimiter)
								{
									wchar_t	rest[MAX_STR_LEN], cc[3] = L"(.";
									*opDelimiter = 0;
									wcscpy (rest, opDelimiter + 1);
									for (wchar_t *p = rest; *p; p++)
									{
										cc[1] = *p;
										wcscat (tmpm->Word, cc);
									}
									{
										for (wchar_t *p = rest; *p; p++) wcscat (tmpm->Word, L")?");
									}
								}
							}
						}
						else if (!FSF.LStricmp (name, L"To"))
							wcscpy (tmpm->MacroText, value);
						else if (!FSF.LStricmp (name, L"Key"))
						{
							validTag = true;
							wcscpy (tmpm->FARKey, value);
						}
						else if (!FSF.LStricmp (name, L"Imm"))
							tmpm->immChar = *value;
						else if (!FSF.LStricmp (name, L"Init"))
						{
							validTag = true;
							tmpm->atStartup = FSF.atoi (value) ? true : false;
						}
						else if (!FSF.LStricmp (name, L"At"))
						{
							if (!FSF.LStricmp (value, L"AnyWhere"))
								wcscpy (tmpm->At, expAnyWhere);
							else if (!FSF.LStricmp (value, L"Start"))
								wcscpy (tmpm->At, expAtStart);
							else if (!FSF.LStricmp (value, L"Middle"))
								wcscpy (tmpm->At, expAtMiddle);
							else if (!FSF.LStricmp (value, L"End"))
								wcscpy (tmpm->At, expAtEnd);
							else if (!FSF.LStricmp (value, L"Blank"))
								wcscpy (tmpm->At, expOnBlank);
							else
								wcscpy (tmpm->At, value);
						}
						else if (!FSF.LStricmp (name, L"Name"))
						{
							validTag = true;
							wcscpy (tmpm->Name, value);
						}
						else if (!FSF.LStricmp (name, L"SubMenu"))
						{
							tmpm->submenu = ((value[0] != L'\0')) && ((value[0] == L'1') && (value[1] == L'\0'));
						}
					}

					if (validTag)
					{
						if (tmpm->At[0])
						{
							wchar_t	*opDelimiter = wcsstr (tmpm->At, L"\\p");
							if (opDelimiter)
							{
								*opDelimiter = 0;
								wcscpy (tmpm->before, tmpm->At);
								wcscpy (tmpm->after, opDelimiter + 2);
							}
						}

						if (tmpm->immChar)
						{
							if (!wcschr (lng->immExp, tmpm->immChar))
							{
								int len = wcslen (lng->immExp);
								lng->immExp[len++] = tmpm->immChar;
								lng->immExp[len] = 0;
							}
						}

						lng->macroColl.insert (tmpm);
					}
					else
						delete tmpm;
				}
				else if (!group && !FSF.LStricmp (name, L"Comment"))
				{
					TComment	*tmpc = new TComment;
					tmpc->mask[0] = 0;
					while (parseItem (&(lng->defineColl), item, name, value))
						if (!FSF.LStricmp (name, L"Pattern")) wcscpy (tmpc->mask, value);
					if (tmpc->mask[0])
						lng->commentColl.insert (tmpc);
					else
						delete tmpc;
				}
				else if (!group && !FSF.LStricmp (name, L"Exec"))
				{
					TExec *tmpe = new TExec (lng->defExec);
					while (parseItem (&(lng->defineColl), item, name, value))
						if (!FSF.LStricmp (name, L"Command"))
							wcscpy (tmpe->cmd, value);
						else
							parseExec (tmpe, name, value);
					if (!tmpe->title[0]) wcscpy (tmpe->title, tmpe->cmd);
					if (!tmpe->title[0]) tmpe->cmd[0] = 0;
					lng->execColl.insert (tmpe);
				}
				else if (!group && !FSF.LStricmp (name, L"Compiler"))
				{
					TCompiler *tmpc = new TCompiler ();
					while (parseItem (&(lng->defineColl), item, name, value))
						if (!FSF.LStricmp (name, L"Name"))
							wcscpy (tmpc->title, value);
						else
							parseCompiler (tmpc, name, value);
					if (tmpc->title[0])
						lng->compilerColl.insert (tmpc);
					else
						delete tmpc;
				}
				else if (!group && !FSF.LStricmp (name, L"Indent"))
				{
					TIndent *tmpi = new TIndent;
					tmpi->mask[0] = tmpi->relative[0] = 0;
					tmpi->BracketsMode = tmpi->start = tmpi->immChar = 0;
					for (int i = 0; i < 2; i++) tmpi->indent[i] = 0xFFFF;
					while (parseItem (&(lng->defineColl), item, name, value))
					{
						if (!FSF.LStricmp (name, L"Pattern"))
							wcscpy (tmpi->mask, value);
						else if (!FSF.LStricmp (name, L"Line"))
							tmpi->indent[0] = FSF.atoi (value);
						else if (!FSF.LStricmp (name, L"Next"))
							tmpi->indent[1] = FSF.atoi (value);
						else if (!FSF.LStricmp (name, L"Imm"))
							tmpi->immChar = *value;
						else if (!FSF.LStricmp (name, L"Start"))
							tmpi->start = FSF.atoi (value);
						else if (!FSF.LStricmp (name, L"Relative"))
							wcscpy (tmpi->relative, value);
					}

					if (tmpi->mask[0])
					{
						if (tmpi->immChar)
						{
							if (!wcsrchr (lng->imm, tmpi->immChar))
							{
								int len = wcslen (lng->imm);
								lng->imm[len++] = tmpi->immChar;
								lng->imm[len] = 0;
							}
						}

						lng->indentColl.insert (tmpi);
					}
					else
						delete tmpi;
				}
				else if (!group && !FSF.LStricmp (name, L"Bracket"))
				{
					TBracket	*tmpb = new TBracket;
					TIndent		*tmp0 = new TIndent;
					TIndent		*tmp1 = new TIndent;
					tmpb->open[0] = tmpb->close[0] = 0;
					tmp0->relative[0] = 0;
					tmp0->start = tmp0->immChar = 0;
					tmp0->BracketsMode = 0;
					tmp1->relative[0] = 0;
					tmp1->start = tmp1->immChar = 0;
					tmp1->BracketsMode = 1;
					tmp1->bracketLink = lng->bracketColl.getCount ();
					tmp0->indent[0] = 0xFFFF;
					tmp0->indent[1] = 1;
					tmp1->indent[0] = 0;
					tmp1->indent[1] = 0xFFFF;
					while (parseItem (&(lng->defineColl), item, name, value))
					{
						if (!FSF.LStricmp (name, L"Pattern"))
							wcscpy (tmp0->mask, wcscpy (tmpb->open, value));
						else if (!FSF.LStricmp (name, L"Match"))
							wcscpy (tmp1->mask, wcscpy (tmpb->close, value));
						else if (!FSF.LStricmp (name, L"Line"))
							tmp0->indent[0] = FSF.atoi (value);
						else if (!FSF.LStricmp (name, L"Shift"))
							tmp0->indent[1] = FSF.atoi (value);
						else if (!FSF.LStricmp (name, L"Next"))
							tmp1->indent[1] = FSF.atoi (value);
						else if (!FSF.LStricmp (name, L"ImmOpen"))
							tmp0->immChar = *value;
						else if (!FSF.LStricmp (name, L"ImmClose"))
							tmp1->immChar = *value;
						else if (!FSF.LStricmp (name, L"Start"))
							tmp0->start = FSF.atoi (value);
					}

					if (tmpb->open[0] && tmpb->close[0])
					{
						if (tmp0->immChar)
						{
							if (!wcschr (lng->imm, tmp0->immChar))
							{
								int len = wcslen (lng->imm);
								lng->imm[len++] = tmp0->immChar;
								lng->imm[len] = 0;
							}
						}

						if (tmp1->immChar)
						{
							if (!wcschr (lng->imm, tmp1->immChar))
							{
								int len = wcslen (lng->imm);
								lng->imm[len++] = tmp1->immChar;
								lng->imm[len] = 0;
							}
						}

						lng->bracketColl.insert (tmpb);
						lng->indentColl.insert (tmp0);
						lng->indentColl.insert (tmp1);
					}
					else
					{
						delete tmpb;
						delete tmp0;
						delete tmp1;
					}
				}
			}
		}

		delete[] fileBuff;
	}

	Info.RestoreScreen (hScreen);
	reloadInProcess = reloadNeeded = false;
	doneThread ();
	initThread ();
}

static void DoneMacro ()
{
	langColl.removeAll ();
	doneThread ();
}

static TMacro *FindMacro
(
	TLang *lng,
	wchar_t	*before,
	wchar_t	*after,
	wchar_t	expChar,
	int		*delCount,
	int		bounds[][2],
	bool	frommenu = false
)
{
	unsigned	i;
	wchar_t			*regEnd = lng->ignoreCase ? L"\\s*$/i" : L"\\s*$/";
	for (i = 0; i < lng->macroColl.getCount (); i++)
	{
		TMacro	*mm = (TMacro *) (lng->macroColl[i]);
		if (expChar && (expChar != mm->immChar)) continue;
		if (!frommenu && !expChar && mm->immChar) continue;
		if (frommenu && mm->Name[0] && (wcscmp (mm->Name, before) == 0)) return (mm);
	}

	for (i = 0; i < lng->macroColl.getCount (); i++)
	{
		TMacro	*mm = (TMacro *) (lng->macroColl[i]);
		if (expChar && (expChar != mm->immChar)) continue;
		if (mm->Word[0] && strMatch (after, mm->after, L"/^\\s*", regEnd, 0))
		{
			wchar_t	realBefore[MAX_REG_LEN];
			wcscat (wcscpy (realBefore, mm->before), mm->Word);
			if (strMatch (before, realBefore, L"/^\\s*", regEnd, 0))
			{
				if (delCount)
				{
					regEnd = lng->ignoreCase ? L")$/i" : L")$/";

					int zero[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15 };
					if (strMatch (before, mm->Word, L"/.*(", regEnd, 16, bounds, zero))
						*delCount = bounds[1][1] - bounds[1][0];
					else
						*delCount = 0;
				}

				return (mm);
			}
		}
	}

	return (NULL);
}

static TMacro *FindMacroKey (TLang *lng, const INPUT_RECORD *Rec)
{
	wchar_t rKeyp[MAX_STR_LEN];
	FSF.FarInputRecordToName (Rec, rKeyp, MAX_STR_LEN);
	for (unsigned i = 0; i < lng->macroColl.getCount (); i++)
	{
		TMacro	*mm = (TMacro *) (lng->macroColl[i]);
		if (wcscmp(mm->FARKey, rKeyp) == 0) return (mm);
	}

	return (NULL);
}

static TMacro *CheckMacroPos (TLang *lng, TMacro *mm, wchar_t *before, wchar_t *after)
{
	wchar_t	*regEnd = lng->ignoreCase ? L"\\s*$/i" : L"\\s*$/";
	if (strMatch (after, mm->after, L"/^\\s*", regEnd, 0))
		if (strMatch (before, mm->before, L"/^\\s*", regEnd, 0)) return (mm);
	return (NULL);
}
