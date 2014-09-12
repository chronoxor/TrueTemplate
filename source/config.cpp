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

	InitDialogItem	InitItems[] =
	{
		{ DI_DOUBLEBOX, 3, 1, 65, 15, 0, 0, (wchar_t *) MTitle },
		{ /* 1 */ DI_CHECKBOX, 5, 2, 0, 0, autocompile, 0, (wchar_t *) MAutoCompile },
		{ /* 2 */ DI_CHECKBOX, 5, 3, 0, 0, autoformat, 0, (wchar_t *) MAutoFormat },
		{ /* 3 */ DI_CHECKBOX, 5, 4, 0, 0, pluginStop, 0, (wchar_t *) MDisable },
		{ /* 4 */ DI_CHECKBOX, 5, 5, 0, 0, scrollStop, 0, (wchar_t *) MPause },
		{ /* 5 */ DI_CHECKBOX, 5, 6, 0, 0, ignoreposn, 0, (wchar_t *) MIgnorePos },
		{ /* 6 */ DI_CHECKBOX, 5, 7, 0, 0, outputmenu, 0, (wchar_t *) MOutputMenu },
		{ /* 7 */ DI_CHECKBOX, 5, 8, 0, 0, filterring, 0, (wchar_t *) MFiltering },
		{ /* 8 */ DI_TEXT, 5, 9, 0, 255, 0, 0, nullptr },
		{ /* 9 */ DI_TEXT, 5, 10, 0, 0, 0, 0, (wchar_t *) MKey },
		{ /* 10 */ DI_EDIT, 25, 10, 63, 0, 0, 0, defExpandFKey },
		{ /* 11 */ DI_TEXT, 5, 11, 0, 255, 0, 0, nullptr },
		{ /* 12 */ DI_BUTTON, 5, 12, 63, 0, 0, DIF_CENTERGROUP, (wchar_t *) MNavConfig },
		{ /* 13 */ DI_TEXT, 5, 13, 0, 255, 0, 0, nullptr },
		{ /* 14 - OK */ DI_BUTTON, 0, 14, 0, 0, 0, DIF_CENTERGROUP, (wchar_t *) MOK },
		{ /* 15 */ DI_BUTTON, 0, 14, 0, 0, 0, DIF_CENTERGROUP, (wchar_t *) MCancel }
	};
	struct FarDialogItem		DialogItems[(sizeof (InitItems) / sizeof (InitItems[0]))];
	InitDialogItems (InitItems, DialogItems, (sizeof (InitItems) / sizeof (InitItems[0])));

	const intptr_t		NV = 12;
	const intptr_t		OK = 14;
	while (true)
	{
		HANDLE hDlg1 = Info.DialogInit(&MainGuid, &ConfigGuid, -1, -1, 69, OK + 3, nullptr, DialogItems, sizeof (InitItems) / sizeof (InitItems[0]), 0, 0, Info.DefDlgProc, 0);
		if (hDlg1 != INVALID_HANDLE_VALUE)
		{
			intptr_t n = Info.DialogRun(hDlg1);
			if (n == NV)
			{
				struct InitDialogItemEx InitItems[] =
				{ //Type X1 Y1 X2 Y2 Fo Se Fl DB Data
					{ /* 00 */ DI_DOUBLEBOX, 3, 1, 55, 12, 0, 0, DIF_BOXCOLOR, 0, GetMsg (MNavConfig), nullptr },
					{ /* 01 */ DI_TEXT, 5, 2, 0, 0, 0, 0, 0, 0, GetMsg (MNavRectLeft), nullptr },
					{ /* 02 */ DI_FIXEDIT, 5, 3, 7, 0, 0, 0, 0, 0, navRectLeft, nullptr },
					{ /* 03 */ DI_TEXT, 5, 4, 0, 0, 0, 0, 0, 0, GetMsg (MNavRectTop), nullptr },
					{ /* 04 */ DI_FIXEDIT, 5, 5, 7, 0, 0, 0, 0, 0, navRectTop, nullptr },
					{ /* 05 */ DI_TEXT, 5, 6, 0, 0, 0, 0, 0, 0, GetMsg (MNavRectRight), nullptr },
					{ /* 06 */ DI_FIXEDIT, 5, 7, 7, 0, 0, 0, 0, 0, navRectRight, nullptr },
					{ /* 07 */ DI_TEXT, 5, 8, 0, 0, 0, 0, 0, 0, GetMsg (MNavRectBottom), nullptr },
					{ /* 08 */ DI_FIXEDIT, 5, 9, 7, 0, 0, 0, 0, 0, navRectBottom, nullptr },
					{ /* 09 */ DI_TEXT, 0, 10, 0, 0, 0, 0, DIF_BOXCOLOR | DIF_SEPARATOR, 0, L"", nullptr },
					{ /* 10 */ DI_BUTTON, 0, 11, 0, 0, 0, 0, DIF_CENTERGROUP, 1, GetMsg (MOK), nullptr },
					{ /* 11 */ DI_BUTTON, 0, 11, 0, 0, 0, 0, DIF_CENTERGROUP, 0, GetMsg (MCancel), nullptr }
				};
				struct FarDialogItem		DialogItems[(sizeof (InitItems) / sizeof (InitItems[0]))];
				InitDialogItemsEx (InitItems, DialogItems, (sizeof (InitItems) / sizeof (InitItems[0])));

				HANDLE hDlg2 = Info.DialogInit(&MainGuid, &ConfigGuidEx, -1, -1, 59, 14, nullptr, DialogItems, sizeof (InitItems) / sizeof (InitItems[0]), 0, 0, Info.DefDlgProc, 0);
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
				FarDialogItemDataEx item;
				item.PtrLength = 256;
				item.PtrData = defExpandFKey;
				Info.SendDlgMessage(hDlg1,DM_GETTEXT,10,(void*)&item);
				settings.Set(0,L"Key", item.PtrData);
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
