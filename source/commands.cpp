//====================================================================================================================
// Get OS name
//====================================================================================================================
static wchar_t *GetOsName ()
{
	static wchar_t		szWin2012R2[] = L"Server 2012 R2";
	static wchar_t		szWindows81[] = L"8.1";
	static wchar_t		szWin2012[] = L"Server 2012";
	static wchar_t		szWindows8[] = L"8";
	static wchar_t		szWin2008R2[] = L"Server 2008 R2";
	static wchar_t		szWindows7[] = L"7";
	static wchar_t		szWin2008[] = L"Server 2008";
	static wchar_t		szWinVista[] = L"Vista";
	static wchar_t		szWin2k3R2[] = L"Server 2003 R2";
	static wchar_t		szWin2k3[] = L"Server 2003";
	static wchar_t		szWinXP[] = L"XP";
	static wchar_t		szWin2k[] = L"2000";
	static wchar_t		szWinMe[] = L"Me";
	static wchar_t		szWin98[] = L"98";
	static wchar_t		szWin95[] = L"95";
	static wchar_t		szWinNT4[] = L"NT 4.0";
	static wchar_t		szWinNT3[] = L"NT 3.51";
	static wchar_t		szWinNT[] = L"NT";
	static wchar_t		szWindows[128];
	wchar_t				*szName;

	OSVERSIONINFOEX ovi;
	ZeroMemory(&ovi, sizeof(OSVERSIONINFOEX));
	ovi.dwOSVersionInfoSize = sizeof (ovi);
	if (GetVersionEx ((LPOSVERSIONINFO)&ovi))
	{
		szName = (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) ? szWinNT : nullptr;
		switch (ovi.dwMajorVersion)
		{
		case 6:
			switch (ovi.dwMinorVersion)
			{
			case 0:
				szName = (ovi.wProductType == VER_NT_WORKSTATION) ? szWinVista : szWin2008;
				break;
			case 1:
				szName = (ovi.wProductType == VER_NT_WORKSTATION) ? szWindows7 : szWin2008R2;
				break;
			case 2:
				szName = (ovi.wProductType == VER_NT_WORKSTATION) ? szWindows8 : szWin2012;
				break;
			case 3:
				szName = (ovi.wProductType == VER_NT_WORKSTATION) ? szWindows81 : szWin2012R2;
				break;
			}
			break;
		case 5:
			switch (ovi.dwMinorVersion)
			{
			case 2:
				szName = (GetSystemMetrics(SM_SERVERR2) == 0) ? szWin2k3 : szWin2k3R2;
				break;
			case 1:
				szName = szWinXP;
				break;
			case 0:
				szName = szWin2k;
				break;
			}
			break;
		case 4:
			switch (ovi.dwMinorVersion)
			{
			case 90:
				szName = szWinMe;
				break;
			case 10:
				szName = szWin98;
				break;
			case 0:
				szName = (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) ? szWinNT4 : szWin95;
				break;
			}
			break;
		case 3:
			if (ovi.dwMinorVersion == 51) szName = szWinNT3;
			break;
		}

		wsprintf (szWindows, L"Windows %s build %u %s", szName, ovi.dwBuildNumber, ovi.szCSDVersion);
		return (szWindows);
	}

	return (nullptr);
}

//====================================================================================================================
// Get OS type
//====================================================================================================================
static wchar_t *GetOsType ()
{
	static wchar_t		szWin32s[] = L"Win32s";
	static wchar_t		szWin9x[] = L"Windows 9x";
	static wchar_t		szWinNT[] = L"Windows NT";
	OSVERSIONINFO ovi;
	ovi.dwOSVersionInfoSize = sizeof (ovi);
	if (GetVersionEx (&ovi))
	{
		switch (ovi.dwPlatformId)
		{
		case VER_PLATFORM_WIN32_NT:
			return (szWinNT);
		case VER_PLATFORM_WIN32_WINDOWS:
			return (szWin9x);
		case VER_PLATFORM_WIN32s:
			return (szWin32s);
		}
	}

	return (nullptr);
}

static size_t	curlength = 0;
static size_t	fullength = 0;
static wchar_t		*output = nullptr;

