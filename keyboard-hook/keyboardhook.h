#pragma once

#ifdef KEYBOARDHOOK_EXPORTS
#define KEYBOARDHOOK_API __declspec(dllexport)
#else
#define KEYBOARDHOOK_API __declspec(dllimport)
#endif

#define PASSKEY_MAX 16
#define PASSKEY_FILE "./data/passkey"

extern HINSTANCE g_Instance;

KEYBOARDHOOK_API void SetKeyboardHook();
KEYBOARDHOOK_API void SetMouseHook();
KEYBOARDHOOK_API void UnhookKeyboardHook();
KEYBOARDHOOK_API void UnhookMouseHook();
KEYBOARDHOOK_API void SetPassKey(const DWORD* arrPassKey, int length);

