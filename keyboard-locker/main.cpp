#include <iostream>
#include <Windows.h>
#include "keyboardhook.h"

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

		SetPassKey(passKey, length);
		fclose(fp);
	}

	SetKeyboardHook();
	SetMouseHook();

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookKeyboardHook();
	UnhookMouseHook();

#ifdef NDEBUG
	if (hwnd)
	{
		ShowWindow(hwnd, SW_SHOW);
	}
#endif // NDEBUG

	system("pause");
}

