struct TCompiler
{
	TCompiler();
	TCompiler (const TCompiler &);
	wchar_t	title[MAX_REG_LEN], err[MAX_STR_LEN];
	int		line, col, fileMatch;
	bool	searchCol;
	void defaults (void)
	{
		*title = *err = 0;
		line = col = fileMatch = -1;
		searchCol = false;
	}
};

TCompiler::TCompiler ()
{
	defaults ();
}

TCompiler::TCompiler (const TCompiler &e)
{
	wcscpy (title, e.title);
	wcscpy (err, e.err);
	line = e.line;
	col = e.col;
	searchCol = e.searchCol;
	fileMatch = e.fileMatch;
}

TEICollection *eList = NULL;

static void initEList (void)
{
	if (eList == NULL) eList = new TEICollection;
}

static int findLngID (const wchar_t *fn)
{
	for (unsigned i = 0; i < langColl.getCount (); i++)
	{
		TLang *lng = (TLang *) (langColl[i]);
		if (matched (lng->mask, fn)) return (i);
	}

	return (-1);
}

static int eListInsert (int EditorID, wchar_t* filename)
{
	int newFile = !isFile (filename);
	return (eList->insert (EditorID, findLngID (filename), filename, newFile));
}

static TCollection 		*compilerColl = NULL;
static FarMenuItemEx		*compilerOut = NULL;
static TCollection		*errColl = NULL;
static int						compilerOutN = 0;
static int						compilerOutP = -1;

struct TErrData
{
	wchar_t	fn[NM];
	int		line, col, msgCount;
	wchar_t	message[64][64];
	wchar_t	colSearch[MAX_REG_LEN];
};

static bool showErrorMsgBox (int res)
{
	int n = (int) compilerOut[res].UserData - 1;
	if (errColl && (n >= 0) && (n < (int) errColl->getCount ()))
	{
		TErrData		*err = (TErrData *) ((*errColl)[n]);
		const wchar_t	*MsgItems[] =
		{
			cmd, err->fn, L"\x01", err->message[0], err->message[1], err->message[2], err->message[3], err->message[4],
			err->message[5], err->message[6], err->message[7], err->message[8], err->message[9], err->message[10],
			err->message[11], err->message[12], err->message[13], err->message[14], err->message[15], err->message[16],
			err->message[17], err->message[18], err->message[19]
		};
		redraw ();
		Info.Message (&MainGuid, &ErrorMsgGuid, FMSG_MB_OK | FMSG_LEFTALIGN, NULL, MsgItems, err->msgCount + 3, 0);
		return (true);
	}

	return (false);
}

static void jumpToError (EditorInfoEx *ei, const wchar_t* path, int aj, bool showMsgBox)
{
	if (compilerOut && (aj >= 0) && (aj <= compilerOutN))
	{
		int n = (int) compilerOut[aj].UserData - 1;
		if (errColl && (n >= 0) && (n < (int) errColl->getCount ()))
		{
			TErrData	*err = (TErrData *) ((*errColl)[n]);
			int				nLine = -1, nCol = -1, sLine = -1;
			if (err->line > 0) nLine = err->line - 1;
			if (err->col > 0) nCol = err->col - 1;

			wchar_t	errfn[NM];
			fExpand (wcscpy (errfn, err->fn ? err->fn : L""), path);

			wchar_t	witypename[NM];
			wchar_t	winame[NM];
			WindowInfoEx wi;
			wi.TypeName = witypename;
			wi.TypeNameSize = NM;
			wi.Name = winame;
			wi.NameSize = NM;
			bool				found = false;
			int					n = Info.AdvControl (&MainGuid, ACTL_GETWINDOWCOUNT, NULL, NULL);
			for (int i = -1; i < n; i++)
			{
				wi.Pos = i;
				Info.AdvControl (&MainGuid, ACTL_GETWINDOWINFO, 0, (void *) &wi);
				if (wi.Type == WTYPE_EDITOR)
				{
					if (!FSF.LStricmp (errfn, wi.Name))
					{
						if (i != -1)
						{
							Info.AdvControl (&MainGuid, ACTL_SETCURRENTWINDOW, i, 0);
							Info.AdvControl (&MainGuid, ACTL_COMMIT, NULL, NULL);
						}

						sLine = nLine;
						found = true;
						break;
					}
				}
			}

			if (!found)
				Info.Editor (err->fn, NULL, 0, 0, -1, -1, EF_NONMODAL | EF_ENABLE_F6 | EF_IMMEDIATERETURN, err->line, nCol, CP_DEFAULT);
			EditorSetPosEx (ei, sLine, nCol, err->colSearch);
			if (showMsgBox) showErrorMsgBox (aj);
		}
	}
}

