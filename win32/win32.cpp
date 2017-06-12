/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2017, SDLPAL development team.
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
#define _CRT_SECURE_NO_WARNINGS
#define NO_DSHOW_STRSAFE

#include <inttypes.h>
#include <string>
#include <tchar.h>
#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <shlobj.h>
#include <SDL_syswm.h>
#include <d3d9.h>
#include <dshow.h>
#include <vmr9.h>
#include "resource.h"
#include "global.h"
#include "util.h"
#include "palcfg.h"
#include "resampler.h"
#include "input.h"
#include "video.h"

#undef ComboBox_AddString
#undef ComboBox_SetCurSel
#undef ComboBox_GetCurSel

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

	if (IsDlgButtonChecked(hwndDlg, IDC_USEMSGFILE) && (textLen = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MSGFILE))) > 0)
	{
		gConfig.pszMsgFile = (char*)realloc(gConfig.pszMsgFile, textLen + 1);
		GetDlgItemTextA(hwndDlg, IDC_MSGFILE, gConfig.pszMsgFile, textLen + 1);
	}
	else
	{
		free(gConfig.pszMsgFile); gConfig.pszMsgFile = nullptr;
	}

	if (IsDlgButtonChecked(hwndDlg, IDC_USELOGFILE) && (textLen = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_LOGFILE))) > 0)
	{
		gConfig.pszLogFile = (char*)realloc(gConfig.pszLogFile, textLen + 1);
		GetDlgItemTextA(hwndDlg, IDC_LOGFILE, gConfig.pszLogFile, textLen + 1);
	}
	else
	{
		free(gConfig.pszLogFile); gConfig.pszLogFile = nullptr;
	}

	if (IsDlgButtonChecked(hwndDlg, IDC_USEFONTFILE) && (textLen = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_FONTFILE))) > 0)
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

	gConfig.fFullScreen = IsDlgButtonChecked(hwndDlg, IDC_FULLSCREEN);
	gConfig.fUseTouchOverlay = IsDlgButtonChecked(hwndDlg, IDC_TOUCHOVERLAY);
	gConfig.fEnableAviPlay = IsDlgButtonChecked(hwndDlg, IDC_ENABLEAVI);
	gConfig.fKeepAspectRatio = IsDlgButtonChecked(hwndDlg, IDC_ASPECTRATIO);
	gConfig.eCDType = (MUSICTYPE)(ComboBox_GetCurSel(hwndDlg, IDC_CD) + MUSIC_MP3);
	gConfig.eMusicType = (MUSICTYPE)ComboBox_GetCurSel(hwndDlg, IDC_BGM);
	gConfig.eOPLType = (OPLTYPE)(ComboBox_GetCurSel(hwndDlg, IDC_OPL));
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

	EnableDlgItem(hwndDlg, IDC_OPL, gConfig.eMusicType == MUSIC_RIX);
	EnableDlgItem(hwndDlg, IDC_SURROUNDOPL, gConfig.eMusicType == MUSIC_RIX);
	EnableDlgItem(hwndDlg, IDC_OPLSR, gConfig.eMusicType == MUSIC_RIX);

	CheckDlgButton(hwndDlg, IDC_FULLSCREEN, gConfig.fFullScreen);
	CheckDlgButton(hwndDlg, IDC_TOUCHOVERLAY, gConfig.fUseTouchOverlay);
	CheckDlgButton(hwndDlg, IDC_ENABLEAVI, gConfig.fEnableAviPlay);
	CheckDlgButton(hwndDlg, IDC_ASPECTRATIO, gConfig.fKeepAspectRatio);
	CheckDlgButton(hwndDlg, IDC_SURROUNDOPL, gConfig.fUseSurroundOPL);
	CheckDlgButton(hwndDlg, IDC_STEREO, gConfig.iAudioChannels == 2);

	CheckDlgButton(hwndDlg, IDC_USEMSGFILE, gConfig.pszMsgFile != nullptr);
	EnableDlgItem(hwndDlg, IDC_BRMSG, gConfig.pszMsgFile != nullptr);
	CheckDlgButton(hwndDlg, IDC_USEFONTFILE, gConfig.pszFontFile != nullptr);
	EnableDlgItem(hwndDlg, IDC_BRFONT, gConfig.pszFontFile != nullptr);
	CheckDlgButton(hwndDlg, IDC_USELOGFILE, gConfig.pszLogFile != nullptr);
	EnableDlgItem(hwndDlg, IDC_BRLOG, gConfig.pszLogFile != nullptr);

	ComboBox_SetCurSel(hwndDlg, IDC_CD, gConfig.eCDType - MUSIC_MP3);
	ComboBox_SetCurSel(hwndDlg, IDC_BGM, gConfig.eMusicType);
	ComboBox_SetCurSel(hwndDlg, IDC_OPL, gConfig.eOPLType);
	ComboBox_SetCurSel(hwndDlg, IDC_LOGLEVEL, gConfig.iLogLevel);

	SetDlgItemText(hwndDlg, IDC_SAMPLERATE, _itot(gConfig.iSampleRate, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_OPLSR, _itot(gConfig.iOPLSampleRate, buffer, 10));
	SetDlgItemText(hwndDlg, IDC_AUDIOBUFFER, _itot(gConfig.wAudioBufferSize, buffer, 10));

	if (gConfig.pszGamePath) SetDlgItemTextA(hwndDlg, IDC_GAMEPATH, gConfig.pszGamePath);
	if (gConfig.pszMsgFile) SetDlgItemTextA(hwndDlg, IDC_MSGFILE, gConfig.pszMsgFile);
	if (gConfig.pszFontFile) SetDlgItemTextA(hwndDlg, IDC_FONTFILE, gConfig.pszFontFile);
	if (gConfig.pszLogFile) SetDlgItemTextA(hwndDlg, IDC_LOGFILE, gConfig.pszLogFile);

	TrackBar_SetPos(hwndDlg, IDC_QUALITY, gConfig.iResampleQuality, TRUE);
	TrackBar_SetPos(hwndDlg, IDC_MUSICVOLUME, gConfig.iMusicVolume, TRUE);
	TrackBar_SetPos(hwndDlg, IDC_SOUNDVOLUME, gConfig.iSoundVolume, TRUE);
}

