static bool inComment (const TLang *lng, const wchar_t *text_line, intptr_t pos)
{
	if (text_line)
	{
		for (size_t i = 0; i < lng->commentColl.getCount(); i++)
		{
			const TComment	*tmpc = lng->commentColl[i];
			intptr_t	bounds[2], zero = 0;
			bool	blMatched = strMatch(text_line, tmpc->mask, L"/", lng->ignoreCase ? L"/i" : L"/", 1, &bounds, &zero);
			if (blMatched && pos >= bounds[0] && pos <= bounds[1])
			{
				return true;
			}
		}
	}

	return false;
}

static intptr_t FindIndent (wchar_t c, const TLang *lng, const wchar_t *str, intptr_t foundBounds[2])
{
	for (size_t j = 0; j < lng->indentColl.getCount(); j++)
	{
		const TIndent *tmpi = lng->indentColl[j];
		wchar_t		ic = lng->indentColl[j]->immChar;
		if (lng->ignoreCase) ic = (wchar_t) FSF.LLower (ic);
		if ((c == L'\0') || (c == ic))
		{
			intptr_t	bounds[2];
			if (strMatch(str, tmpi->mask, L"/^", lng->ignoreCase ? L"$/i" : L"$/", 1, &bounds, &tmpi->start))
			{
				foundBounds[0] = bounds[0];
				foundBounds[1] = bounds[1];
				return (j);
			}
		}
	}

	return (-1);
}

static size_t FindBrackets (const TLang *lng, const wchar_t *str, const wchar_t **foundBrackets)
{
	size_t foundBracketsNum = 0;

	for (size_t j = 0; j < lng->indentColl.getCount(); j++)
	{
		const TIndent *tmpi = lng->indentColl[j];
		wchar_t	*line = new wchar_t[tmpi->mask.length() + 1];

		if (line)
		{
			wcscpy (line, tmpi->mask);
			if (lng->ignoreCase) StrLower (line) /* ?! */ ;

			intptr_t	bounds[2];
			bool	blMatched = strMatch (str, line, L"/^", lng->ignoreCase ? L"$/i" : L"$/", 1, &bounds, &tmpi->start);
			delete[] line;

			if (blMatched)
			{
				if (tmpi->indent[0] != 0xFFFF && tmpi->BracketsMode)
				{
					const TBracket	*br = lng->bracketColl[tmpi->bracketLink];
					foundBrackets[foundBracketsNum++] = br->open;
				}
			}
		}
	}

	return (foundBracketsNum);
}

static const wchar_t *LookForOpenBracket (const EditorInfoEx *ei, TEditorPos &p, const TLang *lng, const wchar_t **openBr, size_t nOpenBr)
{
	static wchar_t							*c, buff[MAX_STR_LEN];
	static EditorGetStringEx	gs;
	ptrdiff_t	ret = -1;
	const wchar_t	*closeReg = lng->ignoreCase ? L"$/i" : L"$/";
	while (--p.Row >= 0)
	{
		EditorSetPos (p);
		EditorGetStr (&gs);

		if (gs.StringLength)
		{
			if ((c = FirstNonSpace (gs.StringText)) != nullptr)
			{
				FSF.Trim (wcscpy (buff, c));
				if (lng->ignoreCase) StrLower (buff);
				for (size_t i = 0; i < nOpenBr; i++)
				{
					if (strMatch (buff, openBr[i], L"/^", closeReg, 0, 0))
					{
						ret = (ptrdiff_t)(c - gs.StringText);
						break;
					}
				}

				if (ret != -1) break;

				size_t	n = lng->bracketColl.getCount ();
				{
					for (size_t i = 0; i < n; i++)
					{
						const TBracket	*br = lng->bracketColl[i];
						if (strMatch (buff, br->close, L"/^", closeReg, 0, 0))
						{
							const wchar_t	*result = nullptr;
							const wchar_t **br4look = new const wchar_t *[n];
							size_t nbr4look = FindBrackets (lng, buff, br4look);
							if (nbr4look) result = LookForOpenBracket (ei, p, lng, br4look, nbr4look);
							delete[] br4look;
							if (result) break;
							return (nullptr);
						}
					}
				}
			}
		}
	}

	if (ret != -1)
	{
		wmemcpy (buff, gs.StringText, ret);
		buff[ret] = 0;
		return (buff);
	}

	return (nullptr);
}

