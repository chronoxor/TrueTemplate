struct TMacro
{
	String	FARKey;
	bool		atStartup;
	bool		submenu;
	String	Name;
	String	At;
	String	Word;
	String	before, after, MacroText;
	wchar_t	immChar;
};

static String ExpandEnv(const wchar_t *env)
{
	const size_t BufSize = 4096;
	wchar_t *buf = new wchar_t[BufSize];
	size_t nChars = ExpandEnvironmentStrings(env, buf, BufSize);
	if (nChars > BufSize)
	{
		delete[] buf;
		buf = new wchar_t[nChars];
		ExpandEnvironmentStrings(env, buf, nChars);
	}
	String result(buf);
	delete[] buf;
	return result;
}

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
				lng->setCP = 0; 
				lng->mask.clear();
				lng->desc.clear(); 
				lng->imm.clear(); 
				lng->immExp.clear();
				lng->defExec.defaults ();
				while (parseItem (&(lng->defineColl), item, name, value))
				{
					if (!FSF.LStricmp (name, L"File"))
						lng->mask = value;
					else if (!FSF.LStricmp (name, L"Desc"))
						lng->desc = value;
					else if (!FSF.LStricmp (name, L"BlockComment"))
						lng->blockcomment = value;
					else if (!FSF.LStricmp (name, L"CP"))
						lng->setCP = FSF.atoi (value);
					else if (!FSF.LStricmp (name, L"IgnoreCase"))
						lng->ignoreCase = FSF.atoi (value) ? true : false;
					else
						parseExec (&(lng->defExec), name, value);
				}

				if (!lng->mask.empty())
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
							tmpd->name = value;
						else if (!FSF.LStricmp (name, L"Value"))
							tmpd->value = value;
					}

					if (!tmpd->name.empty())
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
							tmpn->mask = value;
						else if (!FSF.LStricmp (name, L"Pos"))
							tmpn->pos = FSF.atoi (value);
						else if (!FSF.LStricmp (name, L"Path"))
							tmpn->path = ExpandEnv (value);
						else if (!FSF.LStricmp (name, L"Suffixes"))
							tmpn->suffixes = ExpandEnv (value);
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

					if (!tmpn->mask.empty())
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
							tmpf->name = value;
						else if (!FSF.LStricmp (name, L"Command"))
							tmpf->comm = ExpandEnv (value);
						else if (!FSF.LStricmp (name, L"Echo"))
							tmpf->echo = FSF.atoi (value) ? true : false;
					}

					if (!tmpf->name.empty() && !tmpf->comm.empty())
						lng->formatColl.insert (tmpf);
					else
						delete tmpf;
				}
				else if (!group && !FSF.LStricmp (name, L"Expand"))
				{
					TMacro	*tmpm = new TMacro;
					tmpm->At = expOnBlank;
					tmpm->immChar = 0;
					tmpm->Name.clear();
					tmpm->before.clear(); 
					tmpm->after.clear();
					tmpm->MacroText.clear();
					tmpm->Word.clear();
					tmpm->FARKey.clear();
					tmpm->atStartup = false;
					tmpm->submenu = false;

					bool validTag = false;
					while (parseItem (&(lng->defineColl), item, name, value))
					{
						if (!FSF.LStricmp (name, L"Pattern"))
						{
							tmpm->Word = value;
							if (!tmpm->Word.empty())
							{
								validTag = true;

								const wchar_t	*opDelimiter = wcschr (tmpm->Word, L'|');
								if (opDelimiter)
								{
									String rest(opDelimiter + 1);
									tmpm->Word = String(tmpm->Word, opDelimiter);
									for (const wchar_t *p = rest; *p; p++)
									{
										tmpm->Word += L'(';
										tmpm->Word += *p;
									}
									for (const wchar_t *p = rest; *p; p++)
									{
										tmpm->Word += L')';
										tmpm->Word += L'?';
									}
								}
							}
						}
						else if (!FSF.LStricmp (name, L"To"))
							tmpm->MacroText = value;
						else if (!FSF.LStricmp (name, L"Key"))
						{
							validTag = true;
							tmpm->FARKey = value;
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
								tmpm->At = expAnyWhere;
							else if (!FSF.LStricmp (value, L"Start"))
								tmpm->At = expAtStart;
							else if (!FSF.LStricmp (value, L"Middle"))
								tmpm->At = expAtMiddle;
							else if (!FSF.LStricmp (value, L"End"))
								tmpm->At = expAtEnd;
							else if (!FSF.LStricmp (value, L"Blank"))
								tmpm->At = expOnBlank;
							else
								tmpm->At = value;
						}
						else if (!FSF.LStricmp (name, L"Name"))
						{
							validTag = true;
							tmpm->Name = value;
						}
						else if (!FSF.LStricmp (name, L"SubMenu"))
						{
							tmpm->submenu = (value[0] == L'1') && (value[1] == L'\0');
						}
					}

					if (validTag)
					{
						if (!tmpm->At.empty())
						{
							const wchar_t	*opDelimiter = wcsstr (tmpm->At, L"\\p");
							if (opDelimiter)
							{
								tmpm->before = String(tmpm->At, opDelimiter);
								tmpm->after = opDelimiter + 2;
							}
						}

						if (tmpm->immChar)
						{
							if (!wcschr (lng->immExp, tmpm->immChar))
							{
								lng->immExp += tmpm->immChar;
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
					tmpc->mask.clear();
					while (parseItem (&(lng->defineColl), item, name, value))
						if (!FSF.LStricmp (name, L"Pattern")) tmpc->mask = value;
					if (!tmpc->mask.empty())
						lng->commentColl.insert (tmpc);
					else
						delete tmpc;
				}
				else if (!group && !FSF.LStricmp (name, L"Exec"))
				{
					TExec *tmpe = new TExec (lng->defExec);
					while (parseItem (&(lng->defineColl), item, name, value))
						if (!FSF.LStricmp (name, L"Command"))
							tmpe->cmd = value;
						else
							parseExec (tmpe, name, value);
					if (tmpe->title.empty()) tmpe->title = tmpe->cmd;
					if (tmpe->title.empty()) tmpe->cmd.clear();
					lng->execColl.insert (tmpe);
				}
				else if (!group && !FSF.LStricmp (name, L"Compiler"))
				{
					TCompiler *tmpc = new TCompiler ();
					while (parseItem (&(lng->defineColl), item, name, value))
						if (!FSF.LStricmp (name, L"Name"))
							tmpc->title = value;
						else
							parseCompiler (tmpc, name, value);
					if (!tmpc->title.empty())
						lng->compilerColl.insert (tmpc);
					else
						delete tmpc;
				}
				else if (!group && !FSF.LStricmp (name, L"Indent"))
				{
					TIndent *tmpi = new TIndent;
					tmpi->mask.clear(); 
					tmpi->relative.clear();
					tmpi->BracketsMode = 0;
					tmpi->start = 0;
					tmpi->immChar = L'\0';
					for (int i = 0; i < 2; i++) tmpi->indent[i] = 0xFFFF;
					while (parseItem (&(lng->defineColl), item, name, value))
					{
						if (!FSF.LStricmp (name, L"Pattern"))
							tmpi->mask = value;
						else if (!FSF.LStricmp (name, L"Line"))
							tmpi->indent[0] = FSF.atoi (value);
						else if (!FSF.LStricmp (name, L"Next"))
							tmpi->indent[1] = FSF.atoi (value);
						else if (!FSF.LStricmp (name, L"Imm"))
							tmpi->immChar = *value;
						else if (!FSF.LStricmp (name, L"Start"))
							tmpi->start = FSF.atoi (value);
						else if (!FSF.LStricmp (name, L"Relative"))
							tmpi->relative = value;
					}

					if (!tmpi->mask.empty())
					{
						if (tmpi->immChar)
						{
							if (!wcsrchr (lng->imm, tmpi->immChar))
							{
								lng->imm += tmpi->immChar;
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
					tmpb->open.clear(); 
					tmpb->close.clear();
					tmp0->relative.clear();
					tmp0->start = tmp0->immChar = 0;
					tmp0->BracketsMode = 0;
					tmp1->relative.clear();
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
							tmp0->mask = tmpb->open = value;
						else if (!FSF.LStricmp (name, L"Match"))
							tmp1->mask = tmpb->close = value;
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

					if (!tmpb->open.empty() && !tmpb->close.empty())
					{
						if (tmp0->immChar)
						{
							if (!wcschr (lng->imm, tmp0->immChar))
							{
								lng->imm += tmp0->immChar;
							}
						}

						if (tmp1->immChar)
						{
							if (!wcschr (lng->imm, tmp1->immChar))
							{
								lng->imm += tmp1->immChar;
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
	intptr_t	*delCount,
	intptr_t	bounds[][2],
	bool	frommenu = false
)
{
	wchar_t			*regEnd = lng->ignoreCase ? L"\\s*$/i" : L"\\s*$/";
	for (size_t i = 0; i < lng->macroColl.getCount(); i++)
	{
		TMacro	*mm = (TMacro *) (lng->macroColl[i]);
		if (expChar && (expChar != mm->immChar)) continue;
		if (!frommenu && !expChar && mm->immChar) continue;
		if (frommenu && !mm->Name.empty() && (wcscmp (mm->Name, before) == 0)) return (mm);
	}

	for (size_t i = 0; i < lng->macroColl.getCount(); i++)
	{
		TMacro	*mm = (TMacro *) (lng->macroColl[i]);
		if (expChar && (expChar != mm->immChar)) continue;
		if (!mm->Word.empty() && strMatch (after, mm->after, L"/^\\s*", regEnd, 0))
		{
			wchar_t	realBefore[MAX_REG_LEN];
			wcscat (wcscpy (realBefore, mm->before), mm->Word);
			if (strMatch (before, realBefore, L"/^\\s*", regEnd, 0))
			{
				if (delCount)
				{
					regEnd = lng->ignoreCase ? L")$/i" : L")$/";

					intptr_t zero[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15 };
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
	for (size_t i = 0; i < lng->macroColl.getCount(); i++)
	{
		TMacro	*mm = (TMacro *) (lng->macroColl[i]);
		if (wcscmp(mm->FARKey, rKeyp) == 0) return (mm);
	}

	return (NULL);
}

static TMacro *CheckMacroPos (TLang *lng, TMacro *mm, const wchar_t *before, const wchar_t *after)
{
	wchar_t	*regEnd = lng->ignoreCase ? L"\\s*$/i" : L"\\s*$/";
	if (strMatch (after, mm->after, L"/^\\s*", regEnd, 0))
		if (strMatch (before, mm->before, L"/^\\s*", regEnd, 0)) return (mm);
	return (NULL);
}
