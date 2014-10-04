struct TNavy : TCollectionItem
{
	TNavy();
	String	mask, path, suffixes;
	intptr_t	pos, rect[4];
	bool	viewer;
};

TNavy::TNavy ()
{
	this->pos = 0;
	this->rect[0] = -1;
	this->rect[1] = -1;
	this->rect[2] = -1;
	this->rect[3] = -1;
	this->viewer = false;
}

struct TFoundNav : TCollectionItem
{
	String file;
	intptr_t	rect[4];
	bool	viewer;

	TFoundNav (const wchar_t *aFile, const intptr_t aRect[4], bool aViewer)
		: file(aFile), viewer(aViewer)
	{
		rect[0] = aRect[0];
		rect[1] = aRect[1];
		rect[2] = aRect[2];
		rect[3] = aRect[3];
	}
};

static void NavyInsert(const wchar_t *file, const wchar_t *suffixes, const intptr_t rect[4], bool viewer, TCollection<TFoundNav> &coll)
{
	const wchar_t	*cursuffixes = suffixes;
	wchar_t				tmp[2] = L"x";
	wchar_t				filename[2 * _MAX_PATH + 1];
	wchar_t				filenamex[2 * _MAX_PATH + 1];
	wcscpy (filename, file);
	GetFullPathName (filename, 2 * _MAX_PATH + 1, filenamex, nullptr);
	wcscpy (filename, filenamex);
	do
	{
		if (IsFile (filename))
		{
			for (size_t i = 0; i < coll.getCount (); i++)
				if (_wcsicmp(coll[i]->file, filename) == 0) return;

			TFoundNav *tmp = new TFoundNav(filename, rect, viewer);
			coll.insert (tmp);
		}

		wcscpy (filename, (*cursuffixes) ? file : L"");
		while (*cursuffixes)
		{
			if ((*cursuffixes == L';') || (*cursuffixes == L'\0'))
			{
				cursuffixes++;
				break;
			}
			else
			{
				tmp[0] = *cursuffixes;
				wcscat (filename, tmp);
				GetFullPathName (filename, 2 * _MAX_PATH + 1, filenamex, nullptr);
				wcscpy (filename, filenamex);
				cursuffixes++;
			}
		}
	} while (*filename);
}

static void NavyFind(const wchar_t	*path, const wchar_t *file, const wchar_t *suffixes, const intptr_t rect[4], bool viewer, TCollection<TFoundNav> &coll)
{
	wchar_t	testFile[2 * NM];
	wcscpy(testFile, path);
	wcscat(testFile, file);
	NavyInsert (testFile, suffixes, rect, viewer, coll);

	intptr_t n = Info.AdvControl(&MainGuid, ACTL_GETWINDOWCOUNT, 0, nullptr);
	for (intptr_t i = 0; i < n; i++)
	{
		wchar_t	witypename[NM];
		wchar_t	winame[NM];
		WindowInfoEx wi;
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
				wcscpy (p + 1, file);
				NavyInsert (testFile, suffixes, rect, viewer, coll);
			}
		}
	}
}

static void NavyCollect
(
	const wchar_t	*path,
	const wchar_t	*filename,
	const wchar_t	*includepath,
	const wchar_t	*suffixes,
	const intptr_t	rect[4],
	bool			viewer,
	TCollection<TFoundNav> &coll
)
{
	wchar_t				curdir[2 * _MAX_PATH + 1];
	wchar_t				curfile[2 * _MAX_PATH + 1];
	wchar_t				*tmpDir = curdir;
	const wchar_t	*tmpPath = includepath;

	NavyInsert (filename, suffixes, rect, viewer, coll);
	while (*tmpPath)
	{
		if (*tmpPath == L';')
		{
			*tmpDir = L'\0';
			tmpPath++;
			tmpDir = curdir;
			if ((curdir[0] == L'.') && (curdir[1] == L'\0'))
				NavyFind (path, filename, suffixes, rect, viewer, coll);
			else
			{
				wcscpy (curfile, curdir);
				wcscat (curfile, L"\\");
				wcscat (curfile, filename);
				NavyInsert (curfile, suffixes, rect, viewer, coll);
			}
		}
		else
			*tmpDir++ = *tmpPath++;
	}

	*tmpDir = L'\0';
	tmpDir = curdir;
	if ((curdir[0] == L'.') && (curdir[1] == L'\0'))
		NavyFind (path, filename, suffixes, rect, viewer, coll);
	else
	{
		wcscpy (curfile, curdir);
		wcscat (curfile, L"\\");
		wcscat (curfile, filename);
		NavyInsert (curfile, suffixes, rect, viewer, coll);
	}
}

