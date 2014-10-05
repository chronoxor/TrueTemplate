//====================================================================================================================
// Point to filename in a full path
//====================================================================================================================
static wchar_t *Point2FileName (wchar_t *szPath)
{
	intptr_t i = wcslen (szPath);
	while (true)
	{
		i--;
		if (i < 0) break;
		if (szPath[i] == L'\\') break;
	}

	i++;
	return (szPath + i);
}

//====================================================================================================================
// Delete fileext from full path
//====================================================================================================================
static wchar_t *Point2FileExt (wchar_t *szPath)
{
	intptr_t i, l;
	i = l = wcslen (szPath);
	while (true)
	{
		i--;
		if (i < 0) break;
		if (szPath[i] == L'.') break;
		if (szPath[i] == L'\\')
		{
			i = l;
			szPath[i] = L'.';
			break;
		}
	}

	i++;
	return (szPath + i);
}

static wchar_t *addExt (wchar_t *path, wchar_t *defExt)
{
	intptr_t n = wcslen(path) - 1;
	for (intptr_t i = n; i >= 0; i--)
	{
		if (path[i] == L'.')
		{
			wcscpy (path + i, defExt);
			break;
		}

		if ((path[i] == L'\\') || (path[i] == L':') || (i == 0)) return (wcscat (path, defExt));
	}

	return (path);
}

static bool IsFile (const wchar_t *szFileName)
{
	DWORD d = GetFileAttributes (szFileName);
	return ((d != INVALID_FILE_ATTRIBUTES) && !(d & FILE_ATTRIBUTE_DIRECTORY));
}

static void squeeze (wchar_t *path)
{
	wchar_t	*dest = path;
	wchar_t	*src = path;
	while (*src != 0)
	{
		if (*src != L'.')
			*dest++ = *src++;
		else
		{
			src++;
			if (*src == L'.')
			{
				src += 2;
				dest--;
				while (*--dest != L'\\');
				dest++;
			}
			else
			{
				src++;
				dest += 2;
			}
		}
	}

	*dest = 0;
}

enum enSplitType
{
	enDRIVE			= 1,
	enDIRECTORY = 2,
	enFILENAME	= 4,
	enEXTENTION = 8
};

static int fnSplit (const wchar_t *path, wchar_t *drive, wchar_t *dir, wchar_t *file, wchar_t *ext)
{
	int		flag = *drive = *dir = *file = *ext = 0;
	wchar_t	ch, *p = wcschr ((wchar_t *) path, L':');
	if (p)
	{
		ch = p[1];
		p[1] = 0;
		wcscpy (drive, path);
		flag |= enDRIVE;
		p[1] = ch;
		p++;
	}
	else
		p = (wchar_t *) path;

	wchar_t	*dp = wcsrchr (p, L'\\');
	if (dp)
	{
		ch = dp[1];
		dp[1] = 0;
		wcscpy (dir, p);
		dp[1] = ch;
		p = dp + 1;
		flag |= enDIRECTORY;
	}

	dp = wcschr (p, L'.');
	if (dp)
	{
		wcscpy (ext, dp);
		flag |= enEXTENTION;
		ch = *dp;
		*dp = 0;
		wcscpy (file, p);
		*dp = ch;
	}
	else
		wcscpy (file, p);
	if (*file) flag |= enFILENAME;
	return (flag);
}

static inline wchar_t *fnMerge (wchar_t *path, wchar_t *drive, wchar_t *dir, wchar_t *file, wchar_t *ext)
{
	FSF.AddEndSlash (wcscat (wcscpy (path, drive), dir));
	return (wcscat (wcscat (path, file), ext));
}

