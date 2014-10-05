static HANDLE hReloadCheck = 0, hEvent = 0;
bool					reloadNeeded = false;
bool					reloadInProcess = false;

DWORD WINAPI reloadCheck (LPVOID)
{
	HANDLE	hIdle[2];
	hIdle[0] = FindFirstChangeNotification (confDirectory, true, FILE_NOTIFY_CHANGE_LAST_WRITE);
	if (!hIdle[0] || hIdle[0] == INVALID_HANDLE_VALUE) return (0);
	hIdle[1] = hEvent;
	while (WaitForMultipleObjects (2, hIdle, false, INFINITE) == WAIT_OBJECT_0)
	{
		FindNextChangeNotification (hIdle[0]);
		if (reloadInProcess) continue;
		reloadNeeded = true;
	};
	FindCloseChangeNotification (hIdle[0]);
	return (0);
};

static void initThread (void)
{
	DWORD id;
	if (!hEvent) hEvent = CreateEvent (nullptr, false, false, nullptr);
	hReloadCheck = CreateThread (nullptr, 0, reloadCheck, 0, 0, &id);
}

static void doneThread (void)
{
	if (hReloadCheck)
	{
		SetEvent (hEvent);
		WaitForSingleObject (hReloadCheck, INFINITE);
		CloseHandle (hReloadCheck);
		hReloadCheck = 0;
	}

	if (hEvent)
	{
		CloseHandle (hEvent);
		hEvent = 0;
	}
}
