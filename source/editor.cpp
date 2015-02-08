struct TEditorPos
{
	TEditorPos()
	{
		Default();
	}
	intptr_t Row, Col;
	intptr_t TopRow, LeftCol;
	void Default (void)
	{
		Row = Col = TopRow = LeftCol = -1;
	}
};

static TEditorPos EditorGetPos (void)
{
	TEditorPos	r;
	EditorInfoEx	ei;
	Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
	r.Row = ei.CurLine;
	r.Col = ei.CurPos;
	r.TopRow = ei.TopScreenLine;
	r.LeftCol = ei.LeftPos;
	return (r);
}

static void EditorSetPos (const TEditorPos &pos)
{
	EditorSetPositionEx sp;
	sp.CurLine = pos.Row;
	sp.CurPos = pos.Col;
	sp.TopScreenLine = pos.TopRow;
	sp.LeftPos = pos.LeftCol;
	sp.CurTabPos = sp.Overtype = -1;
	Info.EditorControl (-1, ECTL_SETPOSITION, 0, &sp);
}

void EditorSetPos (intptr_t line, intptr_t col, intptr_t topline = -1, intptr_t leftcol = -1)
{
	EditorSetPositionEx sp;
	sp.CurLine = (line < -1) ? -1 : line;
	sp.CurPos = (col < -1) ? -1 : col;
	sp.TopScreenLine = topline;
	sp.LeftPos = leftcol;
	sp.CurTabPos = sp.Overtype = -1;
	Info.EditorControl (-1, ECTL_SETPOSITION, 0, &sp);
}

static uintptr_t EditorGetCodeTable (void)
{
	EditorInfoEx	ei;
	Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
	return ei.CodePage;
}

static void EditorSetCodeTable (intptr_t iCodeTable = 1)
{
	EditorSetParameterEx	etp;
	ZeroMemory (&etp, sizeof (etp));
	etp.Type = ESPT_CODEPAGE;
	etp.iParam = iCodeTable;
	Info.EditorControl (-1, ECTL_SETPARAM, 0, &etp);
}

static void EditorGetStr (EditorGetStringEx *gs, intptr_t line = -1)
{
	gs->StringNumber = line;
	Info.EditorControl (-1, ECTL_GETSTRING, 0, gs);
}

static void EditorSetStr (const wchar_t *src, intptr_t line = -1)
{
	EditorSetStringEx st;
	st.StringNumber = line;
	st.StringText = src;
	st.StringEOL = 0;
	st.StringLength = wcslen (src);
	Info.EditorControl (-1, ECTL_SETSTRING, 0, &st);
}

static String EditorGetSelectionLine (void)
{
	EditorInfoEx	ei;
	Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
	if (ei.BlockType != BTYPE_NONE)
	{
		EditorGetStringEx egs;
		EditorGetStr (&egs, ei.BlockStartLine);
		if (egs.SelEnd == -1)
			return String (egs.StringText + egs.SelStart);
		return String (egs.StringText + egs.SelStart, egs.StringText + egs.SelEnd);
	}
	return L"";
}

static void EditorSaveSelected (void)
{
	int				vblock;
	intptr_t	selstart = 0;
	int				sellength = 0;
	EditorInfoEx	ei;
	Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
	vblock = (ei.BlockType == BTYPE_COLUMN) ? 1 : 0;
	selstart = ei.BlockStartLine;
	if (ei.BlockType != BTYPE_NONE)
	{
		intptr_t  curpos;
		wchar_t   filename[NM];
		ExpandEnvironmentStrings(TEMP_TT_TMP, filename, NM);

		FILE *file = _wfopen (filename, L"wb");
		EditorGetStringEx egs;
		fwrite (&vblock, 1, sizeof (int), file);
		curpos = selstart;
		do
		{
			EditorGetStr (&egs, curpos++);
			if (egs.SelStart != -1)
			{
				sellength++;
			}
		} while ((egs.SelStart != -1) && (curpos < ei.TotalLines));
		fwrite (&sellength, 1, sizeof (int), file);
		curpos = selstart;
		do
		{
			EditorGetStr (&egs, curpos++);
			if (egs.SelStart != -1)
			{
				int len;
				if (egs.SelEnd != -1)
				{
					wchar_t tmp = L' ';
					int	    truelen = ((egs.StringLength - egs.SelStart) > 0) ? (int)(egs.StringLength - egs.SelStart) : 0;
					len = (int)(egs.SelEnd - egs.SelStart);
					fwrite (&len, 1, sizeof (int), file);
					fwrite (egs.StringText + egs.SelStart, (truelen < len) ? truelen : len, sizeof (wchar_t), file);
					truelen = (truelen < len) ? len - truelen : 0;
					for (int i = 0; i < truelen; i++) fwrite (&tmp, 1, sizeof (wchar_t), file);
				}
				else
				{
					len = (int)(egs.StringLength - egs.SelStart);
					fwrite (&len, 1, sizeof (int), file);
					fwrite (egs.StringText + egs.SelStart, len, sizeof (wchar_t), file);
				}
			}
		} while ((egs.SelStart != -1) && (curpos < ei.TotalLines));
		fclose (file);
	}
}

