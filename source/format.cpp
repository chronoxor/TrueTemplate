struct TFormat : TCollectionItem
{
	String	name;
	String	comm;
	bool	echo;

	TFormat() : echo(true)
	{
	}
};

static void runFormatting (const TFormat *tf, const wchar_t *Name = nullptr)
{
	String	old_cmd(tf->comm);
	wchar_t	new_cmd[MAX_STR_LEN];
	nullUserStrings ();
	if (!scanUserInput (false, L'=', old_cmd)) return ;

	wchar_t						filepath[NM];
	wchar_t						filename[NM];
	FILE						*dstfile;
	FILE						*srcfile;
	int							vblock;
	int							strcount;
	TEditorPos			epos, epos_sel;
	EditorInfoEx			ei;
	EditorGetStringEx egs;

	ExpandEnvironmentStrings (TEMP_TT, filepath, NM);
	if (Name == nullptr)
	{
		Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
		if (ei.BlockType != BTYPE_NONE)
		{
			EditorSaveSelected ();
			EditorGetStr (&egs, ei.BlockStartLine);
			epos_sel = epos = EditorGetPos ();
			epos_sel.Row = ei.BlockStartLine;
			epos_sel.Col = egs.SelStart;
		}
		else
		{
			EditorSaveAll ();
			epos_sel = epos = EditorGetPos ();
			epos_sel.Row = 0;
			epos_sel.Col = 0;
		}

		ExpandEnvironmentStrings (TEMP_TT_TMP, filename, NM);
		srcfile = _wfopen (filename, L"rb");
		if (srcfile == nullptr) return ;
		ExpandEnvironmentStrings (TEMP_TT_SRC_TMP, filename, NM);
		dstfile = _wfopen (filename, L"wt");
		if (dstfile == nullptr) return ;
		fread (&vblock, 1, sizeof (int), srcfile);
		fread (&strcount, 1, sizeof (int), srcfile);
		for (int i = 0; i < strcount; i++)
		{
			wchar_t	tmp;
			if (i != 0) fputwc (L'\n', dstfile);

			int len;
			fread (&len, 1, sizeof (int), srcfile);
			for (int i = 0; i < len; i++)
			{
				fread (&tmp, 1, sizeof (wchar_t), srcfile);
				fputwc (tmp, dstfile);
			}
		}

		fclose (dstfile);
		fclose (srcfile);
	}
	else
	{
		ExpandEnvironmentStrings (TEMP_TT_SRC_TMP, filename, NM);
		CopyFile (Name, filename, FALSE);
	}

	const wchar_t	*p_old = old_cmd;
	wchar_t	*p_new = new_cmd;
	*p_new = L'\0';
	wcscat (p_new, L"%COMSPEC% /C ");
	p_new += wcslen (L"%COMSPEC% /C ");
	while (*p_old)
	{
		if (*p_old == L'=')
		{
			p_old++;
			if (*p_old == L'=')
			{
				*p_new = L'=';
				p_new++;
				p_old++;
			}

			if ((*p_old == L'D') || (*p_old == L'd'))
			{
				*p_new = L'\0';
				wcscat (p_new, TEMP_TT_DST_TMP);
				p_new += wcslen (TEMP_TT_DST_TMP);
				p_old++;
			}

			if ((*p_old == L'S') || (*p_old == L's'))
			{
				*p_new = L'\0';
				wcscat (p_new, TEMP_TT_SRC_TMP);
				p_new += wcslen (TEMP_TT_SRC_TMP);
				p_old++;
			}

			if (*p_old == L'?')
			{
				p_old++;
				while ((*p_old) && (!((*p_old == L'?') && (*(p_old - 1) != L'\\')))) p_old++;
				p_old++;
			}

			if ((*p_old >= L'0') && (*p_old <= L'9'))
			{
				*p_new = L'\0';
				wcscat (p_new, userString[*p_old - L'0']);
				p_new += wcslen (userString[*p_old - L'0']);
				p_old++;
			}
		}
		else
		{
			*p_new = *p_old;
			p_new++;
			p_old++;
		}
	}

	*p_new = L'\0';

	wchar_t temp[MAX_STR_LEN];

	wcscpy(temp, new_cmd);
	ExpandEnvironmentStrings (temp, new_cmd, MAX_STR_LEN);

	ShowFlag = tf->echo;
	ClearFlag = tf->echo;

	HANDLE	hScreen = Info.SaveScreen (0, 0, -1, -1);
	SetConsoleTitle (new_cmd);

	DWORD OldConsoleMode;
	GetConsoleMode (GetStdHandle (STD_INPUT_HANDLE), &OldConsoleMode);
	SetConsoleMode
	(
		GetStdHandle (STD_INPUT_HANDLE),
		ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT
	);

	//ProcessInstall ();

	STARTUPINFO					si;
	PROCESS_INFORMATION pi;
	memset (&si, 0, sizeof (si));
	memset (&pi, 0, sizeof (pi));
	si.cb = sizeof (si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
	si.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
	si.hStdError = GetStdHandle (STD_ERROR_HANDLE);
	if
	(
		CreateProcess
			(
				nullptr,
				new_cmd,
				nullptr,
				nullptr,
				TRUE,
				CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS,
				nullptr,
				filepath,
				&si,
				&pi
			)
	)
	{
		WaitForSingleObject (pi.hProcess, INFINITE);
		CloseHandle (pi.hThread);
		CloseHandle (pi.hProcess);
		putc ('\n', stdout);
	}

	//ProcessUninstall ();

	SetConsoleMode (GetStdHandle (STD_INPUT_HANDLE), OldConsoleMode);
	if (ShowFlag) Info.PanelControl (PANEL_ACTIVE, FCTL_SETUSERSCREEN, 0, nullptr);
	Info.RestoreScreen (hScreen);
	redraw ();

	if (Name == nullptr)
	{
		if (ei.BlockType != BTYPE_NONE)
		{
			Info.EditorControl (-1, ECTL_DELETEBLOCK, 0, nullptr);
			EditorSetPos (epos_sel);
		}
		else
		{
			EditorSelectEx es;
			EditorGetStringEx last;

			EditorGetStr (&last, 0);
			es.BlockType = BTYPE_STREAM;
			es.BlockStartLine = 0;
			es.BlockStartPos = 0;
			es.BlockHeight = ei.TotalLines;
			es.BlockWidth = last.StringLength;
			Info.EditorControl (-1, ECTL_SELECT, 0, (void *) &es);
			Info.EditorControl (-1, ECTL_DELETEBLOCK, 0, nullptr);
			EditorSetPos (epos_sel);
		}

		ExpandEnvironmentStrings (TEMP_TT_DST_TMP, filename, NM);
		dstfile = _wfopen (filename, L"rt");
		if (dstfile == nullptr) return ;

		wchar_t				c[2] = L"x";
		TEditorPos	ep = EditorGetPos ();
		Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);

		intptr_t codeTable = EditorGetCodeTable();
		EditorSetCodeTable (1);					//set dos code table (for normal restoring non-latin text)

		while ((c[0] = fgetwc (dstfile)) != WEOF)
		{
			if (c[0] != L'\n')
				Info.EditorControl (-1, ECTL_INSERTTEXT, 0, c);
			else
			{
				if (vblock == 0)
				{
					Info.EditorControl (-1, ECTL_INSERTSTRING, 0, 0);
				}
				else
				{
					ep.Row++;
					if (ep.Row >= ei.TotalLines) Info.EditorControl (-1, ECTL_INSERTSTRING, 0, 0);
					EditorSetPos (ep);
				}
			}
		}

		fclose (dstfile);
		EditorSetCodeTable (codeTable); //restore editor code table
		Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
		if (epos.Row >= ei.TotalLines) epos.Row = ei.TotalLines - 1;
		EditorGetStr (&egs, epos.Row);
		if (epos.Col >= egs.StringLength) epos.Col = egs.StringLength - 1;
		EditorSetPos (epos);
		EditorSaveRemove ();
	}
	else
	{
		ExpandEnvironmentStrings (TEMP_TT_DST_TMP, filename, NM);
		CopyFile (filename, Name, FALSE);
	}

	ExpandEnvironmentStrings (TEMP_TT_SRC_TMP, filename, NM);
	_wunlink (filename);
	ExpandEnvironmentStrings (TEMP_TT_DST_TMP, filename, NM);
	_wunlink (filename);
}