static const wchar_t *FindOpenBracket (const EditorInfoEx *ei, const TLang *lng, const wchar_t **openBr, size_t n)
{
	TEditorPos	p, o = EditorGetPos ();
	p.Default ();
	p.Row = o.Row;

	const wchar_t	*ret = LookForOpenBracket (ei, p, lng, openBr, n);
	EditorSetPos (o);
	return (ret);
}

static wchar_t *prevStringIndent (void)
{
	static wchar_t		buff[MAX_STR_LEN];
	EditorGetStringEx	gs;
	ptrdiff_t					ret = 0;
	TEditorPos				p, o = EditorGetPos ();
	p.Default ();
	p.Row = o.Row;
	while (--p.Row >= 0)
	{
		EditorSetPos (p);
		EditorGetStr (&gs);
		if (gs.StringLength)
		{
			const wchar_t	*c = FirstNonSpace (gs.StringText);
			if (c)
			{
				ret = (ptrdiff_t)(c - gs.StringText);
				break;
			}
		}
	}

	EditorSetPos (o);
	wmemcpy (buff, gs.StringText, ret);
	buff[ret] = 0;
	return (buff);
}

static intptr_t shiftLine(intptr_t i)
{
	TEditorPos	pos = EditorGetPos ();
	pluginBusy = 1;
	if (i > 0)
	{
		for (intptr_t j = 0; j < i; j++)
			EditorProcessFARKey (VK_TAB, false);
	}
	else if (i < 0)
	{
		EditorInfoEx	ei;
		Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);

		intptr_t ind = -i * ei.TabSize;
		for (intptr_t j = 0; j < ind; j++) EditorProcessFARKey(VK_BACK, false);
	}

	TEditorPos	pos2 = EditorGetPos ();
	pluginBusy = 0;
	return (pos2.Col - pos.Col);
}

static int TryIndent
(
	const EditorInfoEx	*ei,
	bool					ret,
	const TLang		*lng,
	intptr_t			j,
	wchar_t				*str,
	wchar_t				*realStr,
	intptr_t			oldIndent,
	intptr_t			foundBounds[2]
)
{
	const TIndent		*tmpi = lng->indentColl[j];
	intptr_t	iCurr = tmpi->indent[0];
	intptr_t	iNext = tmpi->indent[1];
	if (iCurr != 0xFFFF)
	{
		bool					processIndent = true;
		const wchar_t	*opencol = nullptr;
		TEditorPos	pos = EditorGetPos ();
		if (!tmpi->relative.empty())
		{
			const wchar_t	*rel = tmpi->relative;
			if ((opencol = FindOpenBracket (ei, lng, &rel, 1)) == nullptr) processIndent = false;
		}
		else if (tmpi->BracketsMode)
		{
			size_t	n = lng->bracketColl.getCount ();
			const wchar_t	**br4look = new const wchar_t *[n];
			size_t	nbr4look = FindBrackets (lng, str, br4look);
			processIndent = false;
			if (nbr4look) opencol = FindOpenBracket (ei, lng, br4look, nbr4look);
			if (opencol) processIndent = true;
			delete[] br4look;
		}

		if (processIndent)
		{
			if (opencol == nullptr) opencol = prevStringIndent ();

			intptr_t	olen = wcslen(opencol);
			wchar_t		*ptr = new wchar_t[olen + wcslen (str) + 1];
			wcscat (wcscpy (ptr, opencol), realStr);
			EditorSetStr (ptr);
			EditorSetPos (-1, olen);

			intptr_t sh = shiftLine(iCurr);
			EditorSetPos (-1, pos.Col + olen - oldIndent + sh);
		}
	}

	if (ret)
	{
		EditorProcessReturnKey (1, 0);
		if (tmpi->start)
		{
			pluginBusy = 1;
			for (intptr_t k = foundBounds[0]; k < foundBounds[1]; k++) EditorProcessKey(realStr[k]);
			pluginBusy = 0;
		}

		if (iNext != 0xFFFF) shiftLine (iNext);
	}
	else
		redraw ();
	return (IGNORE_EVENT);
}

