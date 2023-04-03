#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <thread>
using namespace std;

// Winlogon
typedef LONG(NTAPI* _NtSuspendProcess)(IN HANDLE ProcessHandle);
typedef LONG(NTAPI* _NtResumeProcess)(IN HANDLE ProcessHandle);
HANDLE ProcessHandle = NULL;
HMODULE ntdll = NULL;

// Keyboard Hook
#define PASSKEY_FILE "./data/passkey"
#define PASSKEY_MAX 16

HHOOK g_KeyboardHook = NULL;
HHOOK g_MouseHook = NULL;
DWORD g_PassKey[PASSKEY_MAX] = { 38, 38, 40, 40, 37, 39, 37, 39, 66, 65 };
int g_PassKeyLength = 10;


BOOL EnableDebugPriv()
{
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkp;

	//OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		return FALSE;
	}

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
	{
		return FALSE;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = luid;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, false, &tkp, sizeof(tkp), NULL, NULL))
	{
		CloseHandle(hToken);
		return FALSE;
	}
	else
	{
		return TRUE;
	}
	//CloseHandle(hToken);
}

DWORD FindProcessId(const wstring& processName)
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);
	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE)return 0;
	Process32First(processesSnapshot, &processInfo);
	while (Process32Next(processesSnapshot, &processInfo)) {
		if (!processName.compare(processInfo.szExeFile)) {
			CloseHandle(processesSnapshot);
			return processInfo.th32ProcessID;
		}
	}
	CloseHandle(processesSnapshot);
	return 0;
}


string GetTimeString()
{
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	char time[1024] = { 0 };
	sprintf_s(time, "[%4d/%02d/%02d %02d:%02d:%02d]", sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute, sys.wSecond);

	return string(time);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* ks = (KBDLLHOOKSTRUCT*)lParam;
	DWORD code = ks->vkCode;

	static DWORD keyCodes[PASSKEY_MAX] = { 0x00 };
	static int index;
	static bool unlockMode = FALSE;

	string time = GetTimeString();
	char info[1024] = { '\0' };

	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
	{
#if _DEBUG
		/*
		// 防止gg，按 Esc 或 Delet 结束程序
		if (code == VK_ESCAPE || code == VK_DELETE)
		{
			PostQuitMessage(0);
		}
		*/
#endif // _DEBUG

		sprintf_s(info, "%s KEYBOARD:WM_KEYDOWN:%d\n", time.c_str(), code);

		if (code == VK_SPACE)
		{
			unlockMode = TRUE;
			index = 0;
		}
		else if (unlockMode)
		{
			keyCodes[index++] = code;
			sprintf_s(info, "%s KEYBOARD:WM_KEYDOWN:%d [Unlocking(%d/%d)]\n", time.c_str(), code, index, g_PassKeyLength);
			if (index == g_PassKeyLength)
			{
				bool unlocked = FALSE;
				for (int i = 0; i < g_PassKeyLength; i++)
				{
					if (keyCodes[i] != g_PassKey[i])
					{
						break;
					}
					unlocked = TRUE;
				}

				if (unlocked)
				{
					((_NtResumeProcess)GetProcAddress(ntdll, "NtResumeProcess"))(ProcessHandle);
					PostQuitMessage(0);
				}
				unlockMode = FALSE;
			}
		}
	}
	else if (!(wParam == WM_KEYUP || wParam == WM_SYSKEYUP))
	{
		sprintf_s(info, "%s KEYBOARD:Unknown:%d\n", time.c_str(), code);
	}

	cout << info;

	return TRUE; // 吃掉消息
	//return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MOUSEHOOKSTRUCT* ms = (MOUSEHOOKSTRUCT*)lParam;
	POINT pt = ms->pt;

	string time = GetTimeString();
	char info[1024] = { '\0' };
	char state[20] = "Unknown";

	switch (wParam)
	{
	case WM_MOUSEMOVE: strcpy_s(state, "WM_MOUSEMOVE"); break;
	case WM_LBUTTONDOWN: strcpy_s(state, "WM_LBUTTONDOWN"); break;
	case WM_LBUTTONUP: strcpy_s(state, "WM_LBUTTONUP"); break;
	case WM_RBUTTONDOWN: strcpy_s(state, "WM_RBUTTONDOWN"); break;
	case WM_RBUTTONUP: strcpy_s(state, "WM_RBUTTONUP"); break;
	}

	sprintf_s(info, "%s MOUSE:%s (X:%d Y:%d)\n", time.c_str(), state, pt.x, pt.y);
	cout << info;

	return TRUE; // 吃掉消息
	//return CallNextHookEx(NULL, nCode, wParam, lParam);
}


int main()
{
	HWND hwnd;
	char consoleTitle[1024];

	GetConsoleTitleA(consoleTitle, 1024);
	hwnd = FindWindowA("ConsoleWindowClass", consoleTitle);
	SetConsoleTitleA("Keyboard Locker");

#ifdef NDEBUG
	if (hwnd)
	{
		ShowWindow(hwnd, SW_HIDE);
	}
#endif // NDEBUG

	FILE* fp;
	errno_t err = fopen_s(&fp, PASSKEY_FILE, "r");

	if (err == 0)
	{
		DWORD passKey[PASSKEY_MAX] = { 0 };
		int length = 0;

		for (int i = 0; i < PASSKEY_MAX; i++)
		{
			DWORD keyCode;
			if (fscanf_s(fp, "%lu,", &keyCode) == EOF)
			{
				break;
			}
			passKey[length++] = keyCode;
		}

		// Set PassKey
		if (length > 0)
		{
			for (int i = 0; i < length; i++)
			{
				g_PassKey[i] = passKey[i];
			}
			g_PassKeyLength = length;
		}

		fclose(fp);
	}

	EnableDebugPriv();

	HINSTANCE hK = GetModuleHandle(NULL);
	HINSTANCE hM = GetModuleHandle(NULL);
	g_KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, hK, NULL);
	g_MouseHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)MouseProc, hM, 0);

	ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, FindProcessId(L"winlogon.exe"));
	ntdll = GetModuleHandle(L"ntdll");
	if (ntdll)
	{
		((_NtSuspendProcess)GetProcAddress(ntdll, "NtSuspendProcess"))(ProcessHandle);
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(g_KeyboardHook);
	UnhookWindowsHookEx(g_MouseHook);

#ifdef NDEBUG
	if (hwnd)
	{
		ShowWindow(hwnd, SW_SHOW);
	}
#endif // NDEBUG

	system("pause");
}