static void SelectFormatting (const TEInfo *te)
{
	if (te)
	{
		const TLang *lng = langColl[te->lang];
		if (lng)
		{
			if ((autoformat) && (lng->formatColl.getCount () == 1))
			{
				runFormatting (lng->formatColl[0]);
				return ;
			}

			if (lng->formatColl.getCount () > 0)
			{
				FarMenuItemEx *amenu = new FarMenuItemEx[lng->formatColl.getCount ()];
				if (amenu)
				{
					for (size_t i = 0; i < lng->formatColl.getCount (); i++)
					{
						amenu[i].Text = lng->formatColl[i]->name;
						amenu[i].Flags = 0;
					}

					intptr_t res = Info.Menu
						(
							&MainGuid,
							&SelFmtGuid,
							-1,
							-1,
							0,
							FMENU_WRAPMODE,
							GetMsg (MSelectFormatter),
							nullptr,
							nullptr,
							nullptr,
							nullptr,
							amenu,
							lng->formatColl.getCount ()
						);
					if (res != -1) runFormatting (lng->formatColl[res]);
					delete[] amenu;
				}
			}
		}
	}
}

static void FormatMenu (const wchar_t *Name, intptr_t lang)
{
	const TLang *lng = langColl[lang];
	if (lng)
	{
		if ((autoformat) && (lng->formatColl.getCount () == 1))
		{
			runFormatting (lng->formatColl[0], Name);
			return ;
		}

		if (lng->formatColl.getCount () > 0)
		{
			FarMenuItemEx *amenu = new FarMenuItemEx[lng->formatColl.getCount ()];
			if (amenu)
			{
				for (size_t i = 0; i < lng->formatColl.getCount (); i++)
				{
					amenu[i].Text = lng->formatColl[i]->name;
					amenu[i].Flags = 0;
				}

				intptr_t res = Info.Menu
					(
						&MainGuid,
						&SelFmtGuid,
						-1,
						-1,
						0,
						FMENU_WRAPMODE,
						GetMsg (MSelectFormatter),
						nullptr,
						nullptr,
						nullptr,
						nullptr,
						amenu,
						lng->formatColl.getCount ()
					);
				if (res != -1) runFormatting (lng->formatColl[res], Name);
				delete[] amenu;
			}
		}
	}
}
