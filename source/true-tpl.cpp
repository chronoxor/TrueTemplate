//If this code works, it was written by Alexander Nazarenko. If not, I don't know who wrote it. ;
//Сделано по мотивам True-Cpp plugin. (c) Andrey Tretjakov,1999 v.1.0
#include "winmem.h"

#define _FAR_USE_FARFINDDATA

#include <plugin.hpp>
#include <PluginSettings.hpp>

#include "guid.h"
#include "mystring.h"
#include "eicoll.h"
#include "true-tpl.h"
#include "ttpl-lng.h"
#include "farintf.h"
#include "console.h"
#include "stdio.h"
#include "syslog.h"
#include "objbase.h"
#include "process.h"

static HINSTANCE	hInst;
bool							IsOldFar = true;
wchar_t						szIni[_MAX_PATH]; //Plugin INI filename
const wchar_t			cMD = L'@';				//Macro delimiter
const wchar_t			cBOM = 0xFEFF;		//Byte-order mark
const wchar_t			DefaultConfFilename[] = L"true-tpl.xml";

#define expAnyWhere			L".*\\b\\p.*"
#define expAtStart			L"\\p.*"
#define expAtMiddle			L"\\S.*\\b\\p.*\\S"
#define expAtEnd			L".*\\b\\p"
#define expOnBlank			L"\\p"

#define TEMP_TT				L"%TEMP%"
#define TEMP_TT_TMP			L"%TEMP%\\true-template.tmp"
#define TEMP_TT_SRC_TMP		L"%TEMP%\\true-template-src.tmp"
#define TEMP_TT_DST_TMP		L"%TEMP%\\true-template-dst.tmp"

#define MAX_REG_LEN			512
#define MAX_STR_LEN			8192

static bool				pluginBusy;
static bool				pluginStop;
static bool				scrollStop;
static bool				ignoreposn;
static bool				outputmenu;
static bool				filterring;
static bool				autocompile;
static bool				autoformat;
static String			confFilename;
static String			defExpandFKey;

static String			confDirectory;

#include "files.cpp"
#include "editor.cpp"
#include "str.cpp"
#include "defs.cpp"
#include "userinput.cpp"
#include "exec.cpp"
#include "lang.cpp"
#include "compiler.cpp"
#include "format.cpp"
#include "navy.cpp"
#include "reload.cpp"
#include "parse.cpp"
#include "macro.cpp"
#include "commands.cpp"
#include "comments.cpp"
#include "editorinput.cpp"
#include "config.cpp"

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	Info->StructSize = sizeof(struct PluginInfo);
	Info->MinFarVersion=MAKEFARVERSION(3,0,0,4040,VS_RELEASE);
	Info->Version=MAKEFARVERSION(3,0,1,11,VS_RC);
	Info->Guid=MainGuid;
	Info->Title=L"True Template";
	Info->Description=L"True Template Editor Plugin";
	Info->Author=L"Ivan Shynkarenka";
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	::Info = *Info;
	IsOldFar = true;
	if (Info->StructSize >= sizeof(PluginStartupInfo))
	{
		::Info=*Info;
		FSF=*Info->FSF;
		::Info.FSF=&FSF;
		IsOldFar = false;
		::Info.RegExpControl(nullptr, RECTL_CREATE, 0, &RegExpHandle);

		PluginSettings settings(MainGuid, ::Info.SettingsControl);
		autocompile = settings.Get(0, L"AutoCompile", true);
		autoformat = settings.Get(0, L"AutoFormat", true);
		pluginStop = settings.Get(0, L"Disable", false);
		scrollStop = settings.Get(0, L"UseScrollLock", true);
		ignoreposn = settings.Get(0, L"IgnorePosition", true);
		outputmenu = settings.Get(0, L"OutputMenu", true);
		filterring = settings.Get(0, L"OutputFilter", false);
		confFilename = String(settings.Get(0, L"ConfigFilename", DefaultConfFilename));
		if (confFilename.empty())
			confFilename = DefaultConfFilename;
		defExpandFKey = String(settings.Get(0, L"Key", L"Space"));

		InitMacro();
	}
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	if (!IsOldFar)
	{
		static const wchar_t *PluginMenuStrings[1];
		Info->StructSize = sizeof(struct PluginInfo);
		Info->Flags = PF_EDITOR | PF_VIEWER | PF_FULLCMDLINE;
		Info->DiskMenu.Count = 0;
		PluginMenuStrings[0] = GetMsg (MTitle);
		Info->PluginMenu.Guids=&MainGuid;
		Info->PluginMenu.Count = 1;
		Info->PluginMenu.Strings = PluginMenuStrings;

		static const wchar_t *PluginCfgStrings[1];
		PluginCfgStrings[0] = GetMsg (MTitle);
		Info->PluginConfig.Guids=&MainGuid;
		Info->PluginConfig.Count = 1;
		Info->PluginConfig.Strings = PluginCfgStrings;

		static const wchar_t *PluginPrefix = GetMsg (MPrefix);
		Info->CommandPrefix = PluginPrefix;
	}
}

