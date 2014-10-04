////////////////////////////////////////////////////////////////
// If this code works, it was written by Alexander Nazarenko.
// If not, I don't know who wrote it.

#include "winmem.h"
#include "console.h"
#include "true-tpl.h"

#define BUFF_SIZE 8192

static wchar_t LineBuf[BUFF_SIZE];
static int LineBufPtr;

static void cls(HANDLE& StdOutput)
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(StdOutput, &csbi);
  DWORD dummy;
  COORD C;
  C.X = 0;
  for ( int Y = 0 ; Y < csbi.dwSize.Y ; Y++ )
  {
    C.Y = (short)Y;
    FillConsoleOutputCharacter(StdOutput, L' ', csbi.dwSize.X, C, &dummy);
    FillConsoleOutputAttribute(StdOutput, 7, csbi.dwSize.X, C, &dummy);
  }
  C.Y = (short)(csbi.dwSize.Y-1);
  SetConsoleCursorPosition(StdOutput, C);
}

static void OutputLine(HANDLE& h, TCollection<TOutputLine> *AppOutput, bool showExec)
{
	const wchar_t crlf[] = L"\r\n";
	wchar_t *eos = wcspbrk(LineBuf, crlf);
	LineBuf[LineBufPtr] = 0;
	if (eos)
		*eos = 0;
	TOutputLine *outLine = new TOutputLine(LineBuf);
	AppOutput->insert(outLine);
	LineBufPtr = 0;
	DWORD dummy;
	if (showExec)
	{
		WriteConsole(h, outLine->line, (DWORD)outLine->line.length(), &dummy, nullptr);
		WriteConsole(h, crlf, (DWORD)wcslen(crlf), &dummy, nullptr);
	}
}

struct TShowOutputData
{
  bool compilerDone, cls, showExec;
  HANDLE hInput, hOutput;
  TCollection<TOutputLine> *AppOutput;
};

void showPartOfCompilerOut(TShowOutputData *sd)
{
  if ( sd && ( sd->hInput != INVALID_HANDLE_VALUE ) )
  {
    UCHAR ReadBuf[BUFF_SIZE];
    DWORD BytesRead = 0;
    while ( ReadFile(sd->hInput, ReadBuf, sizeof(ReadBuf), &BytesRead, nullptr) )
    {
      for ( int i = 0 ; i < (int)BytesRead ; i++ )
      {
        if ( strchr("\n\r", ReadBuf[i]) )
        {
          if ( ( ReadBuf[i] == '\r' ) && ( ReadBuf[i+1] == '\n' ) )
            i++;
          OutputLine(sd->hOutput, sd->AppOutput, sd->showExec);
        }
        else
        {
          LineBuf[LineBufPtr] = ReadBuf[i];
          LineBufPtr++;
          if ( LineBufPtr >= _countof(LineBuf)-1 )
            OutputLine(sd->hOutput, sd->AppOutput, sd->showExec);
        }
      }
    }
  }
}

DWORD WINAPI ThreadWhatUpdateScreen(LPVOID par)
{
  if ( par )
  {
    TShowOutputData *sd = (TShowOutputData*)par;
    if ( sd->cls )
      cls(sd->hOutput);
    COMMTIMEOUTS timeOut = { 1000, 0, 0, 0, 0 };
    SetCommTimeouts(sd->hInput, &timeOut);
    for ( ; ; )
    {
      if ( sd->compilerDone )
        break;
      showPartOfCompilerOut(sd);
      Sleep(500);
    }
    showPartOfCompilerOut(sd);
  }
  return 0;
}

static inline const wchar_t *getExt(const wchar_t *FileName)
{
	wchar_t *ext = wcsrchr((wchar_t*)FileName, L'.');
	return (const wchar_t *)(ext ? ext : wcsrchr((wchar_t*)FileName, 0));
}

enum CmdType { none, com, exe, bat, cmd, gui };

static bool ExeExist(const wchar_t *FilePath, const wchar_t *FileName, const wchar_t *ext, const wchar_t *testExt, wchar_t *FullName, size_t sizeofFullName)
{
  if ( !*ext || !_wcsicmp(ext, testExt) )
  {
    wchar_t *FilePart;
	wchar_t fullpath[NM];
	wcscpy(fullpath, FilePath);
	wcscat(fullpath, L"\\");
	wcscat(fullpath, FileName);
    if ( SearchPath(nullptr, FileName, testExt, (DWORD)sizeofFullName, FullName, &FilePart) || SearchPath(nullptr, fullpath, testExt, (DWORD)sizeofFullName, FullName, &FilePart))
      return true;
  }
  return false;
}

