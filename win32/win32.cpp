/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2018, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// win32.cpp: WIN32-specific codes.
//   @Author: Lou Yihua <louyihua@21cn.com>, 2016.
//

#define UNICODE
#define _UNICODE

#include <tchar.h>
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include "resource.h"
#include "../global.h"
#include "../util.h"
#include "../palcfg.h"
#include "../resampler.h"

#ifndef GCLP_HICON
# define GCLP_HICON (-14)
#endif

PAL_C_LINKAGE char* stoupper(char* s)
{
	char* p = strdup(s);
	char* p1 = p;
	while (*p = toupper(*p)) p++;
	return p1;
}

#define ComboBox_AddString(hwndDlg, idCtrl, lpsz) \
            (BOOL)SNDMSG(GetDlgItem((hwndDlg), (idCtrl)), CB_ADDSTRING, (WPARAM)(0), (LPARAM)(lpsz))
#define ComboBox_SetCurSel(hwndDlg, idCtrl, index) \
            (BOOL)SNDMSG(GetDlgItem((hwndDlg), (idCtrl)), CB_SETCURSEL, (WPARAM)(index), (LPARAM)(0))
#define ComboBox_GetCurSel(hwndDlg, idCtrl) \
            (int)SNDMSG(GetDlgItem((hwndDlg), (idCtrl)), CB_GETCURSEL, (WPARAM)(0), (LPARAM)(0))
#define TrackBar_SetRange(hwndDlg, idCtrl, min, max, redraw) \
            (BOOL)SNDMSG(GetDlgItem((hwndDlg), (idCtrl)), TBM_SETRANGE, (WPARAM)(redraw), (LPARAM)(MAKELONG((min), (max))))
#define TrackBar_SetPos(hwndDlg, idCtrl, pos, redraw) \
            (BOOL)SNDMSG(GetDlgItem((hwndDlg), (idCtrl)), TBM_SETPOS, (WPARAM)(redraw), (LPARAM)(pos))
#define TrackBar_GetPos(hwndDlg, idCtrl) \
            (int)SNDMSG(GetDlgItem((hwndDlg), (idCtrl)), TBM_GETPOS, (WPARAM)(0), (LPARAM)(0))
#define EnableDlgItem(hwnd, nIDControl, bEnable) \
			EnableWindow(GetDlgItem((hwnd), (nIDControl)), (bEnable))

HINSTANCE g_hInstance;
WORD g_wLanguage;

int WINAPI LoadStringEx(
	HINSTANCE hInstance,
	UINT      uID,
	LANGID    wLang,
	LPTSTR    lpBuffer,
	int       nBufferMax
)
{
	auto hrc = FindResourceEx(hInstance, RT_STRING, MAKEINTRESOURCE((uID >> 4) + 1), wLang);
	if (nullptr == hrc) return 0;

	auto begin = (LPCWSTR)LockResource(LoadResource(hInstance, hrc));
	for (int idx = 0; idx < (int)(uID & 0xf); idx++)
		begin += *begin + 1;
	if (nBufferMax == 0)
	{
		*((LPCWSTR*)lpBuffer) = begin;
		return sizeof(LPCWSTR);
	}
	else
	{
		wcsncpy(lpBuffer, begin + 1, min(nBufferMax, *begin));
		if (nBufferMax <= *begin)
		{
			lpBuffer[nBufferMax - 1] = '\0';
			return nBufferMax - 1;
		}
		else
		{
			lpBuffer[*begin] = '\0';
			return *begin;
		}
	}
}

std::wstring LoadResourceString(UINT uID)
{
	auto hrc = FindResourceEx(g_hInstance, RT_STRING, MAKEINTRESOURCE((uID >> 4) + 1), g_wLanguage);
	if (hrc)
	{
		auto begin = (LPCWSTR)LockResource(LoadResource(g_hInstance, hrc));
		for (int idx = 0; idx < (int)(uID & 0xf); idx++)
			begin += *begin + 1;
		return std::wstring(begin + 1, *begin);
	}
	return L"";
}