intptr_t WINAPI ConfigureW(const struct ConfigureInfo *Info)
{
	return (Config ());
}

intptr_t WINAPI ProcessEditorEventW(const struct ProcessEditorEventInfo *Info)
{
	wchar_t				filename[NM];
	EditorInfoEx	ei;
	TEInfo				*te;

	initEList ();
	switch (Info->Event)
	{
	case EE_READ:
		::Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
		::Info.EditorControl (ei.EditorID, ECTL_GETFILENAME, NM, filename);
		te = (*eList) [eListInsert (ei.EditorID, filename)];
		if (te->newFile)
		{
			te->newFile = false;
			intptr_t	lngid = te->lang;
			if (lngid != -1)
			{
				const TLang	*lng = langColl[lngid];
				if (lng)
					InsertTemplate (ei.EditorID, lng);
			}
		}
		return (0);

	case EE_CLOSE:
		eList->removeID (Info->EditorID);
		return (0);

	case EE_REDRAW:
		if (reloadNeeded)
		{
			reloadInProcess = true;
			DoneMacro ();
			InitMacro ();
		}
		return (0);

	default:
		return (0);
	}
}

static void SelectTemplate (TEInfo *te)
{
	if (te)
	{
		const TLang *lng = langColl[te->lang];
		if (lng)
		{
			size_t	count = 0;
			for (size_t i = 0; i < lng->macroColl.getCount(); i++)
			{
				const TMacro	*mm = lng->macroColl[i];
				if (!mm->Name.empty() && !mm->submenu) count++;
			}

			if (count > 0)
			{
				FarMenuItemEx *amenu = new FarMenuItemEx[count];
				if (amenu)
				{
					count = 0;
					for (size_t i = 0; i < lng->macroColl.getCount(); i++)
					{
						const TMacro	*mm = lng->macroColl[i];
						if (!mm->Name.empty() && !mm->submenu)
						{
							amenu[count].Text = mm->Name;
							amenu[count].Flags &= ~(MIF_SELECTED | MIF_CHECKED | MIF_SEPARATOR);
							count++;
						}
					}

					intptr_t res = Info.Menu
						(
							&MainGuid,
							&SelectTemplateGuid,
							-1,
							-1,
							0,
							FMENU_WRAPMODE,
							GetMsg (MSelectMacro),
							nullptr,
							nullptr,
							nullptr,
							nullptr,
							amenu,
							count
						);
					if (res != -1)
					{
						count = 0;
						for (size_t i = 0; i < lng->macroColl.getCount(); i++)
						{
							const TMacro	*mm = lng->macroColl[i];
							if (!mm->Name.empty() && !mm->submenu)
							{
								if (count == res)
								{
									const TMacro	*fm = lng->macroColl[i];
									wchar_t				line[MAX_STR_LEN];
									EditorInfoEx	ei;
									Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);

									EditorGetStringEx gs;
									TEditorPos			epos = EditorGetPos ();
									EditorGetStr (&gs);
									wcsncpy (line, gs.StringText, gs.StringLength + 1);
									if (!inComment (lng, line, epos.Col))
									{
										wchar_t	before[MAX_STR_LEN], *after = line + epos.Col;
										wcscpy (before, line);
										before[epos.Col] = 0;
										if (ignoreposn)
										{
											if (checkMultipleChoice (lng, fm, L"", L"", 0, nullptr))
											{
												RunMacro (fm, nullptr, nullptr);
												redraw ();
											}
										}
										else if (CheckMacroPos (lng, fm, before, after))
											if (checkMultipleChoice (lng, fm, before, after, 0, nullptr))
											{
												RunMacro (fm, nullptr, nullptr);
												redraw ();
											}
									}
									break;
								}

								count++;
							}
						}
					}

					delete[] amenu;
				}
			}
		}
	}
}

