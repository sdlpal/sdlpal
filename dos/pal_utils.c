/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "main.h"

double fmax(double a, double b) {
	return (a > b) ? a : b;
}
double fmin(double a, double b) {
	return (a > b) ? b : a;
}

BOOL iswspace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}

wchar_t *wcscpy(wchar_t *dest, const wchar_t *src)
{
	wchar_t *d = dest;
	while ((*d++ = *src++) != L'\0');
	return dest;
}

wchar_t *wcsncpy(wchar_t *dest, const wchar_t *src, size_t n)
{
	wchar_t *d = dest;
	while (n-- && (*d++ = *src++) != L'\0');
	if (n + 1 > 0) {
		*d = L'\0'; // Null-terminate if there's space left
	}
	return dest;
}

int vswprintf(wchar_t *buffer, size_t count, const wchar_t *format, va_list argptr)
{
	int result = vsnprintf((char *)buffer, count * sizeof(wchar_t), (const char *)format, argptr);
	if (result < 0 || (size_t)result >= count) {
		return -1; // Error or buffer overflow
	}
	return result;
}

size_t wcslen(const wchar_t *s)
{
	const wchar_t *p = s;
	while (*p != L'\0') p++;
	return (size_t)(p - s);
}

wchar_t *wcschr(const wchar_t *s, wchar_t c)
{
	while (*s) {
		if (*s == c) return (wchar_t *)s; // Found the character
		s++;
	}
	return NULL; // Character not found
}

wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle)
{
	if (!*needle) return (wchar_t *)haystack; // If needle is empty, return haystack
	const wchar_t *p1 = haystack;
	const wchar_t *p2 = needle;
	while (*p1) {
		const wchar_t *p1Begin = p1;
		while (*p1 && *p2 && (*p1 == *p2)) {
			p1++;
			p2++;
		}
		if (!*p2) return (wchar_t *)p1Begin; // Found the needle
		p1 = p1Begin + 1; // Move to the next character in haystack
		p2 = needle; // Reset p2 to the start of needle
	}
	return NULL; // Needle not found
}

int wcstol(const wchar_t *nptr, wchar_t **endptr, int base)
{
	// Convert wide string to long integer
	char buffer[256];
	size_t len = wcslen(nptr);
	if (len >= sizeof(buffer)) {
		len = sizeof(buffer) - 1; // Prevent overflow
	}
	for (size_t i = 0; i < len; i++) {
		buffer[i] = (char)nptr[i];
	}
	buffer[len] = '\0'; // Null-terminate the buffer
	return strtol(buffer, endptr, base);
}

BOOL
UTIL_GetScreenSize(
	DWORD *pdwScreenWidth,
	DWORD *pdwScreenHeight
)
{
	return (pdwScreenWidth && pdwScreenHeight && *pdwScreenWidth && *pdwScreenHeight);
}

BOOL UTIL_IsAbsolutePath(LPCSTR  lpszFileName)
{
	return FALSE;
}

INT
UTIL_Platform_Init(
	int argc,
	char* argv[]
)
{
	gConfig.fLaunchSetting = FALSE;
	return 0;
}

VOID
UTIL_Platform_Quit(
	VOID
)
{
}
