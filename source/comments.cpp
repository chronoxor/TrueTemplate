static void BlockComments (TEInfo *te)
{
	unsigned	micount = 2;
	enum Action
	{
		eComment,
		eUnComment,
		eNone
	} action;
	FarMenuItemEx *cmenu = new FarMenuItemEx[micount];
	if (cmenu)
	{
		ZeroMemory (cmenu, sizeof (FarMenuItemEx) * micount);
		cmenu[0].Text = GetMsg (MBlockComment);
		cmenu[1].Text = GetMsg (MBlockUnComment);
		cmenu[0].Flags |= MIF_SELECTED;

		intptr_t m = Info.Menu(&MainGuid, &BlockCommentsGuid, -1, -1, 0, 0, GetMsg(MTitle), 0, 0, 0, 0, cmenu, micount);
		switch (m)
		{
		case 0:
			action = eComment;
			break;
		case 1:
			action = eUnComment;
			break;
		default:
			action = eNone;
			break;
		}

		delete[] cmenu;
		if (action == eNone) return ;
		intptr_t					iStartString, iStringsCnt, iStringPos;
		wchar_t						line[MAX_STR_LEN];
		EditorInfoEx			ei;
		EditorGetStringEx	gs;
		TEditorPos				epos = EditorGetPos ();
		String						sComment;
		intptr_t					iComment = 0;
		const TLang				*lng = langColl[te->lang];
		if (lng)
		{
			sComment = lng->blockcomment;
			iComment = sComment.length();
			if (iComment == 0) return ;
		}
		Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
		if ((ei.BlockType == BTYPE_STREAM) || (ei.BlockType == BTYPE_COLUMN))
		{
			iStartString = ei.BlockStartLine;
			iStringsCnt = ei.TotalLines - iStartString;
		}
		else
		{
			iStartString = epos.Row;
			iStringsCnt = 1;
		}
		EditorGetStr (&gs, iStartString);
		if (ei.BlockType == BTYPE_COLUMN)
			iStringPos = gs.SelStart;
		else
			iStringPos = 0;

		intptr_t	i = 0;
		wchar_t		*pc;
		while (true)
		{
			pc = nullptr;
			switch (action)
			{
			case eComment:
				{
					pc = wcsncpy (line, gs.StringText, min (iStringPos, gs.StringLength) + 1) + iStringPos;
					if (gs.StringLength > iStringPos)
					{
						pc = wcscpy (pc, sComment) + iComment;
						wcsncpy (pc, gs.StringText + iStringPos, gs.StringLength - iStringPos + 1);
					}
				}
				break;
			case eUnComment:
				{
					if (!memcmp (gs.StringText + iStringPos, sComment, iComment))
					{
						pc = wcsncpy (line, gs.StringText, iStringPos + 1) + iStringPos;
						if (gs.StringLength > (iStringPos + iComment))
						{
							wcsncpy (pc, gs.StringText + iStringPos + iComment, gs.StringLength - iStringPos - iComment + 1);
						}
					}
				}
				break;
			}

			if (pc)
			{
				EditorSetStr (line, iStartString + i);
			}

			if (iStringsCnt == ++i) break;
			EditorGetStr (&gs, iStartString + i);
			if (gs.SelStart == -1 || gs.SelStart == gs.SelEnd) break;
		}

		if (ei.BlockType == BTYPE_STREAM)
		{
			selectBlock (0, iStartString, 0, iStartString + i);
		}

		EditorSetPos (epos);
	}
}