static void showCompileOut (EditorInfoEx *ei, const wchar_t* path)
{
	if (compilerOut)
	{
		FarMenuItemEx *errOut = NULL;
		bool					ErrOnly = (filterring != 0);
		int						ErrCount = 0;
		intptr_t				Code;
		if (compilerOutP >= 0 && compilerOutP < compilerOutN)
		{
			for (int i = 0; i < compilerOutN; i++) compilerOut[i].Flags &= ~MIF_SELECTED;
			compilerOut[compilerOutP].Flags |= MIF_SELECTED;
		}

		for (int i = 0; i < compilerOutN; i++)
		{
			if (!(compilerOut[i].Flags & MIF_DISABLE)) ErrCount++;
		}

		if (ErrCount > 0)
		{
			int j = 0;
			errOut = new FarMenuItemEx[ErrCount];
			for (int i = 0; i < compilerOutN; i++)
			{
				if (!(compilerOut[i].Flags & MIF_DISABLE)) errOut[j++] = compilerOut[i];
			}
		}

		FarKey BreakKeys[5];
		BreakKeys[0].VirtualKeyCode = VK_RETURN;
		BreakKeys[0].ControlKeyState = 0;
		BreakKeys[1].VirtualKeyCode = VK_F4;
		BreakKeys[1].ControlKeyState = 0;
		BreakKeys[2].VirtualKeyCode = VK_F3;
		BreakKeys[2].ControlKeyState = 0;
		BreakKeys[3].VirtualKeyCode = VK_MULTIPLY;
		BreakKeys[3].ControlKeyState = 0;
		BreakKeys[4].VirtualKeyCode = 0;
		BreakKeys[4].ControlKeyState = 0;
	
		for (;;)
		{
			compilerOutP = Info.Menu
				(
					&MainGuid,
					&CompileOutGuid,
					-1,
					-1,
					0,
					FMENU_WRAPMODE | FMENU_SHOWAMPERSAND,
					cmd,
					GetMsg (MMenuBottom),
					NULL,
					BreakKeys,
					&Code,
					(ErrOnly) ? (const FarMenuItemEx *) errOut : (const FarMenuItemEx *) compilerOut,
					(ErrOnly) ? ErrCount : compilerOutN
				);
			if (ErrOnly)
			{
				int j = 0;
				for (int i = 0; i < compilerOutN; i++)
				{
					if (!(compilerOut[i].Flags & MIF_DISABLE))
						if (j == compilerOutP)
						{
							compilerOutP = i;
							break;
						}
						else
							j++;
				}
			}

			if (compilerOutP < 0) break;
			if (Code == 2)
			{
				if (compilerOutP <= compilerOutN && showErrorMsgBox (compilerOutP))
				{
					for (int i = 0; i < compilerOutN; i++) compilerOut[i].Flags &= ~MIF_SELECTED;
					compilerOut[compilerOutP].Flags |= MIF_SELECTED;
				}
			}
			else if (Code == 3)
			{
				ErrOnly = !ErrOnly;
			}
			else
			{
				jumpToError (ei, path, compilerOutP, false);
				break;
			}
		}

		if (ErrCount > 0) delete[] errOut;
	}
}