static void SelectTemplateSet (TEInfo *te)
{
	size_t		lc = langColl.getCount ();
	FarMenuItemEx *amenu = new FarMenuItemEx[lc + 2];
	if (amenu)
	{
		for (size_t i = 0; i < lc; i++)
		{
			amenu[i].Text = langColl[i]->desc;
			amenu[i].Flags &= ~(MIF_CHECKED | MIF_SEPARATOR);
			amenu[i].Flags |= ((size_t) (te->lang) == i) ? MIF_SELECTED : 0;
		}

		amenu[lc].Flags &= ~(MIF_SELECTED | MIF_CHECKED);
		amenu[lc].Flags |= MIF_SEPARATOR;
		amenu[lc].Text = L"";
		amenu[lc + 1].Text = GetMsg (MOtherLang);
		amenu[lc + 1].Flags &= ~(MIF_CHECKED | MIF_SEPARATOR);
		amenu[lc + 1].Flags |= (te->lang == -1) ? MIF_SELECTED : 0;

		intptr_t res = Info.Menu
			(
				&MainGuid,
				&SelectTemplateSetGuid,
				-1,
				-1,
				0,
				FMENU_WRAPMODE | FMENU_AUTOHIGHLIGHT,
				GetMsg (MSelectLang),
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				amenu,
				lc + 2
			);
		if (res != -1)
		{
			te->lang = ((size_t) res < lc) ? res : -1;
		}

		delete[] amenu;
	}
}

