#define USER_INPUT_LENGTH 512

enum UserStringType {
	USER_CHECK,
	USER_COMBO,
	USER_DROPDOWN,
	USER_EDIT,
	USER_FIXEDIT,
	USER_LIST,
	USER_RADIO,
	USER_STATIC,
};

static UserStringType	utypString[10];
static String userString[10];
static String uttlString[10];
static String uformat[1000];
static String ulist[10][10];

static void nullUserStrings (void)
{
	for (int i = 0; i < 10; i++) {
		utypString[i] = USER_CHECK;
		userString[i].clear ();
		uttlString[i].clear ();
	}
	for (int i = 0; i < 1000; i++)
		uformat[i].clear ();
	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 10; j++)
			ulist[i][j].clear ();
}

static void unpackUserString(const String &userStr, String &format, size_t index)
{
	const wchar_t	*tmp = userStr;
	size_t				curindex = 0;
	format.clear ();
	while (*tmp)
	{
		if (tmp[0] == L'\'')
		{
			tmp++;
			while (*tmp)
			{
				if ((tmp[0] == L'\\') && ((tmp[1] == L'\\') || (tmp[1] == L'\'') || (tmp[1] == L'?') || (tmp[1] == L'l')))
				{
					tmp++;
					if (curindex == index)
						format += *tmp++;
					else
						tmp++;
				}
				else
				{
					if (*tmp == L'\'')
					{
						tmp++;
						if (curindex == index)
						{
							return ;
						}
						else
							break;
					}
					else
					{
						if (curindex == index)
							format += *tmp++;
						else
							tmp++;
					}
				}
			}

			curindex++;
		}

		tmp++;
	}
}

static void skipUserInputMacro (const wchar_t *&p)
{
	while (*p)
	{
		if ((p[0] == L'\\') && ((p[1] == L'\\') || (p[1] == L'\'') || (p[1] == L'?') || (p[1] == L'l')))
		{
			p++;
			p++;
		}
		else if (*p == L'?')
			break;
		else
			p++;
	}
}

