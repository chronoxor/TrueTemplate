static int inComment (TLang *lng, const wchar_t *text_line, unsigned pos)
{
	if (text_line)
	{
		for (unsigned i = 0; i < lng->commentColl.getCount (); i++)
		{
			TComment	*tmpc = (TComment *) (lng->commentColl[i]);
			wchar_t			*line = (wchar_t *) malloc ((wcslen (text_line) + 1) * sizeof(wchar_t));

			if (line)
			{
				wcscpy (line, text_line);
				if (lng->ignoreCase) StrLower (line); /*!?*/

				int		bounds[2], zero = 0;
				bool	blMatched = (strMatch (line, tmpc->mask, L"/", lng->ignoreCase ? L"/i" : L"/", 1, &bounds, &zero) != 0);
				free (line);

				if (blMatched && pos >= (unsigned) bounds[0] && pos <= (unsigned) bounds[1])
				{
					return (1);
				}
			}
		}
	}

	return (0);
}

static int FindIndent (wchar_t c, TLang *lng, wchar_t *str, int foundBounds[2])
{
	for (unsigned j = 0; j < lng->indentColl.getCount (); j++)
	{
		TIndent *tmpi = (TIndent *) (lng->indentColl[j]);
		wchar_t		ic = ((TIndent *) (lng->indentColl[j]))->immChar;
		if (lng->ignoreCase) ic = (wchar_t) FSF.LLower (ic);
		if ((c == L'\0') || (c == ic))
		{
			wchar_t	*line = (wchar_t *) malloc ((wcslen (tmpi->mask) + 1) * sizeof(wchar_t));

			if (line)
			{
				wcscpy (line, tmpi->mask);

				int		bounds[2];
				bool	blMatched = (strMatch (str, line, L"/^", lng->ignoreCase ? L"$/i" : L"$/", 1, &bounds, &tmpi->start) != 0);
				free (line);

				if (blMatched)
				{
					foundBounds[0] = bounds[0];
					foundBounds[1] = bounds[1];
					return (j);
				}
			}
		}
	}

	return (-1);
}

static int FindBrackets (TLang *lng, wchar_t *str, wchar_t **foundBrackets)
{
	int foundBracketsNum = 0;

	for (unsigned j = 0; j < lng->indentColl.getCount (); j++)
	{
		TIndent *tmpi = (TIndent *) (lng->indentColl[j]);
		wchar_t		*line = (wchar_t *) malloc ((wcslen (tmpi->mask) + 1) * sizeof(wchar_t));

		if (line)
		{
			wcscpy (line, tmpi->mask);
			if (lng->ignoreCase) StrLower (line) /* ?! */ ;

			int		bounds[2];
			bool	blMatched = (strMatch (str, line, L"/^", lng->ignoreCase ? L"$/i" : L"$/", 1, &bounds, &tmpi->start) != 0);
			free (line);

			if (blMatched)
			{
				if (tmpi->indent[0] != 0xFFFF && tmpi->BracketsMode)
				{
					TBracket	*br = (TBracket *) (lng->bracketColl[tmpi->bracketLink]);
					foundBrackets[foundBracketsNum++] = br->open;
				}
			}
		}
	}

	return (foundBracketsNum);
}