static void EditorSaveAll (void)
{
	int		vblock = 0;
	wchar_t	filename[NM];
	ExpandEnvironmentStrings (TEMP_TT_TMP, filename, NM);

	FILE *file = _wfopen (filename, L"wb");
	EditorInfoEx	ei;
	Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
	fwrite (&vblock, 1, sizeof (int), file);
	fwrite (&ei.TotalLines, 1, sizeof (int), file);
	for (intptr_t i = 0; i < ei.TotalLines; i++)
	{
		EditorGetStringEx egs;
		EditorGetStr (&egs, i);

		int len = (int)egs.StringLength;
		fwrite (&len, 1, sizeof (int), file);
		fwrite (egs.StringText, len, sizeof (wchar_t), file);
	}

	fclose (file);
}

static void EditorSaveRestore (void)
{
	int					vblock;
	int					sellength = 0;
	EditorInfoEx	ei;
	Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);

	TEditorPos	ep = EditorGetPos ();
	wchar_t filename[NM];
	ExpandEnvironmentStrings (TEMP_TT_TMP, filename, NM);

	FILE *file = _wfopen (filename, L"rb");
	if (!file) return ;
	fread (&vblock, 1, sizeof (int), file);
	fread (&sellength, 1, sizeof (int), file);
	for (int i = 0; i < sellength; i++)
	{
		if ((vblock == 0) || (i == 0))
		{
			if (i != 0) Info.EditorControl (-1, ECTL_INSERTSTRING, 0, 0);
		}
		else
		{
			ep.Row++;
			if (ep.Row >= ei.TotalLines) Info.EditorControl (-1, ECTL_INSERTSTRING, 0, 0);
			EditorSetPos (ep);
		}

		int len;
		fread (&len, 1, sizeof (int), file);

		EditorGetStringEx egs;
		EditorGetStr (&egs);

		EditorInfoEx	einfo;
		Info.EditorControl (-1, ECTL_GETINFO, 0, &einfo);

		wchar_t				*buffer = new wchar_t[egs.StringLength + len];
		wchar_t				*pDst = buffer;
		wchar_t				*pDstPos = buffer + einfo.CurPos;
		const wchar_t	*pSrc = egs.StringText;
		const wchar_t	*pSrcEnd = egs.StringText + egs.StringLength;

		while (pDst != pDstPos) *pDst++ = *pSrc++;
		pDst += fread (pDst, sizeof (wchar_t), len, file);
		while (pSrc != pSrcEnd) *pDst++ = *pSrc++;

		EditorSetStringEx ess;
		ess.StringNumber = -1;
		ess.StringText = buffer;
		ess.StringEOL = /* FAR API defect workaround */ egs.StringEOL;
		ess.StringLength = egs.StringLength + len;
		Info.EditorControl (-1, ECTL_SETSTRING, 0, &ess);

		EditorSetPos (-1, einfo.CurPos + len);

		delete[] buffer;
	}

	fclose (file);
}

static void EditorSaveRemove (void)
{
	wchar_t	filename[NM];
	ExpandEnvironmentStrings (TEMP_TT_TMP, filename, NM);
	_wunlink (filename);
}

static void EditorSetPosEx(const EditorInfoEx *ei, intptr_t line, intptr_t initCol, const wchar_t *colSearch)
{
	intptr_t col = (initCol < 0) ? -1 : initCol;
	EditorSetPos (line, col);

	TEditorPos	pos = EditorGetPos ();
	if (pos.Row == pos.TopRow)
	{
		intptr_t tl = line - 5;
		if (tl < 1) tl = 1;
		EditorSetPos (line, -1, tl);
	}

	EditorGetStringEx gs;
	EditorGetStr (&gs);
	if ((initCol == -2) && *colSearch)
	{
		wchar_t	*s = (wchar_t *) malloc (gs.StringLength + 1);
		if (s)
		{
			wcsncpy (s, gs.StringText, gs.StringLength + 1);

			wchar_t	*found = wcsstr(s, colSearch);
			if (found)
			{
				intptr_t newCol = (intptr_t)(found - s);
				if (newCol != col) EditorSetPos (-1, newCol);
			}

			free (s);
		}
	}

	pos = EditorGetPos ();
	if (pos.Col > gs.StringLength) EditorSetPos (-1, gs.StringLength);
}