static bool SaveAll ()
{
	bool				res = true;
	wchar_t	witypename[NM];
	wchar_t	winame[NM];
	WindowInfoEx	wi;
	wi.TypeName = witypename;
	wi.TypeNameSize = NM;
	wi.Name = winame;
	wi.NameSize = NM;
	wi.Pos = -1;
	Info.AdvControl (&MainGuid, ACTL_GETWINDOWINFO, 0, (void *) &wi);

	int home = wi.Pos;
	int n = Info.AdvControl (&MainGuid, ACTL_GETWINDOWCOUNT, NULL, NULL);
	for (int i = 0; i < n; i++)
	{
		wi.Pos = i;
		Info.AdvControl (&MainGuid, ACTL_GETWINDOWINFO, 0, (void *) &wi);
		if (wi.Type == WTYPE_EDITOR && (wi.Flags & WIF_MODIFIED))
		{
			if (Info.AdvControl (&MainGuid, ACTL_SETCURRENTWINDOW, i, 0))
			{
				Info.AdvControl (&MainGuid, ACTL_COMMIT, NULL, NULL);
				if (!Info.EditorControl (-1, ECTL_SAVEFILE, NULL, NULL)) res = false;
			}
			else
				res = false;
		}
	}

	Info.AdvControl (&MainGuid, ACTL_SETCURRENTWINDOW, home, 0);
	Info.AdvControl (&MainGuid, ACTL_COMMIT, NULL, NULL);
	return (res);
}

static bool findBaseFile (const wchar_t *path, const wchar_t *file, wchar_t *baseFile)
{
	bool	found = false;
	wchar_t	testFile[NM];
	int		n = Info.AdvControl (&MainGuid, ACTL_GETWINDOWCOUNT, NULL, NULL);
	wcscpy(testFile, path);
	FSF.AddEndSlash (testFile);
	wcscat (testFile, FSF.PointToName (file));
	if (isFile (testFile))
	{
		if (baseFile) wcscpy (baseFile, testFile);
		found = true;
	}

	if (!found)
	{
		for (int i = -1; i < n; i++)
		{
			wchar_t	witypename[NM];
			wchar_t	winame[NM];
			WindowInfoEx	wi;
			wi.TypeName = witypename;
			wi.TypeNameSize = NM;
			wi.Name = winame;
			wi.NameSize = NM;
			wi.Pos = i;
			Info.AdvControl (&MainGuid, ACTL_GETWINDOWINFO, 0, (void *) &wi);
			if (wi.Type == WTYPE_EDITOR)
			{
				wchar_t	*p = (wchar_t *) wcsrchr (wcscpy (testFile, wi.Name), L'\\');
				if (p)
				{
					wcscpy (p + 1, FSF.PointToName (file));
					if (isFile (testFile))
					{
						if (baseFile) wcscpy (baseFile, testFile);
						found = true;
						break;
					}
				}
			}
		}
	}

	return (found);
}

static bool validMenuItem (const wchar_t *path, const wchar_t *fn, TExec *e)
{
	wchar_t	baseFile[NM] = L"";
	bool	enable = true;
	if (e->enable[0])
	{
		makeCmdLine (false, baseFile, e->enable, path, fn);
		enable = isFile (baseFile);
		if (!enable && e->searchBase) enable = findBaseFile (path, baseFile, NULL);
	}

	if (enable && e->disable[0]) enable = !isFile (makeCmdLine (false, baseFile, e->disable, path, fn));
	return (enable);
}

static wchar_t *makeTitle (const wchar_t *line, size_t size, const wchar_t *path, const wchar_t *fn)
{
	static wchar_t temp[MAX_STR_LEN];
	makeCmdLine (false, temp, line, path, fn);
	return FSF.TruncStr (temp, (int)size);
}

