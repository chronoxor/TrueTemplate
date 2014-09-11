#ifdef DEBUG
static void SysLog(wchar_t *fmt,...)
{
  const wchar_t *Log = L"\\TRUE-TPL.LOG.TMP";
  wchar_t temp[4096];
  va_list argptr;
  va_start(argptr, fmt);
  wvsprintf(temp, fmt, argptr);
  va_end(argptr);
  HANDLE f = CreateFile(Log, GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if ( f != INVALID_HANDLE_VALUE )
  {
	  DWORD dwBytesRead = wcslen(temp), dwBytesWritten = 0;
    DWORD dwPos = SetFilePointer(f, 0, nullptr, FILE_END);
    LockFile(f, dwPos, 0, dwPos+dwBytesRead, 0);
    WriteFile(f, temp, dwBytesRead, &dwBytesWritten, nullptr);
    UnlockFile(f, dwPos, 0, dwPos+dwBytesRead, 0);
  }
  CloseHandle(f);
}
#else
#define SysLog //
#endif