static inline void EditorInsertText (const wchar_t *s)
{
	Info.EditorControl (-1, ECTL_INSERTTEXT, 0, (void *) s);
}

static inline void redraw (void)
{
	Info.EditorControl (-1, ECTL_REDRAW, 0, 0);
}

static inline void insIns (wchar_t *ins, int &insPos)
{
	if (*ins)
	{
		EditorInsertText (ins);
		insPos = *ins = 0;
		redraw ();
	}
}

static void selectBlock (intptr_t X1, intptr_t Y1, intptr_t X2, intptr_t Y2)
{
	EditorSelectEx	es;
	es.BlockType = BTYPE_STREAM;
	if (Y1 < Y2)
	{
		es.BlockStartLine = Y1;
		es.BlockStartPos = X1;
		es.BlockHeight = Y2 - Y1 + 1;
		es.BlockWidth = X2 - X1;
	}
	else if (Y1 > Y2)
	{
		es.BlockStartLine = Y2;
		es.BlockStartPos = X2;
		es.BlockHeight = Y1 - Y2 + 1;
		es.BlockWidth = X1 - X2;
	}
	else
	{
		es.BlockStartLine = Y1;
		es.BlockStartPos = min(X1, X2);
		es.BlockHeight = 1;
		es.BlockWidth = abs(X2 - X1);
	}
	Info.EditorControl (-1, ECTL_SELECT, 0, (void *) &es);
}

static void GetFilePathName (wchar_t* filename, size_t size)
{
	Info.EditorControl (-1, ECTL_GETFILENAME, size, filename);
}

static void EditorProcessKey (wchar_t unicode, WORD keycode = 0)
{
	INPUT_RECORD	tr;
	tr.EventType = KEY_EVENT;
	tr.Event.KeyEvent.bKeyDown = true;
	tr.Event.KeyEvent.wRepeatCount = 1;
	tr.Event.KeyEvent.wVirtualKeyCode = keycode;
	tr.Event.KeyEvent.wVirtualScanCode = 0;
	tr.Event.KeyEvent.uChar.UnicodeChar = unicode;
	tr.Event.KeyEvent.dwControlKeyState = 0;
	Info.EditorControl (-1, ECTL_PROCESSINPUT, 0, &tr);
}

static inline void EditorProcessFARKey (int key, bool ctrl)
{
	INPUT_RECORD	tr;
	tr.EventType = KEY_EVENT;
	tr.Event.KeyEvent.bKeyDown = true;
	tr.Event.KeyEvent.wRepeatCount = 1;
	tr.Event.KeyEvent.wVirtualKeyCode = key;
	tr.Event.KeyEvent.wVirtualScanCode = 0;
	tr.Event.KeyEvent.uChar.UnicodeChar = 0;
	tr.Event.KeyEvent.dwControlKeyState = ctrl ? LEFT_CTRL_PRESSED : 0;
	Info.EditorControl (-1, ECTL_PROCESSINPUT, 0, &tr);
}

static inline void EditorProcessGlobalFARKey (LPCTSTR keys)
{
	MacroSendMacroTextEx macro;
	macro.Flags = KMFLAGS_NONE;
	macro.SequenceText = keys;
	Info.MacroControl (&MainGuid, MCTL_SENDSTRING, MSSC_POST, (void *) &macro);
}

static void EditorProcessReturnKey (int before = -1, int after = -1)
{
	INPUT_RECORD	tr;
	tr.EventType = KEY_EVENT;
	tr.Event.KeyEvent.bKeyDown = true;
	tr.Event.KeyEvent.wRepeatCount = 1;
	tr.Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
	tr.Event.KeyEvent.wVirtualScanCode = 0;
	tr.Event.KeyEvent.uChar.UnicodeChar = 0;
	tr.Event.KeyEvent.dwControlKeyState = 0;
	if (before != -1) pluginBusy = before;
	Info.EditorControl (-1, ECTL_PROCESSINPUT, 0, &tr);
	if (after != -1) pluginBusy = after;
}

static void EditorGetDirectory (wchar_t* path, size_t size)
{
	GetFilePathName(path, size);
	*Point2FileName (path) = 0;
}