static bool parseError (TLang *lng, const wchar_t *compiler, const wchar_t *path, const wchar_t *fn, wchar_t *line, unsigned ci, int &aj)
{
	int				lineBounds[2], colBounds[2], fileBounds[2];
	TCompiler *e = NULL;
	bool			found = false, res = false;
	for (unsigned i = 0; i < lng->compilerColl.getCount (); i++)
	{
		e = (TCompiler *) (lng->compilerColl[i]);
		if (e && e->err[0] && !FSF.LStricmp (compiler, e->title))
		{
			int		bounds[3][2] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
			int		pos[3] = { e->line, e->col, e->fileMatch };
			wchar_t	lineRE[MAX_REG_LEN];
			wcsncpy (lineRE, line, MAX_REG_LEN);
			lineRE[MAX_REG_LEN - 1] = 0;
			if (strMatch (lineRE, e->err, L"/", L".*/i", 3, bounds, pos))
			{
				for (int j = 0; j < 2; j++)
				{
					lineBounds[j] = bounds[0][j];
					colBounds[j] = bounds[1][j];
					fileBounds[j] = bounds[2][j];
				}

				found = true;
				break;
			}
		}
	}

	if (found && e && e->err[0])
	{
		int len, lineNo = -1, colNo = -1;
		compilerOut[ci].Flags = 0;

		wchar_t	fileName[NM], colSearch[MAX_REG_LEN] = L"";
		if (e->line >= 0)
		{
			len = lineBounds[1] - lineBounds[0];
			wcsncpy (fileName, line + lineBounds[0], len + 1);
			fileName[len] = 0;
			if (len) lineNo = FSF.atoi (fileName);
		}

		if (e->col >= 0)
		{
			len = colBounds[1] - colBounds[0];
			wcsncpy (fileName, line + colBounds[0], len + 1);
			fileName[len] = 0;
			if (len)
			{
				if (e->searchCol)
				{
					colNo = -2;
					wcscpy (colSearch, fileName);
				}
				else
					colNo = FSF.atoi (fileName);
			}
		}

		if (e->fileMatch >= 0)
		{
			len = fileBounds[1] - fileBounds[0];
			wcsncpy (fileName, line + fileBounds[0], len + 1);
			fileName[len] = 0;
			if (!len) wcscpy (fileName, fn);
		}
		else
			wcscpy (fileName, fn);

		TErrData	*errData = new TErrData;
		fExpand (wcscpy (errData->fn, fileName), path);

		wchar_t	*p = line;
		errData->msgCount = 0;
		for (;;)
		{
			wchar_t	brk[260];
			//Info.AdvControl (&MainGuid, ACTL_GETSYSWORDDIV, brk);
			brk[0] = L' ';
			brk[1] = L'\t';
			brk[2] = 0;

			wchar_t	*pb = errData->message[errData->msgCount];
			wcsncpy (pb, p, 64);
			pb[63] = 0;
			if (wcspbrk (pb, brk))
			{
				wchar_t	*pp = wcschr (pb, 0);
				if (pp > pb)
				{
					while (!wcschr (brk, *(--pp)))
						if (pp <= pb) break;
					pp[1] = 0;
				}
			}

			p += wcslen (pb);
			FSF.Trim (pb);
			if (!*pb)
			{
				wcscpy (pb, L"\x01");
				errData->msgCount++;
				break;
			}

			errData->msgCount++;
		}

		errData->line = lineNo;
		errData->col = colNo;
		wcscpy (errData->colSearch, colSearch);
		errColl->insert (errData);
		compilerOut[ci].UserData = errColl->getCount ();
		if (aj < 0)
		{
			compilerOut[ci].Flags = MIF_SELECTED;
			aj = (int) ci;
		}

		res = true;
	}

	return (res);
}