static wchar_t *LookForOpenBracket (EditorInfoEx *ei, TEditorPos &p, TLang *lng, wchar_t **openBr, int nOpenBr)
{
	static wchar_t							*c, buff[MAX_STR_LEN];
	static EditorGetStringEx	gs;
	int		ret = -1;
	wchar_t	*closeReg = lng->ignoreCase ? L"$/i" : L"$/";
	while (--p.Row >= 0)
	{
		EditorSetPos (p);
		EditorGetStr (&gs);

		if (gs.StringLength)
		{
			if ((c = FirstNonSpace (gs.StringText)) != NULL)
			{
				FSF.Trim (wcscpy (buff, c));
				if (lng->ignoreCase) StrLower (buff);
				for (int i = 0; i < nOpenBr; i++)
				{
					if (strMatch (buff, openBr[i], L"/^", closeReg, NULL, 0))
					{
						ret = (int)(c - gs.StringText);
						break;
					}
				}

				if (ret != -1) break;

				unsigned	n = lng->bracketColl.getCount ();
				{
					for (unsigned i = 0; i < n; i++)
					{
						TBracket	*br = (TBracket *) (lng->bracketColl[i]);
						if (strMatch (buff, br->close, L"/^", closeReg, NULL, 0))
						{
							wchar_t	*result = NULL, **br4look = new wchar_t *[n];
							int		nbr4look = FindBrackets (lng, buff, br4look);
							if (nbr4look) result = LookForOpenBracket (ei, p, lng, br4look, nbr4look);
							delete[] br4look;
							if (result) break;
							return (NULL);
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

	return (NULL);
}

static wchar_t *FindOpenBracket (EditorInfoEx *ei, TLang *lng, wchar_t **openBr, int n)
{
	TEditorPos	p, o = EditorGetPos ();
	p.Default ();
	p.Row = o.Row;

	wchar_t	*ret = LookForOpenBracket (ei, p, lng, openBr, n);
	EditorSetPos (o);
	return (ret);
}

static wchar_t *prevStringIndent (void)
{
	static wchar_t			buff[MAX_STR_LEN];
	EditorGetStringEx gs;
	int							ret = 0;
	TEditorPos			p, o = EditorGetPos ();
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
				ret = (int)(c - gs.StringText);
				break;
			}
		}
	}

	EditorSetPos (o);
	wmemcpy (buff, gs.StringText, ret);
	buff[ret] = 0;
	return (buff);
}

static int shiftLine (int i)
{
	TEditorPos	pos = EditorGetPos ();
	pluginBusy = 1;
	if (i > 0)
		for (int j = 0; j < i; j++)
			EditorProcessFARKey (VK_TAB, false);
	else if (i < 0)
	{
		EditorInfoEx	ei;
		Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);

		int ind = -i * ei.TabSize;
		for (int j = 0; j < ind; j++) EditorProcessFARKey (VK_BACK, false);
	}

	TEditorPos	pos2 = EditorGetPos ();
	pluginBusy = 0;
	return (pos2.Col - pos.Col);
}

static int TryIndent
(
	EditorInfoEx	*ei,
	int					ret,
	TLang				*lng,
	int					j,
	wchar_t				*str,
	wchar_t				*realStr,
	int					oldIndent,
	int					foundBounds[2]
)
{
	TIndent *tmpi = (TIndent *) (lng->indentColl[j]);
	int			iCurr = tmpi->indent[0];
	int			iNext = tmpi->indent[1];
	if (iCurr != 0xFFFF)
	{
		bool				processIndent = true;
		wchar_t				*opencol = NULL;
		TEditorPos	pos = EditorGetPos ();
		if (*(tmpi->relative))
		{
			wchar_t	*rel = tmpi->relative;
			if ((opencol = FindOpenBracket (ei, lng, &rel, 1)) == NULL) processIndent = false;
		}
		else if (tmpi->BracketsMode)
		{
			unsigned	n = lng->bracketColl.getCount ();
			wchar_t			**br4look = new wchar_t *[n];
			int				nbr4look = FindBrackets (lng, str, br4look);
			processIndent = false;
			if (nbr4look) opencol = FindOpenBracket (ei, lng, br4look, nbr4look);
			if (opencol) processIndent = true;
			delete[] br4look;
		}

		if (processIndent)
		{
			if (opencol == NULL) opencol = prevStringIndent ();

			size_t	olen = wcslen (opencol);
			wchar_t		*ptr = new wchar_t[olen + wcslen (str) + 1];
			wcscat (wcscpy (ptr, opencol), realStr);
			EditorSetStr (ptr);
			EditorSetPos (-1, (int)olen);

			int sh = shiftLine (iCurr);
			EditorSetPos (-1, (int)(pos.Col + olen - oldIndent + sh));
		}
	}

	if (ret)
	{
		EditorProcessReturnKey (1, 0);
		if (tmpi->start)
		{
			pluginBusy = 1;
			for (int k = foundBounds[0]; k < foundBounds[1]; k++) EditorProcessKey (realStr[k]);
			pluginBusy = 0;
		}

		if (iNext != 0xFFFF) shiftLine (iNext);
	}
	else
		redraw ();
	return (IGNORE_EVENT);
}

static bool checkMultipleChoice (TLang *lng, TMacro * &mc, wchar_t *before, wchar_t *after, int len, int bounds[][2])
{
	const wchar_t	*menuPrefix = L"\\~";
	unsigned		pl = wcslen (menuPrefix);
	if ((unsigned) wcslen (mc->MacroText) > pl && !FSF.LStrnicmp (mc->MacroText, menuPrefix, pl))
	{
		unsigned	lc = 0;
		wchar_t			*p = mc->MacroText;
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
			TMacro			**amacro = new TMacro *[lc];
			if (amenu)
			{
				unsigned	i;
				p = mc->MacroText;
				for (i = 0; ((i < lc) && (*p));)
				{
					TMacro	*tm;
					wchar_t		tmpTitle[MAX_STR_LEN];
					wchar_t		tmpBefore[MAX_STR_LEN];
					wcscpy (tmpBefore, before);
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
					tm = FindMacro (lng, tmpBefore, after, 0, NULL, bounds, true);
					amenu[i].Text = new wchar_t[wcslen(tmpTitle) + 1];
					wcscpy(const_cast<wchar_t*>(amenu[i].Text), tmpTitle);
					if (tm)
					{
						amacro[i] = tm;
						amenu[i].Flags &= ~(MIF_SELECTED | MIF_CHECKED | MIF_SEPARATOR);
						i++;
					}
				}

				int		res = Info.Menu
					(
						&MainGuid,
						&CheckMultChoiseGuid,
						-1,
						-1,
						0,
						FMENU_WRAPMODE,
						GetMsg (MSelectMacro),
						NULL,
						NULL,
						NULL,
						NULL,
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

static int TryMacro (EditorInfoEx *ei, TLang *lng, EditorGetStringEx &gs, TEditorPos &ep, wchar_t *line, wchar_t expChar)
{
	if (expChar || !isCharSpace (gs.StringText[ep.Col - 1]))
	{
		if (gs.StringLength != 0)
		{
			int l = gs.StringLength;
			wcsncpy (line, gs.StringText, l + 1);
			for (int i = l; i < MAX_STR_LEN; i++) line[i] = L' ';
			line[MAX_STR_LEN - 1] = L'\0';
		}

		if (!inComment (lng, line, ep.Col))
		{
			wchar_t	before[MAX_STR_LEN];
			wchar_t	*after = line + ep.Col;
			wcscpy (before, line);

			int len = before[ep.Col] = 0;
			if (expChar)
			{
				before[ep.Col] = expChar;
				before[ep.Col + 1] = 0;
			}

			int			bounds[16][2];
			TMacro	*mc = FindMacro (lng, before, after, expChar, &len, bounds);
			if (mc && checkMultipleChoice (lng, mc, before, after, len, bounds))
			{
				if (expChar) len--;
				pluginBusy = 1;
				for (int i = 0; i < len; i++) EditorProcessFARKey (VK_BACK, false);
				pluginBusy = 0;
				RunMacro (mc, before, bounds);
				redraw ();
				return (1);
			}
		}
	}

	return (0);
}

intptr_t WINAPI ProcessEditorInputW(const struct ProcessEditorInputInfo *Info)
{
	if
	(
		pluginBusy ||
		(Info->Rec.EventType != KEY_EVENT && Info->Rec.EventType != 0x8001) ||
		Info->Rec.Event.KeyEvent.bKeyDown == 0
	) return (PROCESS_EVENT);

	int vState = Info->Rec.Event.KeyEvent.dwControlKeyState;
	if (scrollStop && (vState & SCROLLLOCK_ON)) return (PROCESS_EVENT);

	wchar_t filename[NM];
	EditorInfoEx	ei;
	::Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
	::Info.EditorControl (ei.EditorID, ECTL_GETFILENAME, NM, filename);
	initEList ();

	int edId = eList->findID (ei.EditorID);
	if (edId == -1) edId = eListInsert (ei.EditorID, filename);

	TEInfo	*te = (TEInfo *) (*eList)[edId];
	int			id = te->lang;
	if (id != -1)
	{
		TLang *lng = (TLang *) (langColl[id]);
		if (lng)
		{
			//! NEW FILE - START
			if (te->newFile)
			{
				te->newFile = 0;
				if (!Info->Rec.Event.KeyEvent.wVirtualKeyCode)
				{
					if (lng->setCP)
					{
						EditorSetParameter	esp;
						esp.Type = ESPT_CODEPAGE;
						esp.iParam = lng->setCP;
						::Info.EditorControl (ei.EditorID, ECTL_SETPARAM, 0, &esp);
					}

					//Execute init macros
					unsigned	l = 0;
					unsigned	n = 0;
					for (unsigned i = 0; i < lng->macroColl.getCount (); i++)
						if ((((TMacro *) (lng->macroColl[i]))->atStartup))
						{
							l = i;
							n++;
						}

					if (n == 1)
					{
						TMacro	*mm = (TMacro *) (lng->macroColl[l]);
						if (checkMultipleChoice (lng, mm, L"", L"", 0, NULL))
						{
							RunMacro (mm, NULL, NULL);
							redraw ();
						}
					}

					if (n > 1)
					{
						l = 0;

						FarMenuItemEx *pMenu = new FarMenuItemEx[n];
						for (unsigned i = 0; i < lng->macroColl.getCount (); i++)
						{
							if ((((TMacro *) (lng->macroColl[i]))->atStartup))
							{
								if (wcscmp (((TMacro *) (lng->macroColl[i]))->Name, L"") == 0)
									pMenu[l].Text = GetMsg (MUnnamed);
								else
									pMenu[l].Text = ((TMacro *) (lng->macroColl[i]))->Name;
								l++;
							}
						}

						unsigned	res = ::Info.Menu
							(
								&MainGuid,
								&EditorInputGuid,
								-1,
								-1,
								0,
								FMENU_WRAPMODE,
								GetMsg (MSelectMacro),
								NULL,
								NULL,
								NULL,
								NULL,
								pMenu,
								n
							);
						if (res != -1)
						{
							n = 0;
							for (unsigned i = 0; i < lng->macroColl.getCount (); i++)
								if ((((TMacro *) (lng->macroColl[i]))->atStartup))
								{
									if (n == res)
									{
										l = i;
										break;
									}

									n++;
								}

							TMacro	*mm = (TMacro *) (lng->macroColl[l]);
							if (checkMultipleChoice (lng, mm, L"", L"", 0, NULL))
							{
								RunMacro (mm, NULL, NULL);
								redraw ();
							}
						}

						delete[] pMenu;
					}

					return (IGNORE_EVENT);
				}
			} //! NEW FILE - END

			// Don't handle other editor events for disabled plugin.
			if (pluginStop)
				return (PROCESS_EVENT);

			wchar_t	line[MAX_STR_LEN];
			for (int i = 0; i < MAX_STR_LEN; i++) line[i] = L' ';
			line[MAX_STR_LEN - 1] = L'\0';

			TEditorPos	epos = EditorGetPos ();
			TMacro			*fm;

			//! MACRO EXISTS - START
			if ((fm = FindMacroKey (lng, &Info->Rec)) != NULL)
			{
				EditorGetStringEx gs;
				EditorGetStr (&gs);

				if (gs.StringLength != 0)
				{
					int l = gs.StringLength;
					wcsncpy (line, gs.StringText, l + 1);
					for (int i = l; i < MAX_STR_LEN; i++) line[i] = L' ';
					line[MAX_STR_LEN - 1] = L'\0';
				}

				if (!inComment (lng, line, epos.Col))
				{
					wchar_t	before[MAX_STR_LEN], *after = line + epos.Col;
					wcscpy (before, line);
					before[epos.Col] = 0;
					if (CheckMacroPos (lng, fm, before, after))
					{
						if (checkMultipleChoice (lng, fm, before, after, 0, NULL))
						{
							RunMacro (fm, NULL, NULL);
							redraw ();
							return (IGNORE_EVENT);
						}
					}
				}

				return (PROCESS_EVENT);
			} //! MACRO EXISTS - END

			wchar_t fKey[256];
			FSF.FarInputRecordToName (&Info->Rec, fKey, 256);
			vState &= (SHIFT_PRESSED | RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED | RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED);

			int spORret = ((wcscmp(fKey, L"Enter") == 0) || (wcscmp(fKey, defExpandFKey) == 0));
			if (!spORret && (vState &~SHIFT_PRESSED)) return (PROCESS_EVENT);

			int		inImm = 0, inImmExp = 0;
			wchar_t	vChar = Info->Rec.Event.KeyEvent.uChar.UnicodeChar;
			if (!spORret)
			{
				if (lng->ignoreCase)
				{
					wchar_t	lwrStr[MAX_REG_LEN], c = (wchar_t) (FSF.LLower (vChar));
					inImm = wcschr (StrLower (wcscpy (lwrStr, lng->imm)), c) != NULL;
					inImmExp = wcschr (StrLower (wcscpy (lwrStr, lng->immExp)), c) != NULL;
				}
				else
				{
					inImm = wcschr (lng->imm, vChar) != NULL;
					inImmExp = wcschr (lng->immExp, vChar) != NULL;
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
				int							ret = PROCESS_EVENT;
				EditorGetStringEx gs;
				EditorGetStr (&gs);
				if (gs.StringLength != 0)
				{
					int l = gs.StringLength;
					wcsncpy (line, gs.StringText, l + 1);
					for (int i = l; i < MAX_STR_LEN; i++) line[i] = L' ';
					line[MAX_STR_LEN - 1] = L'\0';
				}

				if (!inComment (lng, line, epos.Col))
				{
					if (!spORret)
					{
						pluginBusy = 1;
						EditorProcessKey (vChar);
						pluginBusy = 0;
						ret = IGNORE_EVENT;
						EditorGetStr (&gs);
						if (gs.StringLength != 0)
						{
							int l = gs.StringLength;
							wcsncpy (line, gs.StringText, l + 1);
							for (int i = l; i < MAX_STR_LEN; i++) line[i] = L' ';
							line[MAX_STR_LEN - 1] = L'\0';
						}
					}

					wchar_t	*pstrText = FirstNonSpace (gs.StringText);
					if (pstrText == NULL) return (ret);

					int		lstr = wcslen (pstrText);
					wchar_t	*str = (wchar_t *) malloc ((lstr + 1) * sizeof(wchar_t));
					if (str)
					{
						int spacesCount = (int)(pstrText - gs.StringText);
						wcsncpy (str, pstrText, lstr + 1);
						FSF.Trim (line);

						if (lng->ignoreCase)
						{
							StrLower (line);
							vChar = (wchar_t) FSF.LLower (vChar);
						}

						int bounds[2];
						int j = FindIndent (spORret ? L'\0' : vChar, lng, line, bounds);
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