static wchar_t *fExpand (wchar_t *rpath, const wchar_t *defPath)
{
	static wchar_t path[NM], drive[NM], dir[NM], file[NM], ext[NM];
	static wchar_t defDrive[4], defDir[NM];

	wchar_t xname[NM];
	*xname = 0;
	const wchar_t *tail = rpath, *env1 = nullptr, *env2 = nullptr;
	do
	{
		env1 = wcschr (tail, L'%');
		if (env1 != nullptr)
		{
			env2 = wcschr (env1 + 1, L'%');
		}

		if (env1 != nullptr && env2 != nullptr)
		{
			wcscat (xname, String(tail, env1));
			tail = env2 + 1;

			String env(env1 + 1, env2);
			wchar_t envbuf[NM];
			*envbuf = L'\0';
			GetEnvironmentVariable (env, envbuf, _countof(envbuf));
			wcscat (xname, envbuf);
		}
		else
		{
			wcscat (xname, tail);
		}
	} while (env1 != nullptr && env2 != nullptr && *tail != '\0');

	wcscpy (path, defPath);
	FSF.AddEndSlash (path);
	fnSplit (path, defDrive, defDir, file, ext);

	int flags = fnSplit (xname, drive, dir, file, ext);
	if ((flags & enDRIVE) == 0) wcscpy (drive, defDrive);
	CharUpperBuff (drive, 1);
	if ((flags & enDIRECTORY) == 0 || ((*dir != L'\\') && (*dir != L'/')))
	{
		static wchar_t curdir[NM];
		wcscpy (curdir, dir);
		wcscat (wcscpy (dir, defDir), curdir);
	}

	squeeze (dir);
	fnMerge (path, drive, dir, file, ext);
	return (wcscpy (rpath, path));
}

static wchar_t *getFile(const wchar_t *path, const wchar_t *fn)
{
	wchar_t		file[NM], *fileBuff = nullptr;
	HANDLE	s = CreateFile
		(
			fExpand (wcscpy (file, fn), path),
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);
	if (s != INVALID_HANDLE_VALUE)
	{
		unsigned	fileSize = GetFileSize (s, nullptr);
		if (fileSize)
		{
			if ((fileBuff = new wchar_t[(fileSize  / sizeof(wchar_t)) + 1]) != nullptr)
			{
				DWORD rb = 0;
				if (ReadFile (s, fileBuff, fileSize, &rb, nullptr) && rb)
					fileBuff[fileSize  / sizeof(wchar_t)] = 0;
				else
				{
					delete[] fileBuff;
					fileBuff = nullptr;
				}
			}
		}
	}

	CloseHandle (s);
	return (fileBuff);
}

static inline wchar_t *delBackslash (wchar_t *s)
{
	if (*s && wcscmp (s, L"\\"))
	{
		wchar_t	*b = wcschr (s, 0) - 1;
		if (*b == L'\\') *b = 0;
	}

	return (s);
}

static wchar_t *QuoteText (wchar_t *Str)
{
	if (*Str == L'-' || *Str == L'^' || wcspbrk (Str, L" &+,"))
	{
		intptr_t	l = wcslen(Str);
		for (intptr_t i = l; i > 0; i--) Str[i] = Str[i - 1];
		*Str = Str[l + 1] = L'\"';
		Str[l + 2] = 0;
	}

	return (Str);
}

static inline wchar_t *quote (int doQuote, wchar_t *Str)
{
	return (doQuote ? QuoteText (Str) : Str);
}

static bool isFile (const wchar_t *s)
{
	WIN32_FIND_DATA ff;
	HANDLE					h = FindFirstFile (s, &ff);
	bool						res = h != INVALID_HANDLE_VALUE;
	FindClose (h);
	return (res);
}

static inline size_t matched (const wchar_t *mask, const wchar_t *value)
{
	if (wcschr ((wchar_t *) mask, L'\\'))
		return (FSF.ProcessName (mask, (wchar_t *) value, 0, PN_CMPNAMELIST));
	else
		return (FSF.ProcessName (mask, (wchar_t *) value, 0, PN_CMPNAMELIST | PN_SKIPPATH));
}
