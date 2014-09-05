#define USER_INPUT_LENGTH 512
#define USER_CHECK				0
#define USER_COMBO				1
#define USER_DROPDOWN			2
#define USER_EDIT					3
#define USER_FIXEDIT			4
#define USER_LIST					5
#define USER_RADIO				6
#define USER_STATIC				7

static int	utypString[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static wchar_t userString[10][USER_INPUT_LENGTH] = { L"", L"", L"", L"", L"", L"", L"", L"", L"", L"" };
static wchar_t uttlString[10][USER_INPUT_LENGTH] = { L"", L"", L"", L"", L"", L"", L"", L"", L"", L"" };
static wchar_t uformat[1000][USER_INPUT_LENGTH];
static wchar_t ulist[10][10][USER_INPUT_LENGTH];

static void nullUserStrings (void)
{
	for (int i = 0; i < 10; i++) utypString[i] = *userString[i] = *uttlString[i] = 0;
	for (int i = 0; i < 1000; i++) *uformat[i] = 0;
        for (int i = 0; i < 10; i++)
                for (int j = 0; j < 10; j++)
                        *ulist[i][j] = 0;
}

void unpackUserString (const wchar_t *userStr, wchar_t *format, size_t index)
{
	const wchar_t	*tmp = userStr;
	wchar_t				*tmpname = format;
	size_t				curindex = 0;
	*tmpname = L'\0';
	while (*tmp)
	{
		if (tmp[0] == L'\'')
		{
			tmp++;
			while (*tmp)
			{
				if ((tmp[0] == L'\\') && ((tmp[1] == L'\\') || (tmp[1] == L'\'') || (tmp[1] == L'?')))
				{
					tmp++;
					if (curindex == index)
						*tmpname++ = *tmp++;
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
							*tmpname = L'\0';
							return ;
						}
						else
							break;
					}
					else
					{
						if (curindex == index)
							*tmpname++ = *tmp++;
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

static bool scanUserInput (bool inMacro, wchar_t macro, const wchar_t *MacroText, const wchar_t *title = NULL)
{
	size_t count = 0;
	nullUserStrings ();
	for (wchar_t *p = (wchar_t *) MacroText; *p; p++)
	{
		if (*p == macro)
		{
			//Getting user string type and argument string
			if (*++p == L'?')
			{
				wchar_t	*tmp = p + 1;
				if (*tmp == L'\'')
				{
					tmp++;

					wchar_t	*tmpname = uttlString[count];
					*tmpname = '\0';
					while (*tmp)
					{
						if ((tmp[0] == L'\\') && ((tmp[1] == L'\\') || (tmp[1] == L'\'') || (tmp[1] == L'?')))
						{
							tmp++;
							*tmpname++ = *tmp++;
						}
						else
						{
							if (*tmp == L'\'')
							{
								tmp++;
								*tmpname = L'\0';
								break;
							}
							else
								*tmpname++ = *tmp++;
						}
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

					tmpname = userString[count++];
					while (*tmp)
					{
						if ((tmp[0] == L'\\') && ((tmp[1] == L'\\') || (tmp[1] == L'\'') || (tmp[1] == L'?')))
						{
							*tmpname++ = *tmp++;
							*tmpname++ = *tmp++;
						}
						else
						{
							if (*tmp == L'?')
							{
								tmp++;
								*tmpname = L'\0';
								break;
							}
							else
								*tmpname++ = *tmp++;
						}
					}
				}

				p++;
				while (*p)
				{
					if ((p[0] == L'\\') && ((p[1] == L'\\') || (p[1] == L'\'') || (p[1] == L'?')))
					{
						p++;
						p++;
					}
					else
					{
						if (*p == L'?')
							break;
						else
							p++;
					}
				}
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

				wchar_t	*tmp = userString[i];
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

				wchar_t	*tmp = userString[i];
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
		listBox[i].Items = NULL;
	}

	if (DialogItems != NULL)
	{
    	intptr_t curDlgItem = 0;
    	intptr_t curDlgPos = 2;

		//Dialog frame
		InitItem.Type = DI_DOUBLEBOX;
		InitItem.X1 = 3;
		InitItem.Y1 = 1;
		InitItem.X2 = 55;
		InitItem.Y2 = itemscount + addlength;
		InitItem.Focus = 0;
		InitItem.Selected = 0;
		InitItem.Flags = DIF_BOXCOLOR;
		InitItem.DefaultButton = 0;
		InitItem.Data = (title != NULL) ? title : GetMsg (MParams);
		InitItem.History = NULL;
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
				InitItem.Focus = 0;
				InitItem.Selected = 0;
				InitItem.Flags = 0;
				InitItem.DefaultButton = 0;
				InitItem.Data = uttlString[i];
				InitItem.History = NULL;
				InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);
				break;
			case USER_CHECK:
				{
					int		selected = 0;
					wchar_t	*tmp = userString[i];
					while (*tmp)
					{
						if ((tmp[0] != L'\\') && (tmp[1] == L'\'') && (tmp[2] == L'+') && (tmp[3] == L'\''))
						{
							selected = 1;
							break;
						}

						tmp++;
					}

					InitItem.Type = DI_CHECKBOX;
					InitItem.X1 = 5;
					InitItem.Y1 = curDlgPos++;
					InitItem.X2 = 53;
					InitItem.Y2 = 0;
					InitItem.Focus = (gainfocus) ? 1 : 0;
					InitItem.Selected = selected;
					InitItem.Flags = 0;
					InitItem.DefaultButton = 0;
					InitItem.Data = uttlString[i];
					InitItem.History = NULL;
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
					InitItem.Focus = 0;
					InitItem.Selected = 0;
					InitItem.Flags = 0;
					InitItem.DefaultButton = 0;
					InitItem.Data = uttlString[i];
					InitItem.History = NULL;
					InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);

					intptr_t	ic = 0;
					intptr_t	sel = -1;
					wchar_t	*tmp = userString[i];
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
					InitItem.Focus = (gainfocus) ? 1 : 0;
					InitItem.Selected = (int) (&listBox[i]);
					InitItem.Flags = DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND | DIF_LISTWRAPMODE | ((utypString[i] == USER_COMBO) ? 0 : DIF_DROPDOWNLIST);
					InitItem.DefaultButton = 0;
					InitItem.Data = (sel < 0) ? L"" : listBox[i].Items[sel].Text;
					InitItem.History = NULL;
					InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);
					gainfocus = (gainfocus) ? false : gainfocus;
					break;
				}

			case USER_EDIT:
			case USER_FIXEDIT:
				{
					wchar_t	len[USER_INPUT_LENGTH];
					InitItem.Type = DI_TEXT;
					InitItem.X1 = 5;
					InitItem.Y1 = curDlgPos++;
					InitItem.X2 = 53;
					InitItem.Y2 = 0;
					InitItem.Focus = 0;
					InitItem.Selected = 0;
					InitItem.Flags = 0;
					InitItem.DefaultButton = 0;
					InitItem.Data = uttlString[i];
					InitItem.History = NULL;
					InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);
					unpackUserString (userString[i], uformat[curDlgItem], 0);
					if (utypString[i] == USER_FIXEDIT) unpackUserString (userString[i], len, 1);
					wcscpy (userString[i], uformat[curDlgItem]);
					InitItem.Type = (utypString[i] == USER_EDIT) ? DI_EDIT : DI_FIXEDIT;
					InitItem.X1 = 5;
					InitItem.Y1 = curDlgPos++;
					InitItem.X2 = (utypString[i] == USER_EDIT) ? 53 : (5 + _wtoi (len) - 1);
					InitItem.Y2 = 0;
					InitItem.Focus = (gainfocus) ? 1 : 0;
					InitItem.Selected = 0;
					InitItem.Flags = (utypString[i] == USER_EDIT) ? DIF_HISTORY : 0;
					InitItem.DefaultButton = 0;
					InitItem.Data = uformat[curDlgItem];
					InitItem.History = (utypString[i] == USER_EDIT) ? L"True-Tpl.History.UserInput.Edit" : NULL;
					InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);
					gainfocus = (gainfocus) ? false : gainfocus;
					break;
				}

			case USER_LIST:
				{
					intptr_t	ic = 0;
					intptr_t	sel = -1;
					wchar_t	*tmp = userString[i];
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
					InitItem.Focus = (gainfocus) ? 1 : 0;
					InitItem.Selected = (int) (&listBox[i]);
					InitItem.Flags = DIF_LISTAUTOHIGHLIGHT | DIF_LISTNOAMPERSAND | DIF_LISTWRAPMODE;
					InitItem.DefaultButton = 0;
					InitItem.Data = uttlString[i];
					InitItem.History = NULL;
					InitDialogItemsEx (&InitItem, DialogItems + curDlgItem++, 1);
					gainfocus = (gainfocus) ? false : gainfocus;
					break;
				}

			case USER_RADIO:
				{
					intptr_t	ic = 0;
					wchar_t	*tmp = userString[i];
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
					InitItem.Focus = 0;
					InitItem.Selected = 0;
					InitItem.Flags = 0;
					InitItem.DefaultButton = 0;
					InitItem.Data = uttlString[i];
					InitItem.History = NULL;
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
							InitItem.Focus = (gainfocus) ? 1 : 0;
							InitItem.Selected = (selItem) ? 1 : 0;
							InitItem.Flags = (startGroup) ? DIF_GROUP : 0;
							InitItem.DefaultButton = 0;
							InitItem.Data = uformat[curDlgItem];
							InitItem.History = NULL;
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
		InitItem.Focus = 0;
		InitItem.Selected = 0;
		InitItem.Flags = DIF_BOXCOLOR | DIF_SEPARATOR;
		InitItem.DefaultButton = 0;
		InitItem.Data = L"";
		InitItem.History = NULL;
		InitDialogItemsEx (&InitItem, DialogItems + itemscount - 3, 1);

		//Button: Ok
		InitItem.Type = DI_BUTTON;
		InitItem.X1 = 0;
		InitItem.Y1 = itemscount + addlength - 1;
		InitItem.X2 = 0;
		InitItem.Y2 = 0;
		InitItem.Focus = 0;
		InitItem.Selected = 0;
		InitItem.Flags = DIF_CENTERGROUP;
		InitItem.DefaultButton = 1;
		InitItem.Data = GetMsg (MOK);
		InitItem.History = NULL;
		InitDialogItemsEx (&InitItem, DialogItems + itemscount - 2, 1);

		//Button: Cancel
		InitItem.Type = DI_BUTTON;
		InitItem.X1 = 0;
		InitItem.Y1 = itemscount + addlength - 1;
		InitItem.X2 = 0;
		InitItem.Y2 = 0;
		InitItem.Focus = 0;
		InitItem.Selected = 0;
		InitItem.Flags = DIF_CENTERGROUP;
		InitItem.DefaultButton = 0;
		InitItem.Data = GetMsg (MCancel);
		InitItem.History = NULL;
		InitDialogItemsEx (&InitItem, DialogItems + itemscount - 1, 1);

		HANDLE hDlg = Info.DialogInit(&MainGuid, &UserExecGuid, -1, -1, 59, itemscount + addlength + 2, NULL, DialogItems, itemscount, 0, 0, Info.DefDlgProc, 0);
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
						wcscpy (userString[i], L"");
						curdialogitem += 1;
						break;
					case USER_CHECK:
						unpackUserString (userString[i], uformat[curdialogitem], (Info.SendDlgMessage(hDlg,DM_GETCHECK,curdialogitem,NULL) == BSTATE_CHECKED) ? 0 : 1);
						wcscpy (userString[i], uformat[curdialogitem]);
						curdialogitem += 1;
						break;
					case USER_COMBO:
					case USER_DROPDOWN:
						{
							FarDialogItemDataEx item;
							item.PtrLength = 512;
							item.PtrData = userString[i];
							Info.SendDlgMessage(hDlg, DM_GETTEXT, curdialogitem + 1, (void*)&item);
							curdialogitem += 2;
							break;
						}
					case USER_EDIT:
						{
							FarDialogItemDataEx item;
							item.PtrLength = 512;
							item.PtrData = userString[i];
							Info.SendDlgMessage(hDlg, DM_GETTEXT, curdialogitem + 1, (void*)&item);
							curdialogitem += 2;
							break;
						}
					case USER_FIXEDIT:
						{
							FarDialogItemDataEx item;
							item.PtrLength = 512;
							item.PtrData = userString[i];
							Info.SendDlgMessage(hDlg, DM_GETTEXT, curdialogitem + 1, (void*)&item);
							curdialogitem += 2;
							break;
						}
					case USER_LIST:
						{
							wchar_t	tmpList[USER_INPUT_LENGTH];
							wchar_t	*tmp = userString[i];
							intptr_t	ic = 0;
							wcscpy (tmpList, L"");
							while (*tmp)
							{
								if
								(
									((tmp[0] != L'\\') && (tmp[1] == L'\'') && ((tmp[2] == L'+') || (tmp[2] == L'-')) && (tmp[3] == L'\'')) ||
									((tmp == userString[i]) && ((tmp[0] == L'+') || (tmp[0] == L'-')) && ((tmp[1] == L'\'')))
								)
								{
                                    struct FarListGetItem List={sizeof(FarListGetItem)};
                                    List.ItemIndex=ic;
                                    Info.SendDlgMessage(hDlg,DM_LISTGETITEM,curdialogitem,&List);

									unpackUserString
									(
										userString[i],
										uformat[curdialogitem],
										ic * 3 + ((List.Item.Flags & LIF_SELECTED) ? 1 : 2)
									);
									wcscat (tmpList, uformat[curdialogitem]);
									ic++;
								}

								tmp++;
							}

							wcscpy (userString[i], tmpList);
							curdialogitem += 1;
							break;
						}

					case USER_RADIO:
						{
							wchar_t	tmpRadio[USER_INPUT_LENGTH];
							wchar_t	*tmp = userString[i];
							intptr_t	ic = 0;
							wcscpy (tmpRadio, L"");
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
										ic * 3 + ((Info.SendDlgMessage(hDlg,DM_GETCHECK,curdialogitem + ic + 1,NULL) == BSTATE_CHECKED) ? 1 : 2)
									);
									wcscat (tmpRadio, uformat[curdialogitem]);
									ic++;
								}

								tmp++;
							}

							wcscpy (userString[i], tmpRadio);
							curdialogitem += ic + 1;
							break;
						}
					}
				}
			}
			else
			{
				for (size_t i = 0; i < count; i++) wcscpy(userString[i], L"");
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