static bool runCompiler (EditorInfoEx *ei, TLang *lng, const wchar_t *fn, const wchar_t *path, const wchar_t *title, TExec *e)
{
	wchar_t	cwd[NM] = L"", baseFile[NM] = L"";
	if (e->enable[0])
	{
		wchar_t	temp[NM];
		makeCmdLine (false, baseFile, e->enable, path, fn);

		bool	enable = isFile (baseFile);
		if (!enable && e->searchBase && findBaseFile (path, baseFile, temp)) wcscpy (baseFile, temp);
	}

	if (!makeCmdLine (true, cmd, e->cmd, path, fn, makeTitle (title, 30, path, fn))) return (false);
	wcscpy(cmdpath, path);
	wchar_t	temp[NM];
	wcscpy(temp, cmd);
	ExpandEnvironmentStrings (temp, cmd, NM);

	if (e->saveType != saNone)
	{
		if (e->saveType == saCurrent)
		{
			wchar_t	witypename[NM];
			wchar_t	winame[NM];
			WindowInfoEx	wi;
			wi.TypeName = witypename;
			wi.TypeNameSize = NM;
			wi.Name = winame;
			wi.NameSize = NM;
			wi.Pos = -1;
			Info.AdvControl (&MainGuid, ACTL_GETWINDOWINFO, 0, (void *) &wi);
			if (wi.Type == WTYPE_EDITOR && (wi.Flags & WIF_MODIFIED))
				if (!Info.EditorControl (-1, ECTL_SAVEFILE, NULL, NULL)) return (false);
		}
		else
		{
			if (!SaveAll ())
			{
				const wchar_t *MsgItems[] = { GetMsg (MTitle), GetMsg (MSaveError), GetMsg (MContinue) };
				if (Info.Message (&MainGuid, &RunCompilerGuid, FMSG_MB_YESNO, NULL, MsgItems, dimOf (MsgItems), 0)) return (false);
			}
		}
	}

	if (e->cd != cdNone)
	{
		wchar_t	newDir[NM] = L"", *p;
		if (e->cd == cdFile)
			p = (wchar_t *) wcsrchr (wcscpy (newDir, fn), L'\\');
		else if ((e->cd == cdBase) && *baseFile)
			p = (wchar_t *) wcsrchr (wcscpy (newDir, baseFile), L'\\');
		if (p) *p = 0;
		if (*newDir)
		{
			GetCurrentDirectory (sizeof (cwd), cwd);
			SetCurrentDirectory (newDir);
		}
	}

	if (compilerColl)
		compilerColl->removeAll ();
	else
		compilerColl = new TCollection;

	if (errColl)
		errColl->removeAll ();
	else
		errColl = new TCollection;

	compilerOutP = -1;
	execute (compilerColl, cmd, e->echo);

	unsigned	n = compilerColl->getCount ();
	if (n)
	{
		int nErr = 0, aj = -1;
		compilerOut = new FarMenuItemEx[compilerOutN = n + 1];
		for (unsigned i = 0; i < n; i++)
		{
			wchar_t	*line = (wchar_t *) ((*compilerColl)[i]);
			compilerOut[i].Text = line;
			compilerOut[i].Flags = MIF_DISABLE;
			compilerOut[i].UserData = 0;

			wchar_t	compiler[MAX_STR_LEN];
			wcscpy (compiler, e->compiler);

			wchar_t	*pp, *p = compiler;
			bool	found = false;
			while ((pp = wcschr (p, L',')) != NULL)
			{
				*pp = 0;
				if (parseError (lng, p, path, fn, line, i, aj))
				{
					nErr++;
					found = true;
				}

				p = pp + 1;
				if (found) break;
			}

			if (!found && parseError (lng, p, path, fn, line, i, aj)) nErr++;
		}

		if ((aj >= 0) && ((e->jumpType == jtFirst)) || ((e->jumpType == jtSmart) && (nErr == 1)))
			jumpToError (ei, path, aj, true);
		else
		{
			if (aj < 0)
			{
				if (compilerOut[n - 1].Text[0] == 0)
				{
					compilerOutN--;
					n--;
				}

				compilerOut[n].Text = L"";
				compilerOut[n].Flags = MIF_SELECTED;
			}
			else
				compilerOutN--;
			if ((e->jumpType == jtMenu) || (e->jumpType == jtSmart)) showCompileOut (ei, path);
		}
	}
	else if (compilerOut)
	{
	    compilerColl->done();
		delete compilerColl;
        compilerColl = NULL;
		delete[] compilerOut;
		compilerOut = NULL;
	}

	if (*cwd) SetCurrentDirectory (cwd);
	return (true);
}

static bool sep = true;