static bool checkMultipleChoice (const TLang *lng, const TMacro * &mc, wchar_t *before, wchar_t *after, intptr_t len, intptr_t bounds[][2])
{
	const wchar_t	*menuPrefix = L"\\~";
	size_t	pl = wcslen (menuPrefix);
	if (mc->MacroText.length() > pl && !FSF.LStrnicmp (mc->MacroText, menuPrefix, pl))
	{
		size_t	lc = 0;
		const wchar_t	*p = mc->MacroText;
		while (*p)
		{
			if (p[0] == L'\\')
			{
				if (p[1] == L'~')
				{
					lc++;
					p += 2;
				}
				else
				{
					p++;
					if (p[0] == L'\0')
						break;
					else
						p++;
				}
			}
			else
				p++;
		}

		if (lc)
		{
			FarMenuItemEx *amenu = new FarMenuItemEx[lc];
			const TMacro	**amacro = new const TMacro *[lc];
			if (amenu)
			{
				size_t	i;
				p = mc->MacroText;
				for (i = 0; ((i < lc) && (*p));)
				{
					const TMacro	*tm;
					wchar_t		tmpTitle[MAX_STR_LEN];
					wchar_t		tmpBefore[MAX_STR_LEN];
					tmpTitle[0] = 0;
					wcscpy(tmpBefore, before);
					tmpBefore[wcslen (tmpBefore) - len] = 0;

					wchar_t	*p1 = tmpBefore + wcslen (tmpBefore);
					if ((p[0] == L'\\') && (p[1] == L'~')) p += 2;
					while (*p)
					{
						if (p[0] == L'\\')
						{
							if (p[1] == L'~')
							{
								p += 2;
								break;
							}
							else
							{
								p++;
								if (p[0] == L'\0')
									break;
								else
								{
									*p1 = *p;
									p++;
									p1++;
								}
							}
						}
						else
						{
							if (*p == L'=')
							{
								p++;
								*p1 = L'\0';
								p1 = tmpTitle;
							}
							else
							{
								*p1 = *p;
								p++;
								p1++;
							}
						}
					}

					*p1 = L'\0';
					tm = FindMacro (lng, tmpBefore, after, 0, nullptr, bounds, true);
					amenu[i].Text = new wchar_t[wcslen(tmpTitle) + 1];
					wcscpy(const_cast<wchar_t*>(amenu[i].Text), tmpTitle);
					if (tm)
					{
						amacro[i] = tm;
						amenu[i].Flags &= ~(MIF_SELECTED | MIF_CHECKED | MIF_SEPARATOR);
						i++;
					}
				}

				intptr_t res = Info.Menu
					(
						&MainGuid,
						&CheckMultChoiseGuid,
						-1,
						-1,
						0,
						FMENU_WRAPMODE,
						GetMsg (MSelectMacro),
						nullptr,
						nullptr,
						nullptr,
						nullptr,
						amenu,
						i
					);
				bool	retval = false;
				if (res != -1 && amacro[res])
				{
					mc = amacro[res];
					retval = true;
				}

				for (i = 0; i < lc; i++)
					delete[] amenu[i].Text;

				delete[] amenu;
				delete[] amacro;
				return (retval);
			}
		}
	}

	return (true);
}

static bool TryMacro (EditorInfoEx *ei, const TLang *lng, EditorGetStringEx &gs, TEditorPos &ep, wchar_t *line, wchar_t expChar)
{
	if (expChar || !isCharSpace (gs.StringText[ep.Col - 1]))
	{
		if (gs.StringLength != 0)
		{
			intptr_t l = gs.StringLength;
			wcsncpy (line, gs.StringText, l + 1);
			for (intptr_t i = l; i < MAX_STR_LEN; i++) line[i] = L' ';
			line[MAX_STR_LEN - 1] = L'\0';
		}

		if (!inComment (lng, line, ep.Col))
		{
			wchar_t	before[MAX_STR_LEN];
			wchar_t	*after = line + ep.Col;
			wcscpy (before, line);

			intptr_t len = 0;
			before[ep.Col] = 0;
			if (expChar)
			{
				before[ep.Col] = expChar;
				before[ep.Col + 1] = 0;
			}

			intptr_t	bounds[16][2];
			const TMacro	*mc = FindMacro (lng, before, after, expChar, &len, bounds);
			if (mc && checkMultipleChoice (lng, mc, before, after, len, bounds))
			{
				if (expChar) len--;
				pluginBusy = 1;
				for (intptr_t i = 0; i < len; i++) EditorProcessFARKey (VK_BACK, false);
				pluginBusy = 0;
				RunMacro (mc, before, bounds);
				redraw ();
				return true;
			}
		}
	}

	return false;
}