static CmdType CommandType(bool NT, const wchar_t *Command, const wchar_t *Path)
{
  wchar_t FileName[4096], FullName[4096], *EndName;
  if ( *Command == L'\"' )
  {
	  wcscpy(FileName, Command+1);
	if ( ( EndName = wcschr(FileName,L'\"') ) != nullptr )
      *EndName = 0;
  }
  else
  {
    wcscpy(FileName, Command);
	if ( ( EndName = wcspbrk(FileName, L" \t/")) != nullptr)
      *EndName = 0;
  }
  const wchar_t *ext = getExt(FileName);
  if ( ExeExist(Path, FileName, ext, L".com", FullName, _countof(FullName)) )
    return com;
  if ( ExeExist(Path, FileName, ext, L".exe", FullName, _countof(FullName)) )
  {
    SHFILEINFO sfi;
    DWORD_PTR ExeType = SHGetFileInfo(FullName, 0, &sfi, sizeof(sfi), SHGFI_EXETYPE);
    bool GUIType = HIWORD(ExeType) >= 0x0300 && HIWORD(ExeType) <= 0x1000 && HIBYTE(ExeType) == 'E' && ( LOBYTE(ExeType) == 'N' || LOBYTE(ExeType) == 'P' );
    return GUIType ? gui : exe;
  }
  if ( ExeExist(Path, FileName, ext, L".bat", FullName, _countof(FullName)) )
    return bat;
  if ( NT && ExeExist(Path, FileName, ext, L".cmd", FullName, _countof(FullName)) )
    return cmd;
  return none;
}

DWORD ExecConsoleApp(const wchar_t *CmdStr, const wchar_t *path, TCollection<TOutputLine> *OutputColl, bool clear, bool showExec)
{
  if ( !OutputColl )
    return -1;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  SECURITY_ATTRIBUTES sa;
  OSVERSIONINFO WinVer;
  wchar_t ExecLine[1024], CommandName[1024];
  WinVer.dwOSVersionInfoSize = sizeof(WinVer);
  GetVersionEx(&WinVer);
  bool NT = WinVer.dwPlatformId == VER_PLATFORM_WIN32_NT;
  bool OldNT = NT && WinVer.dwMajorVersion < 4;
  *CommandName=0;
  GetEnvironmentVariable(L"COMSPEC", CommandName, _countof(CommandName));
  CmdType GUIType = CommandType(NT, CmdStr, path);
  wcscat(wcscpy(ExecLine, CommandName), L" /C");
  if ( !OldNT && ( ( GUIType == gui ) || ( GUIType == none ) ) )
  {
	  wcscat(ExecLine,L" start");
    if ( GUIType == gui )
      wcscat(ExecLine,L" /wait");
    if ( NT && *CmdStr == '\"' )
      wcscat(ExecLine,L" \"\"");
  }
  wcscat(wcscat(ExecLine,L" "), CmdStr);
  HANDLE WriteHandle = INVALID_HANDLE_VALUE, ReadHandle = INVALID_HANDLE_VALUE;
  HANDLE StdInput = GetStdHandle(STD_INPUT_HANDLE);
  DWORD ConsoleMode, ExitCode = -1;
  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  memset(&sa, 0, sizeof(sa));
  LineBufPtr = 0;
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = true;
  if ( CreatePipe(&ReadHandle, &WriteHandle, &sa, 0) )
  {
    if ( NT )
      SetHandleInformation(ReadHandle, HANDLE_FLAG_INHERIT, 0);
    else
    {
      HANDLE TempHandle;
      DuplicateHandle(GetCurrentProcess(), ReadHandle, GetCurrentProcess(), &TempHandle, 0, true, DUPLICATE_SAME_ACCESS);
      ReadHandle = TempHandle;
    }
    GetConsoleMode(StdInput,&ConsoleMode);
    SetConsoleMode(StdInput, ENABLE_PROCESSED_INPUT|ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_MOUSE_INPUT);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = StdInput;
    si.hStdOutput = WriteHandle;
    si.hStdError = WriteHandle;
    SetConsoleTitle(CmdStr);
    DWORD dummy;
    TShowOutputData *sd = (TShowOutputData*)GlobalAlloc(GPTR,sizeof(TShowOutputData));
    if ( sd )
    {
      sd->compilerDone = false;
      sd->cls = showExec && clear;
      sd->showExec = showExec;
      sd->hInput = ReadHandle;
      sd->hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
      sd->AppOutput = OutputColl;
      ExitCode = CreateProcess(nullptr, ExecLine, nullptr, nullptr, true, CREATE_NO_WINDOW, nullptr, path, &si, &pi);
      if ( ExitCode )
      {
        HANDLE hThread = CreateThread(nullptr, 0xf000, ThreadWhatUpdateScreen, sd, 0, &dummy);
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &ExitCode);
        CloseHandle(WriteHandle);
        sd->compilerDone = true;
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(hThread);
      }
      else
        ExitCode = -1;
      showPartOfCompilerOut(sd);
      OutputLine(sd->hOutput, OutputColl, showExec);
      GlobalFree((LPVOID)sd);
    }
    CloseHandle(ReadHandle);
  }
  return ExitCode;
}