static void Navigate(const TLang *lng, const wchar_t *path, wchar_t *realLine, intptr_t pos, intptr_t start, intptr_t end)
{
	wchar_t				filename[2 * _MAX_PATH + 1];
	TCollection<TFoundNav> found;
	for (size_t i = 0; i < lng->navyColl.getCount (); i++)
	{
		const TNavy *nav = lng->navyColl[i];
		intptr_t	bounds[1][2] = { { 0, 0 } };
		if (start != -1)
		{
			intptr_t len = min(end - start, _MAX_PATH);
			wcsncpy (filename, realLine + start, len);
            filename[len] = 0;
			NavyCollect (path, filename, nav->path, nav->suffixes, nav->rect, nav->viewer, found);
		}
		else if (strMatch (realLine, nav->mask, L"/^\\s*", (lng->ignoreCase ? L"\\s*$/i" : L"\\s*$/"), 1, bounds, &nav->pos))
		{
			if ((pos >= bounds[0][0]) && (pos <= bounds[0][1]))
			{
				intptr_t len = min(bounds[0][1] - bounds[0][0], _MAX_PATH);
				wcsncpy (filename, realLine + bounds[0][0], len);
                filename[len] = 0;
				NavyCollect (path, filename, nav->path, nav->suffixes, nav->rect, nav->viewer, found);
			}
		}
	}

	FarMenuItemEx *mMenu = nullptr;
	if (found.getCount () > 1) mMenu = new FarMenuItemEx[found.getCount ()];
	for (size_t i = 0; ((i < found.getCount()) && (found.getCount() > 1)); i++)
	{
		mMenu[i].Text = found[i]->file;
	}

	FarKey BreakKeys[2];
	BreakKeys[0].VirtualKeyCode = VK_F3;
	BreakKeys[0].ControlKeyState = 0;
	BreakKeys[1].VirtualKeyCode = 0;
	BreakKeys[1].ControlKeyState = 0;

	intptr_t BreakCode;
	intptr_t ExitCode = 0;
	while (found.getCount () > 1)
	{
		ExitCode = Info.Menu
			(
				&MainGuid,
				&NavigateGuid,
				-1,
				-1,
				0,
				FMENU_WRAPMODE,
				GetMsg (MNavyMenu),
				GetMsg (MNavyMenuBottom),
				nullptr,
				BreakKeys,
				&BreakCode,
				(const FarMenuItemEx *) mMenu,
				found.getCount ()
			);
		if (ExitCode >= 0)
		{
			if (BreakCode == 0)
			{
				const wchar_t	*Msg[5];
				Msg[0] = GetMsg (MNavigation);
				Msg[1] = L"\1";
				Msg[2] = found[ExitCode]->file;
				Msg[3] = L"\1";
				Msg[4] = GetMsg (MOK);
				Info.Message (&MainGuid, &NavigateMessageGuid, FMSG_LEFTALIGN, nullptr, Msg, _countof (Msg), 1);
				continue;
			}
			else
				break;
		}
		else
			break;
	}

	if (found.getCount () > 1) delete[] mMenu;
	if ((found.getCount ()) && (ExitCode >= 0))
	{
		CONSOLE_SCREEN_BUFFER_INFO	ConBuffInfo;
		HANDLE	ConHnd = GetStdHandle (STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo (ConHnd, &ConBuffInfo);

		wchar_t	navRectLeft[32];
		wchar_t	navRectRight[32];
		wchar_t	navRectTop[32];
		wchar_t	navRectBottom[32];

		PluginSettings settings(MainGuid, ::Info.SettingsControl);
		settings.Get(0,L"NavigationRectLeft", navRectLeft, 32, L"0");
		settings.Get(0,L"NavigationRectTop", navRectTop, 32, L"50");
		settings.Get(0,L"NavigationRectRight", navRectRight, 32, L"100");
		settings.Get(0,L"NavigationRectBottom", navRectBottom, 32, L"100");

		const TFoundNav *exitFound = found[ExitCode];
		intptr_t	x1, y1, x2, y2;
		x1 = ((exitFound->rect[0] >= 0) && (exitFound->rect[0] <= 100)) ? exitFound->rect[0] : FSF.atoi(navRectLeft);
		y1 = ((exitFound->rect[1] >= 0) && (exitFound->rect[1] <= 100)) ? exitFound->rect[1] : FSF.atoi(navRectTop);
		x2 = ((exitFound->rect[2] >= 0) && (exitFound->rect[2] <= 100)) ? exitFound->rect[2] : FSF.atoi(navRectRight);
		y2 = ((exitFound->rect[3] >= 0) && (exitFound->rect[3] <= 100)) ? exitFound->rect[3] : FSF.atoi(navRectBottom);
		x1 = ConBuffInfo.dwSize.X * x1 / 100;
		y1 = ConBuffInfo.dwSize.Y * y1 / 100;
		x2 = ConBuffInfo.dwSize.X * x2 / 100;
		y2 = ConBuffInfo.dwSize.Y * y2 / 100;
		if (exitFound->viewer)
		{
			Info.Viewer
				(
					exitFound->file,
					nullptr,
					x1,
					y1,
					x2,
					y2,
					VF_NONMODAL | VF_IMMEDIATERETURN | VF_DISABLEHISTORY | VF_ENABLE_F6,
					CP_DEFAULT
				);
		}
		else
		{
			Info.Editor
				(
					exitFound->file,
					nullptr,
					x1,
					y1,
					x2,
					y2,
					EF_NONMODAL | EF_IMMEDIATERETURN | EF_DISABLEHISTORY,
					0,
					1,
					CP_DEFAULT
				);
		}
	}

	found.removeAll ();
}

static void SelectNavigation (const TEInfo *te, const wchar_t* path)
{
	if (te)
	{
		const TLang *lng = langColl[te->lang];
		if (lng)
		{
			wchar_t				line[MAX_STR_LEN];
			EditorInfoEx	ei;
			Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);

			EditorGetStringEx gs;
			TEditorPos			epos = EditorGetPos ();
			EditorGetStr (&gs);
			wcscpy(line, gs.StringText);
			Navigate (lng, path, line, epos.Col, gs.SelStart, gs.SelEnd);
		}
	}
}

