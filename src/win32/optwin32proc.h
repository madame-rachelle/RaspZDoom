#pragma once

#define WIN32_LEAN_AND_MEAN
#define WINVER 0x0500
#include <windows.h>

// Helper template so that we can access newer Win32 functions with a single static
// variable declaration. If this were C++11 it could be totally transparent.
template<typename Proto>
class TOptWin32Proc
{
	static Proto GetOptionalWin32Proc(const char* module, const char* function)
	{
		HMODULE hmodule = GetModuleHandle(module);
		if (hmodule == NULL)
			return NULL;

		return (Proto)GetProcAddress(hmodule, function);
	}

public:
	const Proto Call;

	TOptWin32Proc(const char* module, const char* function)
		: Call(GetOptionalWin32Proc(module, function)) {}

	// Wrapper object can be tested against NULL, but not directly called.
	operator const void*() const { return (const void*)Call; }
};
