#include "pch.h"
#include <Windows.h>
#include <iostream>

HINSTANCE g_Instance = NULL;

HHOOK g_KeyboardHook = NULL;
HHOOK g_MouseHook = NULL;

DWORD g_PassKey[PASSKEY_MAX] = { 38, 38, 40, 40, 37, 39, 37, 39, 66, 65 };
int g_PassKeyLength = 10;

std::string getTime()
{
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	char time[1024] = { 0 };
	sprintf_s(time, "[%4d/%02d/%02d %02d:%02d:%02d]", sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute, sys.wSecond);

	return std::string(time);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	/*
	typedef struct tagKBDLLHOOKSTRUCT {
	DWORD     vkCode;		// ��������
	DWORD     scanCode;		// Ӳ��ɨ����ţ�ͬ vkCode Ҳ������Ϊ�����Ĵ��š�
	DWORD     flags;		// �¼����ͣ�һ�㰴������Ϊ 0 ̧��Ϊ 128��
	DWORD     time;			// ��Ϣʱ���
	ULONG_PTR dwExtraInfo;	// ��Ϣ������Ϣ��һ��Ϊ 0��
	}KBDLLHOOKSTRUCT,*LPKBDLLHOOKSTRUCT,*PKBDLLHOOKSTRUCT;
	*/
	KBDLLHOOKSTRUCT* ks = (KBDLLHOOKSTRUCT*)lParam; // �����ͼ����������¼���Ϣ
	DWORD code = ks->vkCode;

	static DWORD keyCodes[PASSKEY_MAX] = { 0x00 };
	static int index;
	static bool unlockMode = FALSE;

	std::string time = getTime();
	char info[1024] = { '\0' };

	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
	{
#if _DEBUG
		// ��ֹgg
		if (code == VK_ESCAPE || code == VK_DELETE)
		{
			PostQuitMessage(0);
		}
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
					PostQuitMessage(0);
				}
				unlockMode = FALSE;
			}
		}
	}
	else if(!(wParam == WM_KEYUP || wParam == WM_SYSKEYUP))
	{
		sprintf_s(info, "%s KEYBOARD:Unknown:%d\n", time.c_str(), code);
	}

	std::cout << info;

	return TRUE; // �Ե���Ϣ
	//return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	/*
	typedef struct tagMOUSEHOOKSTRUCT {
		POINT   pt;					// Point����
		HWND    hwnd;				// ���������Ϣ�Ĵ���ľ��
		UINT    wHitTestCode;		// ָ���������ֵ
		ULONG_PTR dwExtraInfo;		// ָ���͸���Ϣ������ĸ�����Ϣ��
	} MOUSEHOOKSTRUCT, FAR* LPMOUSEHOOKSTRUCT, * PMOUSEHOOKSTRUCT;
	*/

	MOUSEHOOKSTRUCT* ms = (MOUSEHOOKSTRUCT*)lParam;
	POINT pt = ms->pt;

	std::string time = getTime();
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
	std::cout << info;

	return TRUE; // �Ե���Ϣ
	//return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void SetKeyboardHook()
{
	g_KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, g_Instance, NULL);
}

void SetMouseHook()
{
	g_MouseHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)MouseProc, g_Instance, 0);
}

void UnhookKeyboardHook()
{
	UnhookWindowsHookEx(g_KeyboardHook);
	g_KeyboardHook = NULL;
}

void UnhookMouseHook()
{
	UnhookWindowsHookEx(g_MouseHook);
	g_MouseHook = NULL;
}

void SetPassKey(const DWORD* arrPassKey, int length)
{
	if (length > 0)
	{
		for (int i = 0; i < length; i++)
		{
			g_PassKey[i] = arrPassKey[i];
		}
		g_PassKeyLength = length;
	}
}