static void SelectNavigationList (const TEInfo *te, const wchar_t* path)
{
	if (te)
	{
		const TLang *lng = langColl[te->lang];
		if (lng)
		{
			wchar_t				line[MAX_STR_LEN];
			wchar_t				filename[2 * _MAX_PATH + 1];
			TCollection<TFoundNav> found;
			EditorInfoEx	ei;
			Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
			for (intptr_t l = 0; l < ei.TotalLines; l++)
			{
				EditorGetStringEx egs;
				EditorGetStr (&egs, l);
				wcsncpy (line, egs.StringText, egs.StringLength + 1);
				for (size_t i = 0; i < lng->navyColl.getCount(); i++)
				{
					const TNavy *nav = lng->navyColl[i];
					intptr_t	bounds[1][2] = { { 0, 0 } };
					if (strMatch (line, nav->mask, L"/^\\s*", (lng->ignoreCase ? L"\\s*$/i" : L"\\s*$/"), 1, bounds, &nav->pos))
					{
						intptr_t len = min(bounds[0][1] - bounds[0][0], _MAX_PATH);
						wcsncpy (filename, line + bounds[0][0], len);
						filename[len] = 0;
						NavyCollect (path, filename, nav->path, nav->suffixes, nav->rect, nav->viewer, found);
					}
				}
			}

			FarMenuItemEx *mMenu = nullptr;
			if (found.getCount () > 0) mMenu = new FarMenuItemEx[found.getCount ()];
			for (size_t i = 0; ((i < found.getCount()) && (found.getCount() > 1)); i++)
			{
				mMenu[i].Text = found[i]->file;
			}

			FarKey BreakKeys[2];
			BreakKeys[0].VirtualKeyCode = VK_F3;
			BreakKeys[0].ControlKeyState = 0;
			BreakKeys[1].VirtualKeyCode = 0;
			BreakKeys[1].ControlKeyState = 0;

			intptr_t BreakCode;
			intptr_t ExitCode = 0;
			while (found.getCount () > 0)
			{
				ExitCode = Info.Menu
					(
						&MainGuid,
						&SelNavigateGuid,
						-1,
						-1,
						0,
						FMENU_WRAPMODE,
						GetMsg (MNavyMenu),
						GetMsg (MNavyMenuBottom),
						nullptr,
						BreakKeys,
						&BreakCode,
						(const FarMenuItemEx *) mMenu,
						found.getCount ()
					);
				if (ExitCode >= 0)
				{
					if (BreakCode == 0)
					{
						const wchar_t	*Msg[5];
						Msg[0] = GetMsg (MNavigation);
						Msg[1] = L"\1";
						Msg[2] = found[ExitCode]->file;
						Msg[3] = L"\1";
						Msg[4] = GetMsg (MOK);
						Info.Message (&MainGuid, &SelNavigateMessageGuid, FMSG_LEFTALIGN, nullptr, Msg, _countof (Msg), 1);
						continue;
					}
					else
						break;
				}
				else
					break;
			}

			if (found.getCount () > 0) delete[] mMenu;
			if ((found.getCount ()) && (ExitCode >= 0))
			{
				CONSOLE_SCREEN_BUFFER_INFO	ConBuffInfo;
				HANDLE	ConHnd = GetStdHandle (STD_OUTPUT_HANDLE);
				GetConsoleScreenBufferInfo (ConHnd, &ConBuffInfo);

				wchar_t	navRectLeft[32];
				wchar_t	navRectRight[32];
				wchar_t	navRectTop[32];
				wchar_t	navRectBottom[32];

				PluginSettings settings(MainGuid, ::Info.SettingsControl);
				settings.Get(0,L"NavigationRectLeft", navRectLeft, 32, L"0");
				settings.Get(0,L"NavigationRectTop", navRectTop, 32, L"50");
				settings.Get(0,L"NavigationRectRight", navRectRight, 32, L"100");
				settings.Get(0,L"NavigationRectBottom", navRectBottom, 32, L"100");

				const TFoundNav *exitFound = found[ExitCode];
				intptr_t	x1, y1, x2, y2;
				x1 = ((exitFound->rect[0] >= 0) && (exitFound->rect[0] <= 100)) ? exitFound->rect[0] : FSF.atoi(navRectLeft);
				y1 = ((exitFound->rect[1] >= 0) && (exitFound->rect[1] <= 100)) ? exitFound->rect[1] : FSF.atoi(navRectTop);
				x2 = ((exitFound->rect[2] >= 0) && (exitFound->rect[2] <= 100)) ? exitFound->rect[2] : FSF.atoi(navRectRight);
				y2 = ((exitFound->rect[3] >= 0) && (exitFound->rect[3] <= 100)) ? exitFound->rect[3] : FSF.atoi(navRectBottom);
				x1 = ConBuffInfo.dwSize.X * x1 / 100;
				y1 = ConBuffInfo.dwSize.Y * y1 / 100;
				x2 = ConBuffInfo.dwSize.X * x2 / 100;
				y2 = ConBuffInfo.dwSize.Y * y2 / 100;
				if (exitFound->viewer)
				{
					Info.Viewer
						(
							exitFound->file,
							nullptr,
							x1,
							y1,
							x2,
							y2,
							VF_NONMODAL | VF_IMMEDIATERETURN | VF_DISABLEHISTORY | VF_ENABLE_F6,
							CP_DEFAULT
						);
				}
				else
				{
					Info.Editor
						(
							exitFound->file,
							nullptr,
							x1,
							y1,
							x2,
							y2,
							EF_NONMODAL | EF_IMMEDIATERETURN | EF_DISABLEHISTORY,
							0,
							1,
							CP_DEFAULT
						);
				}
			}

			found.removeAll ();
		}
	}
}