HANDLE WINAPI OpenW(const struct OpenInfo *Info)
{
	if (!IsOldFar)
	{
		ptrdiff_t 		n;
		EditorInfoEx	ei;
		wchar_t filepath[NM];
		wchar_t filename[NM];
		switch (Info->OpenFrom)
		{
		case OPEN_VIEWER:
			{
				ViewerInfoEx vi;
				::Info.ViewerControl(-1, VCTL_GETINFO, 0, &vi);
				::Info.ViewerControl(-1, VCTL_GETFILENAME, NM, filename);
				wcscpy(filepath, filename);
				*Point2FileName (filepath) = 0;
				n = findLngID (filename);
				if (n != -1) CompilerMenu (nullptr, filename, filepath, n);
				break;
			}
		case OPEN_COMMANDLINE:
			{
				OpenCommandLineInfo* ocli = (OpenCommandLineInfo*)Info->Data;
				wchar_t tmp[NM];
				wcsncpy (tmp, (wchar_t *) ocli->CommandLine, NM);
				FSF.Unquote (tmp);
				wchar_t* filename = tmp + 4;
				wchar_t file[NM];
				wchar_t path[NM];
				wcscpy(path, filename);
				wchar_t* ch = wcsrchr(path, L'\\');
				if (ch == nullptr)
				{
					FarPanelDirectory* dirInfo = (FarPanelDirectory*)malloc(sizeof(FarPanelDirectory) + NM);
					if (dirInfo)
					{
						ZeroMemory(dirInfo, sizeof(FarPanelDirectory));
						dirInfo->StructSize = sizeof(FarPanelDirectory);
						::Info.PanelControl (PANEL_ACTIVE, FCTL_GETPANELDIRECTORY, sizeof(FarPanelDirectory) + NM, dirInfo);

						wcscpy (path, dirInfo->Name);
						wcscpy (file, dirInfo->Name);
						wcscat (file, L"\\");
						wcscat (file, filename);

						free(dirInfo);
					}
				}
				else
				{
					wcscpy (file, filename);
					*ch = L'\0';
				}
				if (tmp[2] == L'l')
				{
					n = findLngID (file);
					if (n != -1) CompilerMenu (nullptr, file, path, n);
				}

				if (tmp[2] == L'f')
				{
					n = findLngID (file);
					if (n != -1) FormatMenu (file, n);
				}
				break;
			}
		case OPEN_EDITOR:
			::Info.EditorControl (-1, ECTL_GETINFO, 0, &ei);
			::Info.EditorControl (ei.EditorID, ECTL_GETFILENAME, NM, filename);
			wcscpy(filepath, filename);
			*Point2FileName (filepath) = 0;
			initEList ();
			n = eList->findID (ei.EditorID);
			if (n == -1) n = eListInsert (ei.EditorID, filename);
			if (n != -1)
			{
				TEInfo	*te = (*eList)[n];
				if (te)
				{
					FarMenuItemEx mMenu[9];
					int						count = _countof (mMenu);
					for (int i = 0; i < count; i++) mMenu[i].Flags = 0;
					mMenu[0].Text = GetMsg (MMenuTemplates);
					mMenu[1].Text = GetMsg (MFormatting);
					mMenu[2].Text = GetMsg (MNavigation);
					mMenu[3].Text = GetMsg (MNavigationList);
					mMenu[4].Text = GetMsg (MComments);
					mMenu[5].Text = GetMsg (MMenuSelectLang);
					mMenu[6].Text = GetMsg (MMenuCompile);
					mMenu[8].Text = GetMsg (MShowOutput);
					mMenu[7].Flags |= MIF_SEPARATOR;
					if (!compilerOut) mMenu[8].Flags |= MIF_DISABLE;

					intptr_t mode = ::Info.Menu
						(
							&MainGuid,
							&OpenEditorGuid,
							-1,
							-1,
							0,
							FMENU_WRAPMODE,
							GetMsg (MTitle),
							nullptr,
							nullptr,
							nullptr,
							nullptr,
							(const FarMenuItemEx *) mMenu,
							count
						);
					switch (mode)
					{
					case 0:
						SelectTemplate (te);
						break;
					case 1:
						SelectFormatting (te);
						break;
					case 2:
						SelectNavigation (te, filepath);
						break;
					case 3:
						SelectNavigationList (te, filepath);
						break;
					case 4:
						BlockComments (te);
						break;
					case 5:
						SelectTemplateSet (te);
						break;
					case 6:
						SelectCompiler (te);
						break;
					case 8:
						ShowOutput (filepath);
						break;
					}
				}
			}
			break;
		case OPEN_PLUGINSMENU:
			FarMenuItemEx mMenu[3];

			int count = _countof (mMenu);
			mMenu[0].Text = GetMsg (MMenuCompile);
			mMenu[2].Text = GetMsg (MShowOutput);
			mMenu[1].Flags |= MIF_SEPARATOR;
			if (!compilerOut) mMenu[2].Flags |= MIF_DISABLE;

			intptr_t mode = ::Info.Menu
				(
					&MainGuid,
					&OpenMenuGuid,
					-1,
					-1,
					0,
					FMENU_WRAPMODE,
					GetMsg (MTitle),
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					(const FarMenuItemEx *) mMenu,
					count
				);
			switch (mode)
			{
			case 0:
			case 2:
				PanelInfo PInfo;
				if (::Info.PanelControl (PANEL_ACTIVE, FCTL_CHECKPANELSEXIST, 0, &PInfo) == TRUE)
				{
					::Info.PanelControl (PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &PInfo);
					if (PInfo.ItemsNumber > 0)
					{
						FarPanelDirectory* dirInfo = (FarPanelDirectory*)malloc(sizeof(FarPanelDirectory) + NM);
						if (dirInfo)
						{
							ZeroMemory(dirInfo, sizeof(FarPanelDirectory));
							dirInfo->StructSize = sizeof(FarPanelDirectory);
							::Info.PanelControl (PANEL_ACTIVE, FCTL_GETPANELDIRECTORY, sizeof(FarPanelDirectory) + NM, dirInfo);

							wchar_t	path[NM];
							wcscpy (path, dirInfo->Name);

							wchar_t	buf[NM];
							wcscpy (buf, dirInfo->Name);
							wcscat (buf, L"\\");

							free(dirInfo);

							if (mode == 0)
							{
								size_t Size=::Info.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, PInfo.CurrentItem, 0);
								PluginPanelItem *PPI=(PluginPanelItem*)malloc(Size);
								if (PPI)
								{
									FarGetPluginPanelItem FGPPI={sizeof(FarGetPluginPanelItem),Size,PPI};
									::Info.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, PInfo.CurrentItem, &FGPPI);
									wcscat (buf, PPI->FileName);
									free(PPI);
								}

								n = findLngID (buf);
								if (n != -1) CompilerMenu (nullptr, buf, path, n);
							}
							else if (mode == 2)
							{
								ShowOutput (path);
							}
						}
					}
				}
				break;
			}
			break;
		}
	}

	return (nullptr);
}

void WINAPI ExitFARW(const struct ExitInfo *Info)
{
	DoneMacro ();
	if (eList)
	{
		delete eList;
	}

	if (compilerOut)
	{
		delete[] compilerOut;
		compilerOut = nullptr;
	}

	if (errColl)
	{
		delete errColl;
	}

	::Info.RegExpControl(RegExpHandle, RECTL_FREE, 0, nullptr);
}

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		hInst = hinstDLL;
		GetModuleFileName (hInst, szIni, _MAX_PATH);
		wcscpy (Point2FileExt (szIni), L"ini");
	default:
		break;
	}

	return (TRUE);
}
