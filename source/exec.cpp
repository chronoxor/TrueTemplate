struct TExec : TCollectionItem
{
	TExec();
	String		title, cmd, compiler;
	String		enable, disable;
	TJumpType jumpType;
	TSaveType saveType;
	TCDType		cd;
	bool			searchBase, echo;
	void defaults (void)
	{
		title.clear(); cmd.clear(); compiler.clear();
		enable.clear(); disable.clear();
		saveType = saCurrent;
		jumpType = jtSmart;
		cd = cdNone;
		searchBase = false;
		echo = true;
	}
};

TExec::TExec ()
{
	defaults ();
}

static wchar_t cmd[MAX_STR_LEN];
static wchar_t cmdpath[MAX_STR_LEN];

static wchar_t *makeCmdLine
(
	bool			ask,
	wchar_t			*buffer,
	const wchar_t	*mask,
	const wchar_t	*dir,
	const wchar_t	*fn,
	const wchar_t	*title = nullptr,
	const wchar_t	macro = L'='
)
{
	static wchar_t tmp[NM];
	static wchar_t n1[NM], drv1[NM], dir1[NM], fil1[NM], ext1[NM];
	static wchar_t n2[NM], drv2[NM], dir2[NM], fil2[NM], ext2[NM];
	wchar_t				*i, *j, *e1 = ext1, *e2 = ext2, *smartQuote = nullptr;
	fExpand (wcscpy (tmp, fn), dir);
	GetFullPathName (tmp, _countof(n1), n1, &j);
	fnSplit (n1, drv1, dir1, fil1, ext1);
	GetShortPathName (tmp, n2, _countof(n2));
	fnSplit (n2, drv2, dir2, fil2, ext2);
	wcscat (wcscat (wcscat (wcscpy (n1, drv1), dir1), fil1), ext1);
	wcscat (wcscat (wcscat (wcscpy (n2, drv2), dir2), fil2), ext2);
	if (*e1 == L'.') e1++;
	if (*e2 == L'.') e2++;
	i = (wchar_t *) mask;
	if (ask)
	{
		nullUserStrings ();
		if (!scanUserInput (false, L'=', mask, title)) return (nullptr);
	}

	*tmp = 0;
	*(j = buffer) = 0;
	while (*i)
	{
		int		dosName = 0, noDrive = 0, noBackslash = 0, doQuote = 1;
		wchar_t	*m = wcschr (i, macro);
		if (m)
		{
			*m = 0;
			wcscat (j, i);
			j = wcschr (j, 0);
			*m = macro;
			if (*++m == macro)
			{
				j[0] = macro;
				j[1] = 0;
				j = wcschr (j, 0);
				i = m + 1;
			}
			else if (ask && wcschr (L"0123456789?", *m))
			{
				if (*m != L'?') wcscpy (j, userString[*m - L'0']);
				j = wcschr (j, 0);
				if (*m == L'?')
				{
					m++;
					while (*m)
					{
						if ((m[0] == L'\\') && ((m[1] == L'\\') || (m[1] == L'\'') || (m[1] == L'?')))
						{
							m++;
							m++;
						}
						else
						{
							if (*m == L'?')
								break;
							else
								m++;
						}
					}
				}

				i = m + 1;
			}
			else
			{
				int modifyer = 1;
				while (modifyer)
				{
					switch (*m)
					{
					case L'd':
						dosName = 1;
						break;
					case L'm':
						noDrive = 1;
						break;
					case L't':
						noBackslash = 1;
						break;
					case L'o':
						doQuote = 0;
						break;
					default:
						m--;
						modifyer = 0;
						break;
					}

					m++;
				}

				switch (*m)
				{
				case L'M':
					wcscat (j, drv1);
					break;
				case L'N':
					wcscat (j, quote (doQuote, wcscpy (tmp, dosName ? fil2 : fil1)));
					break;
				case L'E':
					if (*(m + 1) == L's')
					{
						EditorGetStringEx egs;
						EditorGetStr (&egs);
						wcsncpy (j + wcslen (j), egs.StringText, egs.StringLength + 1);
						m++;
					}
					else if (*(m + 1) == L'p')
					{
						TEditorPos	pos = EditorGetPos ();
						FSF.itoa64 (pos.Row + 1, tmp, 10);
						wcscat (j, tmp);
						m++;
					}
					else if (*(m + 1) == L'c')
					{
						TEditorPos	pos = EditorGetPos ();
						FSF.itoa64 (pos.Col + 1, tmp, 10);
						wcscat (j, tmp);
						m++;
					}
					else
						wcscat (j, quote (doQuote, wcscpy (tmp, dosName ? e2 : e1)));
					break;
				case L'F':
					wcscpy (tmp, dosName ? fil2 : fil1);
					if (*(dosName ? e2 : e1)) wcscat (wcscat (tmp, L"."), dosName ? e2 : e1);
					wcscat (j, quote (doQuote, tmp));
					break;
				case L'D':
					if (noDrive)
						*tmp = 0;
					else
						wcscpy (tmp, drv1);
					wcscat (tmp, dosName ? dir2 : dir1);
					if (noBackslash) delBackslash (tmp);
					wcscat (j, quote (doQuote, tmp));
					break;
				case L'P':
					if (noDrive)
					{
						wcscpy (tmp, dosName ? dir2 : dir1);
						wcscat (tmp, dosName ? fil2 : fil1);
						if (*(dosName ? e2 : e1)) wcscat (wcscat (tmp, L"."), dosName ? e2 : e1);
						wcscat (j, quote (doQuote, tmp));
					}
					else
						wcscat (j, quote (doQuote, wcscat (tmp, dosName ? n2 : n1)));
					break;
				case L'\'':
					if (smartQuote)
					{
						for (wchar_t *p = smartQuote; *p; p++)
							if (*p == L'\"') *p = L'\'';
						quote (1, smartQuote);
						smartQuote = nullptr;
					}
					else
						smartQuote = wcschr (j, 0);
					break;
				}

				i = m + 1;
				j = wcschr (j, 0);
			}
		}
		else
			break;
	}

	wcscat (j, i);
	return (buffer);
}

static void execute(TCollection<TOutputLine> *coll, const wchar_t *title, bool showExec)
{
	HANDLE	hScreen = Info.SaveScreen (0, 0, -1, -1);
	if (!showExec)
	{
		const wchar_t	*MsgItems[] = { GetMsg (MTitle), title };
		Info.Message (&MainGuid, &ExecGuid, 0, nullptr, MsgItems, _countof (MsgItems), 0);
	}

	ExecConsoleApp (cmd, cmdpath, coll, true, showExec);
	if (showExec) Info.PanelControl (0, FCTL_SETUSERSCREEN, 0, nullptr);
	Info.RestoreScreen (hScreen);
	redraw ();
}