INT_PTR InitProc(HWND hwndDlg, HWND hwndCtrl, LPARAM lParam)
{
	InitCommonControls();

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

	ComboBox_AddString(hwndDlg, IDC_OPL, TEXT("DOSBOX"));
	ComboBox_AddString(hwndDlg, IDC_OPL, TEXT("MAME"));
	ComboBox_AddString(hwndDlg, IDC_OPL, TEXT("DOSBOXNEW"));

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
			size_t n = _tcslen(szName);
			if (szName[n - 1] != '\\') _tcscat(szName, L"\\");
			SetDlgItemText(hwndDlg, IDC_GAMEPATH, szName);
		}
		return TRUE;
	}

	case IDC_BRFONT:
	case IDC_BRMSG:
	case IDC_BRLOG:
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
			OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | (idControl != IDC_BRLOG ? OFN_FILEMUSTEXIST : (DWORD)0)
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
		EnableDlgItem(hwndDlg, idControl + 1, IsDlgButtonChecked(hwndDlg, idControl));
		return TRUE;

	default: return FALSE;
	}
}

INT_PTR ComboBoxProc(HWND hwndDlg, WORD idControl, HWND hwndCtrl)
{
	switch (idControl)
	{
	case IDC_BGM:
		EnableDlgItem(hwndDlg, IDC_OPL, ComboBox_GetCurSel(hwndDlg, IDC_BGM) == MUSIC_RIX);
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

// DirectShow-based AVI player for Windows

template<class T>
class com_ptr
{
public:
	com_ptr() : _obj(nullptr) {}
	com_ptr(T* obj) : _obj(obj) {}
	~com_ptr() { _obj && _obj->Release(); }

	operator T*() { return _obj; }
	T* operator->() { return _obj; }
	T** operator&() { return &_obj; }

	com_ptr& operator=(T* other)
	{
		if (_obj != other)
		{
			_obj && _obj->Release();
			(_obj = other) && _obj->AddRef();
		}
		return *this;
	}

private:
	T* _obj;
};

extern "C"
BOOL PAL_PlayAVI_Native(const char* lpszPath)
{
	SDL_SysWMinfo info;
	if (!VIDEO_GetWindowInfo(&info)) return FALSE;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	HWND hwnd = info.info.win.window;
#else
	HWND hwnd = info.window;
#endif

	RECT rc;
	if (!GetClientRect(hwnd, &rc)) return FALSE;

	int buf_len = MultiByteToWideChar(CP_ACP, 0, lpszPath, -1, nullptr, 0);
	wchar_t* szPath = (wchar_t*)alloca(sizeof(wchar_t) * buf_len);
	MultiByteToWideChar(CP_ACP, 0, lpszPath, -1, szPath, buf_len);

	HANDLE hAviPlayEvent;
	HRESULT hr;
	
	com_ptr<IGraphBuilder> pGraph;
	com_ptr<IMediaControl> pControl;
	com_ptr<IMediaEvent> pEvent;
	com_ptr<IEnumFilters> pEnum;
	com_ptr<IBaseFilter> pFilter;
	com_ptr<IVMRWindowlessControl9> pWindow;

	if (FAILED(hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&pGraph))) return FALSE;
	if (FAILED(hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl))) return FALSE;
	if (FAILED(hr = pGraph->QueryInterface(IID_IMediaEvent, (void **)&pEvent))) return FALSE;
	if (FAILED(hr = pGraph->RenderFile(szPath, nullptr))) return FALSE;
	if (FAILED(hr = pGraph->EnumFilters(&pEnum))) return FALSE;
	while (S_OK == pEnum->Next(1, &pFilter, nullptr))
	{
		FILTER_INFO fi;
		pFilter->QueryFilterInfo(&fi);
		if (lstrcmpW(fi.achName, L"Video Renderer") == 0)
		{
			com_ptr<IEnumPins> pEnumPins;
			com_ptr<IPin> pOutputPin, pPin;
			if (FAILED(hr = pFilter->EnumPins(&pEnumPins))) return FALSE;
			while (S_OK == pEnumPins->Next(1, &pPin, nullptr))
			{
				PIN_DIRECTION dir;
				if (SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_INPUT &&
					SUCCEEDED(pPin->ConnectedTo(&pOutputPin)) && pOutputPin)
					break;
				else
					pPin = nullptr;
			}
			if (!pOutputPin || FAILED(hr = pGraph->RemoveFilter(pFilter))) return FALSE;

			pPin = nullptr;
			pEnumPins = nullptr;
			pFilter = nullptr;

			com_ptr<IVMRFilterConfig9> pConfig;
			if (FAILED(hr = CoCreateInstance(CLSID_VideoMixingRenderer9, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **)&pFilter))) return FALSE;
			if (FAILED(hr = pFilter->QueryInterface(&pConfig))) return FALSE;
			if (FAILED(hr = pConfig->SetRenderingMode(VMR9Mode_Windowless))) return FALSE;
			if (FAILED(hr = pConfig->QueryInterface(&pWindow))) return FALSE;
			if (FAILED(hr = pWindow->SetVideoClippingWindow(hwnd))) return FALSE;
			if (FAILED(hr = pWindow->SetVideoPosition(nullptr, &rc))) return FALSE;
			if (FAILED(hr = pGraph->AddFilter(pFilter, L"Video Renderer"))) return FALSE;
			if (FAILED(hr = pFilter->EnumPins(&pEnumPins))) return FALSE;
			while (S_OK == pEnumPins->Next(1, &pPin, nullptr))
			{
				PIN_DIRECTION dir;
				if (SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_INPUT)
					break;
				else
					pPin = nullptr;
			}
			if (!pPin || FAILED(hr = pGraph->Connect(pOutputPin, pPin))) return FALSE;

			break;
		}
		pFilter = nullptr;
	}
	RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
	PAL_ClearKeyState();

	if (FAILED(hr = pEvent->GetEventHandle((OAEVENT*)&hAviPlayEvent))) return FALSE;
	if (FAILED(hr = pControl->Run())) return FALSE;

	BOOL looping = TRUE;
	while (looping && !(g_InputState.dwKeyPress & (kKeyMenu | kKeySearch)))
	{
		if (WaitForSingleObject(hAviPlayEvent, 50) == WAIT_OBJECT_0)
		{
			long evCode;
			LONG_PTR param1, param2;
			while (S_OK == pEvent->GetEvent(&evCode, &param1, &param2, 0))
			{
				pEvent->FreeEventParams(evCode, param1, param2);
				if (evCode == EC_COMPLETE)
					looping = FALSE;
			}
		}
		UTIL_Delay(1);
	}
	hr = pControl->Stop();

	if (looping)
	{
		//
		// Simulate a short delay (like the original game)
		//
		UTIL_Delay(500);
	}

	return TRUE;
}