static bool scanUserInput (bool inMacro, wchar_t macro, const wchar_t *MacroText, const wchar_t *title = nullptr)
{
	size_t count = 0;
	nullUserStrings ();
	for (const wchar_t *p = (wchar_t *) MacroText; *p; p++)
	{
		if (*p == macro)
		{
			//Getting user string type and argument string
			if (*++p == L'?')
			{
				const wchar_t	*tmp = p + 1;
				if (*tmp == L'\'')
				{
					tmp++;

					uttlString[count].clear ();
					while (*tmp)
					{
						if ((tmp[0] == L'\\') && ((tmp[1] == L'\\') || (tmp[1] == L'\'') || (tmp[1] == L'?')))
						{
							tmp++;
							uttlString[count] += *tmp++;
						}
						else if ((tmp[0] == L'\\') && (tmp[1] == L'l'))
						{
							tmp += 2;
							uttlString[count] += EditorGetSelectionLine();
						}
						else if (*tmp == L'\'')
						{
							tmp++;
							break;
						}
						else
							uttlString[count] += *tmp++;
					}

					switch (*tmp)
					{
					case L'?':
						utypString[count] = USER_STATIC;
						break;
					case L'x':
						tmp++;
						utypString[count] = USER_CHECK;
						break;
					case L'c':
						tmp++;
						utypString[count] = USER_COMBO;
						break;
					case L'd':
						tmp++;
						utypString[count] = USER_DROPDOWN;
						break;
					case L'e':
						tmp++;
						utypString[count] = USER_EDIT;
						break;
					case L'f':
						tmp++;
						utypString[count] = USER_FIXEDIT;
						break;
					case L'l':
						tmp++;
						utypString[count] = USER_LIST;
						break;
					case L'r':
						tmp++;
						utypString[count] = USER_RADIO;
						break;
					default:
						return (false);
					}

					while (*tmp)
					{
						if ((tmp[0] == L'\\') && ((tmp[1] == L'\\') || (tmp[1] == L'\'') || (tmp[1] == L'?')))
						{
							userString[count] += *tmp++;
							userString[count] += *tmp++;
						}
						else if ((tmp[0] == L'\\') && (tmp[1] == L'l'))
						{
							tmp += 2;
							userString[count] += EditorGetSelectionLine();
						}
						else if (*tmp == L'?')
						{
							tmp++;
							break;
						}
						else
							userString[count] += *tmp++;
					}
					count++;
				}

				p++;
				skipUserInputMacro (p);
			}
		}
	}

	//Calculating dialog items count
	intptr_t addlength = 0;
	intptr_t itemscount = 4;
	for (size_t i = 0; i < count; i++)
	{
		switch (utypString[i])
		{
		case USER_STATIC:
		case USER_CHECK:
			itemscount += 1;
			break;
		case USER_COMBO:
		case USER_DROPDOWN:
		case USER_EDIT:
		case USER_FIXEDIT:
			itemscount += 2;
			break;
		case USER_LIST:
			{
				addlength += 1;
				itemscount += 1;

				const wchar_t	*tmp = userString[i];
				while (*tmp)
				{
					if
					(
						((tmp[0] != L'\\') && (tmp[1] == L'\'') && ((tmp[2] == L'+') || (tmp[2] == L'-')) && (tmp[3] == L'\'')) ||
						((tmp == userString[i]) && ((tmp[0] == L'+') || (tmp[0] == L'-')) && ((tmp[1] == L'\'')))
					) addlength += 1;
					tmp++;
				}
				break;
			}

		case USER_RADIO:
			{
				addlength += 1;
				itemscount += 1;

				const wchar_t	*tmp = userString[i];
				while (*tmp)
				{
					if
					(
						((tmp[0] != L'\\') && (tmp[1] == L'\'') && ((tmp[2] == L'+') || (tmp[2] == L'-')) && (tmp[3] == L'\'')) ||
						((tmp == userString[i]) && ((tmp[0] == L'+') || (tmp[0] == L'-')) && ((tmp[1] == L'\'')))
					) itemscount += 1;
					tmp++;
				}
				break;
			}
		}
	}

	//Call dialog box
	bool	res = true;
	if (count == 0) return (res);

	struct InitDialogItemEx InitItem;
	struct FarDialogItem		*DialogItems = new FarDialogItem[itemscount];
	struct FarList					*listBox = new FarList[count];
	bool	*needFreeLists = new bool[count];
	for (size_t i = 0; i < count; i++)
	{
		needFreeLists[i] = false;
		listBox[i].ItemsNumber = 0;
		listBox[i].Items = nullptr;
	}

	if (DialogItems != nullptr)
	{
    	intptr_t curDlgItem = 0;
    	intptr_t curDlgPos = 2;

		//Dialog frame
		InitItem.Type = DI_DOUBLEBOX;
		InitItem.X1 = 3;
		InitItem.Y1 = 1;
		InitItem.X2 = 55;
		InitItem.Y2 = itemscount + addlength;
		InitItem.Focus = false;
		InitItem.Selected = false;
		InitItem.Flags = DIF_BOXCOLOR;
		InitItem.DefaultButton = false;
		InitItem.Data = (title != nullptr) ? title : GetMsg (MParams);
		InitItem.History = nullptr;
		InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);

		bool	gainfocus = true;
		for (size_t i = 0; i < count; i++)
		{
			switch (utypString[i])
			{
			case USER_STATIC:
				InitItem.Type = DI_TEXT;
				InitItem.X1 = 5;
				InitItem.Y1 = curDlgPos++;
				InitItem.X2 = 53;
				InitItem.Y2 = 0;
				InitItem.Focus = false;
				InitItem.Selected = false;
				InitItem.Flags = 0;
				InitItem.DefaultButton = false;
				InitItem.Data = uttlString[i];
				InitItem.History = nullptr;
				InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);
				break;
			case USER_CHECK:
				{
					bool		selected = false;
					const wchar_t	*tmp = userString[i];
					while (*tmp)
					{
						if ((tmp[0] != L'\\') && (tmp[1] == L'\'') && (tmp[2] == L'+') && (tmp[3] == L'\''))
						{
							selected = true;
							break;
						}

						tmp++;
					}

					InitItem.Type = DI_CHECKBOX;
					InitItem.X1 = 5;
					InitItem.Y1 = curDlgPos++;
					InitItem.X2 = 53;
					InitItem.Y2 = 0;
					InitItem.Focus = gainfocus;
					InitItem.Selected = selected;
					InitItem.Flags = 0;
					InitItem.DefaultButton = false;
					InitItem.Data = uttlString[i];
					InitItem.History = nullptr;
					InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);
					gainfocus = (gainfocus) ? false : gainfocus;
					break;
				}

			case USER_COMBO:
			case USER_DROPDOWN:
				{
					InitItem.Type = DI_TEXT;
					InitItem.X1 = 5;
					InitItem.Y1 = curDlgPos++;
					InitItem.X2 = 53;
					InitItem.Y2 = 0;
					InitItem.Focus = false;
					InitItem.Selected = false;
					InitItem.Flags = 0;
					InitItem.DefaultButton = false;
					InitItem.Data = uttlString[i];
					InitItem.History = nullptr;
					InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);

					intptr_t	ic = 0;
					intptr_t	sel = -1;
					const wchar_t	*tmp = userString[i];
					while (*tmp)
					{
						if
						(
							((tmp[0] != L'\\') && (tmp[1] == L'\'') && (tmp[2] == L'+') && (tmp[3] == L'\'')) ||
							((tmp == userString[i]) && (tmp[0] == L'+') && (tmp[1] == L'\''))
						) sel = ic++;
						if
						(
							((tmp[0] != L'\\') && (tmp[1] == L'\'') && (tmp[2] == L'-') && (tmp[3] == L'\'')) ||
							((tmp == userString[i]) && (tmp[0] == L'-') && (tmp[1] == L'\''))
						) ic++;
						tmp++;
					}

					if (ic > 0)
					{
						needFreeLists[i] = true;
						listBox[i].ItemsNumber = ic;
						listBox[i].Items = new FarListItem[ic];
					}

					for (intptr_t j = 0; j < ic; j++)
					{
						listBox[i].Items[j].Flags = (j == sel) ? LIF_SELECTED : 0;
						listBox[i].Items[j].Reserved[0] = 0;
						listBox[i].Items[j].Reserved[1] = 0;
						unpackUserString (userString[i], ulist[i][j], (size_t)((utypString[i] == USER_COMBO) ? j : 3 * j));
						listBox[i].Items[j].Text = ulist[i][j];
					}

					InitItem.Type = DI_COMBOBOX;
					InitItem.X1 = 5;
					InitItem.Y1 = curDlgPos++;
					InitItem.X2 = 53;
					InitItem.Y2 = 0;
					InitItem.Focus = gainfocus;
					InitItem.Selected = (bool) (&listBox[i]);
					InitItem.Flags = DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND | DIF_LISTWRAPMODE | ((utypString[i] == USER_COMBO) ? 0 : DIF_DROPDOWNLIST);
					InitItem.DefaultButton = false;
					InitItem.Data = (sel < 0) ? L"" : listBox[i].Items[sel].Text;
					InitItem.History = nullptr;
					InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);
					gainfocus = (gainfocus) ? false : gainfocus;
					break;
				}

			case USER_EDIT:
			case USER_FIXEDIT:
				{
					String	len;
					InitItem.Type = DI_TEXT;
					InitItem.X1 = 5;
					InitItem.Y1 = curDlgPos++;
					InitItem.X2 = 53;
					InitItem.Y2 = 0;
					InitItem.Focus = false;
					InitItem.Selected = false;
					InitItem.Flags = 0;
					InitItem.DefaultButton = false;
					InitItem.Data = uttlString[i];
					InitItem.History = nullptr;
					InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);
					unpackUserString (userString[i], uformat[curDlgItem], 0);
					if (utypString[i] == USER_FIXEDIT) unpackUserString (userString[i], len, 1);
					userString[i] = uformat[curDlgItem];
					InitItem.Type = (utypString[i] == USER_EDIT) ? DI_EDIT : DI_FIXEDIT;
					InitItem.X1 = 5;
					InitItem.Y1 = curDlgPos++;
					InitItem.X2 = (utypString[i] == USER_EDIT) ? 53 : (5 + _wtoi (len) - 1);
					InitItem.Y2 = 0;
					InitItem.Focus = gainfocus;
					InitItem.Selected = false;
					InitItem.Flags = (utypString[i] == USER_EDIT) ? DIF_HISTORY : 0;
					InitItem.DefaultButton = false;
					InitItem.Data = uformat[curDlgItem];
					InitItem.History = (utypString[i] == USER_EDIT) ? L"True-Tpl.History.UserInput.Edit" : nullptr;
					InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);
					gainfocus = (gainfocus) ? false : gainfocus;
					break;
				}

			case USER_LIST:
				{
					intptr_t	ic = 0;
					intptr_t	sel = -1;
					const wchar_t	*tmp = userString[i];
					while (*tmp)
					{
						if
						(
							((tmp[0] != L'\\') && (tmp[1] == L'\'') && (tmp[2] == L'+') && (tmp[3] == L'\'')) ||
							((tmp == userString[i]) && (tmp[0] == L'+') && (tmp[1] == L'\''))
						) sel = ic++;
						if
						(
							((tmp[0] != L'\\') && (tmp[1] == L'\'') && (tmp[2] == L'-') && (tmp[3] == L'\'')) ||
							((tmp == userString[i]) && (tmp[0] == L'-') && (tmp[1] == L'\''))
						) ic++;
						tmp++;
					}

					if (ic > 0)
					{
						needFreeLists[i] = true;
						listBox[i].ItemsNumber = ic;
						listBox[i].Items = new FarListItem[ic];
					}

					for (intptr_t j = 0; j < ic; j++)
					{
						listBox[i].Items[j].Flags = (j == sel) ? LIF_SELECTED : 0;
						listBox[i].Items[j].Reserved[0] = 0;
						listBox[i].Items[j].Reserved[1] = 0;
						unpackUserString(userString[i], ulist[i][j], 3 * (size_t)j);
						listBox[i].Items[j].Text = ulist[i][j];
					}

					InitItem.Type = DI_LISTBOX;
					InitItem.X1 = 5;
					InitItem.Y1 = curDlgPos++;
					InitItem.X2 = 53;
					InitItem.Y2 = (curDlgPos += ic)++;
					InitItem.Focus = gainfocus;
					InitItem.Selected = (bool) (&listBox[i]);
					InitItem.Flags = DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND | DIF_LISTWRAPMODE;
					InitItem.DefaultButton = false;
					InitItem.Data = uttlString[i];
					InitItem.History = nullptr;
					InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);
					gainfocus = (gainfocus) ? false : gainfocus;
					break;
				}

			case USER_RADIO:
				{
					intptr_t	ic = 0;
					const wchar_t	*tmp = userString[i];
					while (*tmp)
					{
						if
						(
							((tmp[0] != L'\\') && (tmp[1] == L'\'') && ((tmp[2] == L'+') || (tmp[2] == L'-')) && (tmp[3] == L'\'')) ||
							((tmp == userString[i]) && ((tmp[0] == L'+') || (tmp[0] == L'-')) && ((tmp[1] == L'\'')))
						) ic += 1;
						tmp++;
					}

					InitItem.Type = DI_SINGLEBOX;
					InitItem.X1 = 5;
					InitItem.Y1 = curDlgPos++;
					InitItem.X2 = 53;
					InitItem.Y2 = curDlgPos + ic;
					InitItem.Focus = false;
					InitItem.Selected = false;
					InitItem.Flags = 0;
					InitItem.DefaultButton = false;
					InitItem.Data = uttlString[i];
					InitItem.History = nullptr;
					InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);
					ic = 0;
					tmp = userString[i];

					bool	selItem = false;
					bool	startGroup = true;
					while (*tmp)
					{
						if
						(
							((tmp[0] != L'\\') && (tmp[1] == L'\'') && (tmp[2] == L'+') && (tmp[3] == L'\'')) ||
							((tmp == userString[i]) && (tmp[0] == L'+') && (tmp[1] == L'\''))
						) selItem = true;
						if
						(
							((tmp[0] != L'\\') && (tmp[1] == L'\'') && (tmp[2] == L'-') && (tmp[3] == L'\'')) ||
							((tmp == userString[i]) && (tmp[0] == L'-') && (tmp[1] == L'\''))
						) selItem = false;
						if
						(
							((tmp[0] != L'\\') && (tmp[1] == L'\'') && ((tmp[2] == L'+') || (tmp[2] == L'-')) && (tmp[3] == L'\'')) ||
							((tmp == userString[i]) && ((tmp[0] == L'+') || (tmp[0] == L'-')) && ((tmp[1] == L'\'')))
						)
						{
							unpackUserString (userString[i], uformat[curDlgItem], ic * 3);
							InitItem.Type = DI_RADIOBUTTON;
							InitItem.X1 = 6;
							InitItem.Y1 = curDlgPos++;
							InitItem.X2 = 52;
							InitItem.Y2 = 0;
							InitItem.Focus = gainfocus;
							InitItem.Selected = selItem;
							InitItem.Flags = (startGroup) ? DIF_GROUP : 0;
							InitItem.DefaultButton = false;
							InitItem.Data = uformat[curDlgItem];
							InitItem.History = nullptr;
							InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);
							gainfocus = (gainfocus) ? false : gainfocus;
							startGroup = (startGroup) ? false : startGroup;
							ic++;
						}

						tmp++;
					}

					curDlgPos++;
					break;
				}
			}
		}

		//Separator
		InitItem.Type = DI_TEXT;
		InitItem.X1 = 0;
		InitItem.Y1 = itemscount + addlength - 2;
		InitItem.X2 = 0;
		InitItem.Y2 = 0;
		InitItem.Focus = false;
		InitItem.Selected = false;
		InitItem.Flags = DIF_BOXCOLOR | DIF_SEPARATOR;
		InitItem.DefaultButton = false;
		InitItem.Data = L"";
		InitItem.History = nullptr;
		InitDialogItemsEx (&InitItem, DialogItems + itemscount - 3, 1);

		//Button: Ok
		InitItem.Type = DI_BUTTON;
		InitItem.X1 = 0;
		InitItem.Y1 = itemscount + addlength - 1;
		InitItem.X2 = 0;
		InitItem.Y2 = 0;
		InitItem.Focus = false;
		InitItem.Selected = false;
		InitItem.Flags = DIF_CENTERGROUP;
		InitItem.DefaultButton = true;
		InitItem.Data = GetMsg (MOK);
		InitItem.History = nullptr;
		InitDialogItemsEx (&InitItem, DialogItems + itemscount - 2, 1);

		//Button: Cancel
		InitItem.Type = DI_BUTTON;
		InitItem.X1 = 0;
		InitItem.Y1 = itemscount + addlength - 1;
		InitItem.X2 = 0;
		InitItem.Y2 = 0;
		InitItem.Focus = false;
		InitItem.Selected = false;
		InitItem.Flags = DIF_CENTERGROUP;
		InitItem.DefaultButton = false;
		InitItem.Data = GetMsg (MCancel);
		InitItem.History = nullptr;
		InitDialogItemsEx (&InitItem, DialogItems + itemscount - 1, 1);

		HANDLE hDlg = Info.DialogInit(&MainGuid, &UserExecGuid, -1, -1, 59, itemscount + addlength + 2, nullptr, DialogItems, itemscount, 0, 0, Info.DefDlgProc, 0);
		if (hDlg != INVALID_HANDLE_VALUE)
		{
			intptr_t n = Info.DialogRun(hDlg);
			if (n == itemscount - 2)
			{
				intptr_t curdialogitem = 1;
				for (size_t i = 0; i < count; i++)
				{
					switch (utypString[i])
					{
					case USER_STATIC:
						userString[i].clear ();
						curdialogitem += 1;
						break;
					case USER_CHECK:
						unpackUserString (userString[i], uformat[curdialogitem], (Info.SendDlgMessage(hDlg,DM_GETCHECK,curdialogitem,nullptr) == BSTATE_CHECKED) ? 0 : 1);
						userString[i] = uformat[curdialogitem];
						curdialogitem += 1;
						break;
					case USER_COMBO:
					case USER_DROPDOWN:
						{
							FarDialogItemDataEx item;
							wchar_t buf[USER_INPUT_LENGTH];
							item.PtrLength = _countof(buf);
							item.PtrData = buf;
							Info.SendDlgMessage(hDlg, DM_GETTEXT, curdialogitem + 1, (void*)&item);
							userString[i] = buf;
							curdialogitem += 2;
							break;
						}
					case USER_EDIT:
						{
							FarDialogItemDataEx item;
							wchar_t buf[USER_INPUT_LENGTH];
							item.PtrLength = _countof(buf);
							item.PtrData = buf;
							Info.SendDlgMessage(hDlg, DM_GETTEXT, curdialogitem + 1, (void*)&item);
							userString[i] = buf;
							curdialogitem += 2;
							break;
						}
					case USER_FIXEDIT:
						{
							FarDialogItemDataEx item;
							wchar_t buf[USER_INPUT_LENGTH];
							item.PtrLength = _countof(buf);
							item.PtrData = buf;
							Info.SendDlgMessage(hDlg, DM_GETTEXT, curdialogitem + 1, (void*)&item);
							userString[i] = buf;
							curdialogitem += 2;
							break;
						}
					case USER_LIST:
						{
							String	tmpList;
							const wchar_t	*tmp = userString[i];
							intptr_t	ic = 0;
							tmpList.clear ();
							while (*tmp)
							{
								if
									(
									((tmp[0] != L'\\') && (tmp[1] == L'\'') && ((tmp[2] == L'+') || (tmp[2] == L'-')) && (tmp[3] == L'\'')) ||
									((tmp == userString[i]) && ((tmp[0] == L'+') || (tmp[0] == L'-')) && ((tmp[1] == L'\'')))
									)
								{
									struct FarListGetItem List = { sizeof(FarListGetItem) };
									List.ItemIndex = ic;
									Info.SendDlgMessage(hDlg, DM_LISTGETITEM, curdialogitem, &List);

									unpackUserString
										(
										userString[i],
										uformat[curdialogitem],
										ic * 3 + ((List.Item.Flags & LIF_SELECTED) ? 1 : 2)
										);
									tmpList += uformat[curdialogitem];
									ic++;
								}

								tmp++;
							}

							userString[i] = tmpList;
							curdialogitem += 1;
							break;
						}

					case USER_RADIO:
						{
							String	tmpRadio;
							const wchar_t	*tmp = userString[i];
							intptr_t	ic = 0;
							tmpRadio.clear ();
							while (*tmp)
							{
								if
								(
									((tmp[0] != L'\\') && (tmp[1] == L'\'') && ((tmp[2] == L'+') || (tmp[2] == L'-')) && (tmp[3] == L'\'')) ||
									((tmp == userString[i]) && ((tmp[0] == L'+') || (tmp[0] == L'-')) && ((tmp[1] == L'\'')))
								)
								{
									unpackUserString
									(
										userString[i],
										uformat[curdialogitem],
										ic * 3 + ((Info.SendDlgMessage(hDlg,DM_GETCHECK,curdialogitem + ic + 1,nullptr) == BSTATE_CHECKED) ? 1 : 2)
									);
									tmpRadio += uformat[curdialogitem];
									ic++;
								}

								tmp++;
							}

							userString[i] = tmpRadio;
							curdialogitem += ic + 1;
							break;
						}
					}
				}
			}
			else
			{
				for (size_t i = 0; i < count; i++)
					userString[i].clear ();
				res = false;
			}
			Info.DialogFree(hDlg);
		}
	}

	for (size_t i = 0; i < count; i++)
		if (!needFreeLists[i]) delete[] listBox[i].Items;
	delete[] needFreeLists;
	delete[] listBox;
	delete[] DialogItems;
	return (res);
}