intptr_t WINAPI ProcessEditorInputW(const struct ProcessEditorInputInfo *Info)
{
	if
	(
		pluginBusy ||
		(Info->Rec.EventType != KEY_EVENT && Info->Rec.EventType != 0x8001) ||
		Info->Rec.Event.KeyEvent.bKeyDown == 0
	) return (PROCESS_EVENT);

	DWORD vState = Info->Rec.Event.KeyEvent.dwControlKeyState;
	if (scrollStop && (vState & SCROLLLOCK_ON)) return (PROCESS_EVENT);

	wchar_t filename[NM];
	EditorInfoEx	ei;
	::Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
	::Info.EditorControl (ei.EditorID, ECTL_GETFILENAME, NM, filename);
	initEList ();

	ptrdiff_t edId = eList->findID (ei.EditorID);
	if (edId == -1) edId = eListInsert (ei.EditorID, filename);

	TEInfo	  *te = (*eList)[edId];
	if (te->lang != -1)
	{
		const TLang *lng = langColl[te->lang];
		if (lng)
		{
			// Don't handle other editor events for disabled plugin.
			if (pluginStop)
				return (PROCESS_EVENT);

			wchar_t	line[MAX_STR_LEN];
			for (intptr_t i = 0; i < MAX_STR_LEN; i++) line[i] = L' ';
			line[MAX_STR_LEN - 1] = L'\0';

			TEditorPos	epos = EditorGetPos ();
			const TMacro			*fm;

			//! MACRO EXISTS - START
			if ((fm = FindMacroKey (lng, &Info->Rec)) != nullptr)
			{
				EditorGetStringEx gs;
				EditorGetStr (&gs);

				if (gs.StringLength != 0)
				{
					intptr_t l = gs.StringLength;
					wcsncpy (line, gs.StringText, l + 1);
					for (intptr_t i = l; i < MAX_STR_LEN; i++) line[i] = L' ';
					line[MAX_STR_LEN - 1] = L'\0';
				}

				if (!inComment (lng, line, epos.Col))
				{
					wchar_t	before[MAX_STR_LEN], *after = line + epos.Col;
					wcscpy (before, line);
					before[epos.Col] = 0;
					if (CheckMacroPos (lng, fm, before, after))
					{
						if (checkMultipleChoice (lng, fm, before, after, 0, nullptr))
						{
							RunMacro (fm, nullptr, nullptr);
							redraw ();
							return (IGNORE_EVENT);
						}
					}
				}

				return (PROCESS_EVENT);
			} //! MACRO EXISTS - END

			wchar_t fKey[256];
			FSF.FarInputRecordToName (&Info->Rec, fKey, _countof(fKey));
			vState &= (SHIFT_PRESSED | RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED);

			bool spORret = ((wcscmp(fKey, L"Enter") == 0) || (wcscmp(fKey, defExpandFKey) == 0));
			if (!spORret && (vState &~SHIFT_PRESSED)) return (PROCESS_EVENT);

			bool	inImm = false, inImmExp = false;
			wchar_t	vChar = Info->Rec.Event.KeyEvent.uChar.UnicodeChar;
			WORD		keycode = Info->Rec.Event.KeyEvent.wVirtualKeyCode;
			if (!spORret)
			{
				if (lng->ignoreCase)
				{
					wchar_t	lwrStr[MAX_REG_LEN], c = (wchar_t) (FSF.LLower (vChar));
					inImm = wcschr (StrLower (wcscpy (lwrStr, lng->imm)), c) != nullptr;
					inImmExp = wcschr (StrLower (wcscpy (lwrStr, lng->immExp)), c) != nullptr;
				}
				else
				{
					inImm = wcschr (lng->imm, vChar) != nullptr;
					inImmExp = wcschr (lng->immExp, vChar) != nullptr;
				}
			}

			if (!(spORret || (vChar && (inImm || inImmExp)))) return (PROCESS_EVENT);
			if ((wcscmp(fKey, defExpandFKey) == 0) || inImmExp)
			{
				EditorGetStringEx gs;
				EditorGetStr (&gs);
				return (TryMacro (&ei, lng, gs, epos, line, inImmExp ? vChar : L'\0') ? IGNORE_EVENT : PROCESS_EVENT);
			}
			else
			{
				intptr_t					ret = PROCESS_EVENT;
				EditorGetStringEx gs;
				EditorGetStr (&gs);
				if (gs.StringLength != 0)
				{
					intptr_t l = gs.StringLength;
					wcsncpy (line, gs.StringText, l + 1);
					for (intptr_t i = l; i < MAX_STR_LEN; i++) line[i] = L' ';
					line[MAX_STR_LEN - 1] = L'\0';
				}

				if (!inComment (lng, line, epos.Col))
				{
					if (!spORret)
					{
						pluginBusy = 1;
						EditorProcessKey (vChar, keycode);
						pluginBusy = 0;
						ret = IGNORE_EVENT;
						EditorGetStr (&gs);
						if (gs.StringLength != 0)
						{
							intptr_t l = gs.StringLength;
							wcsncpy (line, gs.StringText, l + 1);
							for (intptr_t i = l; i < MAX_STR_LEN; i++) line[i] = L' ';
							line[MAX_STR_LEN - 1] = L'\0';
						}
					}

					wchar_t	*pstrText = FirstNonSpace (gs.StringText);
					if (pstrText == nullptr) return (ret);

					size_t	lstr = wcslen (pstrText);
					wchar_t	*str = (wchar_t *) malloc ((lstr + 1) * sizeof(wchar_t));
					if (str)
					{
						intptr_t spacesCount = (intptr_t)(pstrText - gs.StringText);
						wcsncpy (str, pstrText, lstr + 1);
						FSF.Trim (line);

						if (lng->ignoreCase)
						{
							StrLower (line);
							vChar = (wchar_t) FSF.LLower (vChar);
						}

						intptr_t bounds[2];
						intptr_t j = FindIndent (spORret ? L'\0' : vChar, lng, line, bounds);
						if (j != -1) ret = TryIndent (&ei, spORret, lng, j, line, str, spacesCount, bounds);
						free (str);
					}

					return (ret);
				}
			}
		}
	}

	return (PROCESS_EVENT);
}