void SaveSettings(HWND hwndDlg, BOOL fWriteFile)
{
	int textLen;

	if (IsDlgButtonChecked(hwndDlg, IDC_USEMSGFILE) == BST_CHECKED && (textLen = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MSGFILE))) > 0)
	{
		gConfig.pszMsgFile = (char*)realloc(gConfig.pszMsgFile, textLen + 1);
		GetDlgItemTextA(hwndDlg, IDC_MSGFILE, gConfig.pszMsgFile, textLen + 1);
	}
	else
	{
		free(gConfig.pszMsgFile); gConfig.pszMsgFile = nullptr;
	}

	if (IsDlgButtonChecked(hwndDlg, IDC_USELOGFILE) == BST_CHECKED && (textLen = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_LOGFILE))) > 0)
	{
		gConfig.pszLogFile = (char*)realloc(gConfig.pszLogFile, textLen + 1);
		GetDlgItemTextA(hwndDlg, IDC_LOGFILE, gConfig.pszLogFile, textLen + 1);
	}
	else
	{
		free(gConfig.pszLogFile); gConfig.pszLogFile = nullptr;
	}

	if (IsDlgButtonChecked(hwndDlg, IDC_USEFONTFILE) == BST_CHECKED && (textLen = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_FONTFILE))) > 0)
	{
		gConfig.pszFontFile = (char*)realloc(gConfig.pszFontFile, textLen + 1);
		GetDlgItemTextA(hwndDlg, IDC_FONTFILE, gConfig.pszFontFile, textLen + 1);
	}
	else
	{
		free(gConfig.pszFontFile); gConfig.pszFontFile = nullptr;
	}

	if ((textLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_GAMEPATH))) > 0)
	{
		gConfig.pszGamePath = (char*)realloc(gConfig.pszGamePath, textLen + 1);
		GetDlgItemTextA(hwndDlg, IDC_GAMEPATH, gConfig.pszGamePath, textLen + 1);
	}
	else
	{
		free(gConfig.pszGamePath); gConfig.pszGamePath = nullptr;
	}

	if (IsDlgButtonChecked(hwndDlg, IDC_GLSL) == BST_CHECKED && (textLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_SHADERFILE))) > 0)
	{
		gConfig.pszShader = (char*)realloc(gConfig.pszShader, textLen + 1);
		GetDlgItemTextA(hwndDlg, IDC_SHADERFILE, gConfig.pszShader, textLen + 1);
	}
	else
	{
		free(gConfig.pszShader); gConfig.pszShader = nullptr;
	}

	gConfig.fFullScreen = IsDlgButtonChecked(hwndDlg, IDC_FULLSCREEN) == BST_CHECKED;
	gConfig.fUseTouchOverlay = IsDlgButtonChecked(hwndDlg, IDC_TOUCHOVERLAY) == BST_CHECKED;
	gConfig.fEnableAviPlay = IsDlgButtonChecked(hwndDlg, IDC_ENABLEAVI) == BST_CHECKED;
	gConfig.fKeepAspectRatio = IsDlgButtonChecked(hwndDlg, IDC_ASPECTRATIO) == BST_CHECKED;
	gConfig.fEnableGLSL = IsDlgButtonChecked(hwndDlg, IDC_GLSL) == BST_CHECKED;
	gConfig.fEnableHDR = IsDlgButtonChecked(hwndDlg, IDC_HDR) == BST_CHECKED;
	gConfig.dwTextureWidth = GetDlgItemInt(hwndDlg, IDC_TEXTUREWIDTH, nullptr, FALSE);
	gConfig.dwTextureHeight = GetDlgItemInt(hwndDlg, IDC_TEXTUREHEIGHT, nullptr, FALSE);
	gConfig.dwScreenWidth = GetDlgItemInt(hwndDlg, IDC_WINDOWWIDTH, nullptr, FALSE);
	gConfig.dwScreenHeight = GetDlgItemInt(hwndDlg, IDC_WINDOWHEIGHT, nullptr, FALSE);
	gConfig.eCDType = (MUSICTYPE)(ComboBox_GetCurSel(hwndDlg, IDC_CD) + MUSIC_MP3);
	gConfig.eMusicType = (MUSICTYPE)ComboBox_GetCurSel(hwndDlg, IDC_BGM);
	gConfig.eOPLCore = (OPLCORE_TYPE)(ComboBox_GetCurSel(hwndDlg, IDC_OPL_CORE));
	gConfig.eOPLChip = (OPLCHIP_TYPE)(gConfig.eOPLCore == OPLCORE_NUKED ? OPLCHIP_OPL3 : ComboBox_GetCurSel(hwndDlg, IDC_OPL_CHIP));
	gConfig.iLogLevel = (LOGLEVEL)(ComboBox_GetCurSel(hwndDlg, IDC_LOGLEVEL));
	gConfig.iAudioChannels = IsDlgButtonChecked(hwndDlg, IDC_STEREO) ? 2 : 1;
	gConfig.iSampleRate = GetDlgItemInt(hwndDlg, IDC_SAMPLERATE, nullptr, FALSE);
	gConfig.wAudioBufferSize = GetDlgItemInt(hwndDlg, IDC_AUDIOBUFFER, nullptr, FALSE);
	gConfig.iMusicVolume = TrackBar_GetPos(hwndDlg, IDC_MUSICVOLUME);
	gConfig.iSoundVolume = TrackBar_GetPos(hwndDlg, IDC_SOUNDVOLUME);
	gConfig.iResampleQuality = TrackBar_GetPos(hwndDlg, IDC_QUALITY);

	if (gConfig.eMusicType == MUSIC_RIX)
	{
		gConfig.fUseSurroundOPL = IsDlgButtonChecked(hwndDlg, IDC_SURROUNDOPL);
		gConfig.iOPLSampleRate = GetDlgItemInt(hwndDlg, IDC_OPLSR, nullptr, FALSE);
	}

	if (fWriteFile) PAL_SaveConfig();
}