static void addToCompilerMenu (const wchar_t *line, FarMenuItemEx *amenu, int &i, DWORD j, const wchar_t *path, const wchar_t *fn, FarKey AccelKey)
{
	wcscpy (const_cast<wchar_t*>(amenu[i].Text), makeTitle (line, NM - 1, path, fn));
	amenu[i].UserData = j;
	amenu[i].Flags = 0;
	amenu[i].AccelKey = AccelKey;
	if (amenu[i].Text[0])
	{
		i++;
		sep = false;
	}
	else
	{
		if (!sep)
		{
			amenu[i].Flags = MIF_SEPARATOR;
			i++;
		}

		sep = true;
	}
}

static void CompilerMenu (EditorInfoEx *ei, const wchar_t *Name, const wchar_t *Path, int lang)
{
	wchar_t	top[NM], FileName[NM];
	wcscpy (FileName, Name);
	wcscpy (top, GetMsg (MTitle));

	TLang			*lng = (TLang *) (langColl[lang]);
	unsigned	ec = 0;
	if (lng) ec = lng->execColl.getCount ();
	if ((!ec) && (!outputmenu)) return ;
	if (autocompile)
	{
		int i, count = 0;
		if (ec)
		{
			for (unsigned j = 0; j < ec; j++)
			{
				TExec *ex = ((TExec *) lng->execColl[j]);
				if (validMenuItem (Path, FileName, ex))
				{
					i = j;
					count++;
				}
			}
		}

		if (count == 1)
			if (lng)
			{
				TExec *e = (TExec *) lng->execColl[i];
				runCompiler (ei, lng, FileName, Path, e->title, e);
				return ;
			}
	}

	FarMenuItemEx *amenu = new FarMenuItemEx[ec + 2];
	if (amenu)
	{
	    for (unsigned int ii = 0; ii < ec + 2; ii++)
			amenu[ii].Text = new wchar_t[NM];

		const DWORD scId = -1;
		int	i = 0;
		sep = true;
		if (ec)
		{
			for (unsigned j = 0; j < ec; j++)
			{
				TExec *ex = ((TExec *) lng->execColl[j]);
				if (validMenuItem (Path, FileName, ex)) addToCompilerMenu (ex->title, amenu, i, j, Path, FileName, FarKey());
			}

			addToCompilerMenu (L"", amenu, i, 0, Path, FileName, FarKey());
		}

		FarKey Key;
		Key.VirtualKeyCode = VK_MULTIPLY;
		Key.ControlKeyState = 0;
		addToCompilerMenu (GetMsg (MShowOutput), amenu, i, scId, Path, FileName, Key);
		if (!compilerOut) amenu[i - 1].Flags = MIF_DISABLE;
		for (int k = 0; k < i; k++)
			if (!(amenu[k].Flags & (MIF_DISABLE | MIF_SEPARATOR)))
			{
				amenu[k].Flags |= MIF_SELECTED;
				break;
			}

		int res = Info.Menu
			(
				&MainGuid,
				&CompMenuGuid,
				-1,
				-1,
				0,
				FMENU_WRAPMODE | FMENU_AUTOHIGHLIGHT,
				top,
				NULL,
				NULL,
				NULL,
				NULL,
				(const FarMenuItemEx *) amenu,
				(outputmenu) ? i : i - 2
			);
		if (res != -1)
		{
			DWORD id = amenu[res].UserData;
			if (id == scId)
				showCompileOut (ei, Path);
			else if (lng)
			{
				TExec *e = (TExec *) lng->execColl[id];
				runCompiler (ei, lng, FileName, Path, e->title, e);
			}
		}

		for (unsigned int ii = 0; ii < ec + 2; ii++)
			delete[] amenu[ii].Text;

		delete[] amenu;
	}
}

static void SelectCompiler (TEInfo *te)
{
	wchar_t filepath[NM];
	wchar_t filename[NM];
	EditorInfoEx	ei;
	Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
	Info.EditorControl (ei.EditorID, ECTL_GETFILENAME, NM, filename);
	wcscpy(filepath, filename);
	*Point2FileName (filepath) = 0;
	CompilerMenu (&ei, filename, filepath, te->lang);
}

static void ShowOutput (const wchar_t* path)
{
	EditorInfoEx	ei;
	Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
	showCompileOut (&ei, path);
}