static void InsertTemplate(intptr_t EditorID, const TLang *lng)
{
	if (lng->setCP)
	{
		EditorSetParameter	esp;
		esp.Type = ESPT_CODEPAGE;
		esp.iParam = lng->setCP;
		::Info.EditorControl(EditorID, ECTL_SETPARAM, 0, &esp);
	}

	//Execute init macros
	size_t	l = 0;
	size_t	n = 0;
	for (size_t i = 0; i < lng->macroColl.getCount(); i++)
		if (lng->macroColl[i]->atStartup)
		{
		l = i;
		n++;
		}

	if (n == 1)
	{
		const TMacro	*mm = lng->macroColl[l];
		if (checkMultipleChoice(lng, mm, L"", L"", 0, nullptr))
		{
			RunMacro(mm, nullptr, nullptr);
			redraw();
		}
	}

	if (n > 1)
	{
		l = 0;

		FarMenuItemEx *pMenu = new FarMenuItemEx[n];
		for (size_t i = 0; i < lng->macroColl.getCount(); i++)
		{
			if (lng->macroColl[i]->atStartup)
			{
				if (lng->macroColl[i]->Name.empty())
					pMenu[l].Text = GetMsg(MUnnamed);
				else
					pMenu[l].Text = lng->macroColl[i]->Name;
				l++;
			}
		}

		intptr_t	res = ::Info.Menu
			(
			&MainGuid,
			&EditorInputGuid,
			-1,
			-1,
			0,
			FMENU_WRAPMODE,
			GetMsg(MSelectMacro),
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			pMenu,
			n
			);
		if (res != -1)
		{
			n = 0;
			for (size_t i = 0; i < lng->macroColl.getCount(); i++)
				if (lng->macroColl[i]->atStartup)
				{
				if (n == res)
				{
					l = i;
					break;
				}

				n++;
				}

			const TMacro	*mm = lng->macroColl[l];
			if (checkMultipleChoice(lng, mm, L"", L"", 0, nullptr))
			{
				RunMacro(mm, nullptr, nullptr);
				redraw();
			}
		}

		delete[] pMenu;
	}


}
