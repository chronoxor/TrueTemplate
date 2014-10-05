bool Config (void)
{
	wchar_t	navRectLeft[32];
	wchar_t	navRectRight[32];
	wchar_t	navRectTop[32];
	wchar_t	navRectBottom[32];

	PluginSettings settings(MainGuid, ::Info.SettingsControl);
	settings.Get(0,L"NavigationRectLeft", navRectLeft, 32, L"0");
	settings.Get(0,L"NavigationRectTop", navRectTop, 32, L"50");
	settings.Get(0,L"NavigationRectRight", navRectRight, 32, L"100");
	settings.Get(0,L"NavigationRectBottom", navRectBottom, 32, L"100");

	const intptr_t		NV = 14;
	const intptr_t		OK = 16;
	InitDialogItem	InitItems[] =
	{
		{ /* 0 */ DI_DOUBLEBOX, 3, 1, 65, 17, false, 0, (wchar_t *) MTitle },
		{ /* 1 */ DI_CHECKBOX, 5, 2, 0, 0, autocompile, 0, (wchar_t *)MAutoCompile },
		{ /* 2 */ DI_CHECKBOX, 5, 3, 0, 0, autoformat, 0, (wchar_t *) MAutoFormat },
		{ /* 3 */ DI_CHECKBOX, 5, 4, 0, 0, pluginStop, 0, (wchar_t *) MDisable },
		{ /* 4 */ DI_CHECKBOX, 5, 5, 0, 0, scrollStop, 0, (wchar_t *) MPause },
		{ /* 5 */ DI_CHECKBOX, 5, 6, 0, 0, ignoreposn, 0, (wchar_t *) MIgnorePos },
		{ /* 6 */ DI_CHECKBOX, 5, 7, 0, 0, outputmenu, 0, (wchar_t *) MOutputMenu },
		{ /* 7 */ DI_CHECKBOX, 5, 8, 0, 0, filterring, 0, (wchar_t *) MFiltering },
		{ /* 8 */ DI_TEXT, 5, 9, 0, 0, false, 0, (wchar_t *)MTplFilename },
		{ /* 9 */ DI_EDIT, 5, 10, 63, 0, false, 0, confFilename },
		{ /* 10 */ DI_TEXT, 5, 11, 0, 255, false, 0, nullptr },
		{ /* 11 */ DI_TEXT, 5, 12, 0, 0, false, 0, (wchar_t *)MKey },
		{ /* 12 */ DI_EDIT, 25, 12, 63, 0, false, 0, defExpandFKey },
		{ /* 13 */ DI_TEXT, 5, 13, 0, 255, false, 0, nullptr },
		{ /* 14 */ DI_BUTTON, 5, 14, 63, 0, false, DIF_CENTERGROUP, (wchar_t *) MNavConfig },
		{ /* 15 */ DI_TEXT, 5, 15, 0, 255, false, 0, nullptr },
		{ /* 16 - OK */ DI_BUTTON, 0, 16, 0, 0, false, DIF_CENTERGROUP, (wchar_t *)MOK },
		{ /* 17 */ DI_BUTTON, 0, 16, 0, 0, false, DIF_CENTERGROUP, (wchar_t *) MCancel }
	};
	struct FarDialogItem		DialogItems[_countof (InitItems)];
	InitDialogItems (InitItems, DialogItems, _countof (InitItems));
	DialogItems[OK].Flags |= DIF_DEFAULTBUTTON;

	while (true)
	{
		HANDLE hDlg1 = Info.DialogInit(&MainGuid, &ConfigGuid, -1, -1, 69, InitItems[OK].Y1 + 3, nullptr, DialogItems, _countof(InitItems), 0, 0, Info.DefDlgProc, 0);
		if (hDlg1 != INVALID_HANDLE_VALUE)
		{
			intptr_t n = Info.DialogRun(hDlg1);
			if (n == NV)
			{
				struct InitDialogItemEx InitItems[] =
				{ //Type X1 Y1 X2 Y2 Fo Se Fl DB Data
					{ /* 00 */ DI_DOUBLEBOX, 3, 1, 55, 12, 0, 0, 0, DIF_BOXCOLOR, GetMsg (MNavConfig), nullptr },
					{ /* 01 */ DI_TEXT, 5, 2, 0, 0, 0, 0, 0, 0, GetMsg (MNavRectLeft), nullptr },
					{ /* 02 */ DI_FIXEDIT, 5, 3, 7, 0, 0, 0, 0, 0, navRectLeft, nullptr },
					{ /* 03 */ DI_TEXT, 5, 4, 0, 0, 0, 0, 0, 0, GetMsg (MNavRectTop), nullptr },
					{ /* 04 */ DI_FIXEDIT, 5, 5, 7, 0, 0, 0, 0, 0, navRectTop, nullptr },
					{ /* 05 */ DI_TEXT, 5, 6, 0, 0, 0, 0, 0, 0, GetMsg (MNavRectRight), nullptr },
					{ /* 06 */ DI_FIXEDIT, 5, 7, 7, 0, 0, 0, 0, 0, navRectRight, nullptr },
					{ /* 07 */ DI_TEXT, 5, 8, 0, 0, 0, 0, 0, 0, GetMsg (MNavRectBottom), nullptr },
					{ /* 08 */ DI_FIXEDIT, 5, 9, 7, 0, 0, 0, 0, 0, navRectBottom, nullptr },
					{ /* 09 */ DI_TEXT, 0, 10, 0, 0, 0, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, L"", nullptr },
					{ /* 10 */ DI_BUTTON, 0, 11, 0, 0, 0, 0, 1, DIF_CENTERGROUP, GetMsg (MOK), nullptr },
					{ /* 11 */ DI_BUTTON, 0, 11, 0, 0, 0, 0, 0, DIF_CENTERGROUP, GetMsg (MCancel), nullptr }
				};
				struct FarDialogItem		DialogItems[_countof (InitItems)];
				InitDialogItemsEx (InitItems, DialogItems, _countof (InitItems));

				HANDLE hDlg2 = Info.DialogInit(&MainGuid, &ConfigGuidEx, -1, -1, 59, 14, nullptr, DialogItems, _countof (InitItems), 0, 0, Info.DefDlgProc, 0);
				if (hDlg2 != INVALID_HANDLE_VALUE)
				{
					intptr_t n = Info.DialogRun(hDlg2);
					if (n == 10)
					{
						FarDialogItemDataEx item;
						item.PtrLength = 32;
						item.PtrData = navRectLeft;
						Info.SendDlgMessage(hDlg2,DM_GETTEXT,2,(void*)&item);
						settings.Set(0,L"NavigationRectLeft", item.PtrData);
						item.PtrData = navRectTop;
						Info.SendDlgMessage(hDlg2,DM_GETTEXT,4,(void*)&item);
						settings.Set(0,L"NavigationRectTop", item.PtrData);
						item.PtrData = navRectRight;
						Info.SendDlgMessage(hDlg2,DM_GETTEXT,6,(void*)&item);
						settings.Set(0,L"NavigationRectRight", item.PtrData);
						item.PtrData = navRectBottom;
						Info.SendDlgMessage(hDlg2,DM_GETTEXT,8,(void*)&item);
						settings.Set(0,L"NavigationRectBottom", item.PtrData);
					}
					Info.DialogFree(hDlg2);
				}
				Info.DialogFree(hDlg1);
				continue;
			}
			if (n == OK)
			{
				settings.Set(0,L"AutoCompile", (autocompile = (Info.SendDlgMessage(hDlg1,DM_GETCHECK,1,nullptr) == BSTATE_CHECKED) ? 1 : 0));
				settings.Set(0,L"AutoFormat", (autoformat = (Info.SendDlgMessage(hDlg1,DM_GETCHECK,2,nullptr) == BSTATE_CHECKED) ? 1 : 0));
				settings.Set(0,L"Disable", (pluginStop = (Info.SendDlgMessage(hDlg1,DM_GETCHECK,3,nullptr) == BSTATE_CHECKED) ? 1 : 0));
				settings.Set(0,L"UseScrollLock", (scrollStop = (Info.SendDlgMessage(hDlg1,DM_GETCHECK,4,nullptr) == BSTATE_CHECKED) ? 1 : 0));
				settings.Set(0,L"IgnorePosition", (ignoreposn = (Info.SendDlgMessage(hDlg1,DM_GETCHECK,5,nullptr) == BSTATE_CHECKED) ? 1 : 0));
				settings.Set(0,L"OutputMenu", (outputmenu = (Info.SendDlgMessage(hDlg1,DM_GETCHECK,6,nullptr) == BSTATE_CHECKED) ? 1 : 0));
				settings.Set(0,L"OutputFilter", (filterring = (Info.SendDlgMessage(hDlg1,DM_GETCHECK,7,nullptr) == BSTATE_CHECKED) ? 1 : 0));
				settings.Set(0,L"ConfigFilename", (confFilename = GetDialogItemText(hDlg1,9)));
				settings.Set(0,L"Key", (defExpandFKey = GetDialogItemText(hDlg1,12)));

				DoneMacro ();
				InitMacro ();
				Info.DialogFree(hDlg1);
				return (true);
			}
			else
			{
				Info.DialogFree(hDlg1);
				break;
			}
			Info.DialogFree(hDlg1);
		}
	}

	return (false);
}