static bool		RunFlag = false;
static void		*StdOutput = nullptr;
static HANDLE PipeStdOutputDup = nullptr;
static HANDLE StdOutputThread = nullptr;
static DWORD WINAPI StdOutputThreadFunction (LPVOID lpParameter)
{
	BOOL	OkFlag;
	UCHAR	Buffer[4096];
	DWORD ReadCount;
	while (!RunFlag) Sleep (1);
	while (true)
	{
		OkFlag = ReadFile (PipeStdOutputDup, Buffer, 4096, &ReadCount, nullptr);
		if ((OkFlag) && (ReadCount > 0))
		{
			if (curlength + ReadCount + 1 > fullength)
			{
				wchar_t	*temp = new wchar_t[fullength + MAX_STR_LEN];
				wmemcpy (temp, output, curlength);
				delete[] output;
				output = temp;
				fullength += MAX_STR_LEN;
			}

			for (DWORD i = 0; i < ReadCount; i++) output[curlength + i] = Buffer[i];
			curlength += ReadCount;
		}
		else
			break;
		Sleep (1);
	}

	return (0);
}

static void ParseExec (wchar_t *pf, const wchar_t *path)
{
	if (!pf) return ;
	output = new wchar_t[MAX_STR_LEN];
	curlength = 0;
	fullength = MAX_STR_LEN;
	StdOutput = GetStdHandle (STD_OUTPUT_HANDLE);

	BOOL							CreateStdOutput;
	HANDLE							PipeStdOutputWrite, PipeStdOutputRead;
	SECURITY_ATTRIBUTES saAttrStdOutput;
	saAttrStdOutput.nLength = sizeof (SECURITY_ATTRIBUTES);
	saAttrStdOutput.bInheritHandle = true;
	saAttrStdOutput.lpSecurityDescriptor = nullptr;
	CreateStdOutput = CreatePipe (&PipeStdOutputRead, &PipeStdOutputWrite, &saAttrStdOutput, 0);
	CreateStdOutput = SetStdHandle (STD_OUTPUT_HANDLE, PipeStdOutputWrite);
	CreateStdOutput = DuplicateHandle
		(
			GetCurrentProcess (),
			PipeStdOutputRead,
			GetCurrentProcess (),
			&PipeStdOutputDup,
			0,
			FALSE,
			DUPLICATE_SAME_ACCESS
		);
	CreateStdOutput = CloseHandle (PipeStdOutputRead);

	DWORD StdOutputThreadID;
	StdOutputThread = CreateThread
		(
			nullptr,
			0,
			(LPTHREAD_START_ROUTINE) StdOutputThreadFunction,
			nullptr,
			0,
			&StdOutputThreadID
		);

	STARTUPINFOW				si;
	PROCESS_INFORMATION pi;
	ZeroMemory (&si, sizeof (si));
	ZeroMemory (&pi, sizeof (pi));
	si.cb = sizeof (si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
	si.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
	si.hStdError = GetStdHandle (STD_ERROR_HANDLE);
	if (CreateProcess (nullptr, pf, nullptr, nullptr, TRUE, NORMAL_PRIORITY_CLASS, nullptr, path, &si, &pi))
	{
		RunFlag = true;
		WaitForSingleObject (pi.hProcess, INFINITE);
		CloseHandle (pi.hProcess);
		CloseHandle (pi.hThread);
	}

	HANDLE	CloseStdOutput;
	CloseStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
	CloseHandle (CloseStdOutput);
	SetStdHandle (STD_OUTPUT_HANDLE, StdOutput);
	WaitForSingleObject (StdOutputThread, INFINITE);
	CloseHandle (PipeStdOutputDup);

	//CharToOemBuff(output,output,curlength);
	for (size_t i = 0; i < curlength; i++)
	{
		if (output[i] == 0x0A)
		{
			for (size_t j = i; j + 1 < curlength; j++) output[j] = output[j + 1];
			curlength--;
			output[curlength] = L'\0';
		}
	}

	EditorInsertText (output);
	for (size_t i = 0; i < curlength; i++)
		if ((i > 0) && (output[i - 1] == L'\0')) EditorInsertText (output + i);
	delete[] output;
}

static wchar_t *GetParamParam (wchar_t *szParam)
{
	if (szParam)
	{
		wchar_t	*p = szParam;
		wchar_t	*o = szParam;
		while (*p)
		{
			if ((*p == L'\\') && ((p[1] == L'\\') || (p[1] == L':') || (p[1] == L'@')))
				p++;
			else if (*p == L':')
			{
				*o = 0;
				return (p + 1);
			}

			if (*p) *o++ = *p++;
		}

		*o = 0;
	}

	return (nullptr);
}

static wchar_t *DoCounter (wchar_t *szName, wchar_t *szParam, wchar_t *szValue)
{
	static wchar_t sz0[] = L"0";
	if (szParam)
	{
		wchar_t	*szParamInt = (*szParam) ? szParam : sz0;
		WritePrivateProfileString (L"COUNTERS", szName, szParamInt, szIni);
		return (szParamInt);
	}
	else
	{
		FSF.itoa (GetPrivateProfileInt (L"COUNTERS", szName, 0, szIni), szValue, 10);
		return (szValue);
	}
}

static wchar_t *DoString (wchar_t *szName, wchar_t *szParam, wchar_t *szValue, DWORD dLength)
{
	if (szParam)
	{
		WritePrivateProfileString (L"STRINGS", szName, szParam, szIni);
		return (szParam);
	}
	else
	{
		GetPrivateProfileString (L"STRINGS", szName, L"", szValue, dLength, szIni);
		return (szValue);
	}
}

static void ParseFile (wchar_t *pf, bool *setPos, TEditorPos *pos);

static void DoCommand (TCOMMAND eCmd, wchar_t *szParam)
{
	const wchar_t	fmtTime[] = L"HH:mm:ss";
	const wchar_t	fmtDate[] = L"dd.MM.yyyy";
	const wchar_t	fmtDateTime[] = L"HH:mm:ss dd.MM.yyyy";
	wchar_t				szDate[128];
	wchar_t				szString[10240] = L"";
	wchar_t				*clip = nullptr, *psz = nullptr, *szParam1, *szParam2, *szParam3;
	bool					toup = false;
	unsigned long	l;
	szParam1 = szParam;
	szParam2 = GetParamParam (szParam1);
	switch (eCmd)
	{
	case CMD_EditorString:
		{
			EditorGetStringEx egs;
			EditorGetStr (&egs);
			psz = wcsncpy (szString, egs.StringText, egs.StringLength + 1);
		}
		break;
	case CMD_EditorPos:
		{
			TEditorPos	pos = EditorGetPos ();
			FSF.itoa64 (pos.Row + 1, szString, 10);
			psz = szString;
		}
		break;
	case CMD_EditorCol:
		{
			TEditorPos	pos = EditorGetPos ();
			FSF.itoa64 (pos.Col + 1, szString, 10);
			psz = szString;
		}
		break;
	case CMD_Argument:
		{
			psz = userString[*szParam1 - 0x30].getBuffer ();
		}
		break;
	case CMD_ClipBoard:
		{
			size_t count = FSF.PasteFromClipboard (FCT_ANY, nullptr, 0);
			if (count > 0)
			{
				clip = (wchar_t *) malloc ((count + 1) * sizeof(wchar_t));
				FSF.PasteFromClipboard (FCT_ANY, clip, (count + 1));
				psz = clip;
			}
		}
		break;
	case CMD_Selected:
		{
			EditorSaveRestore ();
		}
		break;
	case CMD_CompName:
		{
			psz = szString;
			l = _countof (szString);
			GetComputerName (szString, &l);
		}
		break;
	case CMD_Counter:
		{
			if (szParam1 && *szParam1)
			{
				szParam3 = GetParamParam (szParam2);
				psz = DoCounter (szParam1, szParam2, szString);
			}
		}
		break;
	case CMD_Date:
		{
			psz = (szParam1) ? szParam1 : (wchar_t *) fmtDate;
		}
		break;
	case CMD_DateTime:
		{
			psz = (szParam1) ? szParam1 : (wchar_t *) fmtDateTime;
		}
		break;
	case CMD_Decrement:
		{
			szParam3 = GetParamParam (szParam2);
			DoCounter (szParam1, nullptr, szString);
			FSF.itoa (FSF.atoi (szString) - ((szParam2) ? FSF.atoi (szParam2) : 1), szString, 10);
			psz = DoCounter (szParam1, szString, nullptr);
		}
		break;
	case CMD_Exec:
		{
			wchar_t path[NM];
			EditorGetDirectory(path, NM);
			ParseExec (szParam1, path);
		}
		break;
	case CMD_Enviroment:
		{
			psz = szString;
			GetEnvironmentVariable (szParam1, szString, _countof (szString));
		}
		break;
	case CMD_File:
	case CMD_FileUp:
		{
			GetFilePathName(szString, _countof(szString));
			psz = szString;
			toup = (eCmd == CMD_FileUp);
		}
		break;
	case CMD_FileExt:
	case CMD_FileExtUp:
		{
			GetFilePathName(szString, _countof(szString));
			psz = Point2FileExt(szString);
			toup = (eCmd == CMD_FileExtUp);
		}
		break;
	case CMD_FileName:
	case CMD_FileNameUp:
		{
			GetFilePathName(szString, _countof(szString));
			psz = Point2FileName(szString);
			*(Point2FileExt (psz) - 1) = 0;
			toup = (eCmd == CMD_FileNameUp);
		}
		break;
	case CMD_FilePath:
	case CMD_FilePathUp:
		{
			GetFilePathName(szString, _countof(szString));
			psz = szString;
			*Point2FileName (psz) = 0;
			toup = (eCmd == CMD_FilePathUp);
		}
		break;
	case CMD_FileNameExt:
	case CMD_FileNameExtUp:
		{
			GetFilePathName(szString, _countof(szString));
			psz = Point2FileName(szString);
			toup = (eCmd == CMD_FileNameExtUp);
		}
		break;
	case CMD_GUID:
		{
			GUID	Guid;
			wchar_t	*pw = psz = szString;
			CoInitialize (nullptr);
			CoCreateGuid (&Guid);
			StringFromGUID2 (Guid, (LPOLESTR) szString, _countof (szString));
			while (true)
			{
				*psz++ = *pw++;
				if (!*pw) break;
			}

			*psz = 0;
			psz = szString;
			CoUninitialize ();
		}
		break;
	case CMD_Increment:
		{
			szParam3 = GetParamParam (szParam2);
			DoCounter (szParam1, nullptr, szString);
			FSF.itoa (FSF.atoi (szString) + ((szParam2) ? FSF.atoi (szParam2) : 1), szString, 10);
			psz = DoCounter (szParam1, szString, nullptr);
		}
		break;
	case CMD_Input:
	case CMD_InputString:
		{
			wchar_t	*szParam4;
			if (szParam1 && *szParam1)
				szParam3 = GetParamParam (szParam2);
			else
				break;
			if (eCmd == CMD_InputString)
			{
				if (szParam2 && *szParam2)
					szParam4 = GetParamParam (szParam3);
				else
					break;
				DoString (szParam1, nullptr, szString, _countof (szString));
			}

			wchar_t	*p1 = (eCmd != CMD_InputString) ? szParam1 : szParam2;
			wchar_t	*p2 = (eCmd != CMD_InputString) ? szParam2 : ((szParam3) ? szParam3 : szString);
			struct InitDialogItemEx InitItems[] =
			{															//Type X1 Y1 X2 Y2 Fo Se Fl DB Data
																		///;
																		///00
				{ DI_DOUBLEBOX, 3, 1, 55, 6, 0, 0, 0, DIF_BOXCOLOR, GetMsg (MInput), nullptr },

				//01
				{ DI_TEXT, 5, 2, 0, 0, 0, 0, 0, 0, p1, nullptr },

				//02
				{ DI_EDIT, 5, 3, 53, 0, 1, 0, 0, DIF_HISTORY, p2, L"True-Tpl.History.UserInput" },

				//03
				{ DI_TEXT, 0, 4, 0, 0, 0, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, L"", nullptr },

				//04
				{ DI_BUTTON, 0, 5, 0, 0, 0, 0, 1, DIF_CENTERGROUP, GetMsg (MOK), nullptr },

				//05
				{ DI_BUTTON, 0, 5, 0, 0, 0, 0, 0, DIF_CENTERGROUP, GetMsg (MCancel), nullptr }
			};
			struct FarDialogItem		DialogItems[_countof (InitItems)];
			InitDialogItemsEx (InitItems, DialogItems, _countof (InitItems));

			HANDLE hDlg = Info.DialogInit(&MainGuid, &DoCommandGuid, -1, -1, 59, 8, nullptr, DialogItems, _countof (InitItems), 0, 0, Info.DefDlgProc, 0);
			if (hDlg != INVALID_HANDLE_VALUE)
			{
				intptr_t n = Info.DialogRun(hDlg);
				if (n == 4)
				{
					wchar_t temp[10240];
					FarDialogItemDataEx item;
					item.PtrLength = 10240;
					item.PtrData = temp;
					Info.SendDlgMessage(hDlg,DM_GETTEXT,2,(void*)&item);

					if (eCmd != CMD_InputString)
					{
						EditorInsertText (temp);

						wchar_t	*szParam4;
						if (szParam2 && *szParam2)
						{
							szParam4 = GetParamParam (szParam3);
							if (szParam3) DoString (szParam3, temp, nullptr, 0);
						}
					}
					else
						DoString (szParam1, temp, nullptr, 0);
				}
				Info.DialogFree(hDlg);
			}
		}
		break;
	case CMD_OsName:
		{
			psz = GetOsName ();
		}
		break;
	case CMD_OsType:
		{
			psz = GetOsType ();
		}
		break;
	case CMD_Random:
		{
			psz = szString;
			wsprintf (szString, L"%i", rand ());
		}
		break;
	case CMD_String:
		{
			if (szParam1 && *szParam1)
			{
				szParam3 = GetParamParam (szParam2);
				psz = DoString (szParam1, szParam2, szString, _countof (szString));
			}
		}
		break;
	case CMD_Ticks:
		{
			psz = szString;
			wsprintf (szString, L"%ul", GetTickCount ());
		}
		break;
	case CMD_Time:
		{
			psz = (szParam1) ? szParam1 : (wchar_t *) fmtTime;
		}
		break;
	case CMD_UserName:
		{
			psz = szString;
			l = _countof (szString);
			GetUserName (szString, &l);
		}
		break;
	}

	if ((eCmd == CMD_Date) || (eCmd == CMD_Time) || (eCmd == CMD_DateTime))
	{
		SYSTEMTIME	SysDate;
		GetLocalTime (&SysDate);
		if (eCmd != CMD_Date)
		{
			GetTimeFormat (LOCALE_SYSTEM_DEFAULT, 0, &SysDate, psz, szString, _countof(szString));
			psz = szString;
		}

		if (eCmd != CMD_Time)
		{
			GetDateFormat (LOCALE_SYSTEM_DEFAULT, 0, &SysDate, psz, szDate, _countof (szDate));
			psz = szDate;
		}
	}

	if (psz)
	{
		wchar_t	*str = psz;
		if (toup) FSF.LUpperBuf (str, wcslen (str));
		while (*psz)
		{
			if (*psz == 0x0A)
			{
				*psz = 0;										//if (str!=psz)
				EditorInsertText (str);
				str = ++psz;
			}
			else
				psz++;
		}

		if (*str) EditorInsertText (str);
	}

	if (eCmd == CMD_ClipBoard)
	{
		free (clip);
	}																	//free allocated for this case memory
}

static void ParseFile (wchar_t *pf, bool *setPos, TEditorPos *pos)
{
	wchar_t				szFilePath[_MAX_PATH + 1];
	wchar_t				szFileName[_MAX_FNAME + 1];
	TCOMMAND		eCommand;
	wchar_t				*f = pf, *fn = szFilePath;
	if (f && *f)
	{
		if ((*f++ == L'$') && (*f == L'\\'))
		{
			f++;
			GetModuleFileName (hInst, szFilePath, _MAX_PATH);
			fn = Point2FileName (szFilePath);
		}
		else
			f--;
		wcscpy (fn, f);
		wcscpy (szFileName, Point2FileName (szFilePath));
		*Point2FileName (szFilePath) = 0;

		wchar_t	*cBuffer = getFile (szFilePath, szFileName);
		if (cBuffer)
		{
			wchar_t	*s = f = cBuffer;
			if (*f == cBOM)
				s = ++f;
			wchar_t	*macro, *param, *next;
			while (*f)
			{
				switch (*f)
				{
				case 0x0A:
					{
						*f = 0;
						if (s != f) EditorInsertText (s);
						s = ++f;
						break;
					}

				case cMD:
					{
						if (*++f == cMD)
						{
							f++;
							if (*f == cMD)
							{
								f[-1] = L'\0';
								if (s != f) EditorInsertText (s);
								f[-1] = cMD;
								s = --f;
								break;
							}

							bool	blSetPos = false;
							if (wcslen (f) > 5)	//min_para_size==4 + 2 delimiters
							{
								wchar_t	_d;
								next = f;
								macro = nullptr;
								param = nullptr;
								while (*next)
								{
									if (*next == L'\\')
										next++;
									else
									{
										if ((*next == L':') && !param) param = next + 1;
										if ((*next == cMD) && (next[1] == cMD))
										{
											macro = f;
											*next = 0;
											break;
										}
									}

									if (*next) next++;
								}

								if (macro)
								{
									eCommand = CMD_DoNothing;
									if (param)
									{
										_d = param[-1];
										param[-1] = 0;
									}

									if (!wcscmpi2 (macro, L"ARG"))
									{
										unsigned	c = macro[3];
										if ((c < L'0') || (c > L'9'))
											eCommand = CMD_DoNothing;
										else
										{
											param = macro + 3;
											eCommand = CMD_Argument;
										}
									}
									else if (!_wcsicmp (macro, L"EDITOR_STRING"))
										eCommand = CMD_EditorString;
									else if (!_wcsicmp (macro, L"EDITOR_POS"))
										eCommand = CMD_EditorPos;
									else if (!_wcsicmp (macro, L"EDITOR_COL"))
										eCommand = CMD_EditorCol;
									else if (!_wcsicmp (macro, L"CNT"))
										eCommand = CMD_Counter;
									else if (!_wcsicmp (macro, L"STR"))
										eCommand = CMD_String;
									else if (!_wcsicmp (macro, L"INC"))
										eCommand = CMD_Increment;
									else if (!_wcsicmp (macro, L"DEC"))
										eCommand = CMD_Decrement;
									else if (!_wcsicmp (macro, L"HERE"))
									{
										blSetPos = true;
									}
									else if (!_wcsicmp (macro, L"DATE"))
										eCommand = CMD_Date;
									else if (!_wcsicmp (macro, L"TIME"))
										eCommand = CMD_Time;
									else if (!_wcsicmp (macro, L"DATETIME"))
										eCommand = CMD_DateTime;
									else if (!_wcsicmp (macro, L"TICKS"))
										eCommand = CMD_Ticks;
									else if (!_wcsicmp (macro, L"RAND"))
										eCommand = CMD_Random;
									else if (!_wcsicmp (macro, L"INPUT"))
										eCommand = CMD_Input;
									else if (!_wcsicmp (macro, L"INPUTSTR"))
										eCommand = CMD_InputString;
									else if (!_wcsicmp (macro, L"INCLUDE"))
									{
										GetParamParam (param);
										ParseFile (param, setPos, pos);
									}
									else if (!_wcsicmp (macro, L"EXEC"))
										eCommand = CMD_Exec;
									else if (!_wcsicmp (macro, L"CLIP"))
										eCommand = CMD_ClipBoard;
									else if (!_wcsicmp (macro, L"SELECTED"))
										eCommand = CMD_Selected;
									else if (!_wcsicmp (macro, L"ENV"))
										eCommand = CMD_Enviroment;
									else if (!_wcsicmp (macro, L"COMP_NAME"))
										eCommand = CMD_CompName;
									else if (!_wcsicmp (macro, L"USER_NAME"))
										eCommand = CMD_UserName;
									else if (!_wcsicmp (macro, L"OS_NAME"))
										eCommand = CMD_OsName;
									else if (!_wcsicmp (macro, L"OS_TYPE"))
										eCommand = CMD_OsType;
									else if (!_wcsicmp (macro, L"GUID"))
										eCommand = CMD_GUID;
									else if (!wcscmpi2 (macro, L"FILE_"))
									{
										macro += 5;
										if (!_wcsicmp (macro, L"PATH_NAME"))
											eCommand = CMD_File;
										else if (!_wcsicmp (macro, L"PATH"))
											eCommand = CMD_FilePath;
										else if (!_wcsicmp (macro, L"NAME"))
											eCommand = CMD_FileName;
										else if (!_wcsicmp (macro, L"EXT"))
											eCommand = CMD_FileExt;
										else if (!_wcsicmp (macro, L"NAME_EXT"))
											eCommand = CMD_FileNameExt;
										else
											eCommand = CMD_Unknown;
										macro -= 5;
									}
									else if (!wcscmpi2 (macro, L"UPPERFILE_"))
									{
										macro += 10;
										if (!_wcsicmp (macro, L"PATH_NAME"))
											eCommand = CMD_FileUp;
										else if (!_wcsicmp (macro, L"PATH"))
											eCommand = CMD_FilePathUp;
										else if (!_wcsicmp (macro, L"NAME"))
											eCommand = CMD_FileNameUp;
										else if (!_wcsicmp (macro, L"EXT"))
											eCommand = CMD_FileExtUp;
										else if (!_wcsicmp (macro, L"NAME_EXT"))
											eCommand = CMD_FileNameExtUp;
										else
											eCommand = CMD_Unknown;
										macro -= 10;
									}
									else
										eCommand = CMD_Unknown;
									if (eCommand == CMD_Unknown)
									{
										if (param) param[-1] = _d;
										*next = cMD;
										break;
									}
									else
									{
										macro[-2] = 0;
										if (*s) EditorInsertText (s);
										f = next + 2;
										s = f;
										if (blSetPos)
										{
											*setPos = true;
											*pos = EditorGetPos ();
											pos->LeftCol = 0;
										}

										if (eCommand != CMD_DoNothing) DoCommand (eCommand, param);
									}
								}
							}
						}
						break;
					}

				default:
					f++;
				}
			}

			EditorInsertText (s);
			delete[] cBuffer;
		}
	}
}

static void RunMacro(const TMacro *m, const wchar_t *origStr, intptr_t bounds[][2])
{
	bool	firstsel = false;
	for (const wchar_t *p = m->MacroText; *p; p++)
	{
		if (*p == L'\\')
		{
			switch (*++p)
			{
			case L'?':
				skipUserInputMacro (++p);
				break;
			case L'l':
				firstsel = true;
				break;
			}
		}
	}

	if (firstsel)
	{
		EditorInfoEx	ei;
		Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
		if (ei.BlockType != BTYPE_NONE) EditorSaveSelected ();
		Info.EditorControl (ei.EditorID, ECTL_DELETEBLOCK, 0, nullptr);
	}

	if (scanUserInput (true, L'\\', m->MacroText))
	{
		int					userStrN, insPos = 0;
		bool				setPos = false, setStart = false, setEnd = false, makeTime = false;
		wchar_t				ins[MAX_STR_LEN] = L"";
		TEditorPos	pos, curPos, selStart, selEnd;
		pluginBusy = 1;
		for (const wchar_t *p = m->MacroText; *p; p++)
		{
			const wchar_t	*cb;
			if (*p == L'\\')
			{
				insIns (ins, insPos);
				switch (*++p)
				{
				case L'p':
					setPos = true;
					pos = EditorGetPos ();
					break;
				case L's':
					setStart = true;
					selStart = EditorGetPos ();
					break;
				case L'e':
					setEnd = true;
					selEnd = EditorGetPos ();
					break;
				case 0:
					p--;
					break;
				case L't':
					EditorProcessFARKey (VK_TAB, false);
					break;
				case L'b':
					EditorProcessFARKey (VK_BACK, false);
					break;
				case L'^':
					EditorProcessFARKey (VK_UP, false);
					break;
				case L'v':
					EditorProcessFARKey (VK_DOWN, false);
					break;
				case L'<':
					EditorProcessFARKey (VK_LEFT, false);
					break;
				case L'>':
					EditorProcessFARKey (VK_RIGHT, false);
					break;
				case L'[':
					EditorProcessFARKey (VK_HOME, false);
					break;
				case L']':
					EditorProcessFARKey (VK_END, false);
					break;
				case L'(':
					EditorProcessFARKey (VK_LEFT, true);
					break;
				case L')':
					EditorProcessFARKey (VK_RIGHT, true);
					break;
				case L'r':
					EditorProcessReturnKey (0, 1);
					break;
				case L'n':
					EditorProcessReturnKey ();
					break;
				case L'!':
					pluginBusy = 0;
					EditorProcessKey (*++p);
					pluginBusy = 1;
				case L'?':
					p++;
					while (*p)
					{
						bool	inname = false;
						if ((p[-1] != L'\\') && (p[0] == L'\'')) inname = !inname;
						if ((!inname) && (p[-1] != L'\\') && (p[0] == L'?'))
							break;
						else
							p++;
					}
					break;
				case L'0':
				case L'1':
				case L'2':
				case L'3':
				case L'4':
				case L'5':
				case L'6':
				case L'7':
				case L'8':
				case L'9':
					userStrN = *p - L'0';
					EditorInsertText (userString[userStrN]);
					break;
				case L'$':
					switch (p[1])
					{
					case L'0':
					case L'1':
					case L'2':
					case L'3':
					case L'4':
					case L'5':
					case L'6':
					case L'7':
					case L'8':
					case L'9':
						{
							p++;

							wchar_t	*ss = makeSubstr (*p - L'0' + 1, origStr, bounds, 11);
							if (ss)
							{
								EditorInsertText (ss);
								free (ss);
							}
						}
						break;
					default:
						EditorProcessKey (*p);
						break;
					}
					break;
				case L'{':
					if ((cb = wcschr (p + 1, L'}')) == nullptr)
						EditorProcessKey (*p);
					else
					{
						wchar_t	ch = *cb;
						*(wchar_t *)cb = 0;
						EditorProcessGlobalFARKey (p + 1);
						*(wchar_t *)cb = ch;
						p = cb;
					}
					break;
				case L'w':
					makeTime = true;
				case L'd':
					{
						wchar_t	format[80] = L"", *fmt = nullptr;
						if (p[1] == L'\'')
						{
							wchar_t	*t = format;
							p++;
							while (*++p && *p != L'\'') *t++ = (*p == L'`') ? L'\'' : *p;
							*t = 0;
							fmt = format;
						}

						if (makeTime)
							DoCommand (CMD_Time, fmt);
						else
							DoCommand (CMD_Date, fmt);
						break;
					}

				case L'c':
					{
						DoCommand (CMD_ClipBoard, nullptr);
						break;
					}

				case L'l':
					{
						DoCommand (CMD_Selected, nullptr);
						break;
					}

				case L'f':
					{
						p++;
						if (*p == L'f')
							DoCommand (CMD_File, nullptr);
						else if (*p == L'p')
							DoCommand (CMD_FilePath, nullptr);
						else if (*p == L'n')
							DoCommand (CMD_FileName, nullptr);
						else if (*p == L'e')
							DoCommand (CMD_FileExt, nullptr);
						else if (*p == L'x')
							DoCommand (CMD_FileNameExt, nullptr);
						break;
					}

				case L'F':
					{
						p++;
						if (*p == L'F')
							DoCommand (CMD_FileUp, nullptr);
						else if (*p == L'P')
							DoCommand (CMD_FilePathUp, nullptr);
						else if (*p == L'N')
							DoCommand (CMD_FileNameUp, nullptr);
						else if (*p == L'E')
							DoCommand (CMD_FileExtUp, nullptr);
						else if (*p == L'X')
							DoCommand (CMD_FileNameExtUp, nullptr);
						break;
					}

				case L'E':
					{
						p++;
						if (*p == L's')
							DoCommand (CMD_EditorString, nullptr);
						else if (*p == L'p')
							DoCommand (CMD_EditorPos, nullptr);
						else if (*p == L'c')
							DoCommand (CMD_EditorCol, nullptr);
						break;
					}

				case L'u':
					{
						DoCommand (CMD_UserName, nullptr);
						break;
					}

				case L'i':
					{
						ParseFile (GetXmlParam (&p), &setPos, &pos);
						break;
					}

				case L'o':
					{
						wchar_t path[NM];
						EditorGetDirectory(path, NM);
						ParseExec (GetXmlParam (&p), path);
						break;
					}

				default:
					pluginBusy = 0;
					EditorProcessKey (*p);
					pluginBusy = 1;
					break;
				}
			}
			else
			{
				ins[insPos++] = *p;
				ins[insPos] = 0;
			}
		}

		insIns (ins, insPos);
		pluginBusy = 0;
		if (setPos) EditorSetPos (pos);
		if (setStart && setEnd) selectBlock (selStart.Col, selStart.Row, selEnd.Col, selEnd.Row);
	}

	if (firstsel) EditorSaveRemove ();
}