void ResetControls(HWND hwndDlg)
{
	TCHAR buffer[100];

	EnableDlgItem(hwndDlg, IDC_OPL_CORE, gConfig.eMusicType == MUSIC_RIX ? TRUE : FALSE);
	EnableDlgItem(hwndDlg, IDC_OPL_CHIP, gConfig.eMusicType == MUSIC_RIX ? TRUE : FALSE);
	EnableDlgItem(hwndDlg, IDC_SURROUNDOPL, gConfig.eMusicType == MUSIC_RIX ? TRUE : FALSE);
	EnableDlgItem(hwndDlg, IDC_OPLSR, gConfig.eMusicType == MUSIC_RIX ? TRUE : FALSE);

	CheckDlgButton(hwndDlg, IDC_FULLSCREEN, gConfig.fFullScreen ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndDlg, IDC_TOUCHOVERLAY, gConfig.fUseTouchOverlay ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndDlg, IDC_ENABLEAVI, gConfig.fEnableAviPlay ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndDlg, IDC_ASPECTRATIO, gConfig.fKeepAspectRatio ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndDlg, IDC_SURROUNDOPL, gConfig.fUseSurroundOPL ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndDlg, IDC_STEREO, gConfig.iAudioChannels == 2 ? BST_CHECKED : BST_UNCHECKED);

	CheckDlgButton(hwndDlg, IDC_USEMSGFILE, gConfig.pszMsgFile ? BST_CHECKED : BST_UNCHECKED);
	EnableDlgItem(hwndDlg, IDC_BRMSG, gConfig.pszMsgFile ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndDlg, IDC_USEFONTFILE, gConfig.pszFontFile ? BST_CHECKED : BST_UNCHECKED);
	EnableDlgItem(hwndDlg, IDC_BRFONT, gConfig.pszFontFile ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndDlg, IDC_USELOGFILE, gConfig.pszLogFile ? BST_CHECKED : BST_UNCHECKED);
	EnableDlgItem(hwndDlg, IDC_BRLOG, gConfig.pszLogFile ? BST_CHECKED : BST_UNCHECKED);

	CheckDlgButton(hwndDlg, IDC_GLSL, gConfig.fEnableGLSL ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwndDlg, IDC_HDR, gConfig.fEnableHDR ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemText(hwndDlg, IDC_TEXTUREWIDTH, _itot(gConfig.dwTextureWidth, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_TEXTUREHEIGHT, _itot(gConfig.dwTextureHeight, buffer, 10));
	EnableDlgItem(hwndDlg, IDC_HDR, gConfig.fEnableGLSL ? TRUE : FALSE);
	EnableDlgItem(hwndDlg, IDC_TEXTUREWIDTH, gConfig.fEnableGLSL ? TRUE : FALSE);
	EnableDlgItem(hwndDlg, IDC_TEXTUREHEIGHT, gConfig.fEnableGLSL ? TRUE : FALSE);
	EnableDlgItem(hwndDlg, IDC_BRSHADER, gConfig.fEnableGLSL ? TRUE : FALSE);

	ComboBox_SetCurSel(hwndDlg, IDC_CD, gConfig.eCDType - MUSIC_MP3);
	ComboBox_SetCurSel(hwndDlg, IDC_BGM, gConfig.eMusicType);
	ComboBox_SetCurSel(hwndDlg, IDC_OPL_CORE, gConfig.eOPLCore);
	ComboBox_SetCurSel(hwndDlg, IDC_OPL_CHIP, gConfig.eOPLChip);
	ComboBox_SetCurSel(hwndDlg, IDC_LOGLEVEL, gConfig.iLogLevel);

	SetDlgItemText(hwndDlg, IDC_SAMPLERATE, _itot(gConfig.iSampleRate, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_OPLSR, _itot(gConfig.iOPLSampleRate, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_AUDIOBUFFER, _itot(gConfig.wAudioBufferSize, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_WINDOWWIDTH, _itot(gConfig.dwScreenWidth, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_WINDOWHEIGHT, _itot(gConfig.dwScreenHeight, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_TEXTUREWIDTH, _itot(gConfig.dwTextureWidth, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_TEXTUREHEIGHT, _itot(gConfig.dwTextureHeight, buffer, 10));

	if (gConfig.pszGamePath) SetDlgItemTextA(hwndDlg, IDC_GAMEPATH, gConfig.pszGamePath);
	if (gConfig.pszMsgFile) SetDlgItemTextA(hwndDlg, IDC_MSGFILE, gConfig.pszMsgFile);
	if (gConfig.pszFontFile) SetDlgItemTextA(hwndDlg, IDC_FONTFILE, gConfig.pszFontFile);
	if (gConfig.pszLogFile) SetDlgItemTextA(hwndDlg, IDC_LOGFILE, gConfig.pszLogFile);
	if (gConfig.pszShader) SetDlgItemTextA(hwndDlg, IDC_SHADERFILE, gConfig.pszShader);

	TrackBar_SetPos(hwndDlg, IDC_QUALITY, gConfig.iResampleQuality, TRUE);
	TrackBar_SetPos(hwndDlg, IDC_MUSICVOLUME, gConfig.iMusicVolume, TRUE);
	TrackBar_SetPos(hwndDlg, IDC_SOUNDVOLUME, gConfig.iSoundVolume, TRUE);
}

INT_PTR InitProc(HWND hwndDlg, HWND hwndCtrl, LPARAM lParam)
{
	InitCommonControls();
	SetClassLongPtr(hwndDlg, GCLP_HICON, (LONG_PTR)LoadIcon((HINSTANCE)GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_SDLPAL)));

	auto log_levels = LoadResourceString(IDC_LOGLEVEL);
	for (size_t pos = 0; pos != std::string::npos; )
	{
		std::wstring item;
		auto next = log_levels.find(L';', pos);
		if (next != std::string::npos)
		{
			item.assign(log_levels.c_str() + pos, next - pos);
			pos = next + 1;
		}
		else
		{
			item.assign(log_levels.c_str() + pos);
			pos = next;
		}
		if (item.length() > 0)
		{
			ComboBox_AddString(hwndDlg, IDC_LOGLEVEL, item.c_str());
		}
	}

	ComboBox_AddString(hwndDlg, IDC_CD, TEXT("MP3"));
	ComboBox_AddString(hwndDlg, IDC_CD, TEXT("OGG"));

	ComboBox_AddString(hwndDlg, IDC_BGM, TEXT("MIDI"));
	ComboBox_AddString(hwndDlg, IDC_BGM, TEXT("RIX"));
	ComboBox_AddString(hwndDlg, IDC_BGM, TEXT("MP3"));
	ComboBox_AddString(hwndDlg, IDC_BGM, TEXT("OGG"));

	ComboBox_AddString(hwndDlg, IDC_OPL_CORE, TEXT("MAME"));
	ComboBox_AddString(hwndDlg, IDC_OPL_CORE, TEXT("DBFLT"));
	ComboBox_AddString(hwndDlg, IDC_OPL_CORE, TEXT("DBINT"));
	ComboBox_AddString(hwndDlg, IDC_OPL_CORE, TEXT("NUKED"));
	ComboBox_SetCurSel(hwndDlg, IDC_OPL_CORE, 1);

	ComboBox_AddString(hwndDlg, IDC_OPL_CHIP, TEXT("OPL2"));
	ComboBox_AddString(hwndDlg, IDC_OPL_CHIP, TEXT("OPL3"));

	TrackBar_SetRange(hwndDlg, IDC_QUALITY, RESAMPLER_QUALITY_MIN, RESAMPLER_QUALITY_MAX, FALSE);
	TrackBar_SetRange(hwndDlg, IDC_MUSICVOLUME, 0, PAL_MAX_VOLUME, FALSE);
	TrackBar_SetRange(hwndDlg, IDC_SOUNDVOLUME, 0, PAL_MAX_VOLUME, FALSE);

	ResetControls(hwndDlg);

	WINDOWINFO wi = { sizeof(WINDOWINFO) };
	RECT rcWork;
	if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0) && GetWindowInfo(hwndDlg, &wi))
	{
		int x = ((rcWork.right - rcWork.left) - (wi.rcWindow.right - wi.rcWindow.left)) / 2;
		int y = ((rcWork.bottom - rcWork.top) - (wi.rcWindow.bottom - wi.rcWindow.top)) / 2;
		SetWindowPos(hwndDlg, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	}

	return TRUE;
}

INT_PTR ButtonProc(HWND hwndDlg, WORD idControl, HWND hwndCtrl)
{
	switch (idControl)
	{
	case IDOK:
		gConfig.fLaunchSetting = FALSE;
		SaveSettings(hwndDlg, TRUE);
		EndDialog(hwndDlg, IDOK);
		return TRUE;

	case IDCANCEL:
		EndDialog(hwndDlg, IDCANCEL);
		return TRUE;

	case IDC_DEFAULT:
		PAL_LoadConfig(FALSE);
		ResetControls(hwndDlg);
		return TRUE;

	case IDC_BRGAME:
	{
		TCHAR szName[MAX_PATH * 2], szTitle[200];
		BROWSEINFO bi = { hwndDlg, nullptr, szName, szTitle, BIF_USENEWUI, nullptr, NULL, 0 };
		LoadStringEx(g_hInstance, IDC_BRGAME, g_wLanguage, szTitle, 200);
		auto pidl = SHBrowseForFolder(&bi);
		if (pidl)
		{
			SHGetPathFromIDList(pidl, szName);
			int n = _tcslen(szName);
			if (szName[n - 1] != '\\') _tcscat(szName, L"\\");
			SetDlgItemText(hwndDlg, IDC_GAMEPATH, szName);
		}
		return TRUE;
	}

	case IDC_BRFONT:
	case IDC_BRMSG:
	case IDC_BRLOG:
	case IDC_BRSHADER:
	{
		TCHAR szFilePath[MAX_PATH * 2] = { 0 };
		auto filter = LoadResourceString(idControl + 1);
		auto title = LoadResourceString(idControl);
		for (auto i = filter.begin(); i != filter.end(); *i = (*i == '|') ? '\0' : *i, i++);
		OPENFILENAME ofn = {
			sizeof(OPENFILENAME), hwndDlg, nullptr,
			filter.c_str(), nullptr, 0, 0,
			szFilePath, sizeof(szFilePath) / sizeof(TCHAR),
			nullptr, 0, nullptr, title.c_str(),
			OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | (idControl != IDC_BRLOG ? OFN_FILEMUSTEXIST : (DWORD)0) | OFN_NOCHANGEDIR
		};
		if (idControl == IDC_BRLOG ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn))
		{
			SetDlgItemText(hwndDlg, idControl + 1, ofn.lpstrFile);
		}
		return TRUE;
	}

	case IDC_USEMSGFILE:
	case IDC_USEFONTFILE:
	case IDC_USELOGFILE:
		EnableDlgItem(hwndDlg, idControl + 1, IsDlgButtonChecked(hwndDlg, idControl) == BST_CHECKED ? TRUE : FALSE);
		return TRUE;

	case IDC_GLSL:
		EnableDlgItem(hwndDlg, IDC_HDR, IsDlgButtonChecked(hwndDlg, idControl) == BST_CHECKED ? TRUE : FALSE);
		EnableDlgItem(hwndDlg, IDC_TEXTUREWIDTH, IsDlgButtonChecked(hwndDlg, idControl) == BST_CHECKED ? TRUE : FALSE);
		EnableDlgItem(hwndDlg, IDC_TEXTUREHEIGHT, IsDlgButtonChecked(hwndDlg, idControl) == BST_CHECKED ? TRUE : FALSE);
		EnableDlgItem(hwndDlg, IDC_BRSHADER, IsDlgButtonChecked(hwndDlg, idControl) == BST_CHECKED ? TRUE : FALSE);
		return TRUE;

	default: return FALSE;
	}
}

INT_PTR ComboBoxProc(HWND hwndDlg, WORD idControl, HWND hwndCtrl)
{
	switch (idControl)
	{
	case IDC_BGM:
		EnableDlgItem(hwndDlg, IDC_OPL_CORE, ComboBox_GetCurSel(hwndDlg, IDC_BGM) == MUSIC_RIX);
		EnableDlgItem(hwndDlg, IDC_OPL_CHIP, ComboBox_GetCurSel(hwndDlg, IDC_BGM) == MUSIC_RIX);
		EnableDlgItem(hwndDlg, IDC_SURROUNDOPL, ComboBox_GetCurSel(hwndDlg, IDC_BGM) == MUSIC_RIX);
		EnableDlgItem(hwndDlg, IDC_OPLSR, ComboBox_GetCurSel(hwndDlg, IDC_BGM) == MUSIC_RIX);
		return TRUE;

	default:
		return FALSE;
	}
}

INT_PTR CommandProc(HWND hwndDlg, WORD command, WORD idControl, HWND hwndCtrl)
{
	switch (command)
	{
	case BN_CLICKED: return ButtonProc(hwndDlg, idControl, hwndCtrl);
	case CBN_SELCHANGE: return ComboBoxProc(hwndDlg, idControl, hwndCtrl);
	default: return FALSE;
	}
}

INT_PTR CALLBACK LauncherDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG: return InitProc(hwndDlg, (HWND)wParam, lParam);
	case WM_COMMAND: return CommandProc(hwndDlg, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
	case WM_CLOSE: return EndDialog(hwndDlg, 0);
	default: return FALSE;
	}
}

typedef LANGID(__stdcall *GETLANGUAGEID)(void);

extern "C" int UTIL_Platform_Startup(int argc, char *argv[]) {
	SDL_setenv("SDL_AUDIODRIVER", "directsound", 1);
	return 0;
}

extern "C" int UTIL_Platform_Init(int argc, char* argv[])
{
	// Try to get Vista+ API at runtime, and falls back to XP's API if not found
	GETLANGUAGEID GetLanguage = (GETLANGUAGEID)GetProcAddress(GetModuleHandle(TEXT("Kernel32.dll")), "GetThreadUILanguage");
	if (!GetLanguage) GetLanguage = GetUserDefaultLangID;

	// Defaults log to debug output
	UTIL_LogAddOutputCallback([](LOGLEVEL, const char* str, const char*)->void {
		OutputDebugStringA(str);
	}, PAL_DEFAULT_LOGLEVEL);

	g_hInstance = GetModuleHandle(nullptr);
	g_wLanguage = GetLanguage();
	if (PRIMARYLANGID(g_wLanguage) == LANG_CHINESE)
	{
		if (SUBLANGID(g_wLanguage) == SUBLANG_CHINESE_SIMPLIFIED || SUBLANGID(g_wLanguage) == SUBLANG_CHINESE_SINGAPORE)
			g_wLanguage = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
		else
			g_wLanguage = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
	}
	else
		g_wLanguage = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
	auto dlg = (LPCDLGTEMPLATE)LockResource(LoadResource(g_hInstance, FindResourceEx(g_hInstance, RT_DIALOG, MAKEINTRESOURCE(IDD_LAUNCHER), g_wLanguage)));
	if (gConfig.fLaunchSetting && DialogBoxIndirect(GetModuleHandle(nullptr), dlg, nullptr, LauncherDialogProc) != IDOK)
		return -1;
	else
		return 0;
}

extern "C" VOID UTIL_Platform_Quit(VOID) {}

extern "C" BOOL
UTIL_GetScreenSize(
	DWORD *pdwScreenWidth,
	DWORD *pdwScreenHeight
)
{
	return (pdwScreenWidth && pdwScreenHeight && *pdwScreenWidth && *pdwScreenHeight);
}

extern "C"
BOOL UTIL_IsAbsolutePath(LPCSTR  lpszFileName)
{
	char szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szFname[_MAX_FNAME], szExt[_MAX_EXT];
#if !defined(__MINGW32__) // MinGW Distro's win32 api lacks this...Anyway, winxp lacks this too
	if (_splitpath_s(lpszFileName, szDrive, szDir, szFname, szExt) == 0)
#else
	_splitpath(lpszFileName, szDrive, szDir, szFname, szExt);
	if ( errno !=EINVAL )
#endif
		return (strlen(szDrive) > 0 && (szDir[0] == '\\' || szDir[0] == '/'));
	else
		return FALSE;
}
